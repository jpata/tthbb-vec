#ifndef MATRIXELEMENTHIGGSMUMU_H
#define MATRIXELEMENTHIGGSMUMU_H

#include <array>

#include "TLorentzVector.h"

//Include the standalone amplitude libraries from madgraph
//These were created using the following MG5 commands:

//MG5_aMC>import model sm-full
//MG5_aMC>add model hgg_plugin
//MG5_aMC>generate g g > h , h > mu+ mu-
//MG5_aMC>add process d d~ > z , z > mu+ mu-
//MG5_aMC>output standalone_cpp OUT_hmumu
//and then renaming CPPProcess -> ProcessQQZ or ProcessGGH
#include "ProcessQQZ.h"
#include "ProcessGGH.h"

//declarations
class MatrixElementHiggsMuMu {
 public:
  MatrixElementHiggsMuMu(string mg_card_path);

  //number of external particles (2 initial state, 2 final state)
  static const int num_external = 4;

  //number of dimensions per particle (E, px, py, pz) 
  static const int num_p4 = 4;

  //number of dimenstions in phase space
  static const int num_ps = num_external * num_p4;

  //Cartesian four-momentum
  typedef array<double, num_p4> p4;

  //Phase space point
  typedef array<double, num_ps> pspoint;

  //Given a phase space point, compute the amplitude of the given process (gg->H->mumu, qqZ->Z->mumu)
  double compute_aplitude_gghmumu(pspoint phase_space_point);
  double compute_aplitude_qqZmumu(pspoint phase_space_point);

  //Convenience functions to prepare the phase space point, given the 4-momenta of the initial and final state particles
  //Four momenta should be in the LAB frame
  pspoint make_phase_space_4(p4 i1, p4 i2, p4 f1, p4 f2);
  pspoint make_phase_space_4_lv(const TLorentzVector& i1, const TLorentzVector& i2, const TLorentzVector& f1, const TLorentzVector& f2);

 private:
  //Madgraph standalone processes
  ProcessQQZ proc_qqz;
  ProcessGGH proc_ggh;
};
#endif