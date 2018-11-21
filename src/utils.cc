#include "utils.h"

TLorentzVector make_lv(float pt, float eta, float phi, float mass) {
  TLorentzVector lv;
  lv.SetPtEtaPhiM(pt, eta, phi, mass);
  return lv;
}