//
// Created by nikhil on 12/11/2019.
//

#ifndef CPR_RUNTIME_H
#define CPR_RUNTIME_H


#define CPR_OUTPUT(id, typestr, value) \
  __cpr_output(id, typestr, value);

#ifdef DAFL_ASSERT
#define klee_assert(x) assert(x)
#define fabs_cpr(x) fabs(x)
#endif
  
int __cpr_choice(char* lid, char* typestr,
                    long long* rvals, char** rvals_ids, int rvals_size,
                    int** lvals, char** lvals_ids, int lvals_size);

int __cpr_output(char* id, char* typestr, int value);


#endif //CPR_RUNTIME_H
