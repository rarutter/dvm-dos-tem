
/* When running a region with many concurrent batches, outputting at every
 * year timestep creates a bottleneck in the run. Output at the monthly
 * timestep is correspondingly worse.
 *
 * This class is a basic repository for output data to be held until
 * outputting is triggered based on the user's specifications.
 * */


#ifndef OUTPUTHOLDER_H
#define OUTPUTHOLDER_H

#include "temconst.h"
#include "layerconst.h"

class OutputHolder{
public:

//  OutputHolder();
//  ~OutputHolder();

  void clear();

  int months_held;
  int years_held;

  std::vector<int> ysd_for_output;

  std::vector<double> ald_for_output;
  std::vector<double> burnsoil2airc_for_output;
  std::vector<double> burnthick_for_output;
  std::vector<double> burnveg2airc_for_output;
  std::vector<double> burnveg2deadc_for_output;
  std::vector<double> burnveg2soilabvc_for_output;
  std::vector<double> burnveg2soilblwc_for_output;
  std::vector<double> ch4efflux_for_output;
  std::vector<double> ch4effdiff_for_output;
  std::vector<double> ch4pool_for_output;
  std::vector<double> deadc_for_output;
  std::vector<double> deepc_for_output;
  std::vector<double> dwdc_for_output;
  std::vector<double> dwdrh_for_output;
  std::vector<double> minec_for_output;
  std::vector<double> qdrain_for_output;
  std::vector<double> qinfil_for_output;
  std::vector<double> qrunoff_for_output;
  std::vector<double> shlwc_for_output;
  std::vector<double> snowthick_for_output;
  std::vector<double> swe_for_output;
  std::vector<double> watertab_for_output;

  //Variables by layer
  std::vector<double> rh_tot_for_output;
  std::vector<std::array<double, MAX_SOI_LAY>> rh_for_output;

  std::vector<double> ch4diff_sum_for_output;
  std::vector<std::array<double, MAX_SOI_LAY>> ch4diff_layer_for_output;

  std::vector<double> ch4ebul_sum_for_output;
  std::vector<std::array<double, MAX_SOI_LAY>> ch4ebul_layer_for_output;

  std::vector<double> ch4oxid_sum_for_output;
  std::vector<std::array<double, MAX_SOI_LAY>> ch4oxid_layer_for_output;

  std::vector<std::array<double, MAX_SOI_LAY>> ch4pool_layer_for_output;

  std::vector<double> ch4prod_sum_for_output;
  std::vector<std::array<double, MAX_SOI_LAY>> ch4prod_layer_for_output;

  std::vector<double> ch4transport_sum_for_output;
  std::vector<std::array<double, MAX_SOI_LAY>> ch4transport_layer_for_output;
  std::vector<std::array<std::array<double, NUM_PFT>, MAX_SOI_LAY>> ch4transport_for_output; 

  std::vector<std::array<double, MAX_SOI_LAY>> layerdepth_for_output;
  std::vector<std::array<double, MAX_SOI_LAY>> layerdz_for_output;
  std::vector<std::array<int, MAX_SOI_LAY>> layertype_for_output;
  std::vector<std::array<double, MAX_SOI_LAY>> tlayer_for_output;
  std::vector<std::array<double, MAX_SOI_LAY>> vwclayer_for_output;

  std::vector<std::array<double, MAX_SNW_LAY>> snowlayerdz_for_output;
  std::vector<std::array<double, MAX_SNW_LAY>> snowlayertemp_for_output;

  //Variables by PFT
  std::vector<double> eet_tot_for_output;
  std::vector<std::array<double, NUM_PFT>> eet_for_output;

  std::vector<double> pet_tot_for_output;
  std::vector<std::array<double, NUM_PFT>> pet_for_output;

  std::vector<double> trans_tot_for_output; //transpiration, not transport
  std::vector<std::array<double, NUM_PFT>> trans_for_output;

  //Variables by PFT and Compartment both
  std::vector<double> gpp_tot_for_output;
  std::vector<std::array<double, NUM_PFT_PART>> gpp_part_for_output;
  std::vector<std::array<double, NUM_PFT>> gpp_pft_for_output;
  std::vector<std::array<std::array<double, NUM_PFT>, NUM_PFT_PART>> gpp_for_output; 

  std::vector<double> ltrfalc_tot_for_output;
  std::vector<std::array<double, NUM_PFT_PART>> ltrfalc_part_for_output;
  std::vector<std::array<double, NUM_PFT>> ltrfalc_pft_for_output;
  std::vector<std::array<std::array<double, NUM_PFT>, NUM_PFT_PART>> ltrfalc_for_output; 

  std::vector<double> npp_tot_for_output;
  std::vector<std::array<double, NUM_PFT_PART>> npp_part_for_output;
  std::vector<std::array<double, NUM_PFT>> npp_pft_for_output;
  std::vector<std::array<std::array<double, NUM_PFT>, NUM_PFT_PART>> npp_for_output; 

  std::vector<double> vegc_tot_for_output;
  std::vector<std::array<double, NUM_PFT_PART>> vegc_part_for_output;
  std::vector<std::array<double, NUM_PFT>> vegc_pft_for_output;
  std::vector<std::array<std::array<double, NUM_PFT>, NUM_PFT_PART>> vegc_for_output; 

};


#endif /* OUTPUTHOLDER_H */
