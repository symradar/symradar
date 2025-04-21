#include "uni_klee_runtime.h"
#include <stdlib.h>
#include <stdio.h>

int uni_klee_patch_id;

void klee_select_patch(int *patch_id) {
  *patch_id = atoi(getenv("DAFL_PATCH_ID"));
}

void uni_klee_add_patch(int *patch_results, int patch_id, int result) {
  patch_results[patch_id] = result;
}

int uni_klee_choice(int *patch_results, int patch_id) {
  FILE *fp=fopen(getenv("DAFL_RESULT_FILE"),"a");
  if (fp == NULL) {
    fprintf(stderr, "Error opening file!\n");
    exit(1);
  }
  fprintf(fp, "%d ", patch_results[patch_id]);
  fclose(fp);
  return patch_results[patch_id];
}

// UNI_KLEE_START
int __cpr_choice(char* lid, char* typestr,
                     long long* rvals, char** rvals_ids, int rvals_size,
                     int** lvals, char** lvals_ids, int lvals_size){
  // int patch_results[4096];
  int result;
  long long col_sep_length = rvals[0];
  long long constant_a;
  int patch_results[129];
  // Patch buggy # 0
  result = (1);
  uni_klee_add_patch(patch_results, 0, result);
  // Patch 1-0 # 1
  constant_a = -10;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 1, result);
  // Patch 1-1 # 2
  constant_a = -9;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 2, result);
  // Patch 1-2 # 3
  constant_a = -8;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 3, result);
  // Patch 1-3 # 4
  constant_a = -7;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 4, result);
  // Patch 1-4 # 5
  constant_a = -6;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 5, result);
  // Patch 1-5 # 6
  constant_a = -5;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 6, result);
  // Patch 1-6 # 7
  constant_a = -4;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 7, result);
  // Patch 1-7 # 8
  constant_a = -3;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 8, result);
  // Patch 1-8 # 9
  constant_a = -2;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 9, result);
  // Patch 1-9 # 10
  constant_a = -1;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 10, result);
  // Patch 1-10 # 11
  constant_a = 0;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 11, result);
  // Patch 1-11 # 12
  constant_a = 1;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 12, result);
  // Patch 1-12 # 13
  constant_a = 2;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 13, result);
  // Patch 1-13 # 14
  constant_a = 3;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 14, result);
  // Patch 1-14 # 15
  constant_a = 4;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 15, result);
  // Patch 1-15 # 16
  constant_a = 5;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 16, result);
  // Patch 1-16 # 17
  constant_a = 6;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 17, result);
  // Patch 1-17 # 18
  constant_a = 7;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 18, result);
  // Patch 1-18 # 19
  constant_a = 8;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 19, result);
  // Patch 1-19 # 20
  constant_a = 9;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 20, result);
  // Patch 1-20 # 21
  constant_a = 10;
  result = (constant_a == col_sep_length);
  uni_klee_add_patch(patch_results, 21, result);
  // Patch 2-0 # 22
  result = (col_sep_length != col_sep_length);
  uni_klee_add_patch(patch_results, 22, result);
  // Patch 3-0 # 23
  constant_a = -10;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 23, result);
  // Patch 3-1 # 24
  constant_a = -9;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 24, result);
  // Patch 3-2 # 25
  constant_a = -8;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 25, result);
  // Patch 3-3 # 26
  constant_a = -7;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 26, result);
  // Patch 3-4 # 27
  constant_a = -6;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 27, result);
  // Patch 3-5 # 28
  constant_a = -5;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 28, result);
  // Patch 3-6 # 29
  constant_a = -4;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 29, result);
  // Patch 3-7 # 30
  constant_a = -3;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 30, result);
  // Patch 3-8 # 31
  constant_a = -2;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 31, result);
  // Patch 3-9 # 32
  constant_a = -1;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 32, result);
  // Patch 3-10 # 33
  constant_a = 0;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 33, result);
  // Patch 3-11 # 34
  constant_a = 1;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 34, result);
  // Patch 3-12 # 35
  constant_a = 2;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 35, result);
  // Patch 3-13 # 36
  constant_a = 3;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 36, result);
  // Patch 3-14 # 37
  constant_a = 4;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 37, result);
  // Patch 3-15 # 38
  constant_a = 5;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 38, result);
  // Patch 3-16 # 39
  constant_a = 6;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 39, result);
  // Patch 3-17 # 40
  constant_a = 7;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 40, result);
  // Patch 3-18 # 41
  constant_a = 8;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 41, result);
  // Patch 3-19 # 42
  constant_a = 9;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 42, result);
  // Patch 3-20 # 43
  constant_a = 10;
  result = (constant_a != col_sep_length);
  uni_klee_add_patch(patch_results, 43, result);
  // Patch 4-0 # 44
  constant_a = -10;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 44, result);
  // Patch 4-1 # 45
  constant_a = -9;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 45, result);
  // Patch 4-2 # 46
  constant_a = -8;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 46, result);
  // Patch 4-3 # 47
  constant_a = -7;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 47, result);
  // Patch 4-4 # 48
  constant_a = -6;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 48, result);
  // Patch 4-5 # 49
  constant_a = -5;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 49, result);
  // Patch 4-6 # 50
  constant_a = -4;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 50, result);
  // Patch 4-7 # 51
  constant_a = -3;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 51, result);
  // Patch 4-8 # 52
  constant_a = -2;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 52, result);
  // Patch 4-9 # 53
  constant_a = -1;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 53, result);
  // Patch 4-10 # 54
  constant_a = 0;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 54, result);
  // Patch 4-11 # 55
  constant_a = 1;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 55, result);
  // Patch 4-12 # 56
  constant_a = 2;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 56, result);
  // Patch 4-13 # 57
  constant_a = 3;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 57, result);
  // Patch 4-14 # 58
  constant_a = 4;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 58, result);
  // Patch 4-15 # 59
  constant_a = 5;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 59, result);
  // Patch 4-16 # 60
  constant_a = 6;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 60, result);
  // Patch 4-17 # 61
  constant_a = 7;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 61, result);
  // Patch 4-18 # 62
  constant_a = 8;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 62, result);
  // Patch 4-19 # 63
  constant_a = 9;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 63, result);
  // Patch 4-20 # 64
  constant_a = 10;
  result = (constant_a < col_sep_length);
  uni_klee_add_patch(patch_results, 64, result);
  // Patch 5-0 # 65
  constant_a = -10;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 65, result);
  // Patch 5-1 # 66
  constant_a = -9;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 66, result);
  // Patch 5-2 # 67
  constant_a = -8;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 67, result);
  // Patch 5-3 # 68
  constant_a = -7;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 68, result);
  // Patch 5-4 # 69
  constant_a = -6;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 69, result);
  // Patch 5-5 # 70
  constant_a = -5;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 70, result);
  // Patch 5-6 # 71
  constant_a = -4;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 71, result);
  // Patch 5-7 # 72
  constant_a = -3;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 72, result);
  // Patch 5-8 # 73
  constant_a = -2;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 73, result);
  // Patch 5-9 # 74
  constant_a = -1;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 74, result);
  // Patch 5-10 # 75
  constant_a = 0;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 75, result);
  // Patch 5-11 # 76
  constant_a = 1;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 76, result);
  // Patch 5-12 # 77
  constant_a = 2;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 77, result);
  // Patch 5-13 # 78
  constant_a = 3;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 78, result);
  // Patch 5-14 # 79
  constant_a = 4;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 79, result);
  // Patch 5-15 # 80
  constant_a = 5;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 80, result);
  // Patch 5-16 # 81
  constant_a = 6;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 81, result);
  // Patch 5-17 # 82
  constant_a = 7;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 82, result);
  // Patch 5-18 # 83
  constant_a = 8;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 83, result);
  // Patch 5-19 # 84
  constant_a = 9;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 84, result);
  // Patch 5-20 # 85
  constant_a = 10;
  result = (col_sep_length < constant_a);
  uni_klee_add_patch(patch_results, 85, result);
  // Patch 6-0 # 86
  constant_a = -10;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 86, result);
  // Patch 6-1 # 87
  constant_a = -9;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 87, result);
  // Patch 6-2 # 88
  constant_a = -8;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 88, result);
  // Patch 6-3 # 89
  constant_a = -7;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 89, result);
  // Patch 6-4 # 90
  constant_a = -6;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 90, result);
  // Patch 6-5 # 91
  constant_a = -5;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 91, result);
  // Patch 6-6 # 92
  constant_a = -4;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 92, result);
  // Patch 6-7 # 93
  constant_a = -3;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 93, result);
  // Patch 6-8 # 94
  constant_a = -2;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 94, result);
  // Patch 6-9 # 95
  constant_a = -1;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 95, result);
  // Patch 6-10 # 96
  constant_a = 0;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 96, result);
  // Patch 6-11 # 97
  constant_a = 1;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 97, result);
  // Patch 6-12 # 98
  constant_a = 2;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 98, result);
  // Patch 6-13 # 99
  constant_a = 3;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 99, result);
  // Patch 6-14 # 100
  constant_a = 4;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 100, result);
  // Patch 6-15 # 101
  constant_a = 5;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 101, result);
  // Patch 6-16 # 102
  constant_a = 6;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 102, result);
  // Patch 6-17 # 103
  constant_a = 7;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 103, result);
  // Patch 6-18 # 104
  constant_a = 8;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 104, result);
  // Patch 6-19 # 105
  constant_a = 9;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 105, result);
  // Patch 6-20 # 106
  constant_a = 10;
  result = (constant_a <= col_sep_length);
  uni_klee_add_patch(patch_results, 106, result);
  // Patch 7-0 # 107
  constant_a = -10;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 107, result);
  // Patch 7-1 # 108
  constant_a = -9;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 108, result);
  // Patch 7-2 # 109
  constant_a = -8;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 109, result);
  // Patch 7-3 # 110
  constant_a = -7;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 110, result);
  // Patch 7-4 # 111
  constant_a = -6;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 111, result);
  // Patch 7-5 # 112
  constant_a = -5;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 112, result);
  // Patch 7-6 # 113
  constant_a = -4;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 113, result);
  // Patch 7-7 # 114
  constant_a = -3;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 114, result);
  // Patch 7-8 # 115
  constant_a = -2;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 115, result);
  // Patch 7-9 # 116
  constant_a = -1;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 116, result);
  // Patch 7-10 # 117
  constant_a = 0;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 117, result);
  // Patch 7-11 # 118
  constant_a = 1;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 118, result);
  // Patch 7-12 # 119
  constant_a = 2;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 119, result);
  // Patch 7-13 # 120
  constant_a = 3;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 120, result);
  // Patch 7-14 # 121
  constant_a = 4;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 121, result);
  // Patch 7-15 # 122
  constant_a = 5;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 122, result);
  // Patch 7-16 # 123
  constant_a = 6;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 123, result);
  // Patch 7-17 # 124
  constant_a = 7;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 124, result);
  // Patch 7-18 # 125
  constant_a = 8;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 125, result);
  // Patch 7-19 # 126
  constant_a = 9;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 126, result);
  // Patch 7-20 # 127
  constant_a = 10;
  result = (col_sep_length <= constant_a);
  uni_klee_add_patch(patch_results, 127, result);
  // Patch correct # 128
  result = (col_sep_length == 1);
  uni_klee_add_patch(patch_results, 128, result);
  klee_select_patch(&uni_klee_patch_id);
  return uni_klee_choice(patch_results, uni_klee_patch_id);
}
// UNI_KLEE_END

int __cpr_output(char* id, char* typestr, int value){
  return value;
}
