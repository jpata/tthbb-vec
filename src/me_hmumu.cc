#include "me_hmumu.h"

using namespace std;

MatrixElementHiggsMuMu::MatrixElementHiggsMuMu(string mg_card_path) : calibration_file(new TFile("data/mem_calibration.root"))
{
  proc_qqz.initProc(mg_card_path.c_str());
  proc_ggh.initProc(mg_card_path.c_str());

  ggh_pdf_logpz = (TH1D*)(*calibration_file).Get("ggh_pdf_logpz");
  qqZ_pdf_logpz = (TH1D*)(*calibration_file).Get("qqz_pdf_logpz");
  assert(ggh_pdf_logpz != nullptr);
  assert(qqZ_pdf_logpz != nullptr);
}

double MatrixElementHiggsMuMu::compute_me_final_mumu_hypo(TLorentzVector total_fs, TLorentzVector f1, TLorentzVector f2, TH1D* h_logpz, function<double(MatrixElementHiggsMuMu::pspoint)> compute_amplitude) {
  const auto E = total_fs.E();
  const auto pz = total_fs.Pz();
  
  const auto rnd = h_logpz->GetRandom();
  const auto pz_i1 = exp(rnd);
  const auto pz_i2 = pz - pz_i1;

  TLorentzVector i1_reco(0, 0, pz_i1, abs(pz_i1));
  TLorentzVector i2_reco(0, 0, pz_i2, abs(pz_i2));

  const auto phase_space_point = make_phase_space_4_lv(i1_reco, i2_reco, f1, f2);
  double amp = compute_amplitude(phase_space_point);
  return amp;
}

MEValues MatrixElementHiggsMuMu::compute_me_final_mumu(TLorentzVector f1, TLorentzVector f2) {
  auto total_fs = f1 + f2;
  auto boost_beta = -TVector3(total_fs.Px()/total_fs.E(), total_fs.Py()/total_fs.E(), 0.0);
  f1.Boost(boost_beta);
  f2.Boost(boost_beta);
  total_fs = f1 + f2;

  MEValues ret;  

  ret.ggh_hmumu = compute_me_final_mumu_hypo(total_fs, f1, f2, ggh_pdf_logpz, [this](MatrixElementHiggsMuMu::pspoint ps){ return this->compute_aplitude_gghmumu(ps); });
  ret.qqz_zmumu = compute_me_final_mumu_hypo(total_fs, f1, f2, qqZ_pdf_logpz, [this](MatrixElementHiggsMuMu::pspoint ps){ return this->compute_aplitude_qqZmumu(ps); });
  return ret;
}

MEValues MatrixElementHiggsMuMu::compute_me_initial_final_mumu(TLorentzVector i1, TLorentzVector i2, TLorentzVector f1, TLorentzVector f2) {
  const auto phase_space_point = make_phase_space_4_lv(i1, i2, f1, f2);
  MEValues ret;
  ret.ggh_hmumu = compute_aplitude_gghmumu(phase_space_point);
  ret.qqz_zmumu = compute_aplitude_qqZmumu(phase_space_point);
  return ret;
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
