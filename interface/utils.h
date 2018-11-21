// In this file, we declare simple helper functions
#ifndef NANOFLOW_UTILS_H
#define NANOFLOW_UTILS_H

#include <TLorentzVector.h>

// Helper function to create a TLorentzVector from spherical coordinates
TLorentzVector make_lv(float pt, float eta, float phi, float mass);

#endif