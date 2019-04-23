/*! \file
 *
 */
#ifndef PEATLAYER_H_
#define PEATLAYER_H_
#include "SoilLayer.h"
#include "physicalconst.h"

#include <string>
#include <cmath>
using namespace std;

class PeatLayer: public SoilLayer {
public:

  PeatLayer(const double & pdz, const int & upper);

  void humify();
  void fireConvert(SoilLayer* sl);

};
#endif /*PEATLAYER_H_*/
