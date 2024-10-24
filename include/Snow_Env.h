#ifndef SNOW_ENV_H_
#define SNOW_ENV_H_

#include "errorcode.h"

#include "CohortData.h"
#include "EnvData.h"
#include "RestartData.h"

#include "Ground.h"

#include "CohortLookup.h"

class Snow_Env {
public:
  Snow_Env();
  ~Snow_Env();

  double wind;     /*! the wind speed class (0, 1)? of a vegetation type*/

  // parameters for snow physics
  snwpar_env snowenvpar;

  void setGround(Ground* grndp);

  void setCohortLookup(CohortLookup* chtlup);
  void setCohortData(CohortData* cdp);
  void setEnvData(EnvData* edp);

  void initializeParameter();
  void initializeNewSnowState();
  void set_state_from_restartdata(const RestartData & rdata);

  void updateDailyM(const double & tdrv);
  void checkSnowLayersT(Layer* frontl);

private:

  CohortLookup * chtlu;
  CohortData * cd;
  EnvData * ed;

  Ground * ground;

  void updateSnowEd(Layer * frontl);
  double meltSnowLayersAfterT(Layer * frontl);

  void updateDailySurfFlux( Layer* frontl, const double & tdrv);

  double getSublimation(double const & rn, double const & swe,
                        double const & ta);

  double getAlbedoVis(const double & tem); //get albedo of visible
                                           //  radition of snow
  double getAlbedoNir(const double & tem); //get albedo of Nir radition of snow

};

#endif /*SNOW_ENV_H_*/
