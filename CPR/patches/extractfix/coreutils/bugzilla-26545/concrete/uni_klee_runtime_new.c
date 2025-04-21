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
  int patch_results[1859];
  // Patch buggy # 0
  result = (size / 2 > i);
  uni_klee_add_patch(patch_results, 0, result);
  // Patch 1-0 # 1
  result = (i == i);
  uni_klee_add_patch(patch_results, 1, result);
  // Patch 2-0 # 2
  constant_a = -10;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 2, result);
  // Patch 2-1 # 3
  constant_a = -9;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 3, result);
  // Patch 2-2 # 4
  constant_a = -8;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 4, result);
  // Patch 2-3 # 5
  constant_a = -7;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 5, result);
  // Patch 2-4 # 6
  constant_a = -6;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 6, result);
  // Patch 2-5 # 7
  constant_a = -5;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 7, result);
  // Patch 2-6 # 8
  constant_a = -4;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 8, result);
  // Patch 2-7 # 9
  constant_a = -3;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 9, result);
  // Patch 2-8 # 10
  constant_a = -2;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 10, result);
  // Patch 2-9 # 11
  constant_a = -1;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 11, result);
  // Patch 2-10 # 12
  constant_a = 0;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 12, result);
  // Patch 2-11 # 13
  constant_a = 1;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 13, result);
  // Patch 2-12 # 14
  constant_a = 2;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 14, result);
  // Patch 2-13 # 15
  constant_a = 3;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 15, result);
  // Patch 2-14 # 16
  constant_a = 4;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 16, result);
  // Patch 2-15 # 17
  constant_a = 5;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 17, result);
  // Patch 2-16 # 18
  constant_a = 6;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 18, result);
  // Patch 2-17 # 19
  constant_a = 7;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 19, result);
  // Patch 2-18 # 20
  constant_a = 8;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 20, result);
  // Patch 2-19 # 21
  constant_a = 9;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 21, result);
  // Patch 2-20 # 22
  constant_a = 10;
  result = (constant_a == i);
  uni_klee_add_patch(patch_results, 22, result);
  // Patch 3-0 # 23
  constant_a = -10;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 23, result);
  // Patch 3-1 # 24
  constant_a = -9;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 24, result);
  // Patch 3-2 # 25
  constant_a = -8;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 25, result);
  // Patch 3-3 # 26
  constant_a = -7;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 26, result);
  // Patch 3-4 # 27
  constant_a = -6;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 27, result);
  // Patch 3-5 # 28
  constant_a = -5;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 28, result);
  // Patch 3-6 # 29
  constant_a = -4;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 29, result);
  // Patch 3-7 # 30
  constant_a = -3;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 30, result);
  // Patch 3-8 # 31
  constant_a = -2;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 31, result);
  // Patch 3-9 # 32
  constant_a = -1;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 32, result);
  // Patch 3-10 # 33
  constant_a = 0;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 33, result);
  // Patch 3-11 # 34
  constant_a = 1;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 34, result);
  // Patch 3-12 # 35
  constant_a = 2;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 35, result);
  // Patch 3-13 # 36
  constant_a = 3;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 36, result);
  // Patch 3-14 # 37
  constant_a = 4;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 37, result);
  // Patch 3-15 # 38
  constant_a = 5;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 38, result);
  // Patch 3-16 # 39
  constant_a = 6;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 39, result);
  // Patch 3-17 # 40
  constant_a = 7;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 40, result);
  // Patch 3-18 # 41
  constant_a = 8;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 41, result);
  // Patch 3-19 # 42
  constant_a = 9;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 42, result);
  // Patch 3-20 # 43
  constant_a = 10;
  result = ((constant_a / i) == i);
  uni_klee_add_patch(patch_results, 43, result);
  // Patch 4-0 # 44
  constant_a = -10;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 44, result);
  // Patch 4-1 # 45
  constant_a = -9;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 45, result);
  // Patch 4-2 # 46
  constant_a = -8;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 46, result);
  // Patch 4-3 # 47
  constant_a = -7;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 47, result);
  // Patch 4-4 # 48
  constant_a = -6;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 48, result);
  // Patch 4-5 # 49
  constant_a = -5;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 49, result);
  // Patch 4-6 # 50
  constant_a = -4;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 50, result);
  // Patch 4-7 # 51
  constant_a = -3;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 51, result);
  // Patch 4-8 # 52
  constant_a = -2;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 52, result);
  // Patch 4-9 # 53
  constant_a = -1;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 53, result);
  // Patch 4-10 # 54
  constant_a = 0;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 54, result);
  // Patch 4-11 # 55
  constant_a = 1;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 55, result);
  // Patch 4-12 # 56
  constant_a = 2;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 56, result);
  // Patch 4-13 # 57
  constant_a = 3;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 57, result);
  // Patch 4-14 # 58
  constant_a = 4;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 58, result);
  // Patch 4-15 # 59
  constant_a = 5;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 59, result);
  // Patch 4-16 # 60
  constant_a = 6;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 60, result);
  // Patch 4-17 # 61
  constant_a = 7;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 61, result);
  // Patch 4-18 # 62
  constant_a = 8;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 62, result);
  // Patch 4-19 # 63
  constant_a = 9;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 63, result);
  // Patch 4-20 # 64
  constant_a = 10;
  result = ((constant_a / size) == i);
  uni_klee_add_patch(patch_results, 64, result);
  // Patch 5-0 # 65
  constant_a = -10;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 65, result);
  // Patch 5-1 # 66
  constant_a = -9;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 66, result);
  // Patch 5-2 # 67
  constant_a = -8;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 67, result);
  // Patch 5-3 # 68
  constant_a = -7;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 68, result);
  // Patch 5-4 # 69
  constant_a = -6;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 69, result);
  // Patch 5-5 # 70
  constant_a = -5;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 70, result);
  // Patch 5-6 # 71
  constant_a = -4;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 71, result);
  // Patch 5-7 # 72
  constant_a = -3;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 72, result);
  // Patch 5-8 # 73
  constant_a = -2;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 73, result);
  // Patch 5-9 # 74
  constant_a = -1;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 74, result);
  // Patch 5-10 # 75
  constant_a = 1;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 75, result);
  // Patch 5-11 # 76
  constant_a = 2;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 76, result);
  // Patch 5-12 # 77
  constant_a = 3;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 77, result);
  // Patch 5-13 # 78
  constant_a = 4;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 78, result);
  // Patch 5-14 # 79
  constant_a = 5;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 79, result);
  // Patch 5-15 # 80
  constant_a = 6;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 80, result);
  // Patch 5-16 # 81
  constant_a = 7;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 81, result);
  // Patch 5-17 # 82
  constant_a = 8;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 82, result);
  // Patch 5-18 # 83
  constant_a = 9;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 83, result);
  // Patch 5-19 # 84
  constant_a = 10;
  result = ((i / constant_a) == i);
  uni_klee_add_patch(patch_results, 84, result);
  // Patch 6-0 # 85
  constant_a = -10;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 85, result);
  // Patch 6-1 # 86
  constant_a = -9;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 86, result);
  // Patch 6-2 # 87
  constant_a = -8;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 87, result);
  // Patch 6-3 # 88
  constant_a = -7;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 88, result);
  // Patch 6-4 # 89
  constant_a = -6;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 89, result);
  // Patch 6-5 # 90
  constant_a = -5;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 90, result);
  // Patch 6-6 # 91
  constant_a = -4;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 91, result);
  // Patch 6-7 # 92
  constant_a = -3;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 92, result);
  // Patch 6-8 # 93
  constant_a = -2;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 93, result);
  // Patch 6-9 # 94
  constant_a = -1;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 94, result);
  // Patch 6-10 # 95
  constant_a = 1;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 95, result);
  // Patch 6-11 # 96
  constant_a = 2;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 96, result);
  // Patch 6-12 # 97
  constant_a = 3;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 97, result);
  // Patch 6-13 # 98
  constant_a = 4;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 98, result);
  // Patch 6-14 # 99
  constant_a = 5;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 99, result);
  // Patch 6-15 # 100
  constant_a = 6;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 100, result);
  // Patch 6-16 # 101
  constant_a = 7;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 101, result);
  // Patch 6-17 # 102
  constant_a = 8;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 102, result);
  // Patch 6-18 # 103
  constant_a = 9;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 103, result);
  // Patch 6-19 # 104
  constant_a = 10;
  result = ((size / constant_a) == i);
  uni_klee_add_patch(patch_results, 104, result);
  // Patch 7-0 # 105
  constant_a = -10;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 105, result);
  // Patch 7-1 # 106
  constant_a = -9;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 106, result);
  // Patch 7-2 # 107
  constant_a = -8;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 107, result);
  // Patch 7-3 # 108
  constant_a = -7;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 108, result);
  // Patch 7-4 # 109
  constant_a = -6;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 109, result);
  // Patch 7-5 # 110
  constant_a = -5;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 110, result);
  // Patch 7-6 # 111
  constant_a = -4;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 111, result);
  // Patch 7-7 # 112
  constant_a = -3;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 112, result);
  // Patch 7-8 # 113
  constant_a = -2;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 113, result);
  // Patch 7-9 # 114
  constant_a = -1;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 114, result);
  // Patch 7-10 # 115
  constant_a = 0;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 115, result);
  // Patch 7-11 # 116
  constant_a = 1;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 116, result);
  // Patch 7-12 # 117
  constant_a = 2;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 117, result);
  // Patch 7-13 # 118
  constant_a = 3;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 118, result);
  // Patch 7-14 # 119
  constant_a = 4;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 119, result);
  // Patch 7-15 # 120
  constant_a = 5;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 120, result);
  // Patch 7-16 # 121
  constant_a = 6;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 121, result);
  // Patch 7-17 # 122
  constant_a = 7;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 122, result);
  // Patch 7-18 # 123
  constant_a = 8;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 123, result);
  // Patch 7-19 # 124
  constant_a = 9;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 124, result);
  // Patch 7-20 # 125
  constant_a = 10;
  result = (constant_a == size);
  uni_klee_add_patch(patch_results, 125, result);
  // Patch 8-0 # 126
  constant_a = -10;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 126, result);
  // Patch 8-1 # 127
  constant_a = -9;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 127, result);
  // Patch 8-2 # 128
  constant_a = -8;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 128, result);
  // Patch 8-3 # 129
  constant_a = -7;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 129, result);
  // Patch 8-4 # 130
  constant_a = -6;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 130, result);
  // Patch 8-5 # 131
  constant_a = -5;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 131, result);
  // Patch 8-6 # 132
  constant_a = -4;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 132, result);
  // Patch 8-7 # 133
  constant_a = -3;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 133, result);
  // Patch 8-8 # 134
  constant_a = -2;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 134, result);
  // Patch 8-9 # 135
  constant_a = -1;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 135, result);
  // Patch 8-10 # 136
  constant_a = 0;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 136, result);
  // Patch 8-11 # 137
  constant_a = 1;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 137, result);
  // Patch 8-12 # 138
  constant_a = 2;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 138, result);
  // Patch 8-13 # 139
  constant_a = 3;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 139, result);
  // Patch 8-14 # 140
  constant_a = 4;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 140, result);
  // Patch 8-15 # 141
  constant_a = 5;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 141, result);
  // Patch 8-16 # 142
  constant_a = 6;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 142, result);
  // Patch 8-17 # 143
  constant_a = 7;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 143, result);
  // Patch 8-18 # 144
  constant_a = 8;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 144, result);
  // Patch 8-19 # 145
  constant_a = 9;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 145, result);
  // Patch 8-20 # 146
  constant_a = 10;
  result = ((constant_a / i) == size);
  uni_klee_add_patch(patch_results, 146, result);
  // Patch 9-0 # 147
  constant_a = -10;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 147, result);
  // Patch 9-1 # 148
  constant_a = -9;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 148, result);
  // Patch 9-2 # 149
  constant_a = -8;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 149, result);
  // Patch 9-3 # 150
  constant_a = -7;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 150, result);
  // Patch 9-4 # 151
  constant_a = -6;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 151, result);
  // Patch 9-5 # 152
  constant_a = -5;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 152, result);
  // Patch 9-6 # 153
  constant_a = -4;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 153, result);
  // Patch 9-7 # 154
  constant_a = -3;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 154, result);
  // Patch 9-8 # 155
  constant_a = -2;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 155, result);
  // Patch 9-9 # 156
  constant_a = -1;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 156, result);
  // Patch 9-10 # 157
  constant_a = 0;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 157, result);
  // Patch 9-11 # 158
  constant_a = 1;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 158, result);
  // Patch 9-12 # 159
  constant_a = 2;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 159, result);
  // Patch 9-13 # 160
  constant_a = 3;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 160, result);
  // Patch 9-14 # 161
  constant_a = 4;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 161, result);
  // Patch 9-15 # 162
  constant_a = 5;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 162, result);
  // Patch 9-16 # 163
  constant_a = 6;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 163, result);
  // Patch 9-17 # 164
  constant_a = 7;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 164, result);
  // Patch 9-18 # 165
  constant_a = 8;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 165, result);
  // Patch 9-19 # 166
  constant_a = 9;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 166, result);
  // Patch 9-20 # 167
  constant_a = 10;
  result = ((constant_a / size) == size);
  uni_klee_add_patch(patch_results, 167, result);
  // Patch 10-0 # 168
  constant_a = -10;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 168, result);
  // Patch 10-1 # 169
  constant_a = -9;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 169, result);
  // Patch 10-2 # 170
  constant_a = -8;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 170, result);
  // Patch 10-3 # 171
  constant_a = -7;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 171, result);
  // Patch 10-4 # 172
  constant_a = -6;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 172, result);
  // Patch 10-5 # 173
  constant_a = -5;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 173, result);
  // Patch 10-6 # 174
  constant_a = -4;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 174, result);
  // Patch 10-7 # 175
  constant_a = -3;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 175, result);
  // Patch 10-8 # 176
  constant_a = -2;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 176, result);
  // Patch 10-9 # 177
  constant_a = -1;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 177, result);
  // Patch 10-10 # 178
  constant_a = 1;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 178, result);
  // Patch 10-11 # 179
  constant_a = 2;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 179, result);
  // Patch 10-12 # 180
  constant_a = 3;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 180, result);
  // Patch 10-13 # 181
  constant_a = 4;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 181, result);
  // Patch 10-14 # 182
  constant_a = 5;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 182, result);
  // Patch 10-15 # 183
  constant_a = 6;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 183, result);
  // Patch 10-16 # 184
  constant_a = 7;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 184, result);
  // Patch 10-17 # 185
  constant_a = 8;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 185, result);
  // Patch 10-18 # 186
  constant_a = 9;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 186, result);
  // Patch 10-19 # 187
  constant_a = 10;
  result = ((size / constant_a) == size);
  uni_klee_add_patch(patch_results, 187, result);
  // Patch 11-0 # 188
  constant_a = -10;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 188, result);
  // Patch 11-1 # 189
  constant_a = -9;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 189, result);
  // Patch 11-2 # 190
  constant_a = -8;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 190, result);
  // Patch 11-3 # 191
  constant_a = -7;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 191, result);
  // Patch 11-4 # 192
  constant_a = -6;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 192, result);
  // Patch 11-5 # 193
  constant_a = -5;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 193, result);
  // Patch 11-6 # 194
  constant_a = -4;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 194, result);
  // Patch 11-7 # 195
  constant_a = -3;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 195, result);
  // Patch 11-8 # 196
  constant_a = -2;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 196, result);
  // Patch 11-9 # 197
  constant_a = -1;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 197, result);
  // Patch 11-10 # 198
  constant_a = 0;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 198, result);
  // Patch 11-11 # 199
  constant_a = 1;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 199, result);
  // Patch 11-12 # 200
  constant_a = 2;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 200, result);
  // Patch 11-13 # 201
  constant_a = 3;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 201, result);
  // Patch 11-14 # 202
  constant_a = 4;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 202, result);
  // Patch 11-15 # 203
  constant_a = 5;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 203, result);
  // Patch 11-16 # 204
  constant_a = 6;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 204, result);
  // Patch 11-17 # 205
  constant_a = 7;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 205, result);
  // Patch 11-18 # 206
  constant_a = 8;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 206, result);
  // Patch 11-19 # 207
  constant_a = 9;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 207, result);
  // Patch 11-20 # 208
  constant_a = 10;
  result = ((size / i) == constant_a);
  uni_klee_add_patch(patch_results, 208, result);
  // Patch 12-0 # 209
  constant_a = -10;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 209, result);
  // Patch 12-1 # 210
  constant_a = -9;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 210, result);
  // Patch 12-2 # 211
  constant_a = -8;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 211, result);
  // Patch 12-3 # 212
  constant_a = -7;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 212, result);
  // Patch 12-4 # 213
  constant_a = -6;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 213, result);
  // Patch 12-5 # 214
  constant_a = -5;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 214, result);
  // Patch 12-6 # 215
  constant_a = -4;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 215, result);
  // Patch 12-7 # 216
  constant_a = -3;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 216, result);
  // Patch 12-8 # 217
  constant_a = -2;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 217, result);
  // Patch 12-9 # 218
  constant_a = -1;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 218, result);
  // Patch 12-10 # 219
  constant_a = 0;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 219, result);
  // Patch 12-11 # 220
  constant_a = 1;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 220, result);
  // Patch 12-12 # 221
  constant_a = 2;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 221, result);
  // Patch 12-13 # 222
  constant_a = 3;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 222, result);
  // Patch 12-14 # 223
  constant_a = 4;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 223, result);
  // Patch 12-15 # 224
  constant_a = 5;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 224, result);
  // Patch 12-16 # 225
  constant_a = 6;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 225, result);
  // Patch 12-17 # 226
  constant_a = 7;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 226, result);
  // Patch 12-18 # 227
  constant_a = 8;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 227, result);
  // Patch 12-19 # 228
  constant_a = 9;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 228, result);
  // Patch 12-20 # 229
  constant_a = 10;
  result = ((constant_a / i) == constant_a);
  uni_klee_add_patch(patch_results, 229, result);
  // Patch 13-0 # 230
  constant_a = -10;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 230, result);
  // Patch 13-1 # 231
  constant_a = -9;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 231, result);
  // Patch 13-2 # 232
  constant_a = -8;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 232, result);
  // Patch 13-3 # 233
  constant_a = -7;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 233, result);
  // Patch 13-4 # 234
  constant_a = -6;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 234, result);
  // Patch 13-5 # 235
  constant_a = -5;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 235, result);
  // Patch 13-6 # 236
  constant_a = -4;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 236, result);
  // Patch 13-7 # 237
  constant_a = -3;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 237, result);
  // Patch 13-8 # 238
  constant_a = -2;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 238, result);
  // Patch 13-9 # 239
  constant_a = -1;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 239, result);
  // Patch 13-10 # 240
  constant_a = 0;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 240, result);
  // Patch 13-11 # 241
  constant_a = 1;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 241, result);
  // Patch 13-12 # 242
  constant_a = 2;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 242, result);
  // Patch 13-13 # 243
  constant_a = 3;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 243, result);
  // Patch 13-14 # 244
  constant_a = 4;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 244, result);
  // Patch 13-15 # 245
  constant_a = 5;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 245, result);
  // Patch 13-16 # 246
  constant_a = 6;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 246, result);
  // Patch 13-17 # 247
  constant_a = 7;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 247, result);
  // Patch 13-18 # 248
  constant_a = 8;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 248, result);
  // Patch 13-19 # 249
  constant_a = 9;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 249, result);
  // Patch 13-20 # 250
  constant_a = 10;
  result = ((i / size) == constant_a);
  uni_klee_add_patch(patch_results, 250, result);
  // Patch 14-0 # 251
  constant_a = -10;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 251, result);
  // Patch 14-1 # 252
  constant_a = -9;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 252, result);
  // Patch 14-2 # 253
  constant_a = -8;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 253, result);
  // Patch 14-3 # 254
  constant_a = -7;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 254, result);
  // Patch 14-4 # 255
  constant_a = -6;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 255, result);
  // Patch 14-5 # 256
  constant_a = -5;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 256, result);
  // Patch 14-6 # 257
  constant_a = -4;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 257, result);
  // Patch 14-7 # 258
  constant_a = -3;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 258, result);
  // Patch 14-8 # 259
  constant_a = -2;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 259, result);
  // Patch 14-9 # 260
  constant_a = -1;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 260, result);
  // Patch 14-10 # 261
  constant_a = 0;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 261, result);
  // Patch 14-11 # 262
  constant_a = 1;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 262, result);
  // Patch 14-12 # 263
  constant_a = 2;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 263, result);
  // Patch 14-13 # 264
  constant_a = 3;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 264, result);
  // Patch 14-14 # 265
  constant_a = 4;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 265, result);
  // Patch 14-15 # 266
  constant_a = 5;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 266, result);
  // Patch 14-16 # 267
  constant_a = 6;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 267, result);
  // Patch 14-17 # 268
  constant_a = 7;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 268, result);
  // Patch 14-18 # 269
  constant_a = 8;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 269, result);
  // Patch 14-19 # 270
  constant_a = 9;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 270, result);
  // Patch 14-20 # 271
  constant_a = 10;
  result = ((constant_a / size) == constant_a);
  uni_klee_add_patch(patch_results, 271, result);
  // Patch 15-0 # 272
  result = (size != i);
  uni_klee_add_patch(patch_results, 272, result);
  // Patch 16-0 # 273
  constant_a = -10;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 273, result);
  // Patch 16-1 # 274
  constant_a = -9;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 274, result);
  // Patch 16-2 # 275
  constant_a = -8;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 275, result);
  // Patch 16-3 # 276
  constant_a = -7;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 276, result);
  // Patch 16-4 # 277
  constant_a = -6;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 277, result);
  // Patch 16-5 # 278
  constant_a = -5;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 278, result);
  // Patch 16-6 # 279
  constant_a = -4;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 279, result);
  // Patch 16-7 # 280
  constant_a = -3;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 280, result);
  // Patch 16-8 # 281
  constant_a = -2;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 281, result);
  // Patch 16-9 # 282
  constant_a = -1;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 282, result);
  // Patch 16-10 # 283
  constant_a = 0;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 283, result);
  // Patch 16-11 # 284
  constant_a = 1;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 284, result);
  // Patch 16-12 # 285
  constant_a = 2;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 285, result);
  // Patch 16-13 # 286
  constant_a = 3;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 286, result);
  // Patch 16-14 # 287
  constant_a = 4;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 287, result);
  // Patch 16-15 # 288
  constant_a = 5;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 288, result);
  // Patch 16-16 # 289
  constant_a = 6;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 289, result);
  // Patch 16-17 # 290
  constant_a = 7;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 290, result);
  // Patch 16-18 # 291
  constant_a = 8;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 291, result);
  // Patch 16-19 # 292
  constant_a = 9;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 292, result);
  // Patch 16-20 # 293
  constant_a = 10;
  result = (constant_a != i);
  uni_klee_add_patch(patch_results, 293, result);
  // Patch 17-0 # 294
  result = ((size / i) != i);
  uni_klee_add_patch(patch_results, 294, result);
  // Patch 18-0 # 295
  constant_a = -10;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 295, result);
  // Patch 18-1 # 296
  constant_a = -9;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 296, result);
  // Patch 18-2 # 297
  constant_a = -8;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 297, result);
  // Patch 18-3 # 298
  constant_a = -7;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 298, result);
  // Patch 18-4 # 299
  constant_a = -6;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 299, result);
  // Patch 18-5 # 300
  constant_a = -5;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 300, result);
  // Patch 18-6 # 301
  constant_a = -4;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 301, result);
  // Patch 18-7 # 302
  constant_a = -3;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 302, result);
  // Patch 18-8 # 303
  constant_a = -2;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 303, result);
  // Patch 18-9 # 304
  constant_a = -1;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 304, result);
  // Patch 18-10 # 305
  constant_a = 0;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 305, result);
  // Patch 18-11 # 306
  constant_a = 1;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 306, result);
  // Patch 18-12 # 307
  constant_a = 2;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 307, result);
  // Patch 18-13 # 308
  constant_a = 3;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 308, result);
  // Patch 18-14 # 309
  constant_a = 4;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 309, result);
  // Patch 18-15 # 310
  constant_a = 5;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 310, result);
  // Patch 18-16 # 311
  constant_a = 6;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 311, result);
  // Patch 18-17 # 312
  constant_a = 7;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 312, result);
  // Patch 18-18 # 313
  constant_a = 8;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 313, result);
  // Patch 18-19 # 314
  constant_a = 9;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 314, result);
  // Patch 18-20 # 315
  constant_a = 10;
  result = ((constant_a / i) != i);
  uni_klee_add_patch(patch_results, 315, result);
  // Patch 19-0 # 316
  result = ((i / size) != i);
  uni_klee_add_patch(patch_results, 316, result);
  // Patch 20-0 # 317
  constant_a = -10;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 317, result);
  // Patch 20-1 # 318
  constant_a = -9;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 318, result);
  // Patch 20-2 # 319
  constant_a = -8;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 319, result);
  // Patch 20-3 # 320
  constant_a = -7;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 320, result);
  // Patch 20-4 # 321
  constant_a = -6;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 321, result);
  // Patch 20-5 # 322
  constant_a = -5;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 322, result);
  // Patch 20-6 # 323
  constant_a = -4;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 323, result);
  // Patch 20-7 # 324
  constant_a = -3;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 324, result);
  // Patch 20-8 # 325
  constant_a = -2;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 325, result);
  // Patch 20-9 # 326
  constant_a = -1;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 326, result);
  // Patch 20-10 # 327
  constant_a = 0;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 327, result);
  // Patch 20-11 # 328
  constant_a = 1;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 328, result);
  // Patch 20-12 # 329
  constant_a = 2;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 329, result);
  // Patch 20-13 # 330
  constant_a = 3;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 330, result);
  // Patch 20-14 # 331
  constant_a = 4;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 331, result);
  // Patch 20-15 # 332
  constant_a = 5;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 332, result);
  // Patch 20-16 # 333
  constant_a = 6;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 333, result);
  // Patch 20-17 # 334
  constant_a = 7;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 334, result);
  // Patch 20-18 # 335
  constant_a = 8;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 335, result);
  // Patch 20-19 # 336
  constant_a = 9;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 336, result);
  // Patch 20-20 # 337
  constant_a = 10;
  result = ((constant_a / size) != i);
  uni_klee_add_patch(patch_results, 337, result);
  // Patch 21-0 # 338
  constant_a = -10;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 338, result);
  // Patch 21-1 # 339
  constant_a = -9;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 339, result);
  // Patch 21-2 # 340
  constant_a = -8;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 340, result);
  // Patch 21-3 # 341
  constant_a = -7;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 341, result);
  // Patch 21-4 # 342
  constant_a = -6;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 342, result);
  // Patch 21-5 # 343
  constant_a = -5;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 343, result);
  // Patch 21-6 # 344
  constant_a = -4;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 344, result);
  // Patch 21-7 # 345
  constant_a = -3;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 345, result);
  // Patch 21-8 # 346
  constant_a = -2;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 346, result);
  // Patch 21-9 # 347
  constant_a = -1;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 347, result);
  // Patch 21-10 # 348
  constant_a = 1;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 348, result);
  // Patch 21-11 # 349
  constant_a = 2;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 349, result);
  // Patch 21-12 # 350
  constant_a = 3;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 350, result);
  // Patch 21-13 # 351
  constant_a = 4;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 351, result);
  // Patch 21-14 # 352
  constant_a = 5;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 352, result);
  // Patch 21-15 # 353
  constant_a = 6;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 353, result);
  // Patch 21-16 # 354
  constant_a = 7;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 354, result);
  // Patch 21-17 # 355
  constant_a = 8;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 355, result);
  // Patch 21-18 # 356
  constant_a = 9;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 356, result);
  // Patch 21-19 # 357
  constant_a = 10;
  result = ((i / constant_a) != i);
  uni_klee_add_patch(patch_results, 357, result);
  // Patch 22-0 # 358
  constant_a = -10;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 358, result);
  // Patch 22-1 # 359
  constant_a = -9;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 359, result);
  // Patch 22-2 # 360
  constant_a = -8;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 360, result);
  // Patch 22-3 # 361
  constant_a = -7;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 361, result);
  // Patch 22-4 # 362
  constant_a = -6;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 362, result);
  // Patch 22-5 # 363
  constant_a = -5;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 363, result);
  // Patch 22-6 # 364
  constant_a = -4;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 364, result);
  // Patch 22-7 # 365
  constant_a = -3;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 365, result);
  // Patch 22-8 # 366
  constant_a = -2;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 366, result);
  // Patch 22-9 # 367
  constant_a = -1;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 367, result);
  // Patch 22-10 # 368
  constant_a = 1;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 368, result);
  // Patch 22-11 # 369
  constant_a = 2;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 369, result);
  // Patch 22-12 # 370
  constant_a = 3;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 370, result);
  // Patch 22-13 # 371
  constant_a = 4;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 371, result);
  // Patch 22-14 # 372
  constant_a = 5;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 372, result);
  // Patch 22-15 # 373
  constant_a = 6;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 373, result);
  // Patch 22-16 # 374
  constant_a = 7;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 374, result);
  // Patch 22-17 # 375
  constant_a = 8;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 375, result);
  // Patch 22-18 # 376
  constant_a = 9;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 376, result);
  // Patch 22-19 # 377
  constant_a = 10;
  result = ((size / constant_a) != i);
  uni_klee_add_patch(patch_results, 377, result);
  // Patch 23-0 # 378
  constant_a = -10;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 378, result);
  // Patch 23-1 # 379
  constant_a = -9;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 379, result);
  // Patch 23-2 # 380
  constant_a = -8;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 380, result);
  // Patch 23-3 # 381
  constant_a = -7;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 381, result);
  // Patch 23-4 # 382
  constant_a = -6;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 382, result);
  // Patch 23-5 # 383
  constant_a = -5;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 383, result);
  // Patch 23-6 # 384
  constant_a = -4;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 384, result);
  // Patch 23-7 # 385
  constant_a = -3;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 385, result);
  // Patch 23-8 # 386
  constant_a = -2;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 386, result);
  // Patch 23-9 # 387
  constant_a = -1;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 387, result);
  // Patch 23-10 # 388
  constant_a = 0;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 388, result);
  // Patch 23-11 # 389
  constant_a = 1;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 389, result);
  // Patch 23-12 # 390
  constant_a = 2;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 390, result);
  // Patch 23-13 # 391
  constant_a = 3;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 391, result);
  // Patch 23-14 # 392
  constant_a = 4;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 392, result);
  // Patch 23-15 # 393
  constant_a = 5;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 393, result);
  // Patch 23-16 # 394
  constant_a = 6;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 394, result);
  // Patch 23-17 # 395
  constant_a = 7;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 395, result);
  // Patch 23-18 # 396
  constant_a = 8;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 396, result);
  // Patch 23-19 # 397
  constant_a = 9;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 397, result);
  // Patch 23-20 # 398
  constant_a = 10;
  result = (constant_a != size);
  uni_klee_add_patch(patch_results, 398, result);
  // Patch 24-0 # 399
  result = ((size / i) != size);
  uni_klee_add_patch(patch_results, 399, result);
  // Patch 25-0 # 400
  constant_a = -10;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 400, result);
  // Patch 25-1 # 401
  constant_a = -9;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 401, result);
  // Patch 25-2 # 402
  constant_a = -8;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 402, result);
  // Patch 25-3 # 403
  constant_a = -7;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 403, result);
  // Patch 25-4 # 404
  constant_a = -6;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 404, result);
  // Patch 25-5 # 405
  constant_a = -5;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 405, result);
  // Patch 25-6 # 406
  constant_a = -4;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 406, result);
  // Patch 25-7 # 407
  constant_a = -3;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 407, result);
  // Patch 25-8 # 408
  constant_a = -2;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 408, result);
  // Patch 25-9 # 409
  constant_a = -1;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 409, result);
  // Patch 25-10 # 410
  constant_a = 0;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 410, result);
  // Patch 25-11 # 411
  constant_a = 1;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 411, result);
  // Patch 25-12 # 412
  constant_a = 2;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 412, result);
  // Patch 25-13 # 413
  constant_a = 3;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 413, result);
  // Patch 25-14 # 414
  constant_a = 4;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 414, result);
  // Patch 25-15 # 415
  constant_a = 5;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 415, result);
  // Patch 25-16 # 416
  constant_a = 6;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 416, result);
  // Patch 25-17 # 417
  constant_a = 7;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 417, result);
  // Patch 25-18 # 418
  constant_a = 8;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 418, result);
  // Patch 25-19 # 419
  constant_a = 9;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 419, result);
  // Patch 25-20 # 420
  constant_a = 10;
  result = ((constant_a / i) != size);
  uni_klee_add_patch(patch_results, 420, result);
  // Patch 26-0 # 421
  result = ((i / size) != size);
  uni_klee_add_patch(patch_results, 421, result);
  // Patch 27-0 # 422
  constant_a = -10;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 422, result);
  // Patch 27-1 # 423
  constant_a = -9;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 423, result);
  // Patch 27-2 # 424
  constant_a = -8;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 424, result);
  // Patch 27-3 # 425
  constant_a = -7;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 425, result);
  // Patch 27-4 # 426
  constant_a = -6;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 426, result);
  // Patch 27-5 # 427
  constant_a = -5;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 427, result);
  // Patch 27-6 # 428
  constant_a = -4;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 428, result);
  // Patch 27-7 # 429
  constant_a = -3;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 429, result);
  // Patch 27-8 # 430
  constant_a = -2;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 430, result);
  // Patch 27-9 # 431
  constant_a = -1;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 431, result);
  // Patch 27-10 # 432
  constant_a = 0;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 432, result);
  // Patch 27-11 # 433
  constant_a = 1;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 433, result);
  // Patch 27-12 # 434
  constant_a = 2;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 434, result);
  // Patch 27-13 # 435
  constant_a = 3;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 435, result);
  // Patch 27-14 # 436
  constant_a = 4;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 436, result);
  // Patch 27-15 # 437
  constant_a = 5;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 437, result);
  // Patch 27-16 # 438
  constant_a = 6;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 438, result);
  // Patch 27-17 # 439
  constant_a = 7;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 439, result);
  // Patch 27-18 # 440
  constant_a = 8;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 440, result);
  // Patch 27-19 # 441
  constant_a = 9;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 441, result);
  // Patch 27-20 # 442
  constant_a = 10;
  result = ((constant_a / size) != size);
  uni_klee_add_patch(patch_results, 442, result);
  // Patch 28-0 # 443
  constant_a = -10;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 443, result);
  // Patch 28-1 # 444
  constant_a = -9;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 444, result);
  // Patch 28-2 # 445
  constant_a = -8;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 445, result);
  // Patch 28-3 # 446
  constant_a = -7;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 446, result);
  // Patch 28-4 # 447
  constant_a = -6;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 447, result);
  // Patch 28-5 # 448
  constant_a = -5;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 448, result);
  // Patch 28-6 # 449
  constant_a = -4;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 449, result);
  // Patch 28-7 # 450
  constant_a = -3;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 450, result);
  // Patch 28-8 # 451
  constant_a = -2;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 451, result);
  // Patch 28-9 # 452
  constant_a = -1;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 452, result);
  // Patch 28-10 # 453
  constant_a = 1;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 453, result);
  // Patch 28-11 # 454
  constant_a = 2;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 454, result);
  // Patch 28-12 # 455
  constant_a = 3;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 455, result);
  // Patch 28-13 # 456
  constant_a = 4;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 456, result);
  // Patch 28-14 # 457
  constant_a = 5;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 457, result);
  // Patch 28-15 # 458
  constant_a = 6;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 458, result);
  // Patch 28-16 # 459
  constant_a = 7;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 459, result);
  // Patch 28-17 # 460
  constant_a = 8;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 460, result);
  // Patch 28-18 # 461
  constant_a = 9;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 461, result);
  // Patch 28-19 # 462
  constant_a = 10;
  result = ((i / constant_a) != size);
  uni_klee_add_patch(patch_results, 462, result);
  // Patch 29-0 # 463
  constant_a = -10;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 463, result);
  // Patch 29-1 # 464
  constant_a = -9;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 464, result);
  // Patch 29-2 # 465
  constant_a = -8;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 465, result);
  // Patch 29-3 # 466
  constant_a = -7;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 466, result);
  // Patch 29-4 # 467
  constant_a = -6;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 467, result);
  // Patch 29-5 # 468
  constant_a = -5;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 468, result);
  // Patch 29-6 # 469
  constant_a = -4;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 469, result);
  // Patch 29-7 # 470
  constant_a = -3;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 470, result);
  // Patch 29-8 # 471
  constant_a = -2;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 471, result);
  // Patch 29-9 # 472
  constant_a = -1;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 472, result);
  // Patch 29-10 # 473
  constant_a = 1;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 473, result);
  // Patch 29-11 # 474
  constant_a = 2;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 474, result);
  // Patch 29-12 # 475
  constant_a = 3;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 475, result);
  // Patch 29-13 # 476
  constant_a = 4;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 476, result);
  // Patch 29-14 # 477
  constant_a = 5;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 477, result);
  // Patch 29-15 # 478
  constant_a = 6;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 478, result);
  // Patch 29-16 # 479
  constant_a = 7;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 479, result);
  // Patch 29-17 # 480
  constant_a = 8;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 480, result);
  // Patch 29-18 # 481
  constant_a = 9;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 481, result);
  // Patch 29-19 # 482
  constant_a = 10;
  result = ((size / constant_a) != size);
  uni_klee_add_patch(patch_results, 482, result);
  // Patch 30-0 # 483
  constant_a = -10;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 483, result);
  // Patch 30-1 # 484
  constant_a = -9;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 484, result);
  // Patch 30-2 # 485
  constant_a = -8;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 485, result);
  // Patch 30-3 # 486
  constant_a = -7;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 486, result);
  // Patch 30-4 # 487
  constant_a = -6;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 487, result);
  // Patch 30-5 # 488
  constant_a = -5;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 488, result);
  // Patch 30-6 # 489
  constant_a = -4;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 489, result);
  // Patch 30-7 # 490
  constant_a = -3;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 490, result);
  // Patch 30-8 # 491
  constant_a = -2;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 491, result);
  // Patch 30-9 # 492
  constant_a = -1;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 492, result);
  // Patch 30-10 # 493
  constant_a = 0;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 493, result);
  // Patch 30-11 # 494
  constant_a = 1;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 494, result);
  // Patch 30-12 # 495
  constant_a = 2;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 495, result);
  // Patch 30-13 # 496
  constant_a = 3;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 496, result);
  // Patch 30-14 # 497
  constant_a = 4;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 497, result);
  // Patch 30-15 # 498
  constant_a = 5;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 498, result);
  // Patch 30-16 # 499
  constant_a = 6;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 499, result);
  // Patch 30-17 # 500
  constant_a = 7;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 500, result);
  // Patch 30-18 # 501
  constant_a = 8;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 501, result);
  // Patch 30-19 # 502
  constant_a = 9;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 502, result);
  // Patch 30-20 # 503
  constant_a = 10;
  result = ((size / i) != constant_a);
  uni_klee_add_patch(patch_results, 503, result);
  // Patch 31-0 # 504
  constant_a = -10;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 504, result);
  // Patch 31-1 # 505
  constant_a = -9;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 505, result);
  // Patch 31-2 # 506
  constant_a = -8;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 506, result);
  // Patch 31-3 # 507
  constant_a = -7;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 507, result);
  // Patch 31-4 # 508
  constant_a = -6;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 508, result);
  // Patch 31-5 # 509
  constant_a = -5;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 509, result);
  // Patch 31-6 # 510
  constant_a = -4;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 510, result);
  // Patch 31-7 # 511
  constant_a = -3;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 511, result);
  // Patch 31-8 # 512
  constant_a = -2;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 512, result);
  // Patch 31-9 # 513
  constant_a = -1;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 513, result);
  // Patch 31-10 # 514
  constant_a = 0;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 514, result);
  // Patch 31-11 # 515
  constant_a = 1;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 515, result);
  // Patch 31-12 # 516
  constant_a = 2;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 516, result);
  // Patch 31-13 # 517
  constant_a = 3;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 517, result);
  // Patch 31-14 # 518
  constant_a = 4;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 518, result);
  // Patch 31-15 # 519
  constant_a = 5;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 519, result);
  // Patch 31-16 # 520
  constant_a = 6;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 520, result);
  // Patch 31-17 # 521
  constant_a = 7;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 521, result);
  // Patch 31-18 # 522
  constant_a = 8;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 522, result);
  // Patch 31-19 # 523
  constant_a = 9;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 523, result);
  // Patch 31-20 # 524
  constant_a = 10;
  result = ((constant_a / i) != constant_a);
  uni_klee_add_patch(patch_results, 524, result);
  // Patch 32-0 # 525
  constant_a = -10;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 525, result);
  // Patch 32-1 # 526
  constant_a = -9;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 526, result);
  // Patch 32-2 # 527
  constant_a = -8;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 527, result);
  // Patch 32-3 # 528
  constant_a = -7;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 528, result);
  // Patch 32-4 # 529
  constant_a = -6;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 529, result);
  // Patch 32-5 # 530
  constant_a = -5;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 530, result);
  // Patch 32-6 # 531
  constant_a = -4;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 531, result);
  // Patch 32-7 # 532
  constant_a = -3;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 532, result);
  // Patch 32-8 # 533
  constant_a = -2;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 533, result);
  // Patch 32-9 # 534
  constant_a = -1;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 534, result);
  // Patch 32-10 # 535
  constant_a = 0;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 535, result);
  // Patch 32-11 # 536
  constant_a = 1;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 536, result);
  // Patch 32-12 # 537
  constant_a = 2;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 537, result);
  // Patch 32-13 # 538
  constant_a = 3;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 538, result);
  // Patch 32-14 # 539
  constant_a = 4;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 539, result);
  // Patch 32-15 # 540
  constant_a = 5;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 540, result);
  // Patch 32-16 # 541
  constant_a = 6;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 541, result);
  // Patch 32-17 # 542
  constant_a = 7;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 542, result);
  // Patch 32-18 # 543
  constant_a = 8;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 543, result);
  // Patch 32-19 # 544
  constant_a = 9;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 544, result);
  // Patch 32-20 # 545
  constant_a = 10;
  result = ((i / size) != constant_a);
  uni_klee_add_patch(patch_results, 545, result);
  // Patch 33-0 # 546
  constant_a = -10;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 546, result);
  // Patch 33-1 # 547
  constant_a = -9;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 547, result);
  // Patch 33-2 # 548
  constant_a = -8;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 548, result);
  // Patch 33-3 # 549
  constant_a = -7;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 549, result);
  // Patch 33-4 # 550
  constant_a = -6;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 550, result);
  // Patch 33-5 # 551
  constant_a = -5;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 551, result);
  // Patch 33-6 # 552
  constant_a = -4;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 552, result);
  // Patch 33-7 # 553
  constant_a = -3;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 553, result);
  // Patch 33-8 # 554
  constant_a = -2;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 554, result);
  // Patch 33-9 # 555
  constant_a = -1;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 555, result);
  // Patch 33-10 # 556
  constant_a = 0;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 556, result);
  // Patch 33-11 # 557
  constant_a = 1;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 557, result);
  // Patch 33-12 # 558
  constant_a = 2;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 558, result);
  // Patch 33-13 # 559
  constant_a = 3;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 559, result);
  // Patch 33-14 # 560
  constant_a = 4;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 560, result);
  // Patch 33-15 # 561
  constant_a = 5;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 561, result);
  // Patch 33-16 # 562
  constant_a = 6;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 562, result);
  // Patch 33-17 # 563
  constant_a = 7;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 563, result);
  // Patch 33-18 # 564
  constant_a = 8;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 564, result);
  // Patch 33-19 # 565
  constant_a = 9;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 565, result);
  // Patch 33-20 # 566
  constant_a = 10;
  result = ((constant_a / size) != constant_a);
  uni_klee_add_patch(patch_results, 566, result);
  // Patch 34-0 # 567
  constant_a = -10;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 567, result);
  // Patch 34-1 # 568
  constant_a = -9;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 568, result);
  // Patch 34-2 # 569
  constant_a = -8;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 569, result);
  // Patch 34-3 # 570
  constant_a = -7;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 570, result);
  // Patch 34-4 # 571
  constant_a = -6;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 571, result);
  // Patch 34-5 # 572
  constant_a = -5;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 572, result);
  // Patch 34-6 # 573
  constant_a = -4;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 573, result);
  // Patch 34-7 # 574
  constant_a = -3;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 574, result);
  // Patch 34-8 # 575
  constant_a = -2;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 575, result);
  // Patch 34-9 # 576
  constant_a = -1;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 576, result);
  // Patch 34-10 # 577
  constant_a = 1;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 577, result);
  // Patch 34-11 # 578
  constant_a = 2;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 578, result);
  // Patch 34-12 # 579
  constant_a = 3;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 579, result);
  // Patch 34-13 # 580
  constant_a = 4;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 580, result);
  // Patch 34-14 # 581
  constant_a = 5;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 581, result);
  // Patch 34-15 # 582
  constant_a = 6;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 582, result);
  // Patch 34-16 # 583
  constant_a = 7;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 583, result);
  // Patch 34-17 # 584
  constant_a = 8;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 584, result);
  // Patch 34-18 # 585
  constant_a = 9;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 585, result);
  // Patch 34-19 # 586
  constant_a = 10;
  result = ((i / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 586, result);
  // Patch 35-0 # 587
  constant_a = -10;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 587, result);
  // Patch 35-1 # 588
  constant_a = -9;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 588, result);
  // Patch 35-2 # 589
  constant_a = -8;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 589, result);
  // Patch 35-3 # 590
  constant_a = -7;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 590, result);
  // Patch 35-4 # 591
  constant_a = -6;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 591, result);
  // Patch 35-5 # 592
  constant_a = -5;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 592, result);
  // Patch 35-6 # 593
  constant_a = -4;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 593, result);
  // Patch 35-7 # 594
  constant_a = -3;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 594, result);
  // Patch 35-8 # 595
  constant_a = -2;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 595, result);
  // Patch 35-9 # 596
  constant_a = -1;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 596, result);
  // Patch 35-10 # 597
  constant_a = 1;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 597, result);
  // Patch 35-11 # 598
  constant_a = 2;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 598, result);
  // Patch 35-12 # 599
  constant_a = 3;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 599, result);
  // Patch 35-13 # 600
  constant_a = 4;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 600, result);
  // Patch 35-14 # 601
  constant_a = 5;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 601, result);
  // Patch 35-15 # 602
  constant_a = 6;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 602, result);
  // Patch 35-16 # 603
  constant_a = 7;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 603, result);
  // Patch 35-17 # 604
  constant_a = 8;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 604, result);
  // Patch 35-18 # 605
  constant_a = 9;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 605, result);
  // Patch 35-19 # 606
  constant_a = 10;
  result = ((size / constant_a) != constant_a);
  uni_klee_add_patch(patch_results, 606, result);
  // Patch 36-0 # 607
  result = ((size / i) != (i / i));
  uni_klee_add_patch(patch_results, 607, result);
  // Patch 37-0 # 608
  constant_a = -10;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 608, result);
  // Patch 37-1 # 609
  constant_a = -9;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 609, result);
  // Patch 37-2 # 610
  constant_a = -8;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 610, result);
  // Patch 37-3 # 611
  constant_a = -7;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 611, result);
  // Patch 37-4 # 612
  constant_a = -6;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 612, result);
  // Patch 37-5 # 613
  constant_a = -5;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 613, result);
  // Patch 37-6 # 614
  constant_a = -4;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 614, result);
  // Patch 37-7 # 615
  constant_a = -3;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 615, result);
  // Patch 37-8 # 616
  constant_a = -2;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 616, result);
  // Patch 37-9 # 617
  constant_a = -1;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 617, result);
  // Patch 37-10 # 618
  constant_a = 0;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 618, result);
  // Patch 37-11 # 619
  constant_a = 1;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 619, result);
  // Patch 37-12 # 620
  constant_a = 2;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 620, result);
  // Patch 37-13 # 621
  constant_a = 3;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 621, result);
  // Patch 37-14 # 622
  constant_a = 4;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 622, result);
  // Patch 37-15 # 623
  constant_a = 5;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 623, result);
  // Patch 37-16 # 624
  constant_a = 6;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 624, result);
  // Patch 37-17 # 625
  constant_a = 7;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 625, result);
  // Patch 37-18 # 626
  constant_a = 8;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 626, result);
  // Patch 37-19 # 627
  constant_a = 9;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 627, result);
  // Patch 37-20 # 628
  constant_a = 10;
  result = (constant_a < i);
  uni_klee_add_patch(patch_results, 628, result);
  // Patch 38-0 # 629
  result = ((size / i) < i);
  uni_klee_add_patch(patch_results, 629, result);
  // Patch 39-0 # 630
  constant_a = -10;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 630, result);
  // Patch 39-1 # 631
  constant_a = -9;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 631, result);
  // Patch 39-2 # 632
  constant_a = -8;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 632, result);
  // Patch 39-3 # 633
  constant_a = -7;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 633, result);
  // Patch 39-4 # 634
  constant_a = -6;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 634, result);
  // Patch 39-5 # 635
  constant_a = -5;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 635, result);
  // Patch 39-6 # 636
  constant_a = -4;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 636, result);
  // Patch 39-7 # 637
  constant_a = -3;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 637, result);
  // Patch 39-8 # 638
  constant_a = -2;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 638, result);
  // Patch 39-9 # 639
  constant_a = -1;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 639, result);
  // Patch 39-10 # 640
  constant_a = 0;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 640, result);
  // Patch 39-11 # 641
  constant_a = 1;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 641, result);
  // Patch 39-12 # 642
  constant_a = 2;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 642, result);
  // Patch 39-13 # 643
  constant_a = 3;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 643, result);
  // Patch 39-14 # 644
  constant_a = 4;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 644, result);
  // Patch 39-15 # 645
  constant_a = 5;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 645, result);
  // Patch 39-16 # 646
  constant_a = 6;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 646, result);
  // Patch 39-17 # 647
  constant_a = 7;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 647, result);
  // Patch 39-18 # 648
  constant_a = 8;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 648, result);
  // Patch 39-19 # 649
  constant_a = 9;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 649, result);
  // Patch 39-20 # 650
  constant_a = 10;
  result = ((constant_a / i) < i);
  uni_klee_add_patch(patch_results, 650, result);
  // Patch 40-0 # 651
  result = ((i / size) < i);
  uni_klee_add_patch(patch_results, 651, result);
  // Patch 41-0 # 652
  constant_a = -10;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 652, result);
  // Patch 41-1 # 653
  constant_a = -9;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 653, result);
  // Patch 41-2 # 654
  constant_a = -8;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 654, result);
  // Patch 41-3 # 655
  constant_a = -7;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 655, result);
  // Patch 41-4 # 656
  constant_a = -6;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 656, result);
  // Patch 41-5 # 657
  constant_a = -5;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 657, result);
  // Patch 41-6 # 658
  constant_a = -4;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 658, result);
  // Patch 41-7 # 659
  constant_a = -3;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 659, result);
  // Patch 41-8 # 660
  constant_a = -2;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 660, result);
  // Patch 41-9 # 661
  constant_a = -1;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 661, result);
  // Patch 41-10 # 662
  constant_a = 0;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 662, result);
  // Patch 41-11 # 663
  constant_a = 1;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 663, result);
  // Patch 41-12 # 664
  constant_a = 2;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 664, result);
  // Patch 41-13 # 665
  constant_a = 3;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 665, result);
  // Patch 41-14 # 666
  constant_a = 4;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 666, result);
  // Patch 41-15 # 667
  constant_a = 5;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 667, result);
  // Patch 41-16 # 668
  constant_a = 6;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 668, result);
  // Patch 41-17 # 669
  constant_a = 7;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 669, result);
  // Patch 41-18 # 670
  constant_a = 8;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 670, result);
  // Patch 41-19 # 671
  constant_a = 9;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 671, result);
  // Patch 41-20 # 672
  constant_a = 10;
  result = ((constant_a / size) < i);
  uni_klee_add_patch(patch_results, 672, result);
  // Patch 42-0 # 673
  constant_a = -10;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 673, result);
  // Patch 42-1 # 674
  constant_a = -9;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 674, result);
  // Patch 42-2 # 675
  constant_a = -8;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 675, result);
  // Patch 42-3 # 676
  constant_a = -7;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 676, result);
  // Patch 42-4 # 677
  constant_a = -6;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 677, result);
  // Patch 42-5 # 678
  constant_a = -5;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 678, result);
  // Patch 42-6 # 679
  constant_a = -4;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 679, result);
  // Patch 42-7 # 680
  constant_a = -3;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 680, result);
  // Patch 42-8 # 681
  constant_a = -2;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 681, result);
  // Patch 42-9 # 682
  constant_a = -1;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 682, result);
  // Patch 42-10 # 683
  constant_a = 1;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 683, result);
  // Patch 42-11 # 684
  constant_a = 2;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 684, result);
  // Patch 42-12 # 685
  constant_a = 3;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 685, result);
  // Patch 42-13 # 686
  constant_a = 4;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 686, result);
  // Patch 42-14 # 687
  constant_a = 5;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 687, result);
  // Patch 42-15 # 688
  constant_a = 6;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 688, result);
  // Patch 42-16 # 689
  constant_a = 7;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 689, result);
  // Patch 42-17 # 690
  constant_a = 8;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 690, result);
  // Patch 42-18 # 691
  constant_a = 9;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 691, result);
  // Patch 42-19 # 692
  constant_a = 10;
  result = ((i / constant_a) < i);
  uni_klee_add_patch(patch_results, 692, result);
  // Patch 43-0 # 693
  constant_a = -10;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 693, result);
  // Patch 43-1 # 694
  constant_a = -9;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 694, result);
  // Patch 43-2 # 695
  constant_a = -8;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 695, result);
  // Patch 43-3 # 696
  constant_a = -7;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 696, result);
  // Patch 43-4 # 697
  constant_a = -6;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 697, result);
  // Patch 43-5 # 698
  constant_a = -5;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 698, result);
  // Patch 43-6 # 699
  constant_a = -4;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 699, result);
  // Patch 43-7 # 700
  constant_a = -3;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 700, result);
  // Patch 43-8 # 701
  constant_a = -2;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 701, result);
  // Patch 43-9 # 702
  constant_a = -1;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 702, result);
  // Patch 43-10 # 703
  constant_a = 1;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 703, result);
  // Patch 43-11 # 704
  constant_a = 2;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 704, result);
  // Patch 43-12 # 705
  constant_a = 3;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 705, result);
  // Patch 43-13 # 706
  constant_a = 4;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 706, result);
  // Patch 43-14 # 707
  constant_a = 5;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 707, result);
  // Patch 43-15 # 708
  constant_a = 6;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 708, result);
  // Patch 43-16 # 709
  constant_a = 7;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 709, result);
  // Patch 43-17 # 710
  constant_a = 8;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 710, result);
  // Patch 43-18 # 711
  constant_a = 9;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 711, result);
  // Patch 43-19 # 712
  constant_a = 10;
  result = ((size / constant_a) < i);
  uni_klee_add_patch(patch_results, 712, result);
  // Patch 44-0 # 713
  result = (i < size);
  uni_klee_add_patch(patch_results, 713, result);
  // Patch 45-0 # 714
  constant_a = -10;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 714, result);
  // Patch 45-1 # 715
  constant_a = -9;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 715, result);
  // Patch 45-2 # 716
  constant_a = -8;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 716, result);
  // Patch 45-3 # 717
  constant_a = -7;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 717, result);
  // Patch 45-4 # 718
  constant_a = -6;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 718, result);
  // Patch 45-5 # 719
  constant_a = -5;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 719, result);
  // Patch 45-6 # 720
  constant_a = -4;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 720, result);
  // Patch 45-7 # 721
  constant_a = -3;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 721, result);
  // Patch 45-8 # 722
  constant_a = -2;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 722, result);
  // Patch 45-9 # 723
  constant_a = -1;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 723, result);
  // Patch 45-10 # 724
  constant_a = 0;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 724, result);
  // Patch 45-11 # 725
  constant_a = 1;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 725, result);
  // Patch 45-12 # 726
  constant_a = 2;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 726, result);
  // Patch 45-13 # 727
  constant_a = 3;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 727, result);
  // Patch 45-14 # 728
  constant_a = 4;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 728, result);
  // Patch 45-15 # 729
  constant_a = 5;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 729, result);
  // Patch 45-16 # 730
  constant_a = 6;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 730, result);
  // Patch 45-17 # 731
  constant_a = 7;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 731, result);
  // Patch 45-18 # 732
  constant_a = 8;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 732, result);
  // Patch 45-19 # 733
  constant_a = 9;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 733, result);
  // Patch 45-20 # 734
  constant_a = 10;
  result = (constant_a < size);
  uni_klee_add_patch(patch_results, 734, result);
  // Patch 46-0 # 735
  result = ((size / i) < size);
  uni_klee_add_patch(patch_results, 735, result);
  // Patch 47-0 # 736
  constant_a = -10;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 736, result);
  // Patch 47-1 # 737
  constant_a = -9;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 737, result);
  // Patch 47-2 # 738
  constant_a = -8;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 738, result);
  // Patch 47-3 # 739
  constant_a = -7;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 739, result);
  // Patch 47-4 # 740
  constant_a = -6;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 740, result);
  // Patch 47-5 # 741
  constant_a = -5;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 741, result);
  // Patch 47-6 # 742
  constant_a = -4;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 742, result);
  // Patch 47-7 # 743
  constant_a = -3;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 743, result);
  // Patch 47-8 # 744
  constant_a = -2;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 744, result);
  // Patch 47-9 # 745
  constant_a = -1;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 745, result);
  // Patch 47-10 # 746
  constant_a = 0;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 746, result);
  // Patch 47-11 # 747
  constant_a = 1;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 747, result);
  // Patch 47-12 # 748
  constant_a = 2;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 748, result);
  // Patch 47-13 # 749
  constant_a = 3;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 749, result);
  // Patch 47-14 # 750
  constant_a = 4;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 750, result);
  // Patch 47-15 # 751
  constant_a = 5;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 751, result);
  // Patch 47-16 # 752
  constant_a = 6;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 752, result);
  // Patch 47-17 # 753
  constant_a = 7;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 753, result);
  // Patch 47-18 # 754
  constant_a = 8;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 754, result);
  // Patch 47-19 # 755
  constant_a = 9;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 755, result);
  // Patch 47-20 # 756
  constant_a = 10;
  result = ((constant_a / i) < size);
  uni_klee_add_patch(patch_results, 756, result);
  // Patch 48-0 # 757
  result = ((i / size) < size);
  uni_klee_add_patch(patch_results, 757, result);
  // Patch 49-0 # 758
  constant_a = -10;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 758, result);
  // Patch 49-1 # 759
  constant_a = -9;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 759, result);
  // Patch 49-2 # 760
  constant_a = -8;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 760, result);
  // Patch 49-3 # 761
  constant_a = -7;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 761, result);
  // Patch 49-4 # 762
  constant_a = -6;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 762, result);
  // Patch 49-5 # 763
  constant_a = -5;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 763, result);
  // Patch 49-6 # 764
  constant_a = -4;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 764, result);
  // Patch 49-7 # 765
  constant_a = -3;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 765, result);
  // Patch 49-8 # 766
  constant_a = -2;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 766, result);
  // Patch 49-9 # 767
  constant_a = -1;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 767, result);
  // Patch 49-10 # 768
  constant_a = 0;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 768, result);
  // Patch 49-11 # 769
  constant_a = 1;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 769, result);
  // Patch 49-12 # 770
  constant_a = 2;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 770, result);
  // Patch 49-13 # 771
  constant_a = 3;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 771, result);
  // Patch 49-14 # 772
  constant_a = 4;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 772, result);
  // Patch 49-15 # 773
  constant_a = 5;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 773, result);
  // Patch 49-16 # 774
  constant_a = 6;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 774, result);
  // Patch 49-17 # 775
  constant_a = 7;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 775, result);
  // Patch 49-18 # 776
  constant_a = 8;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 776, result);
  // Patch 49-19 # 777
  constant_a = 9;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 777, result);
  // Patch 49-20 # 778
  constant_a = 10;
  result = ((constant_a / size) < size);
  uni_klee_add_patch(patch_results, 778, result);
  // Patch 50-0 # 779
  constant_a = -10;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 779, result);
  // Patch 50-1 # 780
  constant_a = -9;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 780, result);
  // Patch 50-2 # 781
  constant_a = -8;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 781, result);
  // Patch 50-3 # 782
  constant_a = -7;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 782, result);
  // Patch 50-4 # 783
  constant_a = -6;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 783, result);
  // Patch 50-5 # 784
  constant_a = -5;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 784, result);
  // Patch 50-6 # 785
  constant_a = -4;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 785, result);
  // Patch 50-7 # 786
  constant_a = -3;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 786, result);
  // Patch 50-8 # 787
  constant_a = -2;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 787, result);
  // Patch 50-9 # 788
  constant_a = -1;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 788, result);
  // Patch 50-10 # 789
  constant_a = 1;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 789, result);
  // Patch 50-11 # 790
  constant_a = 2;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 790, result);
  // Patch 50-12 # 791
  constant_a = 3;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 791, result);
  // Patch 50-13 # 792
  constant_a = 4;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 792, result);
  // Patch 50-14 # 793
  constant_a = 5;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 793, result);
  // Patch 50-15 # 794
  constant_a = 6;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 794, result);
  // Patch 50-16 # 795
  constant_a = 7;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 795, result);
  // Patch 50-17 # 796
  constant_a = 8;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 796, result);
  // Patch 50-18 # 797
  constant_a = 9;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 797, result);
  // Patch 50-19 # 798
  constant_a = 10;
  result = ((i / constant_a) < size);
  uni_klee_add_patch(patch_results, 798, result);
  // Patch 51-0 # 799
  constant_a = -10;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 799, result);
  // Patch 51-1 # 800
  constant_a = -9;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 800, result);
  // Patch 51-2 # 801
  constant_a = -8;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 801, result);
  // Patch 51-3 # 802
  constant_a = -7;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 802, result);
  // Patch 51-4 # 803
  constant_a = -6;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 803, result);
  // Patch 51-5 # 804
  constant_a = -5;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 804, result);
  // Patch 51-6 # 805
  constant_a = -4;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 805, result);
  // Patch 51-7 # 806
  constant_a = -3;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 806, result);
  // Patch 51-8 # 807
  constant_a = -2;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 807, result);
  // Patch 51-9 # 808
  constant_a = -1;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 808, result);
  // Patch 51-10 # 809
  constant_a = 1;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 809, result);
  // Patch 51-11 # 810
  constant_a = 2;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 810, result);
  // Patch 51-12 # 811
  constant_a = 3;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 811, result);
  // Patch 51-13 # 812
  constant_a = 4;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 812, result);
  // Patch 51-14 # 813
  constant_a = 5;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 813, result);
  // Patch 51-15 # 814
  constant_a = 6;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 814, result);
  // Patch 51-16 # 815
  constant_a = 7;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 815, result);
  // Patch 51-17 # 816
  constant_a = 8;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 816, result);
  // Patch 51-18 # 817
  constant_a = 9;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 817, result);
  // Patch 51-19 # 818
  constant_a = 10;
  result = ((size / constant_a) < size);
  uni_klee_add_patch(patch_results, 818, result);
  // Patch 52-0 # 819
  constant_a = -10;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 819, result);
  // Patch 52-1 # 820
  constant_a = -9;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 820, result);
  // Patch 52-2 # 821
  constant_a = -8;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 821, result);
  // Patch 52-3 # 822
  constant_a = -7;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 822, result);
  // Patch 52-4 # 823
  constant_a = -6;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 823, result);
  // Patch 52-5 # 824
  constant_a = -5;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 824, result);
  // Patch 52-6 # 825
  constant_a = -4;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 825, result);
  // Patch 52-7 # 826
  constant_a = -3;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 826, result);
  // Patch 52-8 # 827
  constant_a = -2;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 827, result);
  // Patch 52-9 # 828
  constant_a = -1;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 828, result);
  // Patch 52-10 # 829
  constant_a = 0;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 829, result);
  // Patch 52-11 # 830
  constant_a = 1;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 830, result);
  // Patch 52-12 # 831
  constant_a = 2;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 831, result);
  // Patch 52-13 # 832
  constant_a = 3;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 832, result);
  // Patch 52-14 # 833
  constant_a = 4;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 833, result);
  // Patch 52-15 # 834
  constant_a = 5;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 834, result);
  // Patch 52-16 # 835
  constant_a = 6;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 835, result);
  // Patch 52-17 # 836
  constant_a = 7;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 836, result);
  // Patch 52-18 # 837
  constant_a = 8;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 837, result);
  // Patch 52-19 # 838
  constant_a = 9;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 838, result);
  // Patch 52-20 # 839
  constant_a = 10;
  result = (i < constant_a);
  uni_klee_add_patch(patch_results, 839, result);
  // Patch 53-0 # 840
  constant_a = -10;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 840, result);
  // Patch 53-1 # 841
  constant_a = -9;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 841, result);
  // Patch 53-2 # 842
  constant_a = -8;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 842, result);
  // Patch 53-3 # 843
  constant_a = -7;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 843, result);
  // Patch 53-4 # 844
  constant_a = -6;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 844, result);
  // Patch 53-5 # 845
  constant_a = -5;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 845, result);
  // Patch 53-6 # 846
  constant_a = -4;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 846, result);
  // Patch 53-7 # 847
  constant_a = -3;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 847, result);
  // Patch 53-8 # 848
  constant_a = -2;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 848, result);
  // Patch 53-9 # 849
  constant_a = -1;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 849, result);
  // Patch 53-10 # 850
  constant_a = 0;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 850, result);
  // Patch 53-11 # 851
  constant_a = 1;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 851, result);
  // Patch 53-12 # 852
  constant_a = 2;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 852, result);
  // Patch 53-13 # 853
  constant_a = 3;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 853, result);
  // Patch 53-14 # 854
  constant_a = 4;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 854, result);
  // Patch 53-15 # 855
  constant_a = 5;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 855, result);
  // Patch 53-16 # 856
  constant_a = 6;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 856, result);
  // Patch 53-17 # 857
  constant_a = 7;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 857, result);
  // Patch 53-18 # 858
  constant_a = 8;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 858, result);
  // Patch 53-19 # 859
  constant_a = 9;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 859, result);
  // Patch 53-20 # 860
  constant_a = 10;
  result = (size < constant_a);
  uni_klee_add_patch(patch_results, 860, result);
  // Patch 54-0 # 861
  constant_a = -10;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 861, result);
  // Patch 54-1 # 862
  constant_a = -9;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 862, result);
  // Patch 54-2 # 863
  constant_a = -8;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 863, result);
  // Patch 54-3 # 864
  constant_a = -7;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 864, result);
  // Patch 54-4 # 865
  constant_a = -6;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 865, result);
  // Patch 54-5 # 866
  constant_a = -5;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 866, result);
  // Patch 54-6 # 867
  constant_a = -4;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 867, result);
  // Patch 54-7 # 868
  constant_a = -3;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 868, result);
  // Patch 54-8 # 869
  constant_a = -2;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 869, result);
  // Patch 54-9 # 870
  constant_a = -1;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 870, result);
  // Patch 54-10 # 871
  constant_a = 0;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 871, result);
  // Patch 54-11 # 872
  constant_a = 1;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 872, result);
  // Patch 54-12 # 873
  constant_a = 2;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 873, result);
  // Patch 54-13 # 874
  constant_a = 3;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 874, result);
  // Patch 54-14 # 875
  constant_a = 4;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 875, result);
  // Patch 54-15 # 876
  constant_a = 5;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 876, result);
  // Patch 54-16 # 877
  constant_a = 6;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 877, result);
  // Patch 54-17 # 878
  constant_a = 7;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 878, result);
  // Patch 54-18 # 879
  constant_a = 8;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 879, result);
  // Patch 54-19 # 880
  constant_a = 9;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 880, result);
  // Patch 54-20 # 881
  constant_a = 10;
  result = ((size / i) < constant_a);
  uni_klee_add_patch(patch_results, 881, result);
  // Patch 55-0 # 882
  constant_a = -10;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 882, result);
  // Patch 55-1 # 883
  constant_a = -9;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 883, result);
  // Patch 55-2 # 884
  constant_a = -8;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 884, result);
  // Patch 55-3 # 885
  constant_a = -7;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 885, result);
  // Patch 55-4 # 886
  constant_a = -6;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 886, result);
  // Patch 55-5 # 887
  constant_a = -5;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 887, result);
  // Patch 55-6 # 888
  constant_a = -4;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 888, result);
  // Patch 55-7 # 889
  constant_a = -3;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 889, result);
  // Patch 55-8 # 890
  constant_a = -2;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 890, result);
  // Patch 55-9 # 891
  constant_a = -1;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 891, result);
  // Patch 55-10 # 892
  constant_a = 0;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 892, result);
  // Patch 55-11 # 893
  constant_a = 1;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 893, result);
  // Patch 55-12 # 894
  constant_a = 2;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 894, result);
  // Patch 55-13 # 895
  constant_a = 3;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 895, result);
  // Patch 55-14 # 896
  constant_a = 4;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 896, result);
  // Patch 55-15 # 897
  constant_a = 5;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 897, result);
  // Patch 55-16 # 898
  constant_a = 6;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 898, result);
  // Patch 55-17 # 899
  constant_a = 7;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 899, result);
  // Patch 55-18 # 900
  constant_a = 8;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 900, result);
  // Patch 55-19 # 901
  constant_a = 9;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 901, result);
  // Patch 55-20 # 902
  constant_a = 10;
  result = ((constant_a / i) < constant_a);
  uni_klee_add_patch(patch_results, 902, result);
  // Patch 56-0 # 903
  constant_a = -10;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 903, result);
  // Patch 56-1 # 904
  constant_a = -9;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 904, result);
  // Patch 56-2 # 905
  constant_a = -8;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 905, result);
  // Patch 56-3 # 906
  constant_a = -7;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 906, result);
  // Patch 56-4 # 907
  constant_a = -6;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 907, result);
  // Patch 56-5 # 908
  constant_a = -5;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 908, result);
  // Patch 56-6 # 909
  constant_a = -4;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 909, result);
  // Patch 56-7 # 910
  constant_a = -3;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 910, result);
  // Patch 56-8 # 911
  constant_a = -2;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 911, result);
  // Patch 56-9 # 912
  constant_a = -1;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 912, result);
  // Patch 56-10 # 913
  constant_a = 0;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 913, result);
  // Patch 56-11 # 914
  constant_a = 1;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 914, result);
  // Patch 56-12 # 915
  constant_a = 2;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 915, result);
  // Patch 56-13 # 916
  constant_a = 3;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 916, result);
  // Patch 56-14 # 917
  constant_a = 4;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 917, result);
  // Patch 56-15 # 918
  constant_a = 5;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 918, result);
  // Patch 56-16 # 919
  constant_a = 6;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 919, result);
  // Patch 56-17 # 920
  constant_a = 7;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 920, result);
  // Patch 56-18 # 921
  constant_a = 8;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 921, result);
  // Patch 56-19 # 922
  constant_a = 9;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 922, result);
  // Patch 56-20 # 923
  constant_a = 10;
  result = ((i / size) < constant_a);
  uni_klee_add_patch(patch_results, 923, result);
  // Patch 57-0 # 924
  constant_a = -10;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 924, result);
  // Patch 57-1 # 925
  constant_a = -9;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 925, result);
  // Patch 57-2 # 926
  constant_a = -8;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 926, result);
  // Patch 57-3 # 927
  constant_a = -7;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 927, result);
  // Patch 57-4 # 928
  constant_a = -6;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 928, result);
  // Patch 57-5 # 929
  constant_a = -5;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 929, result);
  // Patch 57-6 # 930
  constant_a = -4;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 930, result);
  // Patch 57-7 # 931
  constant_a = -3;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 931, result);
  // Patch 57-8 # 932
  constant_a = -2;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 932, result);
  // Patch 57-9 # 933
  constant_a = -1;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 933, result);
  // Patch 57-10 # 934
  constant_a = 0;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 934, result);
  // Patch 57-11 # 935
  constant_a = 1;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 935, result);
  // Patch 57-12 # 936
  constant_a = 2;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 936, result);
  // Patch 57-13 # 937
  constant_a = 3;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 937, result);
  // Patch 57-14 # 938
  constant_a = 4;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 938, result);
  // Patch 57-15 # 939
  constant_a = 5;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 939, result);
  // Patch 57-16 # 940
  constant_a = 6;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 940, result);
  // Patch 57-17 # 941
  constant_a = 7;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 941, result);
  // Patch 57-18 # 942
  constant_a = 8;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 942, result);
  // Patch 57-19 # 943
  constant_a = 9;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 943, result);
  // Patch 57-20 # 944
  constant_a = 10;
  result = ((constant_a / size) < constant_a);
  uni_klee_add_patch(patch_results, 944, result);
  // Patch 58-0 # 945
  constant_a = -10;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 945, result);
  // Patch 58-1 # 946
  constant_a = -9;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 946, result);
  // Patch 58-2 # 947
  constant_a = -8;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 947, result);
  // Patch 58-3 # 948
  constant_a = -7;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 948, result);
  // Patch 58-4 # 949
  constant_a = -6;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 949, result);
  // Patch 58-5 # 950
  constant_a = -5;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 950, result);
  // Patch 58-6 # 951
  constant_a = -4;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 951, result);
  // Patch 58-7 # 952
  constant_a = -3;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 952, result);
  // Patch 58-8 # 953
  constant_a = -2;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 953, result);
  // Patch 58-9 # 954
  constant_a = -1;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 954, result);
  // Patch 58-10 # 955
  constant_a = 1;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 955, result);
  // Patch 58-11 # 956
  constant_a = 2;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 956, result);
  // Patch 58-12 # 957
  constant_a = 3;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 957, result);
  // Patch 58-13 # 958
  constant_a = 4;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 958, result);
  // Patch 58-14 # 959
  constant_a = 5;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 959, result);
  // Patch 58-15 # 960
  constant_a = 6;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 960, result);
  // Patch 58-16 # 961
  constant_a = 7;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 961, result);
  // Patch 58-17 # 962
  constant_a = 8;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 962, result);
  // Patch 58-18 # 963
  constant_a = 9;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 963, result);
  // Patch 58-19 # 964
  constant_a = 10;
  result = ((i / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 964, result);
  // Patch 59-0 # 965
  constant_a = -10;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 965, result);
  // Patch 59-1 # 966
  constant_a = -9;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 966, result);
  // Patch 59-2 # 967
  constant_a = -8;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 967, result);
  // Patch 59-3 # 968
  constant_a = -7;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 968, result);
  // Patch 59-4 # 969
  constant_a = -6;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 969, result);
  // Patch 59-5 # 970
  constant_a = -5;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 970, result);
  // Patch 59-6 # 971
  constant_a = -4;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 971, result);
  // Patch 59-7 # 972
  constant_a = -3;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 972, result);
  // Patch 59-8 # 973
  constant_a = -2;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 973, result);
  // Patch 59-9 # 974
  constant_a = -1;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 974, result);
  // Patch 59-10 # 975
  constant_a = 1;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 975, result);
  // Patch 59-11 # 976
  constant_a = 2;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 976, result);
  // Patch 59-12 # 977
  constant_a = 3;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 977, result);
  // Patch 59-13 # 978
  constant_a = 4;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 978, result);
  // Patch 59-14 # 979
  constant_a = 5;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 979, result);
  // Patch 59-15 # 980
  constant_a = 6;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 980, result);
  // Patch 59-16 # 981
  constant_a = 7;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 981, result);
  // Patch 59-17 # 982
  constant_a = 8;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 982, result);
  // Patch 59-18 # 983
  constant_a = 9;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 983, result);
  // Patch 59-19 # 984
  constant_a = 10;
  result = ((size / constant_a) < constant_a);
  uni_klee_add_patch(patch_results, 984, result);
  // Patch 60-0 # 985
  constant_a = -10;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 985, result);
  // Patch 60-1 # 986
  constant_a = -9;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 986, result);
  // Patch 60-2 # 987
  constant_a = -8;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 987, result);
  // Patch 60-3 # 988
  constant_a = -7;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 988, result);
  // Patch 60-4 # 989
  constant_a = -6;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 989, result);
  // Patch 60-5 # 990
  constant_a = -5;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 990, result);
  // Patch 60-6 # 991
  constant_a = -4;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 991, result);
  // Patch 60-7 # 992
  constant_a = -3;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 992, result);
  // Patch 60-8 # 993
  constant_a = -2;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 993, result);
  // Patch 60-9 # 994
  constant_a = -1;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 994, result);
  // Patch 60-10 # 995
  constant_a = 0;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 995, result);
  // Patch 60-11 # 996
  constant_a = 1;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 996, result);
  // Patch 60-12 # 997
  constant_a = 2;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 997, result);
  // Patch 60-13 # 998
  constant_a = 3;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 998, result);
  // Patch 60-14 # 999
  constant_a = 4;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 999, result);
  // Patch 60-15 # 1000
  constant_a = 5;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 1000, result);
  // Patch 60-16 # 1001
  constant_a = 6;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 1001, result);
  // Patch 60-17 # 1002
  constant_a = 7;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 1002, result);
  // Patch 60-18 # 1003
  constant_a = 8;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 1003, result);
  // Patch 60-19 # 1004
  constant_a = 9;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 1004, result);
  // Patch 60-20 # 1005
  constant_a = 10;
  result = (constant_a < (size / i));
  uni_klee_add_patch(patch_results, 1005, result);
  // Patch 61-0 # 1006
  constant_a = -10;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1006, result);
  // Patch 61-1 # 1007
  constant_a = -9;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1007, result);
  // Patch 61-2 # 1008
  constant_a = -8;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1008, result);
  // Patch 61-3 # 1009
  constant_a = -7;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1009, result);
  // Patch 61-4 # 1010
  constant_a = -6;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1010, result);
  // Patch 61-5 # 1011
  constant_a = -5;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1011, result);
  // Patch 61-6 # 1012
  constant_a = -4;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1012, result);
  // Patch 61-7 # 1013
  constant_a = -3;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1013, result);
  // Patch 61-8 # 1014
  constant_a = -2;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1014, result);
  // Patch 61-9 # 1015
  constant_a = -1;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1015, result);
  // Patch 61-10 # 1016
  constant_a = 0;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1016, result);
  // Patch 61-11 # 1017
  constant_a = 1;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1017, result);
  // Patch 61-12 # 1018
  constant_a = 2;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1018, result);
  // Patch 61-13 # 1019
  constant_a = 3;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1019, result);
  // Patch 61-14 # 1020
  constant_a = 4;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1020, result);
  // Patch 61-15 # 1021
  constant_a = 5;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1021, result);
  // Patch 61-16 # 1022
  constant_a = 6;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1022, result);
  // Patch 61-17 # 1023
  constant_a = 7;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1023, result);
  // Patch 61-18 # 1024
  constant_a = 8;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1024, result);
  // Patch 61-19 # 1025
  constant_a = 9;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1025, result);
  // Patch 61-20 # 1026
  constant_a = 10;
  result = (i < (constant_a / i));
  uni_klee_add_patch(patch_results, 1026, result);
  // Patch 62-0 # 1027
  constant_a = -10;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1027, result);
  // Patch 62-1 # 1028
  constant_a = -9;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1028, result);
  // Patch 62-2 # 1029
  constant_a = -8;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1029, result);
  // Patch 62-3 # 1030
  constant_a = -7;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1030, result);
  // Patch 62-4 # 1031
  constant_a = -6;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1031, result);
  // Patch 62-5 # 1032
  constant_a = -5;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1032, result);
  // Patch 62-6 # 1033
  constant_a = -4;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1033, result);
  // Patch 62-7 # 1034
  constant_a = -3;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1034, result);
  // Patch 62-8 # 1035
  constant_a = -2;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1035, result);
  // Patch 62-9 # 1036
  constant_a = -1;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1036, result);
  // Patch 62-10 # 1037
  constant_a = 0;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1037, result);
  // Patch 62-11 # 1038
  constant_a = 1;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1038, result);
  // Patch 62-12 # 1039
  constant_a = 2;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1039, result);
  // Patch 62-13 # 1040
  constant_a = 3;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1040, result);
  // Patch 62-14 # 1041
  constant_a = 4;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1041, result);
  // Patch 62-15 # 1042
  constant_a = 5;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1042, result);
  // Patch 62-16 # 1043
  constant_a = 6;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1043, result);
  // Patch 62-17 # 1044
  constant_a = 7;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1044, result);
  // Patch 62-18 # 1045
  constant_a = 8;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1045, result);
  // Patch 62-19 # 1046
  constant_a = 9;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1046, result);
  // Patch 62-20 # 1047
  constant_a = 10;
  result = (size < (constant_a / i));
  uni_klee_add_patch(patch_results, 1047, result);
  // Patch 63-0 # 1048
  constant_a = -10;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1048, result);
  // Patch 63-1 # 1049
  constant_a = -9;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1049, result);
  // Patch 63-2 # 1050
  constant_a = -8;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1050, result);
  // Patch 63-3 # 1051
  constant_a = -7;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1051, result);
  // Patch 63-4 # 1052
  constant_a = -6;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1052, result);
  // Patch 63-5 # 1053
  constant_a = -5;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1053, result);
  // Patch 63-6 # 1054
  constant_a = -4;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1054, result);
  // Patch 63-7 # 1055
  constant_a = -3;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1055, result);
  // Patch 63-8 # 1056
  constant_a = -2;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1056, result);
  // Patch 63-9 # 1057
  constant_a = -1;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1057, result);
  // Patch 63-10 # 1058
  constant_a = 0;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1058, result);
  // Patch 63-11 # 1059
  constant_a = 1;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1059, result);
  // Patch 63-12 # 1060
  constant_a = 2;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1060, result);
  // Patch 63-13 # 1061
  constant_a = 3;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1061, result);
  // Patch 63-14 # 1062
  constant_a = 4;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1062, result);
  // Patch 63-15 # 1063
  constant_a = 5;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1063, result);
  // Patch 63-16 # 1064
  constant_a = 6;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1064, result);
  // Patch 63-17 # 1065
  constant_a = 7;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1065, result);
  // Patch 63-18 # 1066
  constant_a = 8;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1066, result);
  // Patch 63-19 # 1067
  constant_a = 9;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1067, result);
  // Patch 63-20 # 1068
  constant_a = 10;
  result = (constant_a < (constant_a / i));
  uni_klee_add_patch(patch_results, 1068, result);
  // Patch 64-0 # 1069
  constant_a = -10;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1069, result);
  // Patch 64-1 # 1070
  constant_a = -9;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1070, result);
  // Patch 64-2 # 1071
  constant_a = -8;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1071, result);
  // Patch 64-3 # 1072
  constant_a = -7;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1072, result);
  // Patch 64-4 # 1073
  constant_a = -6;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1073, result);
  // Patch 64-5 # 1074
  constant_a = -5;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1074, result);
  // Patch 64-6 # 1075
  constant_a = -4;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1075, result);
  // Patch 64-7 # 1076
  constant_a = -3;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1076, result);
  // Patch 64-8 # 1077
  constant_a = -2;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1077, result);
  // Patch 64-9 # 1078
  constant_a = -1;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1078, result);
  // Patch 64-10 # 1079
  constant_a = 0;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1079, result);
  // Patch 64-11 # 1080
  constant_a = 1;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1080, result);
  // Patch 64-12 # 1081
  constant_a = 2;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1081, result);
  // Patch 64-13 # 1082
  constant_a = 3;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1082, result);
  // Patch 64-14 # 1083
  constant_a = 4;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1083, result);
  // Patch 64-15 # 1084
  constant_a = 5;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1084, result);
  // Patch 64-16 # 1085
  constant_a = 6;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1085, result);
  // Patch 64-17 # 1086
  constant_a = 7;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1086, result);
  // Patch 64-18 # 1087
  constant_a = 8;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1087, result);
  // Patch 64-19 # 1088
  constant_a = 9;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1088, result);
  // Patch 64-20 # 1089
  constant_a = 10;
  result = (constant_a < (i / size));
  uni_klee_add_patch(patch_results, 1089, result);
  // Patch 65-0 # 1090
  constant_a = -10;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1090, result);
  // Patch 65-1 # 1091
  constant_a = -9;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1091, result);
  // Patch 65-2 # 1092
  constant_a = -8;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1092, result);
  // Patch 65-3 # 1093
  constant_a = -7;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1093, result);
  // Patch 65-4 # 1094
  constant_a = -6;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1094, result);
  // Patch 65-5 # 1095
  constant_a = -5;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1095, result);
  // Patch 65-6 # 1096
  constant_a = -4;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1096, result);
  // Patch 65-7 # 1097
  constant_a = -3;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1097, result);
  // Patch 65-8 # 1098
  constant_a = -2;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1098, result);
  // Patch 65-9 # 1099
  constant_a = -1;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1099, result);
  // Patch 65-10 # 1100
  constant_a = 0;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1100, result);
  // Patch 65-11 # 1101
  constant_a = 1;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1101, result);
  // Patch 65-12 # 1102
  constant_a = 2;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1102, result);
  // Patch 65-13 # 1103
  constant_a = 3;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1103, result);
  // Patch 65-14 # 1104
  constant_a = 4;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1104, result);
  // Patch 65-15 # 1105
  constant_a = 5;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1105, result);
  // Patch 65-16 # 1106
  constant_a = 6;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1106, result);
  // Patch 65-17 # 1107
  constant_a = 7;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1107, result);
  // Patch 65-18 # 1108
  constant_a = 8;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1108, result);
  // Patch 65-19 # 1109
  constant_a = 9;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1109, result);
  // Patch 65-20 # 1110
  constant_a = 10;
  result = (i < (constant_a / size));
  uni_klee_add_patch(patch_results, 1110, result);
  // Patch 66-0 # 1111
  constant_a = -10;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1111, result);
  // Patch 66-1 # 1112
  constant_a = -9;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1112, result);
  // Patch 66-2 # 1113
  constant_a = -8;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1113, result);
  // Patch 66-3 # 1114
  constant_a = -7;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1114, result);
  // Patch 66-4 # 1115
  constant_a = -6;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1115, result);
  // Patch 66-5 # 1116
  constant_a = -5;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1116, result);
  // Patch 66-6 # 1117
  constant_a = -4;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1117, result);
  // Patch 66-7 # 1118
  constant_a = -3;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1118, result);
  // Patch 66-8 # 1119
  constant_a = -2;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1119, result);
  // Patch 66-9 # 1120
  constant_a = -1;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1120, result);
  // Patch 66-10 # 1121
  constant_a = 0;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1121, result);
  // Patch 66-11 # 1122
  constant_a = 1;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1122, result);
  // Patch 66-12 # 1123
  constant_a = 2;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1123, result);
  // Patch 66-13 # 1124
  constant_a = 3;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1124, result);
  // Patch 66-14 # 1125
  constant_a = 4;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1125, result);
  // Patch 66-15 # 1126
  constant_a = 5;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1126, result);
  // Patch 66-16 # 1127
  constant_a = 6;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1127, result);
  // Patch 66-17 # 1128
  constant_a = 7;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1128, result);
  // Patch 66-18 # 1129
  constant_a = 8;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1129, result);
  // Patch 66-19 # 1130
  constant_a = 9;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1130, result);
  // Patch 66-20 # 1131
  constant_a = 10;
  result = (size < (constant_a / size));
  uni_klee_add_patch(patch_results, 1131, result);
  // Patch 67-0 # 1132
  constant_a = -10;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1132, result);
  // Patch 67-1 # 1133
  constant_a = -9;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1133, result);
  // Patch 67-2 # 1134
  constant_a = -8;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1134, result);
  // Patch 67-3 # 1135
  constant_a = -7;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1135, result);
  // Patch 67-4 # 1136
  constant_a = -6;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1136, result);
  // Patch 67-5 # 1137
  constant_a = -5;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1137, result);
  // Patch 67-6 # 1138
  constant_a = -4;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1138, result);
  // Patch 67-7 # 1139
  constant_a = -3;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1139, result);
  // Patch 67-8 # 1140
  constant_a = -2;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1140, result);
  // Patch 67-9 # 1141
  constant_a = -1;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1141, result);
  // Patch 67-10 # 1142
  constant_a = 0;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1142, result);
  // Patch 67-11 # 1143
  constant_a = 1;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1143, result);
  // Patch 67-12 # 1144
  constant_a = 2;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1144, result);
  // Patch 67-13 # 1145
  constant_a = 3;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1145, result);
  // Patch 67-14 # 1146
  constant_a = 4;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1146, result);
  // Patch 67-15 # 1147
  constant_a = 5;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1147, result);
  // Patch 67-16 # 1148
  constant_a = 6;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1148, result);
  // Patch 67-17 # 1149
  constant_a = 7;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1149, result);
  // Patch 67-18 # 1150
  constant_a = 8;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1150, result);
  // Patch 67-19 # 1151
  constant_a = 9;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1151, result);
  // Patch 67-20 # 1152
  constant_a = 10;
  result = (constant_a < (constant_a / size));
  uni_klee_add_patch(patch_results, 1152, result);
  // Patch 68-0 # 1153
  constant_a = -10;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1153, result);
  // Patch 68-1 # 1154
  constant_a = -9;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1154, result);
  // Patch 68-2 # 1155
  constant_a = -8;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1155, result);
  // Patch 68-3 # 1156
  constant_a = -7;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1156, result);
  // Patch 68-4 # 1157
  constant_a = -6;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1157, result);
  // Patch 68-5 # 1158
  constant_a = -5;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1158, result);
  // Patch 68-6 # 1159
  constant_a = -4;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1159, result);
  // Patch 68-7 # 1160
  constant_a = -3;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1160, result);
  // Patch 68-8 # 1161
  constant_a = -2;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1161, result);
  // Patch 68-9 # 1162
  constant_a = -1;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1162, result);
  // Patch 68-10 # 1163
  constant_a = 1;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1163, result);
  // Patch 68-11 # 1164
  constant_a = 2;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1164, result);
  // Patch 68-12 # 1165
  constant_a = 3;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1165, result);
  // Patch 68-13 # 1166
  constant_a = 4;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1166, result);
  // Patch 68-14 # 1167
  constant_a = 5;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1167, result);
  // Patch 68-15 # 1168
  constant_a = 6;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1168, result);
  // Patch 68-16 # 1169
  constant_a = 7;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1169, result);
  // Patch 68-17 # 1170
  constant_a = 8;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1170, result);
  // Patch 68-18 # 1171
  constant_a = 9;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1171, result);
  // Patch 68-19 # 1172
  constant_a = 10;
  result = (constant_a < (i / constant_a));
  uni_klee_add_patch(patch_results, 1172, result);
  // Patch 69-0 # 1173
  constant_a = -10;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1173, result);
  // Patch 69-1 # 1174
  constant_a = -9;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1174, result);
  // Patch 69-2 # 1175
  constant_a = -8;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1175, result);
  // Patch 69-3 # 1176
  constant_a = -7;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1176, result);
  // Patch 69-4 # 1177
  constant_a = -6;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1177, result);
  // Patch 69-5 # 1178
  constant_a = -5;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1178, result);
  // Patch 69-6 # 1179
  constant_a = -4;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1179, result);
  // Patch 69-7 # 1180
  constant_a = -3;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1180, result);
  // Patch 69-8 # 1181
  constant_a = -2;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1181, result);
  // Patch 69-9 # 1182
  constant_a = -1;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1182, result);
  // Patch 69-10 # 1183
  constant_a = 1;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1183, result);
  // Patch 69-11 # 1184
  constant_a = 2;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1184, result);
  // Patch 69-12 # 1185
  constant_a = 3;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1185, result);
  // Patch 69-13 # 1186
  constant_a = 4;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1186, result);
  // Patch 69-14 # 1187
  constant_a = 5;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1187, result);
  // Patch 69-15 # 1188
  constant_a = 6;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1188, result);
  // Patch 69-16 # 1189
  constant_a = 7;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1189, result);
  // Patch 69-17 # 1190
  constant_a = 8;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1190, result);
  // Patch 69-18 # 1191
  constant_a = 9;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1191, result);
  // Patch 69-19 # 1192
  constant_a = 10;
  result = (i < (size / constant_a));
  uni_klee_add_patch(patch_results, 1192, result);
  // Patch 70-0 # 1193
  constant_a = -10;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1193, result);
  // Patch 70-1 # 1194
  constant_a = -9;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1194, result);
  // Patch 70-2 # 1195
  constant_a = -8;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1195, result);
  // Patch 70-3 # 1196
  constant_a = -7;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1196, result);
  // Patch 70-4 # 1197
  constant_a = -6;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1197, result);
  // Patch 70-5 # 1198
  constant_a = -5;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1198, result);
  // Patch 70-6 # 1199
  constant_a = -4;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1199, result);
  // Patch 70-7 # 1200
  constant_a = -3;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1200, result);
  // Patch 70-8 # 1201
  constant_a = -2;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1201, result);
  // Patch 70-9 # 1202
  constant_a = -1;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1202, result);
  // Patch 70-10 # 1203
  constant_a = 1;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1203, result);
  // Patch 70-11 # 1204
  constant_a = 2;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1204, result);
  // Patch 70-12 # 1205
  constant_a = 3;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1205, result);
  // Patch 70-13 # 1206
  constant_a = 4;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1206, result);
  // Patch 70-14 # 1207
  constant_a = 5;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1207, result);
  // Patch 70-15 # 1208
  constant_a = 6;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1208, result);
  // Patch 70-16 # 1209
  constant_a = 7;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1209, result);
  // Patch 70-17 # 1210
  constant_a = 8;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1210, result);
  // Patch 70-18 # 1211
  constant_a = 9;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1211, result);
  // Patch 70-19 # 1212
  constant_a = 10;
  result = (constant_a < (size / constant_a));
  uni_klee_add_patch(patch_results, 1212, result);
  // Patch 71-0 # 1213
  constant_a = -10;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1213, result);
  // Patch 71-1 # 1214
  constant_a = -9;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1214, result);
  // Patch 71-2 # 1215
  constant_a = -8;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1215, result);
  // Patch 71-3 # 1216
  constant_a = -7;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1216, result);
  // Patch 71-4 # 1217
  constant_a = -6;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1217, result);
  // Patch 71-5 # 1218
  constant_a = -5;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1218, result);
  // Patch 71-6 # 1219
  constant_a = -4;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1219, result);
  // Patch 71-7 # 1220
  constant_a = -3;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1220, result);
  // Patch 71-8 # 1221
  constant_a = -2;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1221, result);
  // Patch 71-9 # 1222
  constant_a = -1;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1222, result);
  // Patch 71-10 # 1223
  constant_a = 0;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1223, result);
  // Patch 71-11 # 1224
  constant_a = 1;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1224, result);
  // Patch 71-12 # 1225
  constant_a = 2;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1225, result);
  // Patch 71-13 # 1226
  constant_a = 3;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1226, result);
  // Patch 71-14 # 1227
  constant_a = 4;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1227, result);
  // Patch 71-15 # 1228
  constant_a = 5;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1228, result);
  // Patch 71-16 # 1229
  constant_a = 6;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1229, result);
  // Patch 71-17 # 1230
  constant_a = 7;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1230, result);
  // Patch 71-18 # 1231
  constant_a = 8;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1231, result);
  // Patch 71-19 # 1232
  constant_a = 9;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1232, result);
  // Patch 71-20 # 1233
  constant_a = 10;
  result = (constant_a <= i);
  uni_klee_add_patch(patch_results, 1233, result);
  // Patch 72-0 # 1234
  result = ((size / i) <= i);
  uni_klee_add_patch(patch_results, 1234, result);
  // Patch 73-0 # 1235
  constant_a = -10;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1235, result);
  // Patch 73-1 # 1236
  constant_a = -9;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1236, result);
  // Patch 73-2 # 1237
  constant_a = -8;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1237, result);
  // Patch 73-3 # 1238
  constant_a = -7;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1238, result);
  // Patch 73-4 # 1239
  constant_a = -6;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1239, result);
  // Patch 73-5 # 1240
  constant_a = -5;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1240, result);
  // Patch 73-6 # 1241
  constant_a = -4;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1241, result);
  // Patch 73-7 # 1242
  constant_a = -3;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1242, result);
  // Patch 73-8 # 1243
  constant_a = -2;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1243, result);
  // Patch 73-9 # 1244
  constant_a = -1;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1244, result);
  // Patch 73-10 # 1245
  constant_a = 0;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1245, result);
  // Patch 73-11 # 1246
  constant_a = 1;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1246, result);
  // Patch 73-12 # 1247
  constant_a = 2;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1247, result);
  // Patch 73-13 # 1248
  constant_a = 3;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1248, result);
  // Patch 73-14 # 1249
  constant_a = 4;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1249, result);
  // Patch 73-15 # 1250
  constant_a = 5;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1250, result);
  // Patch 73-16 # 1251
  constant_a = 6;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1251, result);
  // Patch 73-17 # 1252
  constant_a = 7;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1252, result);
  // Patch 73-18 # 1253
  constant_a = 8;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1253, result);
  // Patch 73-19 # 1254
  constant_a = 9;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1254, result);
  // Patch 73-20 # 1255
  constant_a = 10;
  result = ((constant_a / i) <= i);
  uni_klee_add_patch(patch_results, 1255, result);
  // Patch 74-0 # 1256
  result = ((i / size) <= i);
  uni_klee_add_patch(patch_results, 1256, result);
  // Patch 75-0 # 1257
  constant_a = -10;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1257, result);
  // Patch 75-1 # 1258
  constant_a = -9;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1258, result);
  // Patch 75-2 # 1259
  constant_a = -8;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1259, result);
  // Patch 75-3 # 1260
  constant_a = -7;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1260, result);
  // Patch 75-4 # 1261
  constant_a = -6;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1261, result);
  // Patch 75-5 # 1262
  constant_a = -5;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1262, result);
  // Patch 75-6 # 1263
  constant_a = -4;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1263, result);
  // Patch 75-7 # 1264
  constant_a = -3;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1264, result);
  // Patch 75-8 # 1265
  constant_a = -2;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1265, result);
  // Patch 75-9 # 1266
  constant_a = -1;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1266, result);
  // Patch 75-10 # 1267
  constant_a = 0;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1267, result);
  // Patch 75-11 # 1268
  constant_a = 1;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1268, result);
  // Patch 75-12 # 1269
  constant_a = 2;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1269, result);
  // Patch 75-13 # 1270
  constant_a = 3;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1270, result);
  // Patch 75-14 # 1271
  constant_a = 4;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1271, result);
  // Patch 75-15 # 1272
  constant_a = 5;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1272, result);
  // Patch 75-16 # 1273
  constant_a = 6;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1273, result);
  // Patch 75-17 # 1274
  constant_a = 7;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1274, result);
  // Patch 75-18 # 1275
  constant_a = 8;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1275, result);
  // Patch 75-19 # 1276
  constant_a = 9;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1276, result);
  // Patch 75-20 # 1277
  constant_a = 10;
  result = ((constant_a / size) <= i);
  uni_klee_add_patch(patch_results, 1277, result);
  // Patch 76-0 # 1278
  constant_a = -10;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1278, result);
  // Patch 76-1 # 1279
  constant_a = -9;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1279, result);
  // Patch 76-2 # 1280
  constant_a = -8;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1280, result);
  // Patch 76-3 # 1281
  constant_a = -7;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1281, result);
  // Patch 76-4 # 1282
  constant_a = -6;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1282, result);
  // Patch 76-5 # 1283
  constant_a = -5;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1283, result);
  // Patch 76-6 # 1284
  constant_a = -4;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1284, result);
  // Patch 76-7 # 1285
  constant_a = -3;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1285, result);
  // Patch 76-8 # 1286
  constant_a = -2;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1286, result);
  // Patch 76-9 # 1287
  constant_a = -1;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1287, result);
  // Patch 76-10 # 1288
  constant_a = 1;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1288, result);
  // Patch 76-11 # 1289
  constant_a = 2;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1289, result);
  // Patch 76-12 # 1290
  constant_a = 3;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1290, result);
  // Patch 76-13 # 1291
  constant_a = 4;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1291, result);
  // Patch 76-14 # 1292
  constant_a = 5;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1292, result);
  // Patch 76-15 # 1293
  constant_a = 6;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1293, result);
  // Patch 76-16 # 1294
  constant_a = 7;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1294, result);
  // Patch 76-17 # 1295
  constant_a = 8;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1295, result);
  // Patch 76-18 # 1296
  constant_a = 9;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1296, result);
  // Patch 76-19 # 1297
  constant_a = 10;
  result = ((i / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1297, result);
  // Patch 77-0 # 1298
  constant_a = -10;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1298, result);
  // Patch 77-1 # 1299
  constant_a = -9;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1299, result);
  // Patch 77-2 # 1300
  constant_a = -8;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1300, result);
  // Patch 77-3 # 1301
  constant_a = -7;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1301, result);
  // Patch 77-4 # 1302
  constant_a = -6;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1302, result);
  // Patch 77-5 # 1303
  constant_a = -5;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1303, result);
  // Patch 77-6 # 1304
  constant_a = -4;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1304, result);
  // Patch 77-7 # 1305
  constant_a = -3;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1305, result);
  // Patch 77-8 # 1306
  constant_a = -2;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1306, result);
  // Patch 77-9 # 1307
  constant_a = -1;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1307, result);
  // Patch 77-10 # 1308
  constant_a = 1;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1308, result);
  // Patch 77-11 # 1309
  constant_a = 2;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1309, result);
  // Patch 77-12 # 1310
  constant_a = 3;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1310, result);
  // Patch 77-13 # 1311
  constant_a = 4;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1311, result);
  // Patch 77-14 # 1312
  constant_a = 5;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1312, result);
  // Patch 77-15 # 1313
  constant_a = 6;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1313, result);
  // Patch 77-16 # 1314
  constant_a = 7;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1314, result);
  // Patch 77-17 # 1315
  constant_a = 8;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1315, result);
  // Patch 77-18 # 1316
  constant_a = 9;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1316, result);
  // Patch 77-19 # 1317
  constant_a = 10;
  result = ((size / constant_a) <= i);
  uni_klee_add_patch(patch_results, 1317, result);
  // Patch 78-0 # 1318
  result = (i <= size);
  uni_klee_add_patch(patch_results, 1318, result);
  // Patch 79-0 # 1319
  constant_a = -10;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1319, result);
  // Patch 79-1 # 1320
  constant_a = -9;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1320, result);
  // Patch 79-2 # 1321
  constant_a = -8;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1321, result);
  // Patch 79-3 # 1322
  constant_a = -7;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1322, result);
  // Patch 79-4 # 1323
  constant_a = -6;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1323, result);
  // Patch 79-5 # 1324
  constant_a = -5;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1324, result);
  // Patch 79-6 # 1325
  constant_a = -4;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1325, result);
  // Patch 79-7 # 1326
  constant_a = -3;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1326, result);
  // Patch 79-8 # 1327
  constant_a = -2;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1327, result);
  // Patch 79-9 # 1328
  constant_a = -1;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1328, result);
  // Patch 79-10 # 1329
  constant_a = 0;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1329, result);
  // Patch 79-11 # 1330
  constant_a = 1;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1330, result);
  // Patch 79-12 # 1331
  constant_a = 2;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1331, result);
  // Patch 79-13 # 1332
  constant_a = 3;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1332, result);
  // Patch 79-14 # 1333
  constant_a = 4;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1333, result);
  // Patch 79-15 # 1334
  constant_a = 5;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1334, result);
  // Patch 79-16 # 1335
  constant_a = 6;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1335, result);
  // Patch 79-17 # 1336
  constant_a = 7;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1336, result);
  // Patch 79-18 # 1337
  constant_a = 8;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1337, result);
  // Patch 79-19 # 1338
  constant_a = 9;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1338, result);
  // Patch 79-20 # 1339
  constant_a = 10;
  result = (constant_a <= size);
  uni_klee_add_patch(patch_results, 1339, result);
  // Patch 80-0 # 1340
  result = ((size / i) <= size);
  uni_klee_add_patch(patch_results, 1340, result);
  // Patch 81-0 # 1341
  constant_a = -10;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1341, result);
  // Patch 81-1 # 1342
  constant_a = -9;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1342, result);
  // Patch 81-2 # 1343
  constant_a = -8;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1343, result);
  // Patch 81-3 # 1344
  constant_a = -7;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1344, result);
  // Patch 81-4 # 1345
  constant_a = -6;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1345, result);
  // Patch 81-5 # 1346
  constant_a = -5;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1346, result);
  // Patch 81-6 # 1347
  constant_a = -4;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1347, result);
  // Patch 81-7 # 1348
  constant_a = -3;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1348, result);
  // Patch 81-8 # 1349
  constant_a = -2;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1349, result);
  // Patch 81-9 # 1350
  constant_a = -1;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1350, result);
  // Patch 81-10 # 1351
  constant_a = 0;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1351, result);
  // Patch 81-11 # 1352
  constant_a = 1;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1352, result);
  // Patch 81-12 # 1353
  constant_a = 2;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1353, result);
  // Patch 81-13 # 1354
  constant_a = 3;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1354, result);
  // Patch 81-14 # 1355
  constant_a = 4;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1355, result);
  // Patch 81-15 # 1356
  constant_a = 5;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1356, result);
  // Patch 81-16 # 1357
  constant_a = 6;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1357, result);
  // Patch 81-17 # 1358
  constant_a = 7;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1358, result);
  // Patch 81-18 # 1359
  constant_a = 8;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1359, result);
  // Patch 81-19 # 1360
  constant_a = 9;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1360, result);
  // Patch 81-20 # 1361
  constant_a = 10;
  result = ((constant_a / i) <= size);
  uni_klee_add_patch(patch_results, 1361, result);
  // Patch 82-0 # 1362
  result = ((i / size) <= size);
  uni_klee_add_patch(patch_results, 1362, result);
  // Patch 83-0 # 1363
  constant_a = -10;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1363, result);
  // Patch 83-1 # 1364
  constant_a = -9;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1364, result);
  // Patch 83-2 # 1365
  constant_a = -8;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1365, result);
  // Patch 83-3 # 1366
  constant_a = -7;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1366, result);
  // Patch 83-4 # 1367
  constant_a = -6;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1367, result);
  // Patch 83-5 # 1368
  constant_a = -5;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1368, result);
  // Patch 83-6 # 1369
  constant_a = -4;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1369, result);
  // Patch 83-7 # 1370
  constant_a = -3;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1370, result);
  // Patch 83-8 # 1371
  constant_a = -2;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1371, result);
  // Patch 83-9 # 1372
  constant_a = -1;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1372, result);
  // Patch 83-10 # 1373
  constant_a = 0;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1373, result);
  // Patch 83-11 # 1374
  constant_a = 1;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1374, result);
  // Patch 83-12 # 1375
  constant_a = 2;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1375, result);
  // Patch 83-13 # 1376
  constant_a = 3;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1376, result);
  // Patch 83-14 # 1377
  constant_a = 4;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1377, result);
  // Patch 83-15 # 1378
  constant_a = 5;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1378, result);
  // Patch 83-16 # 1379
  constant_a = 6;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1379, result);
  // Patch 83-17 # 1380
  constant_a = 7;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1380, result);
  // Patch 83-18 # 1381
  constant_a = 8;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1381, result);
  // Patch 83-19 # 1382
  constant_a = 9;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1382, result);
  // Patch 83-20 # 1383
  constant_a = 10;
  result = ((constant_a / size) <= size);
  uni_klee_add_patch(patch_results, 1383, result);
  // Patch 84-0 # 1384
  constant_a = -10;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1384, result);
  // Patch 84-1 # 1385
  constant_a = -9;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1385, result);
  // Patch 84-2 # 1386
  constant_a = -8;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1386, result);
  // Patch 84-3 # 1387
  constant_a = -7;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1387, result);
  // Patch 84-4 # 1388
  constant_a = -6;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1388, result);
  // Patch 84-5 # 1389
  constant_a = -5;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1389, result);
  // Patch 84-6 # 1390
  constant_a = -4;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1390, result);
  // Patch 84-7 # 1391
  constant_a = -3;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1391, result);
  // Patch 84-8 # 1392
  constant_a = -2;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1392, result);
  // Patch 84-9 # 1393
  constant_a = -1;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1393, result);
  // Patch 84-10 # 1394
  constant_a = 1;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1394, result);
  // Patch 84-11 # 1395
  constant_a = 2;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1395, result);
  // Patch 84-12 # 1396
  constant_a = 3;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1396, result);
  // Patch 84-13 # 1397
  constant_a = 4;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1397, result);
  // Patch 84-14 # 1398
  constant_a = 5;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1398, result);
  // Patch 84-15 # 1399
  constant_a = 6;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1399, result);
  // Patch 84-16 # 1400
  constant_a = 7;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1400, result);
  // Patch 84-17 # 1401
  constant_a = 8;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1401, result);
  // Patch 84-18 # 1402
  constant_a = 9;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1402, result);
  // Patch 84-19 # 1403
  constant_a = 10;
  result = ((i / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1403, result);
  // Patch 85-0 # 1404
  constant_a = -10;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1404, result);
  // Patch 85-1 # 1405
  constant_a = -9;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1405, result);
  // Patch 85-2 # 1406
  constant_a = -8;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1406, result);
  // Patch 85-3 # 1407
  constant_a = -7;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1407, result);
  // Patch 85-4 # 1408
  constant_a = -6;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1408, result);
  // Patch 85-5 # 1409
  constant_a = -5;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1409, result);
  // Patch 85-6 # 1410
  constant_a = -4;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1410, result);
  // Patch 85-7 # 1411
  constant_a = -3;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1411, result);
  // Patch 85-8 # 1412
  constant_a = -2;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1412, result);
  // Patch 85-9 # 1413
  constant_a = -1;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1413, result);
  // Patch 85-10 # 1414
  constant_a = 1;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1414, result);
  // Patch 85-11 # 1415
  constant_a = 2;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1415, result);
  // Patch 85-12 # 1416
  constant_a = 3;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1416, result);
  // Patch 85-13 # 1417
  constant_a = 4;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1417, result);
  // Patch 85-14 # 1418
  constant_a = 5;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1418, result);
  // Patch 85-15 # 1419
  constant_a = 6;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1419, result);
  // Patch 85-16 # 1420
  constant_a = 7;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1420, result);
  // Patch 85-17 # 1421
  constant_a = 8;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1421, result);
  // Patch 85-18 # 1422
  constant_a = 9;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1422, result);
  // Patch 85-19 # 1423
  constant_a = 10;
  result = ((size / constant_a) <= size);
  uni_klee_add_patch(patch_results, 1423, result);
  // Patch 86-0 # 1424
  constant_a = -10;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1424, result);
  // Patch 86-1 # 1425
  constant_a = -9;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1425, result);
  // Patch 86-2 # 1426
  constant_a = -8;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1426, result);
  // Patch 86-3 # 1427
  constant_a = -7;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1427, result);
  // Patch 86-4 # 1428
  constant_a = -6;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1428, result);
  // Patch 86-5 # 1429
  constant_a = -5;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1429, result);
  // Patch 86-6 # 1430
  constant_a = -4;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1430, result);
  // Patch 86-7 # 1431
  constant_a = -3;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1431, result);
  // Patch 86-8 # 1432
  constant_a = -2;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1432, result);
  // Patch 86-9 # 1433
  constant_a = -1;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1433, result);
  // Patch 86-10 # 1434
  constant_a = 0;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1434, result);
  // Patch 86-11 # 1435
  constant_a = 1;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1435, result);
  // Patch 86-12 # 1436
  constant_a = 2;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1436, result);
  // Patch 86-13 # 1437
  constant_a = 3;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1437, result);
  // Patch 86-14 # 1438
  constant_a = 4;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1438, result);
  // Patch 86-15 # 1439
  constant_a = 5;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1439, result);
  // Patch 86-16 # 1440
  constant_a = 6;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1440, result);
  // Patch 86-17 # 1441
  constant_a = 7;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1441, result);
  // Patch 86-18 # 1442
  constant_a = 8;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1442, result);
  // Patch 86-19 # 1443
  constant_a = 9;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1443, result);
  // Patch 86-20 # 1444
  constant_a = 10;
  result = (i <= constant_a);
  uni_klee_add_patch(patch_results, 1444, result);
  // Patch 87-0 # 1445
  constant_a = -10;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1445, result);
  // Patch 87-1 # 1446
  constant_a = -9;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1446, result);
  // Patch 87-2 # 1447
  constant_a = -8;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1447, result);
  // Patch 87-3 # 1448
  constant_a = -7;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1448, result);
  // Patch 87-4 # 1449
  constant_a = -6;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1449, result);
  // Patch 87-5 # 1450
  constant_a = -5;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1450, result);
  // Patch 87-6 # 1451
  constant_a = -4;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1451, result);
  // Patch 87-7 # 1452
  constant_a = -3;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1452, result);
  // Patch 87-8 # 1453
  constant_a = -2;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1453, result);
  // Patch 87-9 # 1454
  constant_a = -1;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1454, result);
  // Patch 87-10 # 1455
  constant_a = 0;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1455, result);
  // Patch 87-11 # 1456
  constant_a = 1;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1456, result);
  // Patch 87-12 # 1457
  constant_a = 2;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1457, result);
  // Patch 87-13 # 1458
  constant_a = 3;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1458, result);
  // Patch 87-14 # 1459
  constant_a = 4;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1459, result);
  // Patch 87-15 # 1460
  constant_a = 5;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1460, result);
  // Patch 87-16 # 1461
  constant_a = 6;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1461, result);
  // Patch 87-17 # 1462
  constant_a = 7;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1462, result);
  // Patch 87-18 # 1463
  constant_a = 8;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1463, result);
  // Patch 87-19 # 1464
  constant_a = 9;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1464, result);
  // Patch 87-20 # 1465
  constant_a = 10;
  result = (size <= constant_a);
  uni_klee_add_patch(patch_results, 1465, result);
  // Patch 88-0 # 1466
  constant_a = -10;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1466, result);
  // Patch 88-1 # 1467
  constant_a = -9;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1467, result);
  // Patch 88-2 # 1468
  constant_a = -8;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1468, result);
  // Patch 88-3 # 1469
  constant_a = -7;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1469, result);
  // Patch 88-4 # 1470
  constant_a = -6;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1470, result);
  // Patch 88-5 # 1471
  constant_a = -5;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1471, result);
  // Patch 88-6 # 1472
  constant_a = -4;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1472, result);
  // Patch 88-7 # 1473
  constant_a = -3;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1473, result);
  // Patch 88-8 # 1474
  constant_a = -2;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1474, result);
  // Patch 88-9 # 1475
  constant_a = -1;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1475, result);
  // Patch 88-10 # 1476
  constant_a = 0;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1476, result);
  // Patch 88-11 # 1477
  constant_a = 1;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1477, result);
  // Patch 88-12 # 1478
  constant_a = 2;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1478, result);
  // Patch 88-13 # 1479
  constant_a = 3;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1479, result);
  // Patch 88-14 # 1480
  constant_a = 4;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1480, result);
  // Patch 88-15 # 1481
  constant_a = 5;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1481, result);
  // Patch 88-16 # 1482
  constant_a = 6;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1482, result);
  // Patch 88-17 # 1483
  constant_a = 7;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1483, result);
  // Patch 88-18 # 1484
  constant_a = 8;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1484, result);
  // Patch 88-19 # 1485
  constant_a = 9;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1485, result);
  // Patch 88-20 # 1486
  constant_a = 10;
  result = ((size / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1486, result);
  // Patch 89-0 # 1487
  constant_a = -10;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1487, result);
  // Patch 89-1 # 1488
  constant_a = -9;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1488, result);
  // Patch 89-2 # 1489
  constant_a = -8;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1489, result);
  // Patch 89-3 # 1490
  constant_a = -7;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1490, result);
  // Patch 89-4 # 1491
  constant_a = -6;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1491, result);
  // Patch 89-5 # 1492
  constant_a = -5;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1492, result);
  // Patch 89-6 # 1493
  constant_a = -4;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1493, result);
  // Patch 89-7 # 1494
  constant_a = -3;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1494, result);
  // Patch 89-8 # 1495
  constant_a = -2;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1495, result);
  // Patch 89-9 # 1496
  constant_a = -1;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1496, result);
  // Patch 89-10 # 1497
  constant_a = 0;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1497, result);
  // Patch 89-11 # 1498
  constant_a = 1;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1498, result);
  // Patch 89-12 # 1499
  constant_a = 2;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1499, result);
  // Patch 89-13 # 1500
  constant_a = 3;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1500, result);
  // Patch 89-14 # 1501
  constant_a = 4;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1501, result);
  // Patch 89-15 # 1502
  constant_a = 5;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1502, result);
  // Patch 89-16 # 1503
  constant_a = 6;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1503, result);
  // Patch 89-17 # 1504
  constant_a = 7;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1504, result);
  // Patch 89-18 # 1505
  constant_a = 8;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1505, result);
  // Patch 89-19 # 1506
  constant_a = 9;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1506, result);
  // Patch 89-20 # 1507
  constant_a = 10;
  result = ((constant_a / i) <= constant_a);
  uni_klee_add_patch(patch_results, 1507, result);
  // Patch 90-0 # 1508
  constant_a = -10;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1508, result);
  // Patch 90-1 # 1509
  constant_a = -9;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1509, result);
  // Patch 90-2 # 1510
  constant_a = -8;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1510, result);
  // Patch 90-3 # 1511
  constant_a = -7;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1511, result);
  // Patch 90-4 # 1512
  constant_a = -6;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1512, result);
  // Patch 90-5 # 1513
  constant_a = -5;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1513, result);
  // Patch 90-6 # 1514
  constant_a = -4;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1514, result);
  // Patch 90-7 # 1515
  constant_a = -3;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1515, result);
  // Patch 90-8 # 1516
  constant_a = -2;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1516, result);
  // Patch 90-9 # 1517
  constant_a = -1;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1517, result);
  // Patch 90-10 # 1518
  constant_a = 0;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1518, result);
  // Patch 90-11 # 1519
  constant_a = 1;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1519, result);
  // Patch 90-12 # 1520
  constant_a = 2;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1520, result);
  // Patch 90-13 # 1521
  constant_a = 3;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1521, result);
  // Patch 90-14 # 1522
  constant_a = 4;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1522, result);
  // Patch 90-15 # 1523
  constant_a = 5;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1523, result);
  // Patch 90-16 # 1524
  constant_a = 6;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1524, result);
  // Patch 90-17 # 1525
  constant_a = 7;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1525, result);
  // Patch 90-18 # 1526
  constant_a = 8;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1526, result);
  // Patch 90-19 # 1527
  constant_a = 9;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1527, result);
  // Patch 90-20 # 1528
  constant_a = 10;
  result = ((i / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1528, result);
  // Patch 91-0 # 1529
  constant_a = -10;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1529, result);
  // Patch 91-1 # 1530
  constant_a = -9;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1530, result);
  // Patch 91-2 # 1531
  constant_a = -8;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1531, result);
  // Patch 91-3 # 1532
  constant_a = -7;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1532, result);
  // Patch 91-4 # 1533
  constant_a = -6;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1533, result);
  // Patch 91-5 # 1534
  constant_a = -5;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1534, result);
  // Patch 91-6 # 1535
  constant_a = -4;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1535, result);
  // Patch 91-7 # 1536
  constant_a = -3;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1536, result);
  // Patch 91-8 # 1537
  constant_a = -2;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1537, result);
  // Patch 91-9 # 1538
  constant_a = -1;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1538, result);
  // Patch 91-10 # 1539
  constant_a = 0;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1539, result);
  // Patch 91-11 # 1540
  constant_a = 1;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1540, result);
  // Patch 91-12 # 1541
  constant_a = 2;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1541, result);
  // Patch 91-13 # 1542
  constant_a = 3;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1542, result);
  // Patch 91-14 # 1543
  constant_a = 4;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1543, result);
  // Patch 91-15 # 1544
  constant_a = 5;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1544, result);
  // Patch 91-16 # 1545
  constant_a = 6;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1545, result);
  // Patch 91-17 # 1546
  constant_a = 7;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1546, result);
  // Patch 91-18 # 1547
  constant_a = 8;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1547, result);
  // Patch 91-19 # 1548
  constant_a = 9;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1548, result);
  // Patch 91-20 # 1549
  constant_a = 10;
  result = ((constant_a / size) <= constant_a);
  uni_klee_add_patch(patch_results, 1549, result);
  // Patch 92-0 # 1550
  constant_a = -10;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1550, result);
  // Patch 92-1 # 1551
  constant_a = -9;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1551, result);
  // Patch 92-2 # 1552
  constant_a = -8;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1552, result);
  // Patch 92-3 # 1553
  constant_a = -7;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1553, result);
  // Patch 92-4 # 1554
  constant_a = -6;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1554, result);
  // Patch 92-5 # 1555
  constant_a = -5;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1555, result);
  // Patch 92-6 # 1556
  constant_a = -4;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1556, result);
  // Patch 92-7 # 1557
  constant_a = -3;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1557, result);
  // Patch 92-8 # 1558
  constant_a = -2;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1558, result);
  // Patch 92-9 # 1559
  constant_a = -1;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1559, result);
  // Patch 92-10 # 1560
  constant_a = 1;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1560, result);
  // Patch 92-11 # 1561
  constant_a = 2;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1561, result);
  // Patch 92-12 # 1562
  constant_a = 3;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1562, result);
  // Patch 92-13 # 1563
  constant_a = 4;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1563, result);
  // Patch 92-14 # 1564
  constant_a = 5;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1564, result);
  // Patch 92-15 # 1565
  constant_a = 6;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1565, result);
  // Patch 92-16 # 1566
  constant_a = 7;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1566, result);
  // Patch 92-17 # 1567
  constant_a = 8;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1567, result);
  // Patch 92-18 # 1568
  constant_a = 9;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1568, result);
  // Patch 92-19 # 1569
  constant_a = 10;
  result = ((i / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1569, result);
  // Patch 93-0 # 1570
  constant_a = -10;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1570, result);
  // Patch 93-1 # 1571
  constant_a = -9;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1571, result);
  // Patch 93-2 # 1572
  constant_a = -8;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1572, result);
  // Patch 93-3 # 1573
  constant_a = -7;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1573, result);
  // Patch 93-4 # 1574
  constant_a = -6;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1574, result);
  // Patch 93-5 # 1575
  constant_a = -5;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1575, result);
  // Patch 93-6 # 1576
  constant_a = -4;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1576, result);
  // Patch 93-7 # 1577
  constant_a = -3;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1577, result);
  // Patch 93-8 # 1578
  constant_a = -2;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1578, result);
  // Patch 93-9 # 1579
  constant_a = -1;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1579, result);
  // Patch 93-10 # 1580
  constant_a = 1;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1580, result);
  // Patch 93-11 # 1581
  constant_a = 2;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1581, result);
  // Patch 93-12 # 1582
  constant_a = 3;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1582, result);
  // Patch 93-13 # 1583
  constant_a = 4;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1583, result);
  // Patch 93-14 # 1584
  constant_a = 5;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1584, result);
  // Patch 93-15 # 1585
  constant_a = 6;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1585, result);
  // Patch 93-16 # 1586
  constant_a = 7;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1586, result);
  // Patch 93-17 # 1587
  constant_a = 8;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1587, result);
  // Patch 93-18 # 1588
  constant_a = 9;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1588, result);
  // Patch 93-19 # 1589
  constant_a = 10;
  result = ((size / constant_a) <= constant_a);
  uni_klee_add_patch(patch_results, 1589, result);
  // Patch 94-0 # 1590
  constant_a = -10;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1590, result);
  // Patch 94-1 # 1591
  constant_a = -9;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1591, result);
  // Patch 94-2 # 1592
  constant_a = -8;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1592, result);
  // Patch 94-3 # 1593
  constant_a = -7;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1593, result);
  // Patch 94-4 # 1594
  constant_a = -6;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1594, result);
  // Patch 94-5 # 1595
  constant_a = -5;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1595, result);
  // Patch 94-6 # 1596
  constant_a = -4;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1596, result);
  // Patch 94-7 # 1597
  constant_a = -3;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1597, result);
  // Patch 94-8 # 1598
  constant_a = -2;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1598, result);
  // Patch 94-9 # 1599
  constant_a = -1;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1599, result);
  // Patch 94-10 # 1600
  constant_a = 0;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1600, result);
  // Patch 94-11 # 1601
  constant_a = 1;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1601, result);
  // Patch 94-12 # 1602
  constant_a = 2;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1602, result);
  // Patch 94-13 # 1603
  constant_a = 3;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1603, result);
  // Patch 94-14 # 1604
  constant_a = 4;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1604, result);
  // Patch 94-15 # 1605
  constant_a = 5;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1605, result);
  // Patch 94-16 # 1606
  constant_a = 6;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1606, result);
  // Patch 94-17 # 1607
  constant_a = 7;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1607, result);
  // Patch 94-18 # 1608
  constant_a = 8;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1608, result);
  // Patch 94-19 # 1609
  constant_a = 9;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1609, result);
  // Patch 94-20 # 1610
  constant_a = 10;
  result = (constant_a <= (size / i));
  uni_klee_add_patch(patch_results, 1610, result);
  // Patch 95-0 # 1611
  constant_a = -10;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1611, result);
  // Patch 95-1 # 1612
  constant_a = -9;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1612, result);
  // Patch 95-2 # 1613
  constant_a = -8;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1613, result);
  // Patch 95-3 # 1614
  constant_a = -7;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1614, result);
  // Patch 95-4 # 1615
  constant_a = -6;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1615, result);
  // Patch 95-5 # 1616
  constant_a = -5;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1616, result);
  // Patch 95-6 # 1617
  constant_a = -4;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1617, result);
  // Patch 95-7 # 1618
  constant_a = -3;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1618, result);
  // Patch 95-8 # 1619
  constant_a = -2;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1619, result);
  // Patch 95-9 # 1620
  constant_a = -1;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1620, result);
  // Patch 95-10 # 1621
  constant_a = 0;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1621, result);
  // Patch 95-11 # 1622
  constant_a = 1;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1622, result);
  // Patch 95-12 # 1623
  constant_a = 2;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1623, result);
  // Patch 95-13 # 1624
  constant_a = 3;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1624, result);
  // Patch 95-14 # 1625
  constant_a = 4;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1625, result);
  // Patch 95-15 # 1626
  constant_a = 5;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1626, result);
  // Patch 95-16 # 1627
  constant_a = 6;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1627, result);
  // Patch 95-17 # 1628
  constant_a = 7;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1628, result);
  // Patch 95-18 # 1629
  constant_a = 8;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1629, result);
  // Patch 95-19 # 1630
  constant_a = 9;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1630, result);
  // Patch 95-20 # 1631
  constant_a = 10;
  result = (i <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1631, result);
  // Patch 96-0 # 1632
  constant_a = -10;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1632, result);
  // Patch 96-1 # 1633
  constant_a = -9;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1633, result);
  // Patch 96-2 # 1634
  constant_a = -8;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1634, result);
  // Patch 96-3 # 1635
  constant_a = -7;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1635, result);
  // Patch 96-4 # 1636
  constant_a = -6;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1636, result);
  // Patch 96-5 # 1637
  constant_a = -5;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1637, result);
  // Patch 96-6 # 1638
  constant_a = -4;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1638, result);
  // Patch 96-7 # 1639
  constant_a = -3;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1639, result);
  // Patch 96-8 # 1640
  constant_a = -2;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1640, result);
  // Patch 96-9 # 1641
  constant_a = -1;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1641, result);
  // Patch 96-10 # 1642
  constant_a = 0;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1642, result);
  // Patch 96-11 # 1643
  constant_a = 1;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1643, result);
  // Patch 96-12 # 1644
  constant_a = 2;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1644, result);
  // Patch 96-13 # 1645
  constant_a = 3;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1645, result);
  // Patch 96-14 # 1646
  constant_a = 4;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1646, result);
  // Patch 96-15 # 1647
  constant_a = 5;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1647, result);
  // Patch 96-16 # 1648
  constant_a = 6;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1648, result);
  // Patch 96-17 # 1649
  constant_a = 7;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1649, result);
  // Patch 96-18 # 1650
  constant_a = 8;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1650, result);
  // Patch 96-19 # 1651
  constant_a = 9;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1651, result);
  // Patch 96-20 # 1652
  constant_a = 10;
  result = (size <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1652, result);
  // Patch 97-0 # 1653
  constant_a = -10;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1653, result);
  // Patch 97-1 # 1654
  constant_a = -9;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1654, result);
  // Patch 97-2 # 1655
  constant_a = -8;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1655, result);
  // Patch 97-3 # 1656
  constant_a = -7;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1656, result);
  // Patch 97-4 # 1657
  constant_a = -6;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1657, result);
  // Patch 97-5 # 1658
  constant_a = -5;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1658, result);
  // Patch 97-6 # 1659
  constant_a = -4;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1659, result);
  // Patch 97-7 # 1660
  constant_a = -3;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1660, result);
  // Patch 97-8 # 1661
  constant_a = -2;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1661, result);
  // Patch 97-9 # 1662
  constant_a = -1;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1662, result);
  // Patch 97-10 # 1663
  constant_a = 0;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1663, result);
  // Patch 97-11 # 1664
  constant_a = 1;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1664, result);
  // Patch 97-12 # 1665
  constant_a = 2;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1665, result);
  // Patch 97-13 # 1666
  constant_a = 3;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1666, result);
  // Patch 97-14 # 1667
  constant_a = 4;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1667, result);
  // Patch 97-15 # 1668
  constant_a = 5;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1668, result);
  // Patch 97-16 # 1669
  constant_a = 6;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1669, result);
  // Patch 97-17 # 1670
  constant_a = 7;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1670, result);
  // Patch 97-18 # 1671
  constant_a = 8;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1671, result);
  // Patch 97-19 # 1672
  constant_a = 9;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1672, result);
  // Patch 97-20 # 1673
  constant_a = 10;
  result = (constant_a <= (constant_a / i));
  uni_klee_add_patch(patch_results, 1673, result);
  // Patch 98-0 # 1674
  constant_a = -10;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1674, result);
  // Patch 98-1 # 1675
  constant_a = -9;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1675, result);
  // Patch 98-2 # 1676
  constant_a = -8;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1676, result);
  // Patch 98-3 # 1677
  constant_a = -7;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1677, result);
  // Patch 98-4 # 1678
  constant_a = -6;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1678, result);
  // Patch 98-5 # 1679
  constant_a = -5;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1679, result);
  // Patch 98-6 # 1680
  constant_a = -4;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1680, result);
  // Patch 98-7 # 1681
  constant_a = -3;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1681, result);
  // Patch 98-8 # 1682
  constant_a = -2;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1682, result);
  // Patch 98-9 # 1683
  constant_a = -1;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1683, result);
  // Patch 98-10 # 1684
  constant_a = 0;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1684, result);
  // Patch 98-11 # 1685
  constant_a = 1;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1685, result);
  // Patch 98-12 # 1686
  constant_a = 2;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1686, result);
  // Patch 98-13 # 1687
  constant_a = 3;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1687, result);
  // Patch 98-14 # 1688
  constant_a = 4;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1688, result);
  // Patch 98-15 # 1689
  constant_a = 5;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1689, result);
  // Patch 98-16 # 1690
  constant_a = 6;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1690, result);
  // Patch 98-17 # 1691
  constant_a = 7;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1691, result);
  // Patch 98-18 # 1692
  constant_a = 8;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1692, result);
  // Patch 98-19 # 1693
  constant_a = 9;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1693, result);
  // Patch 98-20 # 1694
  constant_a = 10;
  result = (constant_a <= (i / size));
  uni_klee_add_patch(patch_results, 1694, result);
  // Patch 99-0 # 1695
  constant_a = -10;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1695, result);
  // Patch 99-1 # 1696
  constant_a = -9;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1696, result);
  // Patch 99-2 # 1697
  constant_a = -8;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1697, result);
  // Patch 99-3 # 1698
  constant_a = -7;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1698, result);
  // Patch 99-4 # 1699
  constant_a = -6;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1699, result);
  // Patch 99-5 # 1700
  constant_a = -5;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1700, result);
  // Patch 99-6 # 1701
  constant_a = -4;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1701, result);
  // Patch 99-7 # 1702
  constant_a = -3;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1702, result);
  // Patch 99-8 # 1703
  constant_a = -2;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1703, result);
  // Patch 99-9 # 1704
  constant_a = -1;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1704, result);
  // Patch 99-10 # 1705
  constant_a = 0;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1705, result);
  // Patch 99-11 # 1706
  constant_a = 1;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1706, result);
  // Patch 99-12 # 1707
  constant_a = 2;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1707, result);
  // Patch 99-13 # 1708
  constant_a = 3;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1708, result);
  // Patch 99-14 # 1709
  constant_a = 4;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1709, result);
  // Patch 99-15 # 1710
  constant_a = 5;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1710, result);
  // Patch 99-16 # 1711
  constant_a = 6;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1711, result);
  // Patch 99-17 # 1712
  constant_a = 7;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1712, result);
  // Patch 99-18 # 1713
  constant_a = 8;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1713, result);
  // Patch 99-19 # 1714
  constant_a = 9;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1714, result);
  // Patch 99-20 # 1715
  constant_a = 10;
  result = (i <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1715, result);
  // Patch 100-0 # 1716
  constant_a = -10;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1716, result);
  // Patch 100-1 # 1717
  constant_a = -9;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1717, result);
  // Patch 100-2 # 1718
  constant_a = -8;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1718, result);
  // Patch 100-3 # 1719
  constant_a = -7;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1719, result);
  // Patch 100-4 # 1720
  constant_a = -6;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1720, result);
  // Patch 100-5 # 1721
  constant_a = -5;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1721, result);
  // Patch 100-6 # 1722
  constant_a = -4;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1722, result);
  // Patch 100-7 # 1723
  constant_a = -3;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1723, result);
  // Patch 100-8 # 1724
  constant_a = -2;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1724, result);
  // Patch 100-9 # 1725
  constant_a = -1;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1725, result);
  // Patch 100-10 # 1726
  constant_a = 0;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1726, result);
  // Patch 100-11 # 1727
  constant_a = 1;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1727, result);
  // Patch 100-12 # 1728
  constant_a = 2;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1728, result);
  // Patch 100-13 # 1729
  constant_a = 3;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1729, result);
  // Patch 100-14 # 1730
  constant_a = 4;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1730, result);
  // Patch 100-15 # 1731
  constant_a = 5;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1731, result);
  // Patch 100-16 # 1732
  constant_a = 6;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1732, result);
  // Patch 100-17 # 1733
  constant_a = 7;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1733, result);
  // Patch 100-18 # 1734
  constant_a = 8;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1734, result);
  // Patch 100-19 # 1735
  constant_a = 9;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1735, result);
  // Patch 100-20 # 1736
  constant_a = 10;
  result = (size <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1736, result);
  // Patch 101-0 # 1737
  constant_a = -10;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1737, result);
  // Patch 101-1 # 1738
  constant_a = -9;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1738, result);
  // Patch 101-2 # 1739
  constant_a = -8;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1739, result);
  // Patch 101-3 # 1740
  constant_a = -7;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1740, result);
  // Patch 101-4 # 1741
  constant_a = -6;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1741, result);
  // Patch 101-5 # 1742
  constant_a = -5;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1742, result);
  // Patch 101-6 # 1743
  constant_a = -4;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1743, result);
  // Patch 101-7 # 1744
  constant_a = -3;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1744, result);
  // Patch 101-8 # 1745
  constant_a = -2;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1745, result);
  // Patch 101-9 # 1746
  constant_a = -1;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1746, result);
  // Patch 101-10 # 1747
  constant_a = 0;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1747, result);
  // Patch 101-11 # 1748
  constant_a = 1;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1748, result);
  // Patch 101-12 # 1749
  constant_a = 2;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1749, result);
  // Patch 101-13 # 1750
  constant_a = 3;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1750, result);
  // Patch 101-14 # 1751
  constant_a = 4;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1751, result);
  // Patch 101-15 # 1752
  constant_a = 5;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1752, result);
  // Patch 101-16 # 1753
  constant_a = 6;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1753, result);
  // Patch 101-17 # 1754
  constant_a = 7;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1754, result);
  // Patch 101-18 # 1755
  constant_a = 8;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1755, result);
  // Patch 101-19 # 1756
  constant_a = 9;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1756, result);
  // Patch 101-20 # 1757
  constant_a = 10;
  result = (constant_a <= (constant_a / size));
  uni_klee_add_patch(patch_results, 1757, result);
  // Patch 102-0 # 1758
  constant_a = -10;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1758, result);
  // Patch 102-1 # 1759
  constant_a = -9;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1759, result);
  // Patch 102-2 # 1760
  constant_a = -8;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1760, result);
  // Patch 102-3 # 1761
  constant_a = -7;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1761, result);
  // Patch 102-4 # 1762
  constant_a = -6;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1762, result);
  // Patch 102-5 # 1763
  constant_a = -5;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1763, result);
  // Patch 102-6 # 1764
  constant_a = -4;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1764, result);
  // Patch 102-7 # 1765
  constant_a = -3;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1765, result);
  // Patch 102-8 # 1766
  constant_a = -2;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1766, result);
  // Patch 102-9 # 1767
  constant_a = -1;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1767, result);
  // Patch 102-10 # 1768
  constant_a = 1;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1768, result);
  // Patch 102-11 # 1769
  constant_a = 2;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1769, result);
  // Patch 102-12 # 1770
  constant_a = 3;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1770, result);
  // Patch 102-13 # 1771
  constant_a = 4;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1771, result);
  // Patch 102-14 # 1772
  constant_a = 5;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1772, result);
  // Patch 102-15 # 1773
  constant_a = 6;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1773, result);
  // Patch 102-16 # 1774
  constant_a = 7;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1774, result);
  // Patch 102-17 # 1775
  constant_a = 8;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1775, result);
  // Patch 102-18 # 1776
  constant_a = 9;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1776, result);
  // Patch 102-19 # 1777
  constant_a = 10;
  result = (i <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1777, result);
  // Patch 103-0 # 1778
  constant_a = -10;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1778, result);
  // Patch 103-1 # 1779
  constant_a = -9;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1779, result);
  // Patch 103-2 # 1780
  constant_a = -8;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1780, result);
  // Patch 103-3 # 1781
  constant_a = -7;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1781, result);
  // Patch 103-4 # 1782
  constant_a = -6;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1782, result);
  // Patch 103-5 # 1783
  constant_a = -5;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1783, result);
  // Patch 103-6 # 1784
  constant_a = -4;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1784, result);
  // Patch 103-7 # 1785
  constant_a = -3;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1785, result);
  // Patch 103-8 # 1786
  constant_a = -2;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1786, result);
  // Patch 103-9 # 1787
  constant_a = -1;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1787, result);
  // Patch 103-10 # 1788
  constant_a = 1;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1788, result);
  // Patch 103-11 # 1789
  constant_a = 2;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1789, result);
  // Patch 103-12 # 1790
  constant_a = 3;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1790, result);
  // Patch 103-13 # 1791
  constant_a = 4;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1791, result);
  // Patch 103-14 # 1792
  constant_a = 5;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1792, result);
  // Patch 103-15 # 1793
  constant_a = 6;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1793, result);
  // Patch 103-16 # 1794
  constant_a = 7;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1794, result);
  // Patch 103-17 # 1795
  constant_a = 8;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1795, result);
  // Patch 103-18 # 1796
  constant_a = 9;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1796, result);
  // Patch 103-19 # 1797
  constant_a = 10;
  result = (constant_a <= (i / constant_a));
  uni_klee_add_patch(patch_results, 1797, result);
  // Patch 104-0 # 1798
  constant_a = -10;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1798, result);
  // Patch 104-1 # 1799
  constant_a = -9;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1799, result);
  // Patch 104-2 # 1800
  constant_a = -8;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1800, result);
  // Patch 104-3 # 1801
  constant_a = -7;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1801, result);
  // Patch 104-4 # 1802
  constant_a = -6;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1802, result);
  // Patch 104-5 # 1803
  constant_a = -5;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1803, result);
  // Patch 104-6 # 1804
  constant_a = -4;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1804, result);
  // Patch 104-7 # 1805
  constant_a = -3;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1805, result);
  // Patch 104-8 # 1806
  constant_a = -2;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1806, result);
  // Patch 104-9 # 1807
  constant_a = -1;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1807, result);
  // Patch 104-10 # 1808
  constant_a = 1;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1808, result);
  // Patch 104-11 # 1809
  constant_a = 2;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1809, result);
  // Patch 104-12 # 1810
  constant_a = 3;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1810, result);
  // Patch 104-13 # 1811
  constant_a = 4;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1811, result);
  // Patch 104-14 # 1812
  constant_a = 5;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1812, result);
  // Patch 104-15 # 1813
  constant_a = 6;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1813, result);
  // Patch 104-16 # 1814
  constant_a = 7;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1814, result);
  // Patch 104-17 # 1815
  constant_a = 8;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1815, result);
  // Patch 104-18 # 1816
  constant_a = 9;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1816, result);
  // Patch 104-19 # 1817
  constant_a = 10;
  result = (i <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1817, result);
  // Patch 105-0 # 1818
  constant_a = -10;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1818, result);
  // Patch 105-1 # 1819
  constant_a = -9;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1819, result);
  // Patch 105-2 # 1820
  constant_a = -8;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1820, result);
  // Patch 105-3 # 1821
  constant_a = -7;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1821, result);
  // Patch 105-4 # 1822
  constant_a = -6;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1822, result);
  // Patch 105-5 # 1823
  constant_a = -5;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1823, result);
  // Patch 105-6 # 1824
  constant_a = -4;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1824, result);
  // Patch 105-7 # 1825
  constant_a = -3;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1825, result);
  // Patch 105-8 # 1826
  constant_a = -2;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1826, result);
  // Patch 105-9 # 1827
  constant_a = -1;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1827, result);
  // Patch 105-10 # 1828
  constant_a = 1;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1828, result);
  // Patch 105-11 # 1829
  constant_a = 2;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1829, result);
  // Patch 105-12 # 1830
  constant_a = 3;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1830, result);
  // Patch 105-13 # 1831
  constant_a = 4;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1831, result);
  // Patch 105-14 # 1832
  constant_a = 5;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1832, result);
  // Patch 105-15 # 1833
  constant_a = 6;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1833, result);
  // Patch 105-16 # 1834
  constant_a = 7;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1834, result);
  // Patch 105-17 # 1835
  constant_a = 8;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1835, result);
  // Patch 105-18 # 1836
  constant_a = 9;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1836, result);
  // Patch 105-19 # 1837
  constant_a = 10;
  result = (size <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1837, result);
  // Patch 106-0 # 1838
  constant_a = -10;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1838, result);
  // Patch 106-1 # 1839
  constant_a = -9;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1839, result);
  // Patch 106-2 # 1840
  constant_a = -8;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1840, result);
  // Patch 106-3 # 1841
  constant_a = -7;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1841, result);
  // Patch 106-4 # 1842
  constant_a = -6;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1842, result);
  // Patch 106-5 # 1843
  constant_a = -5;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1843, result);
  // Patch 106-6 # 1844
  constant_a = -4;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1844, result);
  // Patch 106-7 # 1845
  constant_a = -3;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1845, result);
  // Patch 106-8 # 1846
  constant_a = -2;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1846, result);
  // Patch 106-9 # 1847
  constant_a = -1;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1847, result);
  // Patch 106-10 # 1848
  constant_a = 1;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1848, result);
  // Patch 106-11 # 1849
  constant_a = 2;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1849, result);
  // Patch 106-12 # 1850
  constant_a = 3;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1850, result);
  // Patch 106-13 # 1851
  constant_a = 4;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1851, result);
  // Patch 106-14 # 1852
  constant_a = 5;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1852, result);
  // Patch 106-15 # 1853
  constant_a = 6;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1853, result);
  // Patch 106-16 # 1854
  constant_a = 7;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1854, result);
  // Patch 106-17 # 1855
  constant_a = 8;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1855, result);
  // Patch 106-18 # 1856
  constant_a = 9;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1856, result);
  // Patch 106-19 # 1857
  constant_a = 10;
  result = (constant_a <= (size / constant_a));
  uni_klee_add_patch(patch_results, 1857, result);
  // Patch correct # 1858
  result = (size / 2 >= i);
  uni_klee_add_patch(patch_results, 1858, result);
  klee_select_patch(&uni_klee_patch_id);
  return uni_klee_choice(patch_results, uni_klee_patch_id);
}
// UNI_KLEE_END

int __cpr_output(char* id, char* typestr, int value){
  return value;
}
