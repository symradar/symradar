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
  long long x = rvals[0];
  long long y = rvals[1];
  long long constant_a;
  int patch_results[132];
  // Patch buggy # 0
  result = (0);
  uni_klee_add_patch(patch_results, 0, result);
  // Patch 1-0 # 1
  constant_a = -2;
  result = (constant_a == x);
  uni_klee_add_patch(patch_results, 1, result);
  // Patch 2-0 # 2
  result = (x == x);
  uni_klee_add_patch(patch_results, 2, result);
  // Patch 3-0 # 3
  result = (x < y);
  uni_klee_add_patch(patch_results, 3, result);
  // Patch 4-0 # 4
  result = (y != x);
  uni_klee_add_patch(patch_results, 4, result);
  // Patch 5-0 # 5
  result = (x <= y);
  uni_klee_add_patch(patch_results, 5, result);
  // Patch 6-0 # 6
  constant_a = -10;
  result = (constant_a < x);
  uni_klee_add_patch(patch_results, 6, result);
  // Patch 6-1 # 7
  constant_a = -9;
  result = (constant_a < x);
  uni_klee_add_patch(patch_results, 7, result);
  // Patch 6-2 # 8
  constant_a = -8;
  result = (constant_a < x);
  uni_klee_add_patch(patch_results, 8, result);
  // Patch 6-3 # 9
  constant_a = -7;
  result = (constant_a < x);
  uni_klee_add_patch(patch_results, 9, result);
  // Patch 6-4 # 10
  constant_a = -6;
  result = (constant_a < x);
  uni_klee_add_patch(patch_results, 10, result);
  // Patch 6-5 # 11
  constant_a = -5;
  result = (constant_a < x);
  uni_klee_add_patch(patch_results, 11, result);
  // Patch 6-6 # 12
  constant_a = -4;
  result = (constant_a < x);
  uni_klee_add_patch(patch_results, 12, result);
  // Patch 6-7 # 13
  constant_a = -3;
  result = (constant_a < x);
  uni_klee_add_patch(patch_results, 13, result);
  // Patch 7-0 # 14
  constant_a = -10;
  result = (constant_a <= x);
  uni_klee_add_patch(patch_results, 14, result);
  // Patch 7-1 # 15
  constant_a = -9;
  result = (constant_a <= x);
  uni_klee_add_patch(patch_results, 15, result);
  // Patch 7-2 # 16
  constant_a = -8;
  result = (constant_a <= x);
  uni_klee_add_patch(patch_results, 16, result);
  // Patch 7-3 # 17
  constant_a = -7;
  result = (constant_a <= x);
  uni_klee_add_patch(patch_results, 17, result);
  // Patch 7-4 # 18
  constant_a = -6;
  result = (constant_a <= x);
  uni_klee_add_patch(patch_results, 18, result);
  // Patch 7-5 # 19
  constant_a = -5;
  result = (constant_a <= x);
  uni_klee_add_patch(patch_results, 19, result);
  // Patch 7-6 # 20
  constant_a = -4;
  result = (constant_a <= x);
  uni_klee_add_patch(patch_results, 20, result);
  // Patch 7-7 # 21
  constant_a = -3;
  result = (constant_a <= x);
  uni_klee_add_patch(patch_results, 21, result);
  // Patch 7-8 # 22
  constant_a = -2;
  result = (constant_a <= x);
  uni_klee_add_patch(patch_results, 22, result);
  // Patch 8-0 # 23
  constant_a = -1;
  result = (x < constant_a);
  uni_klee_add_patch(patch_results, 23, result);
  // Patch 8-1 # 24
  constant_a = 0;
  result = (x < constant_a);
  uni_klee_add_patch(patch_results, 24, result);
  // Patch 8-2 # 25
  constant_a = 1;
  result = (x < constant_a);
  uni_klee_add_patch(patch_results, 25, result);
  // Patch 8-3 # 26
  constant_a = 2;
  result = (x < constant_a);
  uni_klee_add_patch(patch_results, 26, result);
  // Patch 8-4 # 27
  constant_a = 3;
  result = (x < constant_a);
  uni_klee_add_patch(patch_results, 27, result);
  // Patch 8-5 # 28
  constant_a = 4;
  result = (x < constant_a);
  uni_klee_add_patch(patch_results, 28, result);
  // Patch 8-6 # 29
  constant_a = 5;
  result = (x < constant_a);
  uni_klee_add_patch(patch_results, 29, result);
  // Patch 8-7 # 30
  constant_a = 6;
  result = (x < constant_a);
  uni_klee_add_patch(patch_results, 30, result);
  // Patch 8-8 # 31
  constant_a = 7;
  result = (x < constant_a);
  uni_klee_add_patch(patch_results, 31, result);
  // Patch 8-9 # 32
  constant_a = 8;
  result = (x < constant_a);
  uni_klee_add_patch(patch_results, 32, result);
  // Patch 8-10 # 33
  constant_a = 9;
  result = (x < constant_a);
  uni_klee_add_patch(patch_results, 33, result);
  // Patch 8-11 # 34
  constant_a = 10;
  result = (x < constant_a);
  uni_klee_add_patch(patch_results, 34, result);
  // Patch 9-0 # 35
  constant_a = -2;
  result = (x <= constant_a);
  uni_klee_add_patch(patch_results, 35, result);
  // Patch 9-1 # 36
  constant_a = -1;
  result = (x <= constant_a);
  uni_klee_add_patch(patch_results, 36, result);
  // Patch 9-2 # 37
  constant_a = 0;
  result = (x <= constant_a);
  uni_klee_add_patch(patch_results, 37, result);
  // Patch 9-3 # 38
  constant_a = 1;
  result = (x <= constant_a);
  uni_klee_add_patch(patch_results, 38, result);
  // Patch 9-4 # 39
  constant_a = 2;
  result = (x <= constant_a);
  uni_klee_add_patch(patch_results, 39, result);
  // Patch 9-5 # 40
  constant_a = 3;
  result = (x <= constant_a);
  uni_klee_add_patch(patch_results, 40, result);
  // Patch 9-6 # 41
  constant_a = 4;
  result = (x <= constant_a);
  uni_klee_add_patch(patch_results, 41, result);
  // Patch 9-7 # 42
  constant_a = 5;
  result = (x <= constant_a);
  uni_klee_add_patch(patch_results, 42, result);
  // Patch 9-8 # 43
  constant_a = 6;
  result = (x <= constant_a);
  uni_klee_add_patch(patch_results, 43, result);
  // Patch 9-9 # 44
  constant_a = 7;
  result = (x <= constant_a);
  uni_klee_add_patch(patch_results, 44, result);
  // Patch 9-10 # 45
  constant_a = 8;
  result = (x <= constant_a);
  uni_klee_add_patch(patch_results, 45, result);
  // Patch 9-11 # 46
  constant_a = 9;
  result = (x <= constant_a);
  uni_klee_add_patch(patch_results, 46, result);
  // Patch 9-12 # 47
  constant_a = 10;
  result = (x <= constant_a);
  uni_klee_add_patch(patch_results, 47, result);
  // Patch 10-0 # 48
  constant_a = -10;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 48, result);
  // Patch 10-1 # 49
  constant_a = -9;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 49, result);
  // Patch 10-2 # 50
  constant_a = -8;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 50, result);
  // Patch 10-3 # 51
  constant_a = -7;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 51, result);
  // Patch 10-4 # 52
  constant_a = -6;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 52, result);
  // Patch 10-5 # 53
  constant_a = -5;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 53, result);
  // Patch 10-6 # 54
  constant_a = -4;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 54, result);
  // Patch 10-7 # 55
  constant_a = -3;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 55, result);
  // Patch 10-8 # 56
  constant_a = -1;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 56, result);
  // Patch 10-9 # 57
  constant_a = 0;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 57, result);
  // Patch 10-10 # 58
  constant_a = 1;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 58, result);
  // Patch 10-11 # 59
  constant_a = 2;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 59, result);
  // Patch 10-12 # 60
  constant_a = 3;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 60, result);
  // Patch 10-13 # 61
  constant_a = 4;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 61, result);
  // Patch 10-14 # 62
  constant_a = 5;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 62, result);
  // Patch 10-15 # 63
  constant_a = 6;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 63, result);
  // Patch 10-16 # 64
  constant_a = 7;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 64, result);
  // Patch 10-17 # 65
  constant_a = 8;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 65, result);
  // Patch 10-18 # 66
  constant_a = 9;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 66, result);
  // Patch 10-19 # 67
  constant_a = 10;
  result = (constant_a != x);
  uni_klee_add_patch(patch_results, 67, result);
  // Patch 11-0 # 68
  constant_a = -10;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 68, result);
  // Patch 11-1 # 69
  constant_a = -9;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 69, result);
  // Patch 11-2 # 70
  constant_a = -8;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 70, result);
  // Patch 11-3 # 71
  constant_a = -7;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 71, result);
  // Patch 11-4 # 72
  constant_a = -6;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 72, result);
  // Patch 11-5 # 73
  constant_a = -5;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 73, result);
  // Patch 11-6 # 74
  constant_a = -4;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 74, result);
  // Patch 11-7 # 75
  constant_a = -3;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 75, result);
  // Patch 11-8 # 76
  constant_a = -2;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 76, result);
  // Patch 11-9 # 77
  constant_a = -1;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 77, result);
  // Patch 11-10 # 78
  constant_a = 0;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 78, result);
  // Patch 11-11 # 79
  constant_a = 1;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 79, result);
  // Patch 11-12 # 80
  constant_a = 2;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 80, result);
  // Patch 11-13 # 81
  constant_a = 3;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 81, result);
  // Patch 11-14 # 82
  constant_a = 4;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 82, result);
  // Patch 11-15 # 83
  constant_a = 5;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 83, result);
  // Patch 11-16 # 84
  constant_a = 6;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 84, result);
  // Patch 11-17 # 85
  constant_a = 7;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 85, result);
  // Patch 11-18 # 86
  constant_a = 8;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 86, result);
  // Patch 11-19 # 87
  constant_a = 9;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 87, result);
  // Patch 11-20 # 88
  constant_a = 10;
  result = (constant_a < y);
  uni_klee_add_patch(patch_results, 88, result);
  // Patch 12-0 # 89
  constant_a = -10;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 89, result);
  // Patch 12-1 # 90
  constant_a = -9;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 90, result);
  // Patch 12-2 # 91
  constant_a = -8;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 91, result);
  // Patch 12-3 # 92
  constant_a = -7;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 92, result);
  // Patch 12-4 # 93
  constant_a = -6;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 93, result);
  // Patch 12-5 # 94
  constant_a = -5;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 94, result);
  // Patch 12-6 # 95
  constant_a = -4;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 95, result);
  // Patch 12-7 # 96
  constant_a = -3;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 96, result);
  // Patch 12-8 # 97
  constant_a = -2;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 97, result);
  // Patch 12-9 # 98
  constant_a = -1;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 98, result);
  // Patch 12-10 # 99
  constant_a = 0;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 99, result);
  // Patch 12-11 # 100
  constant_a = 1;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 100, result);
  // Patch 12-12 # 101
  constant_a = 2;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 101, result);
  // Patch 12-13 # 102
  constant_a = 3;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 102, result);
  // Patch 12-14 # 103
  constant_a = 4;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 103, result);
  // Patch 12-15 # 104
  constant_a = 5;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 104, result);
  // Patch 12-16 # 105
  constant_a = 6;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 105, result);
  // Patch 12-17 # 106
  constant_a = 7;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 106, result);
  // Patch 12-18 # 107
  constant_a = 8;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 107, result);
  // Patch 12-19 # 108
  constant_a = 9;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 108, result);
  // Patch 12-20 # 109
  constant_a = 10;
  result = (y != constant_a);
  uni_klee_add_patch(patch_results, 109, result);
  // Patch 13-0 # 110
  constant_a = -10;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 110, result);
  // Patch 13-1 # 111
  constant_a = -9;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 111, result);
  // Patch 13-2 # 112
  constant_a = -8;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 112, result);
  // Patch 13-3 # 113
  constant_a = -7;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 113, result);
  // Patch 13-4 # 114
  constant_a = -6;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 114, result);
  // Patch 13-5 # 115
  constant_a = -5;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 115, result);
  // Patch 13-6 # 116
  constant_a = -4;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 116, result);
  // Patch 13-7 # 117
  constant_a = -3;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 117, result);
  // Patch 13-8 # 118
  constant_a = -2;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 118, result);
  // Patch 13-9 # 119
  constant_a = -1;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 119, result);
  // Patch 13-10 # 120
  constant_a = 0;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 120, result);
  // Patch 13-11 # 121
  constant_a = 1;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 121, result);
  // Patch 13-12 # 122
  constant_a = 2;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 122, result);
  // Patch 13-13 # 123
  constant_a = 3;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 123, result);
  // Patch 13-14 # 124
  constant_a = 4;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 124, result);
  // Patch 13-15 # 125
  constant_a = 5;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 125, result);
  // Patch 13-16 # 126
  constant_a = 6;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 126, result);
  // Patch 13-17 # 127
  constant_a = 7;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 127, result);
  // Patch 13-18 # 128
  constant_a = 8;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 128, result);
  // Patch 13-19 # 129
  constant_a = 9;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 129, result);
  // Patch 13-20 # 130
  constant_a = 10;
  result = (constant_a <= y);
  uni_klee_add_patch(patch_results, 130, result);
  // Patch correct # 131
  result = (x <= 0);
  uni_klee_add_patch(patch_results, 131, result);
  klee_select_patch(&uni_klee_patch_id);
  return uni_klee_choice(patch_results, uni_klee_patch_id);
}
// UNI_KLEE_END

int __cpr_output(char* id, char* typestr, int value){
  return value;
}
