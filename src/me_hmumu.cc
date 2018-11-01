#include "me_hmumu.h"

MatrixElementHiggsMuMu::MatrixElementHiggsMuMu(string mg_card_path) {
  proc_qqz.initProc(mg_card_path.c_str());
  proc_ggh.initProc(mg_card_path.c_str());
}

MatrixElementHiggsMuMu::pspoint MatrixElementHiggsMuMu::make_phase_space_4(
    MatrixElementHiggsMuMu::p4 i1, MatrixElementHiggsMuMu::p4 i2, MatrixElementHiggsMuMu::p4 f1, MatrixElementHiggsMuMu::p4 f2) {
  array<double, MatrixElementHiggsMuMu::num_external*MatrixElementHiggsMuMu::num_p4> phase_space_point;

  // Set initial state momentum (E,Px,Py,Pz,M)
  phase_space_point[0 * MatrixElementHiggsMuMu::num_p4 + 0] = i1[0];
  phase_space_point[0 * MatrixElementHiggsMuMu::num_p4 + 1] = i1[1];
  phase_space_point[0 * MatrixElementHiggsMuMu::num_p4 + 2] = i1[2];
  phase_space_point[0 * MatrixElementHiggsMuMu::num_p4 + 3] = i1[3];

  phase_space_point[1 * MatrixElementHiggsMuMu::num_p4 + 0] = i2[0];
  phase_space_point[1 * MatrixElementHiggsMuMu::num_p4 + 1] = i2[1];
  phase_space_point[1 * MatrixElementHiggsMuMu::num_p4 + 2] = i2[2];
  phase_space_point[1 * MatrixElementHiggsMuMu::num_p4 + 3] = i2[3];

  // Set final state momentum (E,Px,Py,Pz,M)
  phase_space_point[2 * MatrixElementHiggsMuMu::num_p4 + 0] = f1[0];
  phase_space_point[2 * MatrixElementHiggsMuMu::num_p4 + 1] = f1[1];
  phase_space_point[2 * MatrixElementHiggsMuMu::num_p4 + 2] = f1[2];
  phase_space_point[2 * MatrixElementHiggsMuMu::num_p4 + 3] = f1[3];

  phase_space_point[3 * MatrixElementHiggsMuMu::num_p4 + 0] = f2[0];
  phase_space_point[3 * MatrixElementHiggsMuMu::num_p4 + 1] = f2[1];
  phase_space_point[3 * MatrixElementHiggsMuMu::num_p4 + 2] = f2[2];
  phase_space_point[3 * MatrixElementHiggsMuMu::num_p4 + 3] = f2[3];

  return phase_space_point;
}

MatrixElementHiggsMuMu::pspoint MatrixElementHiggsMuMu::make_phase_space_4_lv(
    const TLorentzVector& i1, const TLorentzVector& i2, const TLorentzVector& f1, const TLorentzVector& f2) {
  return MatrixElementHiggsMuMu::make_phase_space_4({i1.Energy(), i1.Px(), i1.Py(), i1.Pz()},
                              {i2.Energy(), i2.Px(), i2.Py(), i2.Pz()},
                              {f1.Energy(), f1.Px(), f1.Py(), f1.Pz()},
                              {f2.Energy(), f2.Px(), f2.Py(), f2.Pz()});
}

double MatrixElementHiggsMuMu::compute_aplitude_gghmumu(
    MatrixElementHiggsMuMu::pspoint phase_space_point) {
  
  vector<double*> p;
  p.push_back(&(phase_space_point[0]));
  p.push_back(&(phase_space_point[4]));
  p.push_back(&(phase_space_point[8]));
  p.push_back(&(phase_space_point[12]));

  proc_ggh.setMomenta(p);
  proc_ggh.sigmaKin();
  const double* me = proc_ggh.getMatrixElements();
  return me[0];
}

double MatrixElementHiggsMuMu::compute_aplitude_qqZmumu(
    MatrixElementHiggsMuMu::pspoint phase_space_point) {

  vector<double*> p;
  p.push_back(&(phase_space_point[0]));
  p.push_back(&(phase_space_point[4]));
  p.push_back(&(phase_space_point[8]));
  p.push_back(&(phase_space_point[12]));

  proc_qqz.setMomenta(p);
  proc_qqz.sigmaKin();
  const double* me = proc_qqz.getMatrixElements();
  return me[0];
}