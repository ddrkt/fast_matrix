#ifndef C_ARRAY_OPERATIONS
#define C_ARRAY_OPERATIONS

#include  <stdbool.h>

void fill_d_array(int len, double* a, double v);
void multiply_d_array(int len, double* a, double v);
void copy_d_array(int len, const double* input, double* output);
void add_d_arrays_to_result(int len, const double* a1, const double* a2, double* result);
void add_d_arrays_to_first(int len, double* sum, const double* added);
void sub_d_arrays_to_result(int len, const double* dec, const double* sub, double* dif);
void sub_d_arrays_to_first(int len, double* dif, const double* sub);
bool equal_d_arrays(int len, const double* A, const double* B);
void abs_d_array(int len, const double* A, double* B);
bool greater_or_equal_d_array(int len, const double* A, const double* B);

#endif  /*C_ARRAY_OPERATIONS*/
