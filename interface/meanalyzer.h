#ifndef MATRIXELEMENTHIGGSMUMU_H
#define MATRIXELEMENTHIGGSMUMU_H

#include <array>

#include "TLorentzVector.h"

// OpenLoops Interface
extern "C" {
void ol_setparameter_int(const char* param, int val);
void ol_setparameter_string(const char* param, const char* arg);
void ol_setparameter_double(const char* param, double val);
int ol_register_process(const char* process, int amptype);
int ol_n_external(int id);
void ol_phase_space_point(int id, double sqrt_s, double* pp);
void ol_start();
void ol_finish();

void ol_evaluate_tree(int id, double* pp, double* m2_tree);
void ol_evaluate_loop(int id, double* pp, double* m2_tree, double* m2_loop,
                       double* acc);
void ol_evaluate_loop2(int id, double* pp, double* m2_tree, double* m2_loop,
                       double* acc);
}

//declarations
class MatrixElementHiggsMuMu {
 public:
  MatrixElementHiggsMuMu(const std::string& openloops_path);

  double compute_aplitude_ggh(array<double, 15> phase_space_point);
  double compute_aplitude_qqZ(array<double, 15> phase_space_point);

  array<double, 15> make_phase_space_3(array<double, 5> g1, array<double, 5> g2,
                                       array<double, 5> h);
  array<double, 15> make_phase_space_3(const TLorentzVector& p1, const TLorentzVector& p2,
                                       const TLorentzVector& p3);

  array<double, 15> make_phase_space_from_final(TLorentzVector mu1, TLorentzVector mu2);
  array<double, 15> make_phase_space_from_mediator(TLorentzVector med);

 private:
  int ol_proc_id_ggh;
  int ol_proc_id_qqZ;
};
#endif