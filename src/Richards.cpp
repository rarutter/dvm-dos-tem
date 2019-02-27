

#include "../include/Richards.h"

#include "../include/TEMLogger.h"
extern src::severity_logger< severity_level > glg;

//lapacke must be included after TEMLogger, to avoid BOOST-related
// build errors
#include <lapacke/lapacke.h>

Richards::Richards() {
  //TSTEPMIN = 1.e-5;      //
  //TSTEPMAX = 0.2;
  //TSTEPORG = 0.1;
  //LIQTOLE = 0.05;  // tolearance is in fraction of 'maxliq' in a layer
  mindzlay = 0.005;
};

Richards::~Richards() {
};

void Richards::clearRichardsArrays(){
  for(int ii=0; ii<=MAX_SOI_LAY; ii++){

    //Incoming values
    Bsw[ii] = 0.0;
    ksat[ii] = 0.0;
    psisat[ii] = 0.0;

    //First round of calculated values
    k[ii] = 0.0;
    psi[ii] = 0.0;
    psiE[ii] = 0.0;
    theta[ii] = 0.0;
    thetasat[ii] = 0.0;
    z_h[ii] = 0.0;
    thetaE[ii] = 0.0;
    thetaE_unsat[ii] = 0.0;

    //Intermediate calculated values
    q_iminus1_n[ii] = 0.0;
    q_i_n[ii] = 0.0;
    eq7121[ii] = 0.0;
    eq7122[ii] = 0.0;
    eq7123[ii] = 0.0;
    eq7124[ii] = 0.0;
    eq7125[ii] = 0.0;

    eq7117[ii] = 0.0;
    eq7118[ii] = 0.0;
    eq7119[ii] = 0.0;
    eq7120[ii] = 0.0;

    coeffA[ii] = 0.0;
    coeffB[ii] = 0.0;
    coeffC[ii] = 0.0;
    coeffR[ii] = 0.0;

    //Solution
    deltathetaliq[ii] = 0.0;

    //unsorted TODO
    dzmm[ii] = 0.0;
    nodemm[ii] = 0.0;
    effliq[ii] = 0.0;
    effminliq[ii] = 0.0;
    effmaxliq[ii] = 0.0;
    qout[ii] = 0.0;
    layer_drain[ii] = 0.0;
    percolation[ii] = 0.0;
  }
};


//
void Richards::update(Layer *fstsoill, Layer* bdrainl,
                      const double & bdraindepth, const double & fbaseflow,
                      const double & watertab,
                      double trans[], const double & evap,
                      const double & infil, const double & cell_slope,
                      const double &ts) {
  //timestep = ts;
  drainl = bdrainl;

  z_watertab = watertab * 1.e3;

  if (bdraindepth<=0.) {
    return; // the drainage occurs in the surface, no need to update the SM
  }

  //all fluxes already in mm/sec as input
  qinfil = infil;
  qevap  = evap;

//  for(int il=1; il<=MAX_SOI_LAY; il++) {
//    qtrans[il] = trans[il-1]; // trans[] starting from 0, while here all arrays starting from 1
//  }

  qdrain = 0.;
  // loop for continuous unfrozen soil column section
  // in a soil profile, there may be a few or none
  Layer* currl=fstsoill;

  //Excluding moss layer(s) due to the lack of validated
  // hydraulic parameters.
  //If no exclusion of moss layer, comment out this 'while' loop
  while (currl != NULL && currl->isMoss) {
    currl = currl->nextl;
  }

  //These are correct because the arrays are forced to 1-based indexing
  int topind = currl->solind;
  indx0sl = topind;//TODO
  int drainind = drainl->solind;

  Layer* topsoill = currl;

  //Start of conversion to CLM 4.5

  //For now we're using a full day as a timestep, but introducing
  // a local variable in case that changes.
  double delta_t = SEC_IN_DAY;

  //Clear arrays before use
  clearRichardsArrays();

  //Prepare soil column - collect already known values into
  // arrays for ease of use, calculate the basic values needed
  // in the following (more complex) calculations.
  prepareSoilColumn(topsoill, bdraindepth);

  //Re-index trans to match the other arrays used in Richards
  for(int il=indx0al; il<MAX_SOI_LAY; il++){
    qtrans[il] = trans[il-indx0al];
  }

  //If there is only one active layer, the equations below are... TODO
  //calculate drain by something
  if(numal==1){

    int ind = currl->solind;
    double water_in, water_out = 0.;
    double water_change;

    //This duplicates some of the equations from below.
    theta[ind] = effliq[ind] / DENLIQ / (dzmm[ind]/1.e3);
    //Hydraulic conductivity, for drain layer only!
    k[ind] = ksat[ind] * pow( theta[ind]/thetasat[ind],
                              (2 * Bsw[ind] + 3) );


    //top of the soil stack or in the middle?
    //plus infil, minus trans[], minus evap
    double avail_water = infil - evap;

    //Calculate how much more liquid could fit in layer 
    double space_for_liq = fmax(0.0, effmaxliq[ind] - currl->liq) / delta_t;

    if(avail_water > space_for_liq){
      water_in = space_for_liq;
      excess_runoff = avail_water - space_for_liq;
    }
    else{
      water_in = avail_water;
    }

    //Need to use trans[ind-1] because trans is zero based
    // while layer solind is one based.
    //No percolation if single layer
    water_out = qtrans[ind];

    water_change = (water_in - water_out) * delta_t;

    //calculate qdrain?

    currl->liq += water_change; 

    currl->hcond = k[ind];

    //Re-calculate effliq so that theta can be recalculated for
    // lateral drainage (below the multi-layer section)
    effliq[ind] = fmax(0.0001, currl->liq-effminliq[ind]);
  }

  else{ //multiple active layers

    //Move currl up the soil column in order to fill in necessary
    // array elements
    if(currl->prevl != NULL){
  //    currl = currl->prevl;
    }

    //For each thawed and partially thawed layer, run first round
    // of calculations. The results of these are needed by the
    // equations in the second round.
    //while( (currl != NULL) && (currl->solind <= drainl->solind+1) )
    while( (currl != NULL) && (currl->solind <= drainl->solind) ){

      int ind = currl->solind; //Correct because arrays are forced to 1-based

      //CLM 4.5 page 157
      //Theta - volumetric soil water content mm^3 water/ mm^3 soil
      //theta[ind] = effliq[ind] / DENLIQ / (currl->dz * 1.e3); //unitless
      theta[ind] = effliq[ind] / DENLIQ / (dzmm[ind]/1.e3); //unitless

      //Equation 7.94
      //psi_i - soil matric potential (mm)
      double theta_thetasat = theta[ind] / thetasat[ind];
      if(theta_thetasat < 0.01) {
        BOOST_LOG_SEV(glg, debug)<<"theta_thetasat out of range: "
                                 <<theta_thetasat;
        theta_thetasat = 0.01;
      }
      else if(theta_thetasat > 1.) {
        BOOST_LOG_SEV(glg, debug)<<"theta_thetasat out of range: "
                                 <<theta_thetasat;
        theta_thetasat = 1.;
      }
      psi[ind] =  psisat[ind] * pow( theta_thetasat, -Bsw[ind]);
      //logging psi out-of-range violations
      if(psi[ind] < -1e8){
        BOOST_LOG_SEV(glg, err)<<"psi["<<ind<<"] out of range: "<<psi[ind];
        psi[ind] = fmax(psi[ind], -1.e8);
      }

      //Equation 7.131
      //Unsaturated portion of ThetaE?
      //Needed for Equation 7.130
      thetaE_unsat[ind] = thetasat[ind] * psisat[ind]
                        / ( (z_watertab - z_h[ind-1])
                            * (1 - 1 / Bsw[ind]) )
                        * ( 1 - pow( (psisat[ind] - z_watertab + z_h[ind-1])
                                     / psisat[ind], (1 - 1 / Bsw[ind])) );

      //thetaE_i - layer-average equilibrium volumetric water content 
      //Equation 7.129
      //This should only be used when the current layer and previous
      // layer are both above the water table.
      //if water table below layer i
      //if(z_watertab >= z_h[ind]){
      if(z_watertab > z_h[ind]){
        thetaE[ind] = thetasat[ind] * psisat[ind] 
                   / ( (z_h[ind] - z_h[ind-1]) * (1 - 1 / Bsw[ind]) ) 
                     * ( 
                       pow( (psisat[ind] - z_watertab + z_h[ind])/psisat[ind], 
                            (1-1/Bsw[ind]) ) 
                       - pow( (psisat[ind] - z_watertab + z_h[ind-1])/psisat[ind], 
                            (1-1/Bsw[ind]) ) 
                      );
      }
      //else if water table is in layer i
      //else if(z_watertab < z_h[ind] && z_watertab >= z_h[ind-1]){
      else if(z_watertab < z_h[ind] && z_watertab > z_h[ind-1]){
        //Equation 7.130
        //As noted in the paper, thetaE_sat_i = theta_sat_i
        thetaE[ind] = thetasat[ind] * ( (z_h[ind] - z_watertab)
                                       /(z_h[ind] - z_h[ind-1]) )
                    + thetaE_unsat[ind] * ( (z_watertab - z_h[ind-1])
                                           /(z_h[ind] - z_h[ind-1]) );
      }
      else{//water table above layer i
        thetaE[ind] = thetasat[ind]; //per text following equation 7.131
      }


      //Equation 7.134
      //psiE_i - equilibrium soil matric potential
      psiE[ind] = psisat[ind] * pow( thetaE[ind]/thetasat[ind], -Bsw[ind] );
      //Logging violations of the limits
      if(thetaE[ind]/thetasat[ind] < 0.01){
        BOOST_LOG_SEV(glg, err)<<"thetaE_i/thetasat_i out of range";
      }
      if(psiE[ind] < -1e8){
        BOOST_LOG_SEV(glg, err)<<"psiE["<<ind<<"] out of range: "<<psiE[ind];
      }

      currl = currl->nextl; //Move down the soil column
    }

    //reset currl for the next round of calculations
    currl = topsoill;
    if(currl->prevl != NULL){
//      currl = currl->prevl;
    }

    //For each thawed and partially thawed layer, run second round
    // of calculations.
    while( (currl != NULL) && (currl->solind <= drainl->solind) ){

      int ind = currl->solind; //Correct because arrays are forced to 1-based

      //Equation 7.89 (ignoring ice)
      //k: hydraulic conductivity
      //The 0.5 values should cancel - leaving them in for easy comparison
      //  with equations.
      //Given the large range of ksat between horizon types, we might want
      // to average the values instead of taking the value at the bottom
      // of the upper layer.
      //For all but drain layer
      if(ind<drainind){
        k[ind] = ksat[ind]
               * pow( (0.5*(theta[ind] + theta[ind+1]))
                    / (0.5*(thetasat[ind] + thetasat[ind+1])),
                     (2 * Bsw[ind] + 3) );
      }
      //Drain layer
      else{
        k[ind] = ksat[ind] * pow( theta[ind]/thetasat[ind],
                                  (2 * Bsw[ind] + 3) );
      }

      //Equation 7.115
      //q_iminus1^n
//      q_iminus1_n[ind] = -k[ind-1]
//                       * ( (psi[ind-1] - psi[ind] + psiE[ind] - psiE[ind-1])
//                           / (nodemm[ind] - nodemm[ind-1]) );
      //CLM5 Equation 7.74
      q_iminus1_n[ind] = -k[ind-1]
                       * ( (psi[ind-1] - psi[ind] + nodemm[ind] - nodemm[ind-1])
                           / (nodemm[ind] - nodemm[ind-1]) );


      //Equation 7.116
      //q_i^n
//      q_i_n[ind] = -k[ind]
//                 * ( (psi[ind] - psi[ind+1] + psiE[ind+1] - psiE[ind])
//                     / (nodemm[ind+1] - nodemm[ind]) );
      //CLM5 Equation 7.75
      q_i_n[ind] = -k[ind]
                 * ( (psi[ind] - psi[ind+1] + nodemm[ind+1] - nodemm[ind])
                     / (nodemm[ind+1] - nodemm[ind]) );


      //Equation 7.121
      //deltapsi_iminus1 / deltatheta_liq_iminus1
      eq7121[ind] = -Bsw[ind-1] * ( psi[ind-1] / theta[ind-1] );

      //Equation 7.122
      //deltapsi_i / deltatheta_liq_i
      eq7122[ind] = -Bsw[ind] * psi[ind] / theta[ind];

      //Equation 7.123
      //deltapsi_iplus1 / deltatheta_liq_iplus1
      eq7123[ind] = -Bsw[ind+1] * psi[ind+1] / theta[ind+1];

      //The first element of the following two equations is actually:
      //  (1 - (f_frz[ind-1] + f_frz[ind])/2)
      //However, because we restrict the Richards calculations to unfrozen
      // soil, this reduces to 1.

      //Equation 7.124
      //deltak[z_h_iminus1] / deltatheta_liq_iminus1
      //deltak[z_h_iminus1] / deltatheta_liq_i
      eq7124[ind] = 1
                  * (2 * Bsw[ind-1] + 3) * ksat[ind-1]
                  * pow( (0.5*(theta[ind-1] + theta[ind]))
                         / (0.5*(thetasat[ind-1] + thetasat[ind])),
                        (2 * Bsw[ind-1] + 2) )
                  * ( 0.5 / (0.5*(thetasat[ind-1] + thetasat[ind])) );

      //Equation 7.125
      //deltak[z_h_i] / deltatheta_liq_i
      //deltak[z_h_i] / deltatheta_liq_iplus1
      eq7125[ind] = 1
                  * (2 * Bsw[ind] + 3) * ksat[ind]
                  * pow( (0.5*(theta[ind] + theta[ind+1]))
                         / (0.5*(thetasat[ind] + thetasat[ind+1])),
                        (2 * Bsw[ind] + 2) )
                  * ( 0.5 / (0.5*(thetasat[ind] + thetasat[ind+1])) );


      //Equations 7.117-7.120 are not in numerical order because they
      // require some of the higher-numbered equations.
      //They also have sections replaced by the equations above, and
      // so do not precisely match the text
      // (i.e. eq7121[] instead of deltapsi[]/deltatheta_liq[])
      //Equation 7.117
      //deltaq_iminus1 / deltatheta_liq_iminus1
      eq7117[ind] = - ( (k[ind-1]/(nodemm[ind]-nodemm[ind-1])) * eq7121[ind] )
                    - eq7124[ind] 
                    * ( (psi[ind-1] - psi[ind] + nodemm[ind]
                         - nodemm[ind-1])
                        /(nodemm[ind] - nodemm[ind-1]) );


      //Equation 7.118
      //deltaq_iminus1 / deltatheta_liq_i
//      eq7118[ind] = ( (k[ind-1]/(nodemm[ind]-nodemm[ind-1])) * eq7122[ind] )
//                  - eq7124[ind] 
//                  * ( psi[ind-1] - psi[ind] + psiE[ind] - psiE[ind-1]
//                      /(nodemm[ind] - nodemm[ind-1]) );
      //CLM5 Equation 7.77
      eq7118[ind] = ( (k[ind-1]/(nodemm[ind]-nodemm[ind-1])) * eq7122[ind] )
                  - eq7124[ind] 
                  * ( (psi[ind-1] - psi[ind] + nodemm[ind] - nodemm[ind-1])
                      /(nodemm[ind] - nodemm[ind-1]) );


      //Equation 7.119
      //deltaq_i / deltatheta_liq_i
//      eq7119[ind] = - ( (k[ind]/(nodemm[ind+1] - nodemm[ind])) * eq7122[ind] )
//                    - eq7125[ind] 
//                    * ( psi[ind] - psi[ind+1] + psiE[ind+1] - psiE[ind]
//                        /(nodemm[ind+1] - nodemm[ind]) );
      //CLM5 Equation 7.78
      eq7119[ind] = - ( (k[ind]/(nodemm[ind+1] - nodemm[ind])) * eq7122[ind] )
                    - eq7125[ind] 
                    * ( (psi[ind] - psi[ind+1] + nodemm[ind+1] - nodemm[ind])
                        /(nodemm[ind+1] - nodemm[ind]) );



      //Equation 7.120
      //deltaq_i / deltatheta_liq_iplus1
//      eq7120[ind] = (k[ind]/(nodemm[ind+1] - nodemm[ind])) * eq7123[ind]
//                  - eq7125[ind]  
//                  * ( psi[ind] - psi[ind+1] + psiE[ind+1] - psiE[ind]
//                      /(nodemm[ind+1] - nodemm[ind]) );
      //CLM5 Equation 7.79
      eq7120[ind] = (k[ind]/(nodemm[ind+1] - nodemm[ind])) * eq7123[ind]
                  - eq7125[ind]  
                  * ( (psi[ind] - psi[ind+1] + nodemm[ind+1] - nodemm[ind])
                      /(nodemm[ind+1] - nodemm[ind]) );


     //This is the top active layer
      if(ind==topind){

        //Equation 7.136
        coeffA[ind] = 0.0; 

        //deltaz_i / deltat = layer thickness / 86,400
        // (because all data is in mm/s)
        //Equation 7.137. Uses 7.119
        coeffB[ind] = eq7119[ind] - dzmm[ind] / delta_t;

        //Equation 7.138. Uses 7.120
        coeffC[ind] = eq7120[ind];

        //Equation 7.139. Uses 7.116
        //Need to subtract evap here because we allow for it differently
        // than CLM. See section 7.3.3 for CLM approach. 
        // TODO verify the index modification - 1 or 2?
        //The sign convention between CLM 4.5 and ddt are different
        coeffR[ind] = -infil - q_i_n[ind] + (evap + qtrans[ind]); 
      }
      //This is for the middle layers - neither top nor drain
      else if(ind>topind && ind<drainind){

        //Equation 7.140. Uses 7.117
        coeffA[ind] = - eq7117[ind];

        //Equation 7.141. Uses 7.119 and 7.118
        coeffB[ind] = eq7119[ind] - eq7118[ind] - dzmm[ind] / delta_t;

        //Equation 7.142. Uses 7.120
        coeffC[ind] = eq7120[ind];

        //Equation 7.143. Uses 7.115 and 7.116
        coeffR[ind] = q_iminus1_n[ind] - q_i_n[ind] + qtrans[ind];
      }
      //This is the drain layer
      else if(ind==drainind){

        //Equation 7.144. Uses 7.117
        coeffA[ind] = -eq7117[ind];

        //Equation 7.145. Uses 7.118
        coeffB[ind] = -eq7118[ind] - dzmm[ind] / delta_t;

        //Equation 7.146
        coeffC[ind] = 0.0; 

        //Equation 7.147. Uses 7.115
        coeffR[ind] = q_iminus1_n[ind] + qtrans[ind];
      }

      currl = currl->nextl; //Move down the soil column
    }
  }//end of loop for multiple active layers

  //If there are more than one active layers, we use the Crank Nicholson
  // water solver.
  if(numal > 1){
//    cn.tridiagonal(indx0al, numal, coeffA, coeffB, coeffC, coeffR, deltathetaliq);//water solver


    //copy values into arrays so no blank indices
    double sub_diagonal[numal-1];
    double diagonal[numal];
    double super_diagonal[numal-1];
    double result[numal];

    for(int ii=0; ii<numal; ii++){
      diagonal[ii] = coeffB[ii-indx0al];
      result[ii] = coeffR[ii-indx0al];
    }
    for(int ii=0; ii<numal-1; ii++){
      sub_diagonal[ii] = coeffA[ii-indx0al];
      super_diagonal[ii] = coeffC[ii-indx0al];
    }

    //int number of state variables (layers)
    //int number of columns in matrix B (1)
    //sub-diagonal elements of A
    //diagonal elements of A
    //super-diagonal elements of A
    //RHS vector (coeffR)
    //int nlayers - the leading dimension of matrix rhs?
    //integer err
    lapack_int lapacke_err, num_things, nrhs;
    num_things = numal;
    nrhs = 1;
    LAPACKE_dgtsv(LAPACK_ROW_MAJOR, numal, 1, sub_diagonal, diagonal, super_diagonal, result, lapacke_err);
//    LAPACKE_dgtsv(numal, 1, sub_diagonal, diagonal, super_diagonal, result, LAPACK_ROW_MAJOR, err); 

//    tridiagonal_solver(bounds, lbj, ubj, jtop, numf, filter, a, b, c, r, u);

    //copy values from result into deltathetaliq
    for(int ii=0; ii<numal; ii++){
      deltathetaliq[ii+indx0al] = result[ii];
    }

 
    //A NaN check for debugging purposes
    for(int ii=0; ii<MAX_SOI_LAY; ii++){
      if(deltathetaliq[ii] != deltathetaliq[ii]){
        BOOST_LOG_SEV(glg, err) << "NaN in deltathetaliq";
      }
    }
  

    //do the next section for only active layers TODO
    currl = topsoill;
    while(currl->solind<indx0al){
      currl = currl->nextl;
    } 

    //Modify layer liquid in each active layer by calculated
    // change in liquid.
    while(currl->solind<=drainind){

      int ind = currl->solind;

      double minliq = effminliq[ind];
      double maxliq = effmaxliq[ind];

      //TODO - verify
      double liquid_change = dzmm[ind] * deltathetaliq[ind];
      currl->liq += liquid_change;
      //currl->liq += dzmm[ind]/1.e3 * deltathetaliq[ind];
      //currl->liq += currl->liq + dzmm[ind] * deltathetaliq[ind] + minliq;
      //currl->liq += deltathetaliq[il] + minliq;
      percolation[ind] = liquid_change;

      //Restricting layer liquid to range defined by min and max
      if(currl->liq<minliq){
        currl->liq = minliq;
      }

      if(currl->liq>maxliq){
        currl->liq = maxliq;
      }

      currl->hcond = k[ind];

      //Re-calculate effliq so that theta can be recalculated for
      // lateral drainage (below)
      effliq[ind] = fmax(0.0001, currl->liq-effminliq[ind]);
 
      currl = currl->nextl;
    }
  }

  //Updating theta post percolation so lateral drainage is
  // based on today's values
  //Note that effliq is updated in two locations above (post layer
  // water modification). 
  for(int il=0; il<MAX_SOI_LAY+1; il++){
    if(dzmm[il]>0){
      theta[il] = effliq[il] / DENLIQ / (dzmm[il]/1.e3); //unitless
    }
  }

  //Calculating lateral drainage (only for saturated layers)
  double lateral_drain[MAX_SOI_LAY+1] = {0};
  //double column_drain = 0;//Total lateral drainage, mm/day
  double eq7103_num = 0.;
  double eq7103_den = 0.;
  bool sat_soil = false;//If there is at least one saturated layer

  for(int ii=0; ii<MAX_SOI_LAY; ii++){
    //For any saturated layer
    if(theta[ii] / thetasat[ii] >= 0.9){
      sat_soil = true;

      eq7103_num += ksat[ii] * dzmm[ii] / 1.e3;
      eq7103_den += dzmm[ii] / 1.e3;
    }
  }

  //If there is at least one saturated layer, apply lateral drainage
  if(sat_soil){
    //CLM4.5 Equation 7.167
    //CLM5 Equations 7.103 and 7.102
    //We do not allow Richards to run on frozen soil, so the ice
    // parameter is ignored.
    double slope_rads = cell_slope * PI / 180;//Converting to radians
    double kdrain_perch = 10e-5 * sin(slope_rads)
                        * (eq7103_num / eq7103_den);

    double qdrain_perch = kdrain_perch * (bdraindepth - watertab)
                        * fbaseflow;

    //Applying lateral drainage to saturated layers 
    currl = topsoill;
    while(currl->solind <= drainind){
      //This will skip indices that are empty (from Richards arrays
      // being 1-based)
      //int ind = currl->solind - topind;
      int ind = currl->solind;
      if(theta[ind] / thetasat[ind] >= 0.9){
        double layer_max_drain = currl->liq - effminliq[ind];

        double layer_calc_drain = qdrain_perch * delta_t 
                                * ((dzmm[ind]/1.e3) / eq7103_den);

        layer_drain[ind] = fmin(layer_max_drain, layer_calc_drain);

        if(layer_drain[ind] > 0){
          currl->liq -= layer_drain[ind];
          qdrain += layer_drain[ind];
        }
      }
      currl = currl->nextl;
    }
  }

/*TODO cleanup
  if(bdraindepth*1.e3 - z_watertab >= 0){
    double eq7167_num = 0.;
    double eq7167_den = 0.;

    //Pre-calculate sums for lateral drainage
    //For any saturated layer
    for(int ii=0; ii<MAX_SOI_LAY; ii++){
      if(theta[ii] / thetasat[ii] >= 0.9){

        eq7167_num += ksat[ii] * dzmm[ii] / 1.e3;
        eq7167_den += dzmm[ii] / 1.e3;
      }
    }

    //Calculate lateral drainage
    //Equation 7.167
    //We do not allow Richards to run on frozen soil, so the ice parameter
    // is ignored.
    double slope_rads = cell_slope * PI / 180;//Converting to radians
    double kdrain_perch = 10e-5 * sin(slope_rads)
                        * (eq7167_num / eq7167_den);

    //Equation 7.166, with base flow added
    double qdrain_perch = kdrain_perch * (bdraindepth - watertab)
                        * fbaseflow;
    //need to check that zfrost - z_watertab != 0

    currl = topsoill;
    while(currl->solind <= drainind){
      int ind = currl->solind;
      //For any saturated layer
      if(theta[ind] / thetasat[ind] >= 0.9){
        double layer_drain = qdrain_perch * delta_t 
                           * (dzmm[ind] / eq7167_den);
        currl->liq -= layer_drain;
      }
      currl = currl->nextl;
    }

  }*/

  // for layers above 'topsoill', e.g., 'moss',
  // if excluded from hydrological process
  currl = topsoill->prevl;

  while (currl!=NULL && currl->nextl!=NULL && currl->isMoss) {
//    if (currl->indl<fstsoill->indl) {
//      break;  // if no layer excluded, the 'while' loop will break here
///    }

    double lwc = currl->nextl->getVolLiq();
    currl->liq = currl->dz*(1.0-currl->frozenfrac)*lwc*DENLIQ; //assuming same 'VWC' in the unfrozen portion as below
    currl=currl->prevl;
  }

};

/*
tridiagonal(bounds, 1, nlevsoi+1, jtop(bounds.begc:bounds.endc, num_hydrologyc, filter_hydrologyc, amx, bmx, cmx, rmx, dwat2))
void Richards::tridiagonal_solver(bounds, lbj, ubj, jtop, numf, filter, a, b, c, r, u){
  //bounds = incoming bounds type, begc and endc are beginning and ending column index 
  //lbj = lbinning level index
  //ubj = ubing level index
  //jtop = top level for each column, integer array jtop(bounds.begc:bounds.endc). Default value = 1, then passed in to Tridiagonal
  //numf = filter dimension, incoming num_hydrologyc (number of column soil points in column filter)
  //filter = filter, incoming filter_hydrologyc(:) (column filter for soil points). Defined as input to HydrologyNoDrainage as integer array of unknown size.
  //a = left off diagonal of tridiagonal matrix
  //b = diagonal column of ...
  //c = right off diagonal ...
  //r = forcing term of tridiagonal matrix
  //u = solution

  //j, ci, fc are integer indices
  //gam(bounds.begc:bounds.endc, lbj:ubj)
  //bet(bounds.begc:bounds.endc)

  double gam[][]; //what is gam?
  double bet[]; //what is bet?

  for(int fc=1; fc<=numf; fc++){
    int ci = filter(fc);
    bet[ci] = b(ci, jtop(ci));//jtop is most likely 1 for any column
  }

  for(int j=lbj; j<=ubj; j++){
    for(int fc=1, fc<=numf; fc++){

      ci = filter(fc);

      if((col%itype(ci) == icol_sunwall || col%itype(ci) == icol_shadewall & || col%itype(ci) == icol_roof) && j <= nlevurb){
        if(j >= jtop(ci)){
          if(j == jtop(ci)){
            u(ci,j) = r(ci,j) / bet(ci);
          }
          else{
            gam(ci, j) = c(ci,j-1) / bet(ci);
            bet(ci) = b(ci,j) - a(ci,j) * gam(ci,j);
            u(ci,j) = (r(ci,j) - a(ci,j)*u(ci,j-1)) / bet(ci);
          }
        }
      }
      else if(col%itype(ci) /= icol_sunwall && col%itype(ci) /= icol_shadewall & && col%itype(ci) /= icol_roof){

        if(j >= jtop(ci)){

          if(j == jtop(ci)){
            u(ci,j) = r(ci,j) / bet(ci);
          }
          else{
            gam(ci,j) = c(ci,j-1) / bet(ci);
            bet(ci) = b(ci,j) - a(ci,j) * gam(ci,j);
            u(ci,j) = (r(ci,j) - a(ci,j)*u(ci,j-1)) / bet(ci)
          }

        }

      }
    }
  }

  //CHECK CONDITIONS - TODO
  for(int j=ubj-1; j<=lbj; j--){

    for(int fc=1; fc<=numf; fc++){
      ci = filter(fc);

      if((col%itype(ci) == icol_sunwall || col%itype(ci) == icol_shadewall & || col%itype(ci) == icol_roof) && j <= nlevurb-1){

        if(j >= jtop(ci)){
          u(ci,j) = u(ci,j) - gam(ci,j+1) * u(ci,j+1);
        }

      }
      else if(col%itype(ci) /= icol_sunwall && col%itype(ci) /= icol+shadewall & && col%itype(ci) /= icol_roof){

        if(j >= jtop(ci)){
          u(ci,j) = u(ci,j) - gam(ci,j+1) * u(ci,j+1);
        }

      }
    }

  }

}
*/


//This works on the continuous unfrozen column
//This collects already-known values into arrays for ease of use, and
// calculates basic values needed in the more complex equations later
// in Richards.
void Richards::prepareSoilColumn(Layer* currsoill, const double & draindepth) {
  //TODO rename currsoill to topactivel or something?

  // it is assumed that all layers in Richards will be unfrozen,
  // i.e., from unfrozen 'topsoill' to ''drainl'
  Layer* currl = currsoill; // the first soil layer is 'topsoill'

  int ind = -1;
  indx0al = currsoill->solind;
  numal = 0;

  //Determine if we're in spring or fall. This controls where
  // in the layer the frozen section is (in spring it's at the bottom,
  // in the fall it's at the top)

  //The soil node in a partially frozen layer will be calculated
  //differently if it's a thawing front vs a frozen front. If thawing,
  //the frozen slice of the layer will be a the bottom of the layer,
  //but if freezing, it will be at the top.
  bool spring;

  //If there is no previous layer, determine status from next layer
  if(currl->prevl == NULL){
    //The layer below is frozen
    if(currl->nextl->frozen>=0){ spring = true; }
    //The layer below is thawed
    else{ spring = false; }
  }
  //If there is a previous layer, determine status from it
  else{
    //The layer above is thawed/partially thawed
    if(currl->prevl->frozen <= 0){ spring = true; }
    //The layer above is frozen
    else{ spring = false; }
  }


  while(currl->solind <= drainl->solind){
  //while(currl->solind <= drainl->solind+1){

    ind = currl->solind;

    //Only increment number of active layers if we are in the
    // active soil stack
    if(ind >= indx0al && ind <= drainl->solind){
      numal++;
    }

    //Unfrozen fraction of the layer
    double frac_unfrozen = 1.0-currl->frozenfrac;

    //Thickness of the frozen fraction of the layer
    double dz_frozen = fmax(0., currl->dz*currl->frozenfrac);

    //Thickness of the unfrozen fraction of the layer
    double dz_unfrozen = fmax(0., currl->dz*frac_unfrozen);

    double minvolliq = currl->minliq/DENLIQ/currl->dz;
    //effporo[ind] = fmax(0., currl->poro-minvolliq);
    thetasat[ind] = fmax(0., currl->poro-minvolliq);
    dzmm[ind] = currl->dz*1.e3*frac_unfrozen;//fmin(frac_unfrozen, frac_unsat);
    if(dzmm[ind] <= 0){
      BOOST_LOG_SEV(glg, err)<<"dzmm less than zero: "<<dzmm[ind];
    }

    //Calculate depth of each interface (i.e. the bottom of each layer)
    // and the depth of each node (center of thawed section of each layer)
    if(spring){
      z_h[ind] = (currl->z + currl->dz - dz_frozen)*1.e3;
      nodemm[ind] = (currl->z + currl->dz - dz_frozen)*1.e3 - 0.5*dzmm[ind];
    }
    else{
      z_h[ind] = (currl->z + currl->dz)*1.e3;
      nodemm[ind]  = (currl->z+dz_frozen)*1.e3 + 0.5 *dzmm[ind]; 
    }
 
    //This is a weird formulation, but works out?
    effminliq[ind] = currl->minliq * frac_unfrozen;//*fmin(frac_unfrozen, frac_unsat);
    //effmaxliq[ind] = (effporo[ind]*dzmm[ind]);
    //effmaxliq[ind] = thetasat[ind] * dzmm[ind];
    effmaxliq[ind] = currl->maxliq * frac_unfrozen;

    //This is also a weird formulation, but works out?
    //effliq[ind] = fmax(0.0, currl->liq*frac_unsat-effminliq[ind]);
    //effliq is held to a very small number instead of zero in order
    // to avoid division by zero.
    effliq[ind] = fmax(0.0001, currl->liq-effminliq[ind]);

    if (effliq[ind]<0. || effminliq[ind]<0. || effmaxliq[ind]<0.) {
      BOOST_LOG_SEV(glg, warn) << "Richards::prepareSoilColumn(..) "
                               << "Effective liquid is less than zero!";
    }

    psisat[ind] = currl->psisat;
    ksat[ind] = currl->hksat;
    Bsw[ind] = currl->bsw;

    currl= currl->nextl;
  }

};

/*
void Richards::iterate(const double trans[], const double & evap,
                       const double & infil, const double & fbaseflow) {
  //
  tschanged = true;
  itsum = 0;
  tleft = 1.;    // at beginning of update, tleft is one timestep

  if(infil>0.) {
    TSTEPORG =TSTEPMAX/20.;
  } else {
    TSTEPORG =TSTEPMAX;
  }

  tstep = TSTEPORG;

  for(int il=indx0al; il<indx0al+numal; il++) {
    liqid[il] = effliq[il]; // liq at the begin of one day
    liqld[il] = effliq[il]; // the last determined liq
  }

  qdrain = 0.;   // for accumulate bottom drainage (mm/day)

  while(tleft>0.0) {
    for(int il=indx0al; il<indx0al+numal; il++) {
      liqis[il] = liqld[il];
    }

    //find one solution for one fraction of timestep
    int st = updateOnethTimeStep(fbaseflow);

    if(st==0 || (st!=0 && tstep<=TSTEPMIN)) {  //advance to next timestep
      qdrain += qout[numal]*tstep*timestep; //unit: mm/s*secs
      tleft -= tstep;

      // find the proper timestep for rest period
      if(!tschanged) { // if timestep has not been changed during last time step
        if(tstep<TSTEPMAX) {
          tstep = TSTEPORG;
          tschanged = true;
        }
      } else {
        tschanged =false;
      }

      // make sure tleft is greater than zero
      tstep = fmin(tleft, tstep);

      if(tstep<=0) {  //starting the next iterative-interval
        qdrain = 0.;
      }
    } else {
      tstep = tstep/2.0;   // half the iterative-interval

      if(tstep < TSTEPMIN) {
        tstep = TSTEPMIN;
      }

      tschanged = true;
    }
  } // end of while
};
*/
/*
int Richards::updateOnethTimeStep(const double &fbaseflow) {
  int status =-1;

  for(int i=indx0al; i<indx0al+numal; i++) {
    liqii[i] = liqis[i];
  }

  status = updateOneIteration(fbaseflow);

  if(status==0 || tstep<=TSTEPMIN) { // success OR at the min. tstep allowed
    for(int i=indx0al; i<indx0al+numal; i++) {
      liqld[i] = liqit[i];
    }
  }

  return status;
};
*/
/*
int Richards::updateOneIteration(const double &fbaseflow) {
  double effporo0;
  double effporo2;
  double volliq  = 0.;
  double volliq2 = 0.;;
  double s1;
  double s2;
  double s_node;
  double wimp = 0.001; // mimumum pore for water to exchange between two layers
  double smpmin = -1.e8;
  double dt =tstep*timestep;
  itsum++;

  //Yuan: k-dk/dw-h relationships for all soil layers
  for (int indx=indx0al; indx<indx0al+numal; indx++) {
    effporo0 = effporo[indx];
    volliq = fmax(0., liqii[indx]/dzmm[indx]);
    volliq = fmin(volliq, effporo0);

    if(indx==indx0al+numal-1) {
      s1 = volliq/fmax(wimp, effporo0);
      s2 = hksat[indx] * exp (-2.0*(zmm[indx]/1000.0))
           * pow(s1, 2.0*bsw[indx]+2.0);
      hk[indx] = s1*s2;
      dhkdw[indx] = (2.0*bsw[indx]+3.0)*s2*0.5/fmax(wimp, effporo0);
    } else {
      effporo2 = effporo[indx+1];
      volliq2 = fmax(0., liqii[indx+1]/dzmm[indx+1]);
      volliq2 = fmin(volliq2, effporo2);

      if(effporo0<wimp || effporo2<wimp) {
        hk[indx] = 0.;
        dhkdw[indx] = 0.;
      } else {
        s1 =(volliq2+volliq)/(effporo2+effporo0);
        s2 = hksat[indx+1] * exp (-2.0*(zmm[indx+1]/1000.0))
             * pow(s1, 2.0*bsw[indx+1]+2.0);
        hk[indx] = s1*s2;
        dhkdw[indx] = (2.*bsw[indx]+3.0)*s2*0.5/effporo2;
      }
    }

    if (hk[indx]>=numeric_limits<double>::infinity()
        || dhkdw[indx]>=numeric_limits<double>::infinity()) {
      BOOST_LOG_SEV(glg, warn) << "'hk' or 'dhkdw' is out of bounds!";
    }

    if (volliq>1.0 || volliq2>1.0) {
      BOOST_LOG_SEV(glg, warn) << "vwc is out of bounds! (volliq or volliq2 > 1.0)";
    }

    //
    s_node = volliq/fmax(wimp, effporo0);
    s_node = fmax(0.001, (double)s_node);
    s_node = fmin(1.0, (double)s_node);
    smp[indx] = psisat[indx]*pow(s_node, -bsw[indx]);
    smp[indx] = fmax(smpmin, smp[indx]);
    dsmpdw[indx]= -bsw[indx]*smp[indx]/(s_node*fmax(wimp,effporo0));

    //
    if (smp[indx]>=numeric_limits<double>::infinity()
        || dsmpdw[indx]>=numeric_limits<double>::infinity()) {
      BOOST_LOG_SEV(glg, warn) << "smp[<<"<<indx<<"] or dsmpdw["<<indx<<"] is infinity!";
    }
  }

  // preparing matrice for solution
  double den, num;
  double dqodw1, dqodw2, dqidw0, dqidw1;
  double sdamp =0.;
  int ind=indx0al;

  if(numal>=2) {
    // layer 1
    qin[ind] = 0.;

    if (ind == indx0sl) {//for first soil layer: infiltration/evaporation occurs
      qin[ind] = qinfil -qevap;
    }

    den = zmm[ind+1]-zmm[ind];
    num = smp[ind+1]-smp[ind]-den;
    qout[ind] = -hk[ind] * num/den;
    dqodw1 = -(-hk[ind]*dsmpdw[ind] + num*dhkdw[ind])/den;
    dqodw2 = -(hk[ind]*dsmpdw[ind+1] + num*dhkdw[ind])/den;
    rmx[ind] = qin[ind] - qout[ind] - qtrans[ind];
    amx[ind] = 0.;
    bmx[ind] = dzmm[ind] *(sdamp +1/dt) + dqodw1;
    cmx[ind] = dqodw2;

    if (numal>2) {
      for(ind=indx0al+1; ind<indx0al+numal-1; ind++) { // layer 2 ~ the second last bottom layer
        den = zmm[ind]-zmm[ind-1];
        num = smp[ind]-smp[ind-1] -den;
        qin[ind] = -hk[ind-1]*num/den;
        dqidw0 = -(-hk[ind-1]*dsmpdw[ind-1] + num* dhkdw[ind-1])/den;
        dqidw1 = -(hk[ind-1]*dsmpdw[ind] + num* dhkdw[ind-1])/den;
        den = zmm[ind+1]-zmm[ind];
        num = smp[ind+1]-smp[ind] -den;
        qout[ind] = -hk[ind] * num/den;
        dqodw1 = -(-hk[ind]*dsmpdw[ind] + num* dhkdw[ind])/den;
        dqodw2 = -(hk[ind]*dsmpdw[ind+1] + num* dhkdw[ind])/den;
        rmx[ind] = qin[ind] -qout[ind] -qtrans[ind];
        amx[ind] =-dqidw0;
        bmx[ind] = dzmm[ind] /dt - dqidw1 + dqodw1;
        cmx[ind] = dqodw2;

        if (amx[ind] != amx[ind] || bmx[ind] != bmx[ind] ||
            cmx[ind] != cmx[ind] || rmx[ind] != rmx[ind]) {
          BOOST_LOG_SEV(glg, warn) << "amx, cmx, bmx, or rmx at index "
                                   << ind << " is NaN!";
        }
      }
    }

    //bottom layer
    ind = indx0al+numal-1;
    den = zmm[ind]-zmm[ind-1];
    num = smp[ind]-smp[ind-1]-den;
    qin[ind] = -hk[ind-1]*num/den;
    dqidw0 = -(-hk[ind-1]*dsmpdw[ind-1] + num* dhkdw[ind-1])/den;
    dqidw1 = -(hk[ind-1]*dsmpdw[ind] + num* dhkdw[ind-1])/den;
    dqodw1 = dhkdw[ind];
    qout[ind] = 0.;   //no drainage occurs if not in 'drainl'

    if (ind==drainl->solind) {
      qout[ind] = hk[ind]*fbaseflow;    //free bottom drainage assumed
    }

    rmx[ind] = qin[ind] -qout[ind] -qtrans[ind];
    amx[ind] = -dqidw0;
    bmx[ind] = dzmm[ind]/dt - dqidw1 + dqodw1;
    cmx[ind] = 0.;
  }

  cn.tridiagonal(indx0al, numal, amx, bmx,cmx,rmx, dwat);  //solution

  // soil water for each layer after one iteration
  for(int il=indx0al; il<indx0al+numal; il++) {
    liqit[il] = liqii[il] + dzmm[il] * dwat[il];

    if(liqit[il]!=liqit[il]) {
      BOOST_LOG_SEV(glg, warn) << "Richards::updateOneIteration(..), water is NaN!";
    }

    if (liqit[il]>=numeric_limits<double>::infinity()) {
      BOOST_LOG_SEV(glg, warn) << "liqit["<<il<<"] is greater than infinity.";
    }
  }

  //check the iteration result to determine if need to continue
  for(int il=indx0al; il<indx0al+numal; il++) {
    /* // the '-1' and '-2' status appear causing yearly unstablitity - so removed
          if(liqit[il]<0.0){
            return -1;    // apparently slow down the iteration very much during drying
          }
          if(liqit[il]>effmaxliq[il]){
            return -2;    // apparently slow down the iteration very much during wetting
          }
    //
    if(fabs((liqit[il]-liqii[il])/effmaxliq[il])>LIQTOLE) {
      return -3;
    }
  }

  return 0;
};
*/

