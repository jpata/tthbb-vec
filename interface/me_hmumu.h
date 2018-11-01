#ifndef MATRIXELEMENTHIGGSMUMU_H
#define MATRIXELEMENTHIGGSMUMU_H

#include <array>

#include "TLorentzVector.h"
#include "ProcessQQZ.h"
#include "ProcessGGH.h"

//MG5_aMC>import model sm-full
//MG5_aMC>add model hgg_plugin
//MG5_aMC>generate g g > h , h > mu+ mu-
//MG5_aMC>add process d d~ > z , z > mu+ mu-
//MG5_aMC>output standalone_cpp OUT_hmumu

//declarations
class MatrixElementHiggsMuMu {
 public:
  MatrixElementHiggsMuMu(string mg_card_path);
  static const int num_external = 4;
  static const int num_p4 = 4;
  static const int num_ps = num_external * num_p4;

  typedef array<double, num_p4> p4;
  typedef array<double, num_ps> pspoint;

  double compute_aplitude_gghmumu(pspoint phase_space_point);
  double compute_aplitude_qqZmumu(pspoint phase_space_point);

  pspoint make_phase_space_4(p4 i1, p4 i2, p4 f1, p4 f2);
  pspoint make_phase_space_4_lv(const TLorentzVector& i1, const TLorentzVector& i2, const TLorentzVector& f1, const TLorentzVector& f2);

 private:
  ProcessQQZ proc_qqz;
  ProcessGGH proc_ggh;
};
#endif