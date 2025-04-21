#include "uni_klee_runtime.h"

int uni_klee_patch_id;

void klee_select_patch(int *patch_id) {
  *patch_id = 0;
}

void uni_klee_add_patch(int *patch_results, int patch_id, int result) {
  patch_results[patch_id] = result;
}

int uni_klee_choice(int *patch_results, int patch_id) {
  return patch_results[patch_id];
}

// UNI_KLEE_START
int __cpr_choice(char* lid, char* typestr,
                     long long* rvals, char** rvals_ids, int rvals_size,
                     int** lvals, char** lvals_ids, int lvals_size){
  // int patch_results[4096];
  int result;
  long long size = rvals[0];
  long long i = rvals[1];
  long long constant_a;
  int patch_results[772];
  // Patch buggy # 0
  result = (1);
  uni_klee_add_patch(patch_results, 0, result);
  // Patch 1-0 # 1
  result = (i == i);
  uni_klee_add_patch(patch_results, 1, result);
  // Patch 2-0 # 2
  result = (size == i);
  uni_klee_add_patch(patch_results, 2, result);
  // Patch 3-0 # 3
  constant_a = -10;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 3, result);
  // Patch 3-1 # 4
  constant_a = -9;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 4, result);
  // Patch 3-2 # 5
  constant_a = -8;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 5, result);
  // Patch 3-3 # 6
  constant_a = -7;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 6, result);
  // Patch 3-4 # 7
  constant_a = -6;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 7, result);
  // Patch 3-5 # 8
  constant_a = -5;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 8, result);
  // Patch 3-6 # 9
  constant_a = -4;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 9, result);
  // Patch 3-7 # 10
  constant_a = -3;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 10, result);
  // Patch 3-8 # 11
  constant_a = -2;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 11, result);
  // Patch 3-9 # 12
  constant_a = -1;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 12, result);
  // Patch 3-10 # 13
  constant_a = 0;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 13, result);
  // Patch 3-11 # 14
  constant_a = 1;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 14, result);
  // Patch 3-12 # 15
  constant_a = 2;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 15, result);
  // Patch 3-13 # 16
  constant_a = 3;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 16, result);
  // Patch 3-14 # 17
  constant_a = 4;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 17, result);
  // Patch 3-15 # 18
  constant_a = 5;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 18, result);
  // Patch 3-16 # 19
  constant_a = 6;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 19, result);
  // Patch 3-17 # 20
  constant_a = 7;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 20, result);
  // Patch 3-18 # 21
  constant_a = 8;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 21, result);
  // Patch 3-19 # 22
  constant_a = 9;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 22, result);
  // Patch 3-20 # 23
  constant_a = 10;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 23, result);
  // Patch 4-0 # 24
  result = ((size + i) == i);
  uni_klee_add_patch(patch_results, 24, result);
  // Patch 5-0 # 25
  constant_a = -10;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 25, result);
  // Patch 5-1 # 26
  constant_a = -9;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 26, result);
  // Patch 5-2 # 27
  constant_a = -8;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 27, result);
  // Patch 5-3 # 28
  constant_a = -7;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 28, result);
  // Patch 5-4 # 29
  constant_a = -6;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 29, result);
  // Patch 5-5 # 30
  constant_a = -5;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 30, result);
  // Patch 5-6 # 31
  constant_a = -4;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 31, result);
  // Patch 5-7 # 32
  constant_a = -3;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 32, result);
  // Patch 5-8 # 33
  constant_a = -2;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 33, result);
  // Patch 5-9 # 34
  constant_a = -1;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 34, result);
  // Patch 5-10 # 35
  constant_a = 0;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 35, result);
  // Patch 5-11 # 36
  constant_a = 1;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 36, result);
  // Patch 5-12 # 37
  constant_a = 2;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 37, result);
  // Patch 5-13 # 38
  constant_a = 3;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 38, result);
  // Patch 5-14 # 39
  constant_a = 4;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 39, result);
  // Patch 5-15 # 40
  constant_a = 5;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 40, result);
  // Patch 5-16 # 41
  constant_a = 6;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 41, result);
  // Patch 5-17 # 42
  constant_a = 7;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 42, result);
  // Patch 5-18 # 43
  constant_a = 8;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 43, result);
  // Patch 5-19 # 44
  constant_a = 9;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 44, result);
  // Patch 5-20 # 45
  constant_a = 10;
  result = ((constant_a + i) == i);
  uni_klee_add_patch(patch_results, 45, result);
  // Patch 6-0 # 46
  constant_a = -10;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 46, result);
  // Patch 6-1 # 47
  constant_a = -9;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 47, result);
  // Patch 6-2 # 48
  constant_a = -8;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 48, result);
  // Patch 6-3 # 49
  constant_a = -7;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 49, result);
  // Patch 6-4 # 50
  constant_a = -6;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 50, result);
  // Patch 6-5 # 51
  constant_a = -5;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 51, result);
  // Patch 6-6 # 52
  constant_a = -4;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 52, result);
  // Patch 6-7 # 53
  constant_a = -3;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 53, result);
  // Patch 6-8 # 54
  constant_a = -2;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 54, result);
  // Patch 6-9 # 55
  constant_a = -1;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 55, result);
  // Patch 6-10 # 56
  constant_a = 0;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 56, result);
  // Patch 6-11 # 57
  constant_a = 1;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 57, result);
  // Patch 6-12 # 58
  constant_a = 2;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 58, result);
  // Patch 6-13 # 59
  constant_a = 3;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 59, result);
  // Patch 6-14 # 60
  constant_a = 4;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 60, result);
  // Patch 6-15 # 61
  constant_a = 5;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 61, result);
  // Patch 6-16 # 62
  constant_a = 6;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 62, result);
  // Patch 6-17 # 63
  constant_a = 7;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 63, result);
  // Patch 6-18 # 64
  constant_a = 8;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 64, result);
  // Patch 6-19 # 65
  constant_a = 9;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 65, result);
  // Patch 6-20 # 66
  constant_a = 10;
  result = ((constant_a + size) == i);
  uni_klee_add_patch(patch_results, 66, result);
  // Patch 7-0 # 67
  constant_a = -10;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 67, result);
  // Patch 7-1 # 68
  constant_a = -9;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 68, result);
  // Patch 7-2 # 69
  constant_a = -8;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 69, result);
  // Patch 7-3 # 70
  constant_a = -7;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 70, result);
  // Patch 7-4 # 71
  constant_a = -6;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 71, result);
  // Patch 7-5 # 72
  constant_a = -5;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 72, result);
  // Patch 7-6 # 73
  constant_a = -4;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 73, result);
  // Patch 7-7 # 74
  constant_a = -3;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 74, result);
  // Patch 7-8 # 75
  constant_a = -2;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 75, result);
  // Patch 7-9 # 76
  constant_a = -1;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 76, result);
  // Patch 7-10 # 77
  constant_a = 0;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 77, result);
  // Patch 7-11 # 78
  constant_a = 1;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 78, result);
  // Patch 7-12 # 79
  constant_a = 2;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 79, result);
  // Patch 7-13 # 80
  constant_a = 3;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 80, result);
  // Patch 7-14 # 81
  constant_a = 4;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 81, result);
  // Patch 7-15 # 82
  constant_a = 5;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 82, result);
  // Patch 7-16 # 83
  constant_a = 6;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 83, result);
  // Patch 7-17 # 84
  constant_a = 7;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 84, result);
  // Patch 7-18 # 85
  constant_a = 8;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 85, result);
  // Patch 7-19 # 86
  constant_a = 9;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 86, result);
  // Patch 7-20 # 87
  constant_a = 10;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 87, result);
  // Patch 8-0 # 88
  result = ((size + i) == size);
  uni_klee_add_patch(patch_results, 88, result);
  // Patch 9-0 # 89
  constant_a = -10;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 89, result);
  // Patch 9-1 # 90
  constant_a = -9;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 90, result);
  // Patch 9-2 # 91
  constant_a = -8;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 91, result);
  // Patch 9-3 # 92
  constant_a = -7;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 92, result);
  // Patch 9-4 # 93
  constant_a = -6;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 93, result);
  // Patch 9-5 # 94
  constant_a = -5;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 94, result);
  // Patch 9-6 # 95
  constant_a = -4;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 95, result);
  // Patch 9-7 # 96
  constant_a = -3;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 96, result);
  // Patch 9-8 # 97
  constant_a = -2;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 97, result);
  // Patch 9-9 # 98
  constant_a = -1;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 98, result);
  // Patch 9-10 # 99
  constant_a = 0;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 99, result);
  // Patch 9-11 # 100
  constant_a = 1;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 100, result);
  // Patch 9-12 # 101
  constant_a = 2;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 101, result);
  // Patch 9-13 # 102
  constant_a = 3;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 102, result);
  // Patch 9-14 # 103
  constant_a = 4;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 103, result);
  // Patch 9-15 # 104
  constant_a = 5;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 104, result);
  // Patch 9-16 # 105
  constant_a = 6;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 105, result);
  // Patch 9-17 # 106
  constant_a = 7;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 106, result);
  // Patch 9-18 # 107
  constant_a = 8;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 107, result);
  // Patch 9-19 # 108
  constant_a = 9;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 108, result);
  // Patch 9-20 # 109
  constant_a = 10;
  result = ((constant_a + i) == size);
  uni_klee_add_patch(patch_results, 109, result);
  // Patch 10-0 # 110
  constant_a = -10;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 110, result);
  // Patch 10-1 # 111
  constant_a = -9;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 111, result);
  // Patch 10-2 # 112
  constant_a = -8;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 112, result);
  // Patch 10-3 # 113
  constant_a = -7;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 113, result);
  // Patch 10-4 # 114
  constant_a = -6;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 114, result);
  // Patch 10-5 # 115
  constant_a = -5;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 115, result);
  // Patch 10-6 # 116
  constant_a = -4;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 116, result);
  // Patch 10-7 # 117
  constant_a = -3;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 117, result);
  // Patch 10-8 # 118
  constant_a = -2;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 118, result);
  // Patch 10-9 # 119
  constant_a = -1;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 119, result);
  // Patch 10-10 # 120
  constant_a = 0;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 120, result);
  // Patch 10-11 # 121
  constant_a = 1;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 121, result);
  // Patch 10-12 # 122
  constant_a = 2;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 122, result);
  // Patch 10-13 # 123
  constant_a = 3;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 123, result);
  // Patch 10-14 # 124
  constant_a = 4;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 124, result);
  // Patch 10-15 # 125
  constant_a = 5;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 125, result);
  // Patch 10-16 # 126
  constant_a = 6;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 126, result);
  // Patch 10-17 # 127
  constant_a = 7;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 127, result);
  // Patch 10-18 # 128
  constant_a = 8;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 128, result);
  // Patch 10-19 # 129
  constant_a = 9;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 129, result);
  // Patch 10-20 # 130
  constant_a = 10;
  result = ((constant_a + size) == size);
  uni_klee_add_patch(patch_results, 130, result);
  // Patch 11-0 # 131
  constant_a = -10;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 131, result);
  // Patch 11-1 # 132
  constant_a = -9;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 132, result);
  // Patch 11-2 # 133
  constant_a = -8;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 133, result);
  // Patch 11-3 # 134
  constant_a = -7;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 134, result);
  // Patch 11-4 # 135
  constant_a = -6;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 135, result);
  // Patch 11-5 # 136
  constant_a = -5;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 136, result);
  // Patch 11-6 # 137
  constant_a = -4;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 137, result);
  // Patch 11-7 # 138
  constant_a = -3;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 138, result);
  // Patch 11-8 # 139
  constant_a = -2;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 139, result);
  // Patch 11-9 # 140
  constant_a = -1;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 140, result);
  // Patch 11-10 # 141
  constant_a = 0;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 141, result);
  // Patch 11-11 # 142
  constant_a = 1;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 142, result);
  // Patch 11-12 # 143
  constant_a = 2;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 143, result);
  // Patch 11-13 # 144
  constant_a = 3;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 144, result);
  // Patch 11-14 # 145
  constant_a = 4;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 145, result);
  // Patch 11-15 # 146
  constant_a = 5;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 146, result);
  // Patch 11-16 # 147
  constant_a = 6;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 147, result);
  // Patch 11-17 # 148
  constant_a = 7;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 148, result);
  // Patch 11-18 # 149
  constant_a = 8;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 149, result);
  // Patch 11-19 # 150
  constant_a = 9;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 150, result);
  // Patch 11-20 # 151
  constant_a = 10;
  result = ((size + i) == constant_a);
  uni_klee_add_patch(patch_results, 151, result);
  // Patch 12-0 # 152
  constant_a = -10;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 152, result);
  // Patch 12-1 # 153
  constant_a = -9;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 153, result);
  // Patch 12-2 # 154
  constant_a = -8;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 154, result);
  // Patch 12-3 # 155
  constant_a = -7;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 155, result);
  // Patch 12-4 # 156
  constant_a = -6;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 156, result);
  // Patch 12-5 # 157
  constant_a = -5;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 157, result);
  // Patch 12-6 # 158
  constant_a = -4;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 158, result);
  // Patch 12-7 # 159
  constant_a = -3;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 159, result);
  // Patch 12-8 # 160
  constant_a = -2;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 160, result);
  // Patch 12-9 # 161
  constant_a = -1;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 161, result);
  // Patch 12-10 # 162
  constant_a = 0;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 162, result);
  // Patch 12-11 # 163
  constant_a = 1;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 163, result);
  // Patch 12-12 # 164
  constant_a = 2;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 164, result);
  // Patch 12-13 # 165
  constant_a = 3;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 165, result);
  // Patch 12-14 # 166
  constant_a = 4;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 166, result);
  // Patch 12-15 # 167
  constant_a = 5;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 167, result);
  // Patch 12-16 # 168
  constant_a = 6;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 168, result);
  // Patch 12-17 # 169
  constant_a = 7;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 169, result);
  // Patch 12-18 # 170
  constant_a = 8;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 170, result);
  // Patch 12-19 # 171
  constant_a = 9;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 171, result);
  // Patch 12-20 # 172
  constant_a = 10;
  result = ((constant_a + i) == constant_a);
  uni_klee_add_patch(patch_results, 172, result);
  // Patch 13-0 # 173
  constant_a = -10;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 173, result);
  // Patch 13-1 # 174
  constant_a = -9;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 174, result);
  // Patch 13-2 # 175
  constant_a = -8;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 175, result);
  // Patch 13-3 # 176
  constant_a = -7;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 176, result);
  // Patch 13-4 # 177
  constant_a = -6;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 177, result);
  // Patch 13-5 # 178
  constant_a = -5;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 178, result);
  // Patch 13-6 # 179
  constant_a = -4;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 179, result);
  // Patch 13-7 # 180
  constant_a = -3;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 180, result);
  // Patch 13-8 # 181
  constant_a = -2;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 181, result);
  // Patch 13-9 # 182
  constant_a = -1;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 182, result);
  // Patch 13-10 # 183
  constant_a = 0;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 183, result);
  // Patch 13-11 # 184
  constant_a = 1;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 184, result);
  // Patch 13-12 # 185
  constant_a = 2;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 185, result);
  // Patch 13-13 # 186
  constant_a = 3;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 186, result);
  // Patch 13-14 # 187
  constant_a = 4;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 187, result);
  // Patch 13-15 # 188
  constant_a = 5;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 188, result);
  // Patch 13-16 # 189
  constant_a = 6;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 189, result);
  // Patch 13-17 # 190
  constant_a = 7;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 190, result);
  // Patch 13-18 # 191
  constant_a = 8;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 191, result);
  // Patch 13-19 # 192
  constant_a = 9;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 192, result);
  // Patch 13-20 # 193
  constant_a = 10;
  result = ((constant_a + size) == constant_a);
  uni_klee_add_patch(patch_results, 193, result);
  // Patch 14-0 # 194
  result = (i != i);
  uni_klee_add_patch(patch_results, 194, result);
  // Patch 15-0 # 195
  result = (size != i);
  uni_klee_add_patch(patch_results, 195, result);
  // Patch 16-0 # 196
  constant_a = -10;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 196, result);
  // Patch 16-1 # 197
  constant_a = -9;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 197, result);
  // Patch 16-2 # 198
  constant_a = -8;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 198, result);
  // Patch 16-3 # 199
  constant_a = -7;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 199, result);
  // Patch 16-4 # 200
  constant_a = -6;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 200, result);
  // Patch 16-5 # 201
  constant_a = -5;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 201, result);
  // Patch 16-6 # 202
  constant_a = -4;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 202, result);
  // Patch 16-7 # 203
  constant_a = -3;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 203, result);
  // Patch 16-8 # 204
  constant_a = -2;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 204, result);
  // Patch 16-9 # 205
  constant_a = -1;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 205, result);
  // Patch 16-10 # 206
  constant_a = 0;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 206, result);
  // Patch 16-11 # 207
  constant_a = 1;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 207, result);
  // Patch 16-12 # 208
  constant_a = 2;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 208, result);
  // Patch 16-13 # 209
  constant_a = 3;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 209, result);
  // Patch 16-14 # 210
  constant_a = 4;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 210, result);
  // Patch 16-15 # 211
  constant_a = 5;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 211, result);
  // Patch 16-16 # 212
  constant_a = 6;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 212, result);
  // Patch 16-17 # 213
  constant_a = 7;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 213, result);
  // Patch 16-18 # 214
  constant_a = 8;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 214, result);
  // Patch 16-19 # 215
  constant_a = 9;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 215, result);
  // Patch 16-20 # 216
  constant_a = 10;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 216, result);
  // Patch 17-0 # 217
  result = ((size + i) != i);
  uni_klee_add_patch(patch_results, 217, result);
  // Patch 18-0 # 218
  constant_a = -10;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 218, result);
  // Patch 18-1 # 219
  constant_a = -9;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 219, result);
  // Patch 18-2 # 220
  constant_a = -8;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 220, result);
  // Patch 18-3 # 221
  constant_a = -7;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 221, result);
  // Patch 18-4 # 222
  constant_a = -6;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 222, result);
  // Patch 18-5 # 223
  constant_a = -5;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 223, result);
  // Patch 18-6 # 224
  constant_a = -4;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 224, result);
  // Patch 18-7 # 225
  constant_a = -3;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 225, result);
  // Patch 18-8 # 226
  constant_a = -2;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 226, result);
  // Patch 18-9 # 227
  constant_a = -1;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 227, result);
  // Patch 18-10 # 228
  constant_a = 0;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 228, result);
  // Patch 18-11 # 229
  constant_a = 1;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 229, result);
  // Patch 18-12 # 230
  constant_a = 2;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 230, result);
  // Patch 18-13 # 231
  constant_a = 3;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 231, result);
  // Patch 18-14 # 232
  constant_a = 4;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 232, result);
  // Patch 18-15 # 233
  constant_a = 5;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 233, result);
  // Patch 18-16 # 234
  constant_a = 6;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 234, result);
  // Patch 18-17 # 235
  constant_a = 7;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 235, result);
  // Patch 18-18 # 236
  constant_a = 8;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 236, result);
  // Patch 18-19 # 237
  constant_a = 9;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 237, result);
  // Patch 18-20 # 238
  constant_a = 10;
  result = ((constant_a + i) != i);
  uni_klee_add_patch(patch_results, 238, result);
  // Patch 19-0 # 239
  constant_a = -10;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 239, result);
  // Patch 19-1 # 240
  constant_a = -9;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 240, result);
  // Patch 19-2 # 241
  constant_a = -8;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 241, result);
  // Patch 19-3 # 242
  constant_a = -7;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 242, result);
  // Patch 19-4 # 243
  constant_a = -6;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 243, result);
  // Patch 19-5 # 244
  constant_a = -5;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 244, result);
  // Patch 19-6 # 245
  constant_a = -4;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 245, result);
  // Patch 19-7 # 246
  constant_a = -3;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 246, result);
  // Patch 19-8 # 247
  constant_a = -2;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 247, result);
  // Patch 19-9 # 248
  constant_a = -1;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 248, result);
  // Patch 19-10 # 249
  constant_a = 0;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 249, result);
  // Patch 19-11 # 250
  constant_a = 1;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 250, result);
  // Patch 19-12 # 251
  constant_a = 2;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 251, result);
  // Patch 19-13 # 252
  constant_a = 3;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 252, result);
  // Patch 19-14 # 253
  constant_a = 4;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 253, result);
  // Patch 19-15 # 254
  constant_a = 5;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 254, result);
  // Patch 19-16 # 255
  constant_a = 6;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 255, result);
  // Patch 19-17 # 256
  constant_a = 7;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 256, result);
  // Patch 19-18 # 257
  constant_a = 8;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 257, result);
  // Patch 19-19 # 258
  constant_a = 9;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 258, result);
  // Patch 19-20 # 259
  constant_a = 10;
  result = ((constant_a + size) != i);
  uni_klee_add_patch(patch_results, 259, result);
  // Patch 20-0 # 260
  constant_a = -10;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 260, result);
  // Patch 20-1 # 261
  constant_a = -9;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 261, result);
  // Patch 20-2 # 262
  constant_a = -8;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 262, result);
  // Patch 20-3 # 263
  constant_a = -7;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 263, result);
  // Patch 20-4 # 264
  constant_a = -6;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 264, result);
  // Patch 20-5 # 265
  constant_a = -5;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 265, result);
  // Patch 20-6 # 266
  constant_a = -4;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 266, result);
  // Patch 20-7 # 267
  constant_a = -3;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 267, result);
  // Patch 20-8 # 268
  constant_a = -2;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 268, result);
  // Patch 20-9 # 269
  constant_a = -1;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 269, result);
  // Patch 20-10 # 270
  constant_a = 0;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 270, result);
  // Patch 20-11 # 271
  constant_a = 1;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 271, result);
  // Patch 20-12 # 272
  constant_a = 2;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 272, result);
  // Patch 20-13 # 273
  constant_a = 3;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 273, result);
  // Patch 20-14 # 274
  constant_a = 4;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 274, result);
  // Patch 20-15 # 275
  constant_a = 5;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 275, result);
  // Patch 20-16 # 276
  constant_a = 6;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 276, result);
  // Patch 20-17 # 277
  constant_a = 7;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 277, result);
  // Patch 20-18 # 278
  constant_a = 8;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 278, result);
  // Patch 20-19 # 279
  constant_a = 9;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 279, result);
  // Patch 20-20 # 280
  constant_a = 10;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 280, result);
  // Patch 21-0 # 281
  result = ((size + i) != size);
  uni_klee_add_patch(patch_results, 281, result);
  // Patch 22-0 # 282
  constant_a = -10;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 282, result);
  // Patch 22-1 # 283
  constant_a = -9;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 283, result);
  // Patch 22-2 # 284
  constant_a = -8;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 284, result);
  // Patch 22-3 # 285
  constant_a = -7;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 285, result);
  // Patch 22-4 # 286
  constant_a = -6;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 286, result);
  // Patch 22-5 # 287
  constant_a = -5;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 287, result);
  // Patch 22-6 # 288
  constant_a = -4;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 288, result);
  // Patch 22-7 # 289
  constant_a = -3;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 289, result);
  // Patch 22-8 # 290
  constant_a = -2;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 290, result);
  // Patch 22-9 # 291
  constant_a = -1;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 291, result);
  // Patch 22-10 # 292
  constant_a = 0;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 292, result);
  // Patch 22-11 # 293
  constant_a = 1;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 293, result);
  // Patch 22-12 # 294
  constant_a = 2;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 294, result);
  // Patch 22-13 # 295
  constant_a = 3;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 295, result);
  // Patch 22-14 # 296
  constant_a = 4;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 296, result);
  // Patch 22-15 # 297
  constant_a = 5;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 297, result);
  // Patch 22-16 # 298
  constant_a = 6;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 298, result);
  // Patch 22-17 # 299
  constant_a = 7;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 299, result);
  // Patch 22-18 # 300
  constant_a = 8;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 300, result);
  // Patch 22-19 # 301
  constant_a = 9;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 301, result);
  // Patch 22-20 # 302
  constant_a = 10;
  result = ((constant_a + i) != size);
  uni_klee_add_patch(patch_results, 302, result);
  // Patch 23-0 # 303
  constant_a = -10;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 303, result);
  // Patch 23-1 # 304
  constant_a = -9;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 304, result);
  // Patch 23-2 # 305
  constant_a = -8;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 305, result);
  // Patch 23-3 # 306
  constant_a = -7;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 306, result);
  // Patch 23-4 # 307
  constant_a = -6;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 307, result);
  // Patch 23-5 # 308
  constant_a = -5;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 308, result);
  // Patch 23-6 # 309
  constant_a = -4;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 309, result);
  // Patch 23-7 # 310
  constant_a = -3;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 310, result);
  // Patch 23-8 # 311
  constant_a = -2;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 311, result);
  // Patch 23-9 # 312
  constant_a = -1;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 312, result);
  // Patch 23-10 # 313
  constant_a = 0;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 313, result);
  // Patch 23-11 # 314
  constant_a = 1;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 314, result);
  // Patch 23-12 # 315
  constant_a = 2;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 315, result);
  // Patch 23-13 # 316
  constant_a = 3;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 316, result);
  // Patch 23-14 # 317
  constant_a = 4;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 317, result);
  // Patch 23-15 # 318
  constant_a = 5;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 318, result);
  // Patch 23-16 # 319
  constant_a = 6;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 319, result);
  // Patch 23-17 # 320
  constant_a = 7;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 320, result);
  // Patch 23-18 # 321
  constant_a = 8;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 321, result);
  // Patch 23-19 # 322
  constant_a = 9;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 322, result);
  // Patch 23-20 # 323
  constant_a = 10;
  result = ((constant_a + size) != size);
  uni_klee_add_patch(patch_results, 323, result);
  // Patch 24-0 # 324
  constant_a = -10;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 324, result);
  // Patch 24-1 # 325
  constant_a = -9;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 325, result);
  // Patch 24-2 # 326
  constant_a = -8;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 326, result);
  // Patch 24-3 # 327
  constant_a = -7;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 327, result);
  // Patch 24-4 # 328
  constant_a = -6;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 328, result);
  // Patch 24-5 # 329
  constant_a = -5;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 329, result);
  // Patch 24-6 # 330
  constant_a = -4;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 330, result);
  // Patch 24-7 # 331
  constant_a = -3;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 331, result);
  // Patch 24-8 # 332
  constant_a = -2;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 332, result);
  // Patch 24-9 # 333
  constant_a = -1;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 333, result);
  // Patch 24-10 # 334
  constant_a = 0;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 334, result);
  // Patch 24-11 # 335
  constant_a = 1;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 335, result);
  // Patch 24-12 # 336
  constant_a = 2;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 336, result);
  // Patch 24-13 # 337
  constant_a = 3;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 337, result);
  // Patch 24-14 # 338
  constant_a = 4;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 338, result);
  // Patch 24-15 # 339
  constant_a = 5;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 339, result);
  // Patch 24-16 # 340
  constant_a = 6;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 340, result);
  // Patch 24-17 # 341
  constant_a = 7;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 341, result);
  // Patch 24-18 # 342
  constant_a = 8;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 342, result);
  // Patch 24-19 # 343
  constant_a = 9;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 343, result);
  // Patch 24-20 # 344
  constant_a = 10;
  result = ((size + i) != constant_a);
  uni_klee_add_patch(patch_results, 344, result);
  // Patch 25-0 # 345
  constant_a = -10;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 345, result);
  // Patch 25-1 # 346
  constant_a = -9;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 346, result);
  // Patch 25-2 # 347
  constant_a = -8;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 347, result);
  // Patch 25-3 # 348
  constant_a = -7;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 348, result);
  // Patch 25-4 # 349
  constant_a = -6;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 349, result);
  // Patch 25-5 # 350
  constant_a = -5;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 350, result);
  // Patch 25-6 # 351
  constant_a = -4;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 351, result);
  // Patch 25-7 # 352
  constant_a = -3;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 352, result);
  // Patch 25-8 # 353
  constant_a = -2;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 353, result);
  // Patch 25-9 # 354
  constant_a = -1;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 354, result);
  // Patch 25-10 # 355
  constant_a = 0;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 355, result);
  // Patch 25-11 # 356
  constant_a = 1;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 356, result);
  // Patch 25-12 # 357
  constant_a = 2;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 357, result);
  // Patch 25-13 # 358
  constant_a = 3;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 358, result);
  // Patch 25-14 # 359
  constant_a = 4;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 359, result);
  // Patch 25-15 # 360
  constant_a = 5;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 360, result);
  // Patch 25-16 # 361
  constant_a = 6;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 361, result);
  // Patch 25-17 # 362
  constant_a = 7;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 362, result);
  // Patch 25-18 # 363
  constant_a = 8;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 363, result);
  // Patch 25-19 # 364
  constant_a = 9;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 364, result);
  // Patch 25-20 # 365
  constant_a = 10;
  result = ((constant_a + i) != constant_a);
  uni_klee_add_patch(patch_results, 365, result);
  // Patch 26-0 # 366
  constant_a = -10;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 366, result);
  // Patch 26-1 # 367
  constant_a = -9;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 367, result);
  // Patch 26-2 # 368
  constant_a = -8;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 368, result);
  // Patch 26-3 # 369
  constant_a = -7;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 369, result);
  // Patch 26-4 # 370
  constant_a = -6;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 370, result);
  // Patch 26-5 # 371
  constant_a = -5;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 371, result);
  // Patch 26-6 # 372
  constant_a = -4;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 372, result);
  // Patch 26-7 # 373
  constant_a = -3;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 373, result);
  // Patch 26-8 # 374
  constant_a = -2;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 374, result);
  // Patch 26-9 # 375
  constant_a = -1;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 375, result);
  // Patch 26-10 # 376
  constant_a = 0;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 376, result);
  // Patch 26-11 # 377
  constant_a = 1;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 377, result);
  // Patch 26-12 # 378
  constant_a = 2;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 378, result);
  // Patch 26-13 # 379
  constant_a = 3;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 379, result);
  // Patch 26-14 # 380
  constant_a = 4;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 380, result);
  // Patch 26-15 # 381
  constant_a = 5;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 381, result);
  // Patch 26-16 # 382
  constant_a = 6;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 382, result);
  // Patch 26-17 # 383
  constant_a = 7;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 383, result);
  // Patch 26-18 # 384
  constant_a = 8;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 384, result);
  // Patch 26-19 # 385
  constant_a = 9;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 385, result);
  // Patch 26-20 # 386
  constant_a = 10;
  result = ((constant_a + size) != constant_a);
  uni_klee_add_patch(patch_results, 386, result);
  // Patch 27-0 # 387
  result = (size < i);
  uni_klee_add_patch(patch_results, 387, result);
  // Patch 28-0 # 388
  constant_a = -10;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 388, result);
  // Patch 28-1 # 389
  constant_a = -9;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 389, result);
  // Patch 28-2 # 390
  constant_a = -8;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 390, result);
  // Patch 28-3 # 391
  constant_a = -7;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 391, result);
  // Patch 28-4 # 392
  constant_a = -6;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 392, result);
  // Patch 28-5 # 393
  constant_a = -5;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 393, result);
  // Patch 28-6 # 394
  constant_a = -4;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 394, result);
  // Patch 28-7 # 395
  constant_a = -3;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 395, result);
  // Patch 28-8 # 396
  constant_a = -2;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 396, result);
  // Patch 28-9 # 397
  constant_a = -1;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 397, result);
  // Patch 28-10 # 398
  constant_a = 0;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 398, result);
  // Patch 28-11 # 399
  constant_a = 1;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 399, result);
  // Patch 28-12 # 400
  constant_a = 2;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 400, result);
  // Patch 28-13 # 401
  constant_a = 3;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 401, result);
  // Patch 28-14 # 402
  constant_a = 4;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 402, result);
  // Patch 28-15 # 403
  constant_a = 5;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 403, result);
  // Patch 28-16 # 404
  constant_a = 6;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 404, result);
  // Patch 28-17 # 405
  constant_a = 7;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 405, result);
  // Patch 28-18 # 406
  constant_a = 8;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 406, result);
  // Patch 28-19 # 407
  constant_a = 9;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 407, result);
  // Patch 28-20 # 408
  constant_a = 10;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 408, result);
  // Patch 29-0 # 409
  result = ((size + i) < i);
  uni_klee_add_patch(patch_results, 409, result);
  // Patch 30-0 # 410
  constant_a = -10;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 410, result);
  // Patch 30-1 # 411
  constant_a = -9;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 411, result);
  // Patch 30-2 # 412
  constant_a = -8;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 412, result);
  // Patch 30-3 # 413
  constant_a = -7;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 413, result);
  // Patch 30-4 # 414
  constant_a = -6;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 414, result);
  // Patch 30-5 # 415
  constant_a = -5;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 415, result);
  // Patch 30-6 # 416
  constant_a = -4;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 416, result);
  // Patch 30-7 # 417
  constant_a = -3;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 417, result);
  // Patch 30-8 # 418
  constant_a = -2;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 418, result);
  // Patch 30-9 # 419
  constant_a = -1;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 419, result);
  // Patch 30-10 # 420
  constant_a = 0;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 420, result);
  // Patch 30-11 # 421
  constant_a = 1;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 421, result);
  // Patch 30-12 # 422
  constant_a = 2;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 422, result);
  // Patch 30-13 # 423
  constant_a = 3;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 423, result);
  // Patch 30-14 # 424
  constant_a = 4;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 424, result);
  // Patch 30-15 # 425
  constant_a = 5;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 425, result);
  // Patch 30-16 # 426
  constant_a = 6;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 426, result);
  // Patch 30-17 # 427
  constant_a = 7;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 427, result);
  // Patch 30-18 # 428
  constant_a = 8;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 428, result);
  // Patch 30-19 # 429
  constant_a = 9;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 429, result);
  // Patch 30-20 # 430
  constant_a = 10;
  result = ((constant_a + i) < i);
  uni_klee_add_patch(patch_results, 430, result);
  // Patch 31-0 # 431
  constant_a = -10;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 431, result);
  // Patch 31-1 # 432
  constant_a = -9;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 432, result);
  // Patch 31-2 # 433
  constant_a = -8;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 433, result);
  // Patch 31-3 # 434
  constant_a = -7;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 434, result);
  // Patch 31-4 # 435
  constant_a = -6;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 435, result);
  // Patch 31-5 # 436
  constant_a = -5;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 436, result);
  // Patch 31-6 # 437
  constant_a = -4;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 437, result);
  // Patch 31-7 # 438
  constant_a = -3;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 438, result);
  // Patch 31-8 # 439
  constant_a = -2;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 439, result);
  // Patch 31-9 # 440
  constant_a = -1;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 440, result);
  // Patch 31-10 # 441
  constant_a = 0;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 441, result);
  // Patch 31-11 # 442
  constant_a = 1;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 442, result);
  // Patch 31-12 # 443
  constant_a = 2;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 443, result);
  // Patch 31-13 # 444
  constant_a = 3;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 444, result);
  // Patch 31-14 # 445
  constant_a = 4;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 445, result);
  // Patch 31-15 # 446
  constant_a = 5;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 446, result);
  // Patch 31-16 # 447
  constant_a = 6;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 447, result);
  // Patch 31-17 # 448
  constant_a = 7;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 448, result);
  // Patch 31-18 # 449
  constant_a = 8;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 449, result);
  // Patch 31-19 # 450
  constant_a = 9;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 450, result);
  // Patch 31-20 # 451
  constant_a = 10;
  result = ((constant_a + size) < i);
  uni_klee_add_patch(patch_results, 451, result);
  // Patch 32-0 # 452
  result = (i < size);
  uni_klee_add_patch(patch_results, 452, result);
  // Patch 33-0 # 453
  constant_a = -10;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 453, result);
  // Patch 33-1 # 454
  constant_a = -9;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 454, result);
  // Patch 33-2 # 455
  constant_a = -8;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 455, result);
  // Patch 33-3 # 456
  constant_a = -7;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 456, result);
  // Patch 33-4 # 457
  constant_a = -6;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 457, result);
  // Patch 33-5 # 458
  constant_a = -5;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 458, result);
  // Patch 33-6 # 459
  constant_a = -4;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 459, result);
  // Patch 33-7 # 460
  constant_a = -3;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 460, result);
  // Patch 33-8 # 461
  constant_a = -2;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 461, result);
  // Patch 33-9 # 462
  constant_a = -1;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 462, result);
  // Patch 33-10 # 463
  constant_a = 0;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 463, result);
  // Patch 33-11 # 464
  constant_a = 1;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 464, result);
  // Patch 33-12 # 465
  constant_a = 2;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 465, result);
  // Patch 33-13 # 466
  constant_a = 3;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 466, result);
  // Patch 33-14 # 467
  constant_a = 4;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 467, result);
  // Patch 33-15 # 468
  constant_a = 5;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 468, result);
  // Patch 33-16 # 469
  constant_a = 6;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 469, result);
  // Patch 33-17 # 470
  constant_a = 7;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 470, result);
  // Patch 33-18 # 471
  constant_a = 8;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 471, result);
  // Patch 33-19 # 472
  constant_a = 9;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 472, result);
  // Patch 33-20 # 473
  constant_a = 10;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 473, result);
  // Patch 34-0 # 474
  result = ((size + i) < size);
  uni_klee_add_patch(patch_results, 474, result);
  // Patch 35-0 # 475
  constant_a = -10;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 475, result);
  // Patch 35-1 # 476
  constant_a = -9;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 476, result);
  // Patch 35-2 # 477
  constant_a = -8;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 477, result);
  // Patch 35-3 # 478
  constant_a = -7;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 478, result);
  // Patch 35-4 # 479
  constant_a = -6;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 479, result);
  // Patch 35-5 # 480
  constant_a = -5;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 480, result);
  // Patch 35-6 # 481
  constant_a = -4;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 481, result);
  // Patch 35-7 # 482
  constant_a = -3;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 482, result);
  // Patch 35-8 # 483
  constant_a = -2;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 483, result);
  // Patch 35-9 # 484
  constant_a = -1;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 484, result);
  // Patch 35-10 # 485
  constant_a = 0;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 485, result);
  // Patch 35-11 # 486
  constant_a = 1;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 486, result);
  // Patch 35-12 # 487
  constant_a = 2;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 487, result);
  // Patch 35-13 # 488
  constant_a = 3;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 488, result);
  // Patch 35-14 # 489
  constant_a = 4;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 489, result);
  // Patch 35-15 # 490
  constant_a = 5;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 490, result);
  // Patch 35-16 # 491
  constant_a = 6;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 491, result);
  // Patch 35-17 # 492
  constant_a = 7;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 492, result);
  // Patch 35-18 # 493
  constant_a = 8;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 493, result);
  // Patch 35-19 # 494
  constant_a = 9;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 494, result);
  // Patch 35-20 # 495
  constant_a = 10;
  result = ((constant_a + i) < size);
  uni_klee_add_patch(patch_results, 495, result);
  // Patch 36-0 # 496
  constant_a = -10;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 496, result);
  // Patch 36-1 # 497
  constant_a = -9;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 497, result);
  // Patch 36-2 # 498
  constant_a = -8;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 498, result);
  // Patch 36-3 # 499
  constant_a = -7;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 499, result);
  // Patch 36-4 # 500
  constant_a = -6;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 500, result);
  // Patch 36-5 # 501
  constant_a = -5;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 501, result);
  // Patch 36-6 # 502
  constant_a = -4;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 502, result);
  // Patch 36-7 # 503
  constant_a = -3;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 503, result);
  // Patch 36-8 # 504
  constant_a = -2;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 504, result);
  // Patch 36-9 # 505
  constant_a = -1;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 505, result);
  // Patch 36-10 # 506
  constant_a = 0;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 506, result);
  // Patch 36-11 # 507
  constant_a = 1;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 507, result);
  // Patch 36-12 # 508
  constant_a = 2;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 508, result);
  // Patch 36-13 # 509
  constant_a = 3;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 509, result);
  // Patch 36-14 # 510
  constant_a = 4;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 510, result);
  // Patch 36-15 # 511
  constant_a = 5;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 511, result);
  // Patch 36-16 # 512
  constant_a = 6;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 512, result);
  // Patch 36-17 # 513
  constant_a = 7;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 513, result);
  // Patch 36-18 # 514
  constant_a = 8;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 514, result);
  // Patch 36-19 # 515
  constant_a = 9;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 515, result);
  // Patch 36-20 # 516
  constant_a = 10;
  result = ((constant_a + size) < size);
  uni_klee_add_patch(patch_results, 516, result);
  // Patch 37-0 # 517
  constant_a = -10;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 517, result);
  // Patch 37-1 # 518
  constant_a = -9;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 518, result);
  // Patch 37-2 # 519
  constant_a = -8;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 519, result);
  // Patch 37-3 # 520
  constant_a = -7;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 520, result);
  // Patch 37-4 # 521
  constant_a = -6;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 521, result);
  // Patch 37-5 # 522
  constant_a = -5;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 522, result);
  // Patch 37-6 # 523
  constant_a = -4;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 523, result);
  // Patch 37-7 # 524
  constant_a = -3;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 524, result);
  // Patch 37-8 # 525
  constant_a = -2;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 525, result);
  // Patch 37-9 # 526
  constant_a = -1;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 526, result);
  // Patch 37-10 # 527
  constant_a = 0;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 527, result);
  // Patch 37-11 # 528
  constant_a = 1;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 528, result);
  // Patch 37-12 # 529
  constant_a = 2;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 529, result);
  // Patch 37-13 # 530
  constant_a = 3;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 530, result);
  // Patch 37-14 # 531
  constant_a = 4;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 531, result);
  // Patch 37-15 # 532
  constant_a = 5;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 532, result);
  // Patch 37-16 # 533
  constant_a = 6;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 533, result);
  // Patch 37-17 # 534
  constant_a = 7;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 534, result);
  // Patch 37-18 # 535
  constant_a = 8;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 535, result);
  // Patch 37-19 # 536
  constant_a = 9;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 536, result);
  // Patch 37-20 # 537
  constant_a = 10;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 537, result);
  // Patch 38-0 # 538
  constant_a = -10;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 538, result);
  // Patch 38-1 # 539
  constant_a = -9;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 539, result);
  // Patch 38-2 # 540
  constant_a = -8;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 540, result);
  // Patch 38-3 # 541
  constant_a = -7;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 541, result);
  // Patch 38-4 # 542
  constant_a = -6;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 542, result);
  // Patch 38-5 # 543
  constant_a = -5;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 543, result);
  // Patch 38-6 # 544
  constant_a = -4;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 544, result);
  // Patch 38-7 # 545
  constant_a = -3;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 545, result);
  // Patch 38-8 # 546
  constant_a = -2;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 546, result);
  // Patch 38-9 # 547
  constant_a = -1;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 547, result);
  // Patch 38-10 # 548
  constant_a = 0;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 548, result);
  // Patch 38-11 # 549
  constant_a = 1;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 549, result);
  // Patch 38-12 # 550
  constant_a = 2;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 550, result);
  // Patch 38-13 # 551
  constant_a = 3;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 551, result);
  // Patch 38-14 # 552
  constant_a = 4;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 552, result);
  // Patch 38-15 # 553
  constant_a = 5;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 553, result);
  // Patch 38-16 # 554
  constant_a = 6;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 554, result);
  // Patch 38-17 # 555
  constant_a = 7;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 555, result);
  // Patch 38-18 # 556
  constant_a = 8;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 556, result);
  // Patch 38-19 # 557
  constant_a = 9;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 557, result);
  // Patch 38-20 # 558
  constant_a = 10;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 558, result);
  // Patch 39-0 # 559
  constant_a = -10;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 559, result);
  // Patch 39-1 # 560
  constant_a = -9;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 560, result);
  // Patch 39-2 # 561
  constant_a = -8;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 561, result);
  // Patch 39-3 # 562
  constant_a = -7;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 562, result);
  // Patch 39-4 # 563
  constant_a = -6;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 563, result);
  // Patch 39-5 # 564
  constant_a = -5;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 564, result);
  // Patch 39-6 # 565
  constant_a = -4;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 565, result);
  // Patch 39-7 # 566
  constant_a = -3;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 566, result);
  // Patch 39-8 # 567
  constant_a = -2;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 567, result);
  // Patch 39-9 # 568
  constant_a = -1;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 568, result);
  // Patch 39-10 # 569
  constant_a = 0;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 569, result);
  // Patch 39-11 # 570
  constant_a = 1;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 570, result);
  // Patch 39-12 # 571
  constant_a = 2;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 571, result);
  // Patch 39-13 # 572
  constant_a = 3;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 572, result);
  // Patch 39-14 # 573
  constant_a = 4;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 573, result);
  // Patch 39-15 # 574
  constant_a = 5;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 574, result);
  // Patch 39-16 # 575
  constant_a = 6;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 575, result);
  // Patch 39-17 # 576
  constant_a = 7;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 576, result);
  // Patch 39-18 # 577
  constant_a = 8;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 577, result);
  // Patch 39-19 # 578
  constant_a = 9;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 578, result);
  // Patch 39-20 # 579
  constant_a = 10;
  result = ((size + i) < constant_a);
  uni_klee_add_patch(patch_results, 579, result);
  // Patch 40-0 # 580
  constant_a = -10;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 580, result);
  // Patch 40-1 # 581
  constant_a = -9;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 581, result);
  // Patch 40-2 # 582
  constant_a = -8;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 582, result);
  // Patch 40-3 # 583
  constant_a = -7;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 583, result);
  // Patch 40-4 # 584
  constant_a = -6;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 584, result);
  // Patch 40-5 # 585
  constant_a = -5;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 585, result);
  // Patch 40-6 # 586
  constant_a = -4;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 586, result);
  // Patch 40-7 # 587
  constant_a = -3;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 587, result);
  // Patch 40-8 # 588
  constant_a = -2;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 588, result);
  // Patch 40-9 # 589
  constant_a = -1;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 589, result);
  // Patch 40-10 # 590
  constant_a = 0;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 590, result);
  // Patch 40-11 # 591
  constant_a = 1;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 591, result);
  // Patch 40-12 # 592
  constant_a = 2;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 592, result);
  // Patch 40-13 # 593
  constant_a = 3;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 593, result);
  // Patch 40-14 # 594
  constant_a = 4;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 594, result);
  // Patch 40-15 # 595
  constant_a = 5;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 595, result);
  // Patch 40-16 # 596
  constant_a = 6;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 596, result);
  // Patch 40-17 # 597
  constant_a = 7;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 597, result);
  // Patch 40-18 # 598
  constant_a = 8;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 598, result);
  // Patch 40-19 # 599
  constant_a = 9;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 599, result);
  // Patch 40-20 # 600
  constant_a = 10;
  result = ((constant_a + i) < constant_a);
  uni_klee_add_patch(patch_results, 600, result);
  // Patch 41-0 # 601
  constant_a = -10;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 601, result);
  // Patch 41-1 # 602
  constant_a = -9;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 602, result);
  // Patch 41-2 # 603
  constant_a = -8;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 603, result);
  // Patch 41-3 # 604
  constant_a = -7;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 604, result);
  // Patch 41-4 # 605
  constant_a = -6;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 605, result);
  // Patch 41-5 # 606
  constant_a = -5;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 606, result);
  // Patch 41-6 # 607
  constant_a = -4;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 607, result);
  // Patch 41-7 # 608
  constant_a = -3;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 608, result);
  // Patch 41-8 # 609
  constant_a = -2;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 609, result);
  // Patch 41-9 # 610
  constant_a = -1;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 610, result);
  // Patch 41-10 # 611
  constant_a = 0;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 611, result);
  // Patch 41-11 # 612
  constant_a = 1;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 612, result);
  // Patch 41-12 # 613
  constant_a = 2;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 613, result);
  // Patch 41-13 # 614
  constant_a = 3;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 614, result);
  // Patch 41-14 # 615
  constant_a = 4;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 615, result);
  // Patch 41-15 # 616
  constant_a = 5;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 616, result);
  // Patch 41-16 # 617
  constant_a = 6;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 617, result);
  // Patch 41-17 # 618
  constant_a = 7;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 618, result);
  // Patch 41-18 # 619
  constant_a = 8;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 619, result);
  // Patch 41-19 # 620
  constant_a = 9;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 620, result);
  // Patch 41-20 # 621
  constant_a = 10;
  result = ((constant_a + size) < constant_a);
  uni_klee_add_patch(patch_results, 621, result);
  // Patch 42-0 # 622
  result = (i < (size + i));
  uni_klee_add_patch(patch_results, 622, result);
  // Patch 43-0 # 623
  result = (size < (size + i));
  uni_klee_add_patch(patch_results, 623, result);
  // Patch 44-0 # 624
  constant_a = -10;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 624, result);
  // Patch 44-1 # 625
  constant_a = -9;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 625, result);
  // Patch 44-2 # 626
  constant_a = -8;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 626, result);
  // Patch 44-3 # 627
  constant_a = -7;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 627, result);
  // Patch 44-4 # 628
  constant_a = -6;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 628, result);
  // Patch 44-5 # 629
  constant_a = -5;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 629, result);
  // Patch 44-6 # 630
  constant_a = -4;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 630, result);
  // Patch 44-7 # 631
  constant_a = -3;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 631, result);
  // Patch 44-8 # 632
  constant_a = -2;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 632, result);
  // Patch 44-9 # 633
  constant_a = -1;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 633, result);
  // Patch 44-10 # 634
  constant_a = 0;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 634, result);
  // Patch 44-11 # 635
  constant_a = 1;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 635, result);
  // Patch 44-12 # 636
  constant_a = 2;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 636, result);
  // Patch 44-13 # 637
  constant_a = 3;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 637, result);
  // Patch 44-14 # 638
  constant_a = 4;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 638, result);
  // Patch 44-15 # 639
  constant_a = 5;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 639, result);
  // Patch 44-16 # 640
  constant_a = 6;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 640, result);
  // Patch 44-17 # 641
  constant_a = 7;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 641, result);
  // Patch 44-18 # 642
  constant_a = 8;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 642, result);
  // Patch 44-19 # 643
  constant_a = 9;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 643, result);
  // Patch 44-20 # 644
  constant_a = 10;
  result = (constant_a < (size + i));
  uni_klee_add_patch(patch_results, 644, result);
  // Patch 45-0 # 645
  constant_a = -10;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 645, result);
  // Patch 45-1 # 646
  constant_a = -9;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 646, result);
  // Patch 45-2 # 647
  constant_a = -8;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 647, result);
  // Patch 45-3 # 648
  constant_a = -7;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 648, result);
  // Patch 45-4 # 649
  constant_a = -6;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 649, result);
  // Patch 45-5 # 650
  constant_a = -5;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 650, result);
  // Patch 45-6 # 651
  constant_a = -4;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 651, result);
  // Patch 45-7 # 652
  constant_a = -3;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 652, result);
  // Patch 45-8 # 653
  constant_a = -2;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 653, result);
  // Patch 45-9 # 654
  constant_a = -1;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 654, result);
  // Patch 45-10 # 655
  constant_a = 0;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 655, result);
  // Patch 45-11 # 656
  constant_a = 1;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 656, result);
  // Patch 45-12 # 657
  constant_a = 2;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 657, result);
  // Patch 45-13 # 658
  constant_a = 3;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 658, result);
  // Patch 45-14 # 659
  constant_a = 4;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 659, result);
  // Patch 45-15 # 660
  constant_a = 5;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 660, result);
  // Patch 45-16 # 661
  constant_a = 6;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 661, result);
  // Patch 45-17 # 662
  constant_a = 7;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 662, result);
  // Patch 45-18 # 663
  constant_a = 8;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 663, result);
  // Patch 45-19 # 664
  constant_a = 9;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 664, result);
  // Patch 45-20 # 665
  constant_a = 10;
  result = (i < (constant_a + i));
  uni_klee_add_patch(patch_results, 665, result);
  // Patch 46-0 # 666
  constant_a = -10;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 666, result);
  // Patch 46-1 # 667
  constant_a = -9;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 667, result);
  // Patch 46-2 # 668
  constant_a = -8;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 668, result);
  // Patch 46-3 # 669
  constant_a = -7;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 669, result);
  // Patch 46-4 # 670
  constant_a = -6;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 670, result);
  // Patch 46-5 # 671
  constant_a = -5;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 671, result);
  // Patch 46-6 # 672
  constant_a = -4;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 672, result);
  // Patch 46-7 # 673
  constant_a = -3;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 673, result);
  // Patch 46-8 # 674
  constant_a = -2;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 674, result);
  // Patch 46-9 # 675
  constant_a = -1;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 675, result);
  // Patch 46-10 # 676
  constant_a = 0;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 676, result);
  // Patch 46-11 # 677
  constant_a = 1;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 677, result);
  // Patch 46-12 # 678
  constant_a = 2;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 678, result);
  // Patch 46-13 # 679
  constant_a = 3;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 679, result);
  // Patch 46-14 # 680
  constant_a = 4;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 680, result);
  // Patch 46-15 # 681
  constant_a = 5;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 681, result);
  // Patch 46-16 # 682
  constant_a = 6;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 682, result);
  // Patch 46-17 # 683
  constant_a = 7;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 683, result);
  // Patch 46-18 # 684
  constant_a = 8;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 684, result);
  // Patch 46-19 # 685
  constant_a = 9;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 685, result);
  // Patch 46-20 # 686
  constant_a = 10;
  result = (size < (constant_a + i));
  uni_klee_add_patch(patch_results, 686, result);
  // Patch 47-0 # 687
  constant_a = -10;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 687, result);
  // Patch 47-1 # 688
  constant_a = -9;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 688, result);
  // Patch 47-2 # 689
  constant_a = -8;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 689, result);
  // Patch 47-3 # 690
  constant_a = -7;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 690, result);
  // Patch 47-4 # 691
  constant_a = -6;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 691, result);
  // Patch 47-5 # 692
  constant_a = -5;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 692, result);
  // Patch 47-6 # 693
  constant_a = -4;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 693, result);
  // Patch 47-7 # 694
  constant_a = -3;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 694, result);
  // Patch 47-8 # 695
  constant_a = -2;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 695, result);
  // Patch 47-9 # 696
  constant_a = -1;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 696, result);
  // Patch 47-10 # 697
  constant_a = 0;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 697, result);
  // Patch 47-11 # 698
  constant_a = 1;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 698, result);
  // Patch 47-12 # 699
  constant_a = 2;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 699, result);
  // Patch 47-13 # 700
  constant_a = 3;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 700, result);
  // Patch 47-14 # 701
  constant_a = 4;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 701, result);
  // Patch 47-15 # 702
  constant_a = 5;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 702, result);
  // Patch 47-16 # 703
  constant_a = 6;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 703, result);
  // Patch 47-17 # 704
  constant_a = 7;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 704, result);
  // Patch 47-18 # 705
  constant_a = 8;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 705, result);
  // Patch 47-19 # 706
  constant_a = 9;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 706, result);
  // Patch 47-20 # 707
  constant_a = 10;
  result = (constant_a < (constant_a + i));
  uni_klee_add_patch(patch_results, 707, result);
  // Patch 48-0 # 708
  constant_a = -10;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 708, result);
  // Patch 48-1 # 709
  constant_a = -9;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 709, result);
  // Patch 48-2 # 710
  constant_a = -8;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 710, result);
  // Patch 48-3 # 711
  constant_a = -7;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 711, result);
  // Patch 48-4 # 712
  constant_a = -6;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 712, result);
  // Patch 48-5 # 713
  constant_a = -5;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 713, result);
  // Patch 48-6 # 714
  constant_a = -4;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 714, result);
  // Patch 48-7 # 715
  constant_a = -3;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 715, result);
  // Patch 48-8 # 716
  constant_a = -2;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 716, result);
  // Patch 48-9 # 717
  constant_a = -1;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 717, result);
  // Patch 48-10 # 718
  constant_a = 0;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 718, result);
  // Patch 48-11 # 719
  constant_a = 1;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 719, result);
  // Patch 48-12 # 720
  constant_a = 2;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 720, result);
  // Patch 48-13 # 721
  constant_a = 3;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 721, result);
  // Patch 48-14 # 722
  constant_a = 4;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 722, result);
  // Patch 48-15 # 723
  constant_a = 5;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 723, result);
  // Patch 48-16 # 724
  constant_a = 6;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 724, result);
  // Patch 48-17 # 725
  constant_a = 7;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 725, result);
  // Patch 48-18 # 726
  constant_a = 8;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 726, result);
  // Patch 48-19 # 727
  constant_a = 9;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 727, result);
  // Patch 48-20 # 728
  constant_a = 10;
  result = (i < (constant_a + size));
  uni_klee_add_patch(patch_results, 728, result);
  // Patch 49-0 # 729
  constant_a = -10;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 729, result);
  // Patch 49-1 # 730
  constant_a = -9;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 730, result);
  // Patch 49-2 # 731
  constant_a = -8;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 731, result);
  // Patch 49-3 # 732
  constant_a = -7;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 732, result);
  // Patch 49-4 # 733
  constant_a = -6;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 733, result);
  // Patch 49-5 # 734
  constant_a = -5;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 734, result);
  // Patch 49-6 # 735
  constant_a = -4;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 735, result);
  // Patch 49-7 # 736
  constant_a = -3;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 736, result);
  // Patch 49-8 # 737
  constant_a = -2;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 737, result);
  // Patch 49-9 # 738
  constant_a = -1;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 738, result);
  // Patch 49-10 # 739
  constant_a = 0;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 739, result);
  // Patch 49-11 # 740
  constant_a = 1;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 740, result);
  // Patch 49-12 # 741
  constant_a = 2;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 741, result);
  // Patch 49-13 # 742
  constant_a = 3;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 742, result);
  // Patch 49-14 # 743
  constant_a = 4;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 743, result);
  // Patch 49-15 # 744
  constant_a = 5;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 744, result);
  // Patch 49-16 # 745
  constant_a = 6;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 745, result);
  // Patch 49-17 # 746
  constant_a = 7;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 746, result);
  // Patch 49-18 # 747
  constant_a = 8;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 747, result);
  // Patch 49-19 # 748
  constant_a = 9;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 748, result);
  // Patch 49-20 # 749
  constant_a = 10;
  result = (size < (constant_a + size));
  uni_klee_add_patch(patch_results, 749, result);
  // Patch 50-0 # 750
  constant_a = -10;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 750, result);
  // Patch 50-1 # 751
  constant_a = -9;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 751, result);
  // Patch 50-2 # 752
  constant_a = -8;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 752, result);
  // Patch 50-3 # 753
  constant_a = -7;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 753, result);
  // Patch 50-4 # 754
  constant_a = -6;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 754, result);
  // Patch 50-5 # 755
  constant_a = -5;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 755, result);
  // Patch 50-6 # 756
  constant_a = -4;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 756, result);
  // Patch 50-7 # 757
  constant_a = -3;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 757, result);
  // Patch 50-8 # 758
  constant_a = -2;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 758, result);
  // Patch 50-9 # 759
  constant_a = -1;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 759, result);
  // Patch 50-10 # 760
  constant_a = 0;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 760, result);
  // Patch 50-11 # 761
  constant_a = 1;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 761, result);
  // Patch 50-12 # 762
  constant_a = 2;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 762, result);
  // Patch 50-13 # 763
  constant_a = 3;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 763, result);
  // Patch 50-14 # 764
  constant_a = 4;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 764, result);
  // Patch 50-15 # 765
  constant_a = 5;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 765, result);
  // Patch 50-16 # 766
  constant_a = 6;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 766, result);
  // Patch 50-17 # 767
  constant_a = 7;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 767, result);
  // Patch 50-18 # 768
  constant_a = 8;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 768, result);
  // Patch 50-19 # 769
  constant_a = 9;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 769, result);
  // Patch 50-20 # 770
  constant_a = 10;
  result = (constant_a < (constant_a + size));
  uni_klee_add_patch(patch_results, 770, result);
  // Patch correct # 771
  result = ((i + 1) < size);
  uni_klee_add_patch(patch_results, 771, result);
  klee_select_patch(&uni_klee_patch_id);
  return uni_klee_choice(patch_results, uni_klee_patch_id);
}
// UNI_KLEE_END

int __cpr_output(char* id, char* typestr, int value){
  return value;
}
