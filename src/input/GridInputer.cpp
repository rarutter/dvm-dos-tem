#include "GridInputer.h"

#include "../TEMUtilityFunctions.h"

GridInputer::GridInputer() {
};

GridInputer::~GridInputer() {
}

int GridInputer::init() {
  if(md!=NULL) {
    md->act_gridno  = initGrid(md->grdinputdir);
    md->act_drainno = initDrainType(md->grdinputdir);
    md->act_soilno  = initSoilTexture(md->grdinputdir);
    md->act_gfireno = initFireStatistics(md->grdinputdir);
  } else {
    std::string msg ="GridInputer::init - ModelData is NULL";
    cout<<msg+"\n";
    return -1;
  }

  return 0;
}

int GridInputer::initGrid(string& dir) {

  this->gridfname = dir + "grid.nc";

  NcFile grid_file = temutil::open_ncfile( this->gridfname );

  NcDim* grdD = temutil::get_ncdim(grid_file, "GRIDID");

  return grdD->size(); //actual grid number
}

int GridInputer::initSoilTexture(string& dir) {

  this->soilfname = dir + "soiltexture.nc";

  NcFile soil_file = temutil::open_ncfile( this->soilfname );

  NcDim* soilD = temutil::get_ncdim(soil_file, "SOILID");
  
  return soilD->size(); //actual soil dataset number
}

int GridInputer::initDrainType(string& dir) {

  this->drainfname = dir + "drainage.nc";

  NcFile drainage_file = temutil::open_ncfile( this->drainfname );
  
  NcDim* drainD = temutil::get_ncdim(drainage_file, "DRAINAGEID");
  
  return drainD->size(); //actual drainage type datset number

}

int GridInputer::initFireStatistics(string & dir) {

  this->gfirefname = dir + "firestatistics.nc";

  NcFile fire_file = temutil::open_ncfile( this->gfirefname );

  NcDim* gfireD = temutil::get_ncdim(fire_file, "GFIREID");

  NcDim* gfsizeD = temutil::get_ncdim(fire_file, "GFSIZENO");

  NcDim* gfseasonD = temutil::get_ncdim(fire_file, "GFSEASONNO");
  
  return gfireD->size();  //actual grid fire dataset number
}

//recno - the order (from ZERO) in the .nc file, ids - the real
//  ids in the *.nc files
int GridInputer::getGridids(int & grdid, int &grddrgid, int &grdsoilid,
                            int &grdfireid, const int & recno) {
  
  NcFile grid_file = temutil::open_ncfile(gridfname);
  
  NcVar* grididV = temutil::get_ncvar(grid_file, "GRIDID");
  grididV->set_cur(recno);
  grididV->get(&grdid, 1);
  
  NcVar* drgidV = temutil::get_ncvar(grid_file, "DRAINAGEID");
  drgidV->set_cur(recno);
  drgidV->get(&grddrgid, 1);

  NcVar* soilidV = temutil::get_ncvar(grid_file, "SOILID");
  soilidV->set_cur(recno);
  soilidV->get(&grdsoilid, 1);

  NcVar* gfireidV = temutil::get_ncvar(grid_file, "GFIREID");
  gfireidV->set_cur(recno);
  gfireidV->get(&grdfireid, 1);

  return 0;
}

int GridInputer::getDrainId(int & drainid, const int & recno) {

  NcFile drainage_file = temutil::open_ncfile(drainfname);

  NcVar* drainidV = temutil::get_ncvar(drainage_file, "DRAINAGEID");
  drainidV->set_cur(recno);
  drainidV->get(&drainid, 1);

  return 0;
}

int GridInputer::getSoilId(int & soilid, const int & recno) {

  NcFile soil_file = temutil::open_ncfile(soilfname);

  NcVar* soilidV = temutil::get_ncvar(soil_file, "SOILID");
  soilidV->set_cur(recno);
  soilidV->get(&soilid, 1);

  return 0;
}

int GridInputer::getGfireId(int &gfireid, const int & recno) {

  NcFile grid_fire_file = temutil::open_ncfile(gfirefname);

  NcVar* gfireidV = temutil::get_ncvar(grid_fire_file, "GFIREID");
  gfireidV->set_cur(recno);
  gfireidV->get(&gfireid, 1);

  return 0;
}

//recid - the order (from ZERO) in the .nc file, gridid - the grid
//  id user-defined in the dataset
void GridInputer::getLatlon(float & lat, float & lon, const int & recid ) {
  
  NcFile grid_file = temutil::open_ncfile(gridfname);

  NcVar* latV = temutil::get_ncvar(grid_file, "LAT");
  latV->set_cur(recid);
  latV->get(&lat, 1);

  NcVar* lonV = temutil::get_ncvar(grid_file, "LON");
  lonV->set_cur(recid);
  lonV->get(&lon, 1);
}

void GridInputer::getDrainType(int & dtype, const int & recid) {

  NcFile drainage_file = temutil::open_ncfile(drainfname);

  NcVar* drainV = temutil::get_ncvar(drainage_file, "DRAINAGETYPE");
  drainV->set_cur(recid);
  drainV->get(&dtype, 1);

}

void GridInputer::getSoilTexture(int & topsoil, int & botsoil,
                                 const int & recid ) {

  NcFile soil_file = temutil::open_ncfile(soilfname);

  NcVar* topsoilV = temutil::get_ncvar(soil_file, "TOPSOIL");
  topsoilV->set_cur(recid);
  topsoilV->get(&topsoil, 1);

  NcVar* botsoilV = temutil::get_ncvar(soil_file, "BOTSOIL");
  botsoilV->set_cur(recid);
  botsoilV->get(&botsoil, 1);
}

void GridInputer::getGfire(int &fri, double pfseason[],
                           double pfsize[], const int & recid ) {
  //the following are the currently used in model, and can be
  //  modified (actually suggested)
  int fseasonno = 4; //fire season class no.: 0 - pre-fireseason; 1 - early;
                     //                       2 - late; 3 - post-fireseason
  int fsizeno   = 5; //fire-size year class no.: 0 - small; 1 - intermediate;
                     //           2 - large; 3 - very large; 4 - ultra-large
  
  // Have to set the error to non-fatal, or things break here in
  // single site run while trying to deal with PFSEASON...
  NcError err(NcError::verbose_nonfatal);

  NcFile fire_file = temutil::open_ncfile(this->gfirefname);

  NcVar* friV = temutil::get_ncvar(fire_file, "FRI");
  friV->set_cur(recid);
  friV->get(&fri, 1);

  NcVar* fseasonV = temutil::get_ncvar(fire_file, "PFSEASON");
  fseasonV->set_cur(recid);
  fseasonV->get(&pfseason[0], fseasonno);

  NcVar* pfsizeV = temutil::get_ncvar(fire_file, "PFSIZE");
  pfsizeV->set_cur(recid);
  pfsizeV->get(&pfsize[0], fsizeno);

};

void GridInputer::setModelData(ModelData* mdp) {
  md = mdp;
};




