#include "uni_klee_runtime.h"

int uni_klee_patch_id;

void klee_select_patch(int *patch_id) {
  *patch_id = 0;
}

// UNI_KLEE_START
int __cpr_choice(char* lid, char* typestr,
                     long long* rvals, char** rvals_ids, int rvals_size,
                     int** lvals, char** lvals_ids, int lvals_size){
  klee_select_patch(&uni_klee_patch_id);
  int result;
  // REPLACE
  return result;
}
// UNI_KLEE_END

int __cpr_output(char* id, char* typestr, int value){
  return value;
}
