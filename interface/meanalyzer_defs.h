MatrixElementHiggsMuMu::MatrixElementHiggsMuMu(
    const std::string& openloops_path) {
  // gluon gluon -> Higgs, 12 = loop^2
  ol_setparameter_string("install_path", openloops_path.c_str());
  //ol_setparameter_int("verbose", 3);

  //two-loop level
  ol_proc_id_ggh = ol_register_process("21 21 -> 25", 12);
  if (ol_proc_id_ggh == -1) {
    throw runtime_error("could not register process");
  }

  //one-loop level
  ol_proc_id_qqZ = ol_register_process("2 -2 -> 23", 11);
  if (ol_proc_id_qqZ == -1) {
    throw runtime_error("could not register process");
  }

  ol_start();
}

array<double, 15> MatrixElementHiggsMuMu::make_phase_space_from_mediator(TLorentzVector lv_mediator) {
  lv_mediator.Boost(-TVector3(lv_mediator.Px()/lv_mediator.E(), lv_mediator.Py()/lv_mediator.E(), 0));  
  const auto E = lv_mediator.E();
  const auto pz = lv_mediator.Pz();

  //split mediator momentum equally between two parent particles
  const TLorentzVector i1(0, 0, (E + pz) / 2.0, (E + pz) / 2.0);
  const TLorentzVector i2(0, 0, -(E - pz) / 2.0, (E - pz) / 2.0);
  return make_phase_space_3(i1, i2, lv_mediator);
}

array<double, 15> MatrixElementHiggsMuMu::make_phase_space_from_final(TLorentzVector mu1, TLorentzVector mu2) {
  const auto lv_mediator = mu1 + mu2;
  return make_phase_space_from_mediator(lv_mediator);
}

array<double, 15> MatrixElementHiggsMuMu::make_phase_space_3(
    array<double, 5> g1, array<double, 5> g2, array<double, 5> h) {
  array<double, 15> phase_space_point;

  // Set initial state momentum (E,Px,Py,Pz,M)
  phase_space_point[0 + 0] = g1[0];
  phase_space_point[0 + 1] = g1[1];
  phase_space_point[0 + 2] = g1[2];
  phase_space_point[0 + 3] = g1[3];
  phase_space_point[0 + 4] = g1[4];

  phase_space_point[5 + 0] = g2[0];
  phase_space_point[5 + 1] = g2[1];
  phase_space_point[5 + 2] = g2[2];
  phase_space_point[5 + 3] = g2[3];
  phase_space_point[5 + 4] = g2[4];

  // Set Higgs momentum (E,Px,Py,Pz,M)
  phase_space_point[10 + 0] = h[0];
  phase_space_point[10 + 1] = h[1];
  phase_space_point[10 + 2] = h[2];
  phase_space_point[10 + 3] = h[3];
  phase_space_point[10 + 4] = h[4];

  return phase_space_point;
}

array<double, 15> MatrixElementHiggsMuMu::make_phase_space_3(
    const TLorentzVector& p1, const TLorentzVector& p2, const TLorentzVector& p3) {
  return make_phase_space_3({p1.Energy(), p1.Px(), p1.Py(), p1.Pz(), p1.M()},
                              {p2.Energy(), p2.Px(), p2.Py(), p2.Pz(), p2.M()},
                              {p3.Energy(), p3.Px(), p3.Py(), p3.Pz(), p3.M()});
}

double MatrixElementHiggsMuMu::compute_aplitude_ggh(
    array<double, 15> phase_space_point) {
  double m2_tree, m2_loop[3], acc;
  m2_tree = 0.0;
  m2_loop[0] = 0.0;
  m2_loop[1] = 0.0;
  m2_loop[2] = 0.0;
  acc = 0.0;
  ol_evaluate_loop2(ol_proc_id_ggh, phase_space_point.data(), &m2_tree, m2_loop,
                    &acc);

  return m2_loop[0];
}

double MatrixElementHiggsMuMu::compute_aplitude_qqZ(
    array<double, 15> phase_space_point) {
  double m2_tree, m2_loop[3], acc;
  m2_tree = 0.0;
  m2_loop[0] = 0.0;
  m2_loop[1] = 0.0;
  m2_loop[2] = 0.0;
  acc = 0.0;
  ol_evaluate_loop(ol_proc_id_qqZ, phase_space_point.data(), &m2_tree, m2_loop, &acc);
  return m2_loop[0];
}