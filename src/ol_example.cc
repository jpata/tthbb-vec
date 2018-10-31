#include <cstdlib>
#include <iostream>

#include "openloops.h"

using namespace std;

int main(int argc, char* argv[]) { 
  
  ol_setparameter_int("verbose", 4);


  if (argc != 4) {
    cerr << "Usage: ./ol_example order_ew order_qcd amptype" << endl;
    return 1;
  }
  int order_ew = atoi(argv[1]);
  int order_qcd = atoi(argv[2]);
  int amptype = atoi(argv[3]);

  if (order_ew != -1) {
    ol_setparameter_int("order_ew", order_ew);
  }
  if (order_qcd != -1) {
    ol_setparameter_int("order_qcd", order_qcd);
  }

  auto ol_proc_id_qqZ = ol_register_process("2 -2 -> 23", amptype);
  if (ol_proc_id_qqZ == -1) {
    cerr << "could not register process" << endl;
    return 1;
  }
  cout << "process registered: " << ol_proc_id_qqZ << endl;
  return 0;
}
