#include "matrix.h"
#include "c_array_operations.h"
#include "errors.h"
#include "vector.h"

VALUE cMatrix;

void matrix_free(void* data);
size_t matrix_size(const void* data);

const rb_data_type_t matrix_type =
{
    .wrap_struct_name = "matrix",
    .function =
    {
        .dmark = NULL,
        .dfree = matrix_free,
        .dsize = matrix_size,
    },
    .data = NULL,
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

void matrix_free(void* data)
{
    free(((*(struct matrix*)data)).data);
    free(data);
}

size_t matrix_size(const void* data)
{
	return sizeof(struct matrix);
}

VALUE matrix_alloc(VALUE self)
{
	struct matrix* mtx = malloc(sizeof(struct matrix));
    mtx->data = NULL;
	return TypedData_Wrap_Struct(self, &matrix_type, mtx);
}

void c_matrix_init(struct matrix* mtr, int m, int n)
{
    mtr->m = m;
    mtr->n = n;
    mtr->data = malloc(m * n * sizeof(double));
}

VALUE matrix_initialize(VALUE self, VALUE rows_count, VALUE columns_count)
{
	struct matrix* data;
    int m = raise_rb_value_to_int(columns_count);
    int n = raise_rb_value_to_int(rows_count);

    if(m <= 0 || n <= 0)
        rb_raise(fm_eIndexError, "Size cannot be negative or zero");

	TypedData_Get_Struct(self, struct matrix, &matrix_type, data);

    c_matrix_init(data, m, n);

	return self;
}

//  []=
VALUE matrix_set(VALUE self, VALUE row, VALUE column, VALUE v)
{
    int m = raise_rb_value_to_int(column);
    int n = raise_rb_value_to_int(row);
    double x = raise_rb_value_to_double(v);

	struct matrix* data;
	TypedData_Get_Struct(self, struct matrix, &matrix_type, data);
    
    m = (m < 0) ? data->m + m : m;
    n = (n < 0) ? data->n + n : n;

    raise_check_range(m, 0, data->m);
    raise_check_range(n, 0, data->n);

    data->data[m + data->m * n] = x;
    return v;
}

//  []
VALUE matrix_get(VALUE self, VALUE row, VALUE column)
{
    int m = raise_rb_value_to_int(column);
    int n = raise_rb_value_to_int(row);

	struct matrix* data;
	TypedData_Get_Struct(self, struct matrix, &matrix_type, data);
    
    m = (m < 0) ? data->m + m : m;
    n = (n < 0) ? data->n + n : n;
    
    if(m < 0 || n < 0 || n >= data->n || m >= data->m)
        return Qnil;

    return DBL2NUM(data->data[m + data->m * n]);
}

// in  - matrix m x n
// out - matrix n x m
void matrix_transpose(int m, int n, const double* in, double* out)
{
    for(int i = 0; i < m; ++i)
        for(int j = 0; j < n; ++j)
            out[j + n * i] = in[i + m * j];
}

// A - matrix k x n
// B - matrix m x k
// C - matrix m x n
void c_matrix_multiply(int n, int k, int m, const double* A, const double* B, double* C)
{
    fill_d_array(m * n, C, 0);

    for(int j = 0; j < n; ++j)
    {
        double* p_c = C + m * j;
        const double* p_a = A + k * j;

        for(int t = 0; t < k; ++t)
        {
            const double* p_b = B + m * t;
            double d_a = p_a[t];
            for(int i = 0; i < m; ++i)
                p_c[i] += d_a * p_b[i];
        }
    }
}

// M - matrix m x n
// V - vector m
// R - vector n
void c_matrix_vector_multiply(int n, int m, const double* M, const double* V, double* R)
{
    fill_d_array(n, R, 0);

    for(int j = 0; j < n; ++j)
    {
        const double* p_m = M + m * j;
        for(int i = 0; i < m; ++i)
            R[j] += V[i] * p_m[i];
    }
}

VALUE matrix_multiply_mv(VALUE self, VALUE other)
{
    struct matrix* M;
    struct vector* V;
    TypedData_Get_Struct(self, struct matrix, &matrix_type, M);
    TypedData_Get_Struct(other, struct vector, &vector_type, V);

    if(M->m != V->n)
        rb_raise(fm_eIndexError, "Matrix columns differs from vector size");

    int m = M->m;
    int n = M->n;

    struct vector* R;
    VALUE result = TypedData_Make_Struct(cVector, struct vector, &vector_type, R);

    c_vector_init(R, n);
    c_matrix_vector_multiply(n, m, M->data, V->data, R->data);

    return result;
}

// A - matrix k x n
// B - matrix m x k
// C - matrix m x n
void strassen_iteration(int n, int k, int m, const double* A, const double* B, double* C, int s_a, int s_b, int s_c)
{
    for(int j = 0; j < n; ++j)
    {
        double* p_c = C + s_c * j;
        const double* p_a = A + s_a * j;

        for(int t = 0; t < k; ++t)
        {
            const double* p_b = B + s_b * t;
            double d_a = p_a[t];
            for(int i = 0; i < m; ++i)
                p_c[i] += d_a * p_b[i];
        }
    }
}

bool check_strassen(int m, int n, int k)
{
    return n > 2 && m > 2 && k > 2 && (double)m * (double)n * (double)k > 100000000;
}


void strassen_copy(int m, int n, const double* A, double* B, int s_a, int s_b)
{
    for(int i = 0; i < n; ++i)
    {
        const double* p_A = A + i * s_a;
        double* p_B = B + i * s_b;
        for(int j = 0; j < m; ++j)
            p_B[j] = p_A[j];
    }
}

void strassen_sum_to_first(int m, int n, double* A, const double* B, int s_a, int s_b)
{
    for(int i = 0; i < n; ++i)
    {
        double* p_A = A + i * s_a;
        const double* p_B = B + i * s_b;
        for(int j = 0; j < m; ++j)
            p_A[j] += p_B[j];
    }
}

void strassen_sub_to_first(int m, int n, double* A, const double* B, int s_a, int s_b)
{
    for(int i = 0; i < n; ++i)
    {
        double* p_A = A + i * s_a;
        const double* p_B = B + i * s_b;
        for(int j = 0; j < m; ++j)
            p_A[j] -= p_B[j];
    }
}

// A - matrix k x n
// B - matrix m x k
// C - matrix m x n
void recursive_strassen(int n, int k, int m, const double* A, const double* B, double* C)
{
    if(!check_strassen(m, n, k))
        return c_matrix_multiply(n, k, m, A, B, C);

    int k2 = k / 2;
    int k1 = k - k2;
    int m2 = m / 2;
    int m1 = m - m2;
    int n2 = n / 2;
    int n1 = n - n2;

    double* termA = malloc(k1 * n1 * sizeof(double));
    double* termB = malloc(m1 * k1 * sizeof(double));

    double* P1 = malloc(7 * m1 * n1 * sizeof(double));
    double* P2 = P1 + m1 * n1;
    double* P3 = P2 + m1 * n1;
    double* P4 = P3 + m1 * n1;
    double* P5 = P4 + m1 * n1;
    double* P6 = P5 + m1 * n1;
    double* P7 = P6 + m1 * n1;
    fill_d_array(7 * m1 * n1, P1, 0);
    fill_d_array(k1 * n1, termA, 0);
    fill_d_array(m1 * k1, termB, 0);

    //  -----------P1-----------
    strassen_copy(k1, n1, A, termA, k, k1);
    strassen_sum_to_first(k2, n2, termA, A + k1 + k * n1, k1, k);
    
    strassen_copy(m1, k1, B, termB, m, m1);
    strassen_sum_to_first(m2, k2, termB, B + m1 + m * k1, m1, m);

    recursive_strassen(n1, k1, m1, termA, termB, P1);
    fill_d_array(k1 * n1, termA, 0);
    //  -----------P2-----------
    strassen_copy(k1, n2, A + k * n1, termA, k, k1);
    strassen_sum_to_first(k2, n2, termA, A + k1 + k * n1, k1, k);
    
    strassen_copy(m1, k1, B, termB, m, m1);

    recursive_strassen(n1, k1, m1, termA, termB, P2);
    fill_d_array(m1 * k1, termB, 0);
    //  -----------P3-----------
    strassen_copy(k1, n1, A, termA, k, k1);
    
    strassen_copy(m2, k1, B + m1, termB, m, m1);
    strassen_sub_to_first(m2, k2, termB, B + m1 + m * k1, m1, m);
    
    recursive_strassen(n1, k1, m1, termA, termB, P3);
    fill_d_array(k1 * n1, termA, 0);
    fill_d_array(m1 * k1, termB, 0);
    //  -----------P4-----------
    strassen_copy(k2, n2, A + k1 + k * n1, termA, k, k1);
    
    strassen_copy(m1, k2, B + m * k1, termB, m, m1);
    strassen_sub_to_first(m1, k1, termB, B, m1, m);
    
    recursive_strassen(n1, k1, m1, termA, termB, P4);
    fill_d_array(m1 * k1, termB, 0);
    //  -----------P5-----------
    strassen_copy(k1, n1, A, termA, k, k1);
    strassen_sum_to_first(k2, n1, termA, A + k1, k1, k);
    
    strassen_copy(m2, k2, B + m1 + m * k1, termB, m, m1);
    
    recursive_strassen(n1, k1, m1, termA, termB, P5);
    fill_d_array(k1 * n1, termA, 0);
    //  -----------P6-----------
    strassen_copy(k1, n2, A + k * n1, termA, k, k1);
    strassen_sub_to_first(k1, n1, termA, A, k1, k);
    
    strassen_copy(m1, k1, B, termB, m, m1);
    strassen_sum_to_first(m2, k1, termB, B + m1, m1, m);
    
    recursive_strassen(n1, k1, m1, termA, termB, P6);
    fill_d_array(k1 * n1, termA, 0);
    fill_d_array(m1 * k1, termB, 0);
    //  -----------P7-----------
    strassen_copy(k2, n1, A + k1, termA, k, k1);
    strassen_sub_to_first(k2, n2, termA, A + k1 + k * n1, k1, k);
    
    strassen_copy(m1, k2, B + k1 * m, termB, m, m1);
    strassen_sum_to_first(m2, k2, termB, B + m1 + m * k1, m1, m);
    
    recursive_strassen(n1, k1, m1, termA, termB, P7);

    //  -----------C11-----------
    double* C11 = C;
    strassen_copy(m1, n1, P1, C11, m1, m);
    strassen_sum_to_first(m1, n1, C11, P4, m, m1);
    strassen_sub_to_first(m1, n1, C11, P5, m, m1);
    strassen_sum_to_first(m1, n1, C11, P7, m, m1);
    //  -----------C12-----------
    double* C12 = C + m1;
    strassen_copy(m2, n1, P3, C12, m1, m);
    strassen_sum_to_first(m2, n1, C12, P5, m, m1);
    //  -----------C21-----------
    double* C21 = C + m * n1;
    strassen_copy(m1, n2, P2, C21, m1, m);
    strassen_sum_to_first(m1, n2, C21, P4, m, m1);
    //  -----------C22-----------
    double* C22 = C + m1 + m * n1;
    strassen_copy(m2, n2, P1, C22, m1, m);
    strassen_sub_to_first(m2, n2, C22, P2, m, m1);
    strassen_sum_to_first(m2, n2, C22, P3, m, m1);
    strassen_sum_to_first(m2, n2, C22, P6, m, m1);
    
    free(termA);
    free(termB);
    free(P1);
}

VALUE strassen(VALUE self, VALUE other)
{
	struct matrix* A;
    struct matrix* B;
	TypedData_Get_Struct(self, struct matrix, &matrix_type, A);
	TypedData_Get_Struct(other, struct matrix, &matrix_type, B);

    if(A->m != B->n)
        rb_raise(fm_eIndexError, "First columns differs from second rows");

    int m = B->m;
    int k = A->m;
    int n = A->n;

    struct matrix* C;
    VALUE result = TypedData_Make_Struct(cMatrix, struct matrix, &matrix_type, C);

    c_matrix_init(C, m, n);
    fill_d_array(m * n, C->data, 0);
    recursive_strassen(n, k, m, A->data, B->data, C->data);
    return result;
}

VALUE matrix_multiply_mm(VALUE self, VALUE other)
{
	struct matrix* A;
    struct matrix* B;
	TypedData_Get_Struct(self, struct matrix, &matrix_type, A);
	TypedData_Get_Struct(other, struct matrix, &matrix_type, B);

    if(A->m != B->n)
        rb_raise(fm_eIndexError, "First columns differs from second rows");

    int m = B->m;
    int k = A->m;
    int n = A->n;

    struct matrix* C;
    VALUE result = TypedData_Make_Struct(cMatrix, struct matrix, &matrix_type, C);

    c_matrix_init(C, m, n);
    c_matrix_multiply(n, k, m, A->data, B->data, C->data);

    return result;
}

VALUE matrix_multiply_mn(VALUE self, VALUE value)
{
	struct matrix* A;
	TypedData_Get_Struct(self, struct matrix, &matrix_type, A);

    double d = NUM2DBL(value);

    struct matrix* R;
    VALUE result = TypedData_Make_Struct(cMatrix, struct matrix, &matrix_type, R);

    c_matrix_init(R, A->m, A->n);
    copy_d_array(A->m * A->n, A->data, R->data);
    multiply_d_array(R->m * R->n, R->data, d);

    return result;
}

VALUE matrix_multiply(VALUE self, VALUE v)
{
    if(RB_FLOAT_TYPE_P(v) || FIXNUM_P(v)
        || RB_TYPE_P(v, T_BIGNUM))
        return matrix_multiply_mn(self, v);
    if(RBASIC_CLASS(v) == cMatrix)
        return matrix_multiply_mm(self, v);
    if(RBASIC_CLASS(v) == cVector);
        return matrix_multiply_mv(self, v);
    rb_raise(fm_eTypeError, "Invalid klass for multiply");
}

VALUE matrix_copy(VALUE mtrx)
{
	struct matrix* M;
	TypedData_Get_Struct(mtrx, struct matrix, &matrix_type, M);

    struct matrix* R;
    VALUE result = TypedData_Make_Struct(cMatrix, struct matrix, &matrix_type, R);

    c_matrix_init(R, M->m, M->n);
    copy_d_array(M->m * M->n, M->data, R->data);

    return result;
}

VALUE row_size(VALUE self)
{
	struct matrix* data;
	TypedData_Get_Struct(self, struct matrix, &matrix_type, data);
    return INT2NUM(data->m);
}

VALUE column_size(VALUE self)
{
	struct matrix* data;
	TypedData_Get_Struct(self, struct matrix, &matrix_type, data);
    return INT2NUM(data->n);
}

VALUE transpose(VALUE self)
{
	struct matrix* M;
	TypedData_Get_Struct(self, struct matrix, &matrix_type, M);

    struct matrix* R;
    VALUE result = TypedData_Make_Struct(cMatrix, struct matrix, &matrix_type, R);

    c_matrix_init(R, M->n, M->m);
    matrix_transpose(M->m, M->n, M->data, R->data);

    return result;
}

VALUE matrix_add_with(VALUE self, VALUE value)
{
	struct matrix* A;
    struct matrix* B;
	TypedData_Get_Struct(self, struct matrix, &matrix_type, A);
	TypedData_Get_Struct(value, struct matrix, &matrix_type, B);

    if(A->m != B->m && A->n != B->n)
        rb_raise(fm_eIndexError, "Different sizes matrices");

    int m = B->m;
    int n = A->n;

    struct matrix* C;
    VALUE result = TypedData_Make_Struct(cMatrix, struct matrix, &matrix_type, C);

    c_matrix_init(C, m, n);
    add_d_arrays_to_result(n * m, A->data, B->data, C->data);

    return result;
}

VALUE matrix_add_from(VALUE self, VALUE value)
{
	struct matrix* A;
    struct matrix* B;
	TypedData_Get_Struct(self, struct matrix, &matrix_type, A);
	TypedData_Get_Struct(value, struct matrix, &matrix_type, B);

    if(A->m != B->m && A->n != B->n)
        rb_raise(fm_eIndexError, "Different sizes matrices");

    int m = B->m;
    int n = A->n;

    add_d_arrays_to_first(n * m, A->data, B->data);

    return self;
}


VALUE matrix_sub_with(VALUE self, VALUE value)
{
	struct matrix* A;
    struct matrix* B;
	TypedData_Get_Struct(self, struct matrix, &matrix_type, A);
	TypedData_Get_Struct(value, struct matrix, &matrix_type, B);

    if(A->m != B->m && A->n != B->n)
        rb_raise(fm_eIndexError, "Different sizes matrices");

    int m = B->m;
    int n = A->n;

    struct matrix* C;
    VALUE result = TypedData_Make_Struct(cMatrix, struct matrix, &matrix_type, C);

    c_matrix_init(C, m, n);
    sub_d_arrays_to_result(n * m, A->data, B->data, C->data);

    return result;
}

double determinant(int n, const double* A)
{
    double* M = malloc(n * n * sizeof(double));
    double det = 1;
    copy_d_array(n * n, A, M);

    for(int i = 0; i < n; ++i)
    {
        const double* line_p = M + i + i * n;
        double current = *line_p; 
        det *= current;

        if(current == 0)
        {
            free(M);
            return 0;
        }

        for(int j = i + 1; j < n; ++j)
        {
            double* t_line = M + i + j * n;
            double head = *t_line;
            for(int k = 1; k < n - i; ++k)
                t_line[k] -= line_p[k] * head / current;
        }
    }

    free(M);
    return det;
}

VALUE matrix_determinant(VALUE self)
{
    struct matrix* A;
    TypedData_Get_Struct(self, struct matrix, &matrix_type, A);

    
    int m = A->m;
    int n = A->n;
    if(m != n)
        rb_raise(fm_eIndexError, "Not a square matrix");

    return DBL2NUM(determinant(n, A->data));
}

VALUE matrix_sub_from(VALUE self, VALUE value)
{
	struct matrix* A;
    struct matrix* B;
	TypedData_Get_Struct(self, struct matrix, &matrix_type, A);
	TypedData_Get_Struct(value, struct matrix, &matrix_type, B);

    if(A->m != B->m && A->n != B->n)
        rb_raise(fm_eIndexError, "Different sizes matrices");

    int m = B->m;
    int n = A->n;

    sub_d_arrays_to_first(n * m, A->data, B->data);

    return self;
}

VALUE matrix_fill(VALUE self, VALUE value)
{
    double d = raise_rb_value_to_double(value);
	struct matrix* A;
	TypedData_Get_Struct(self, struct matrix, &matrix_type, A);

    fill_d_array(A->m * A->n, A->data, d);

    return self;
}

VALUE matrix_equal(VALUE self, VALUE value)
{
	struct matrix* A;
    struct matrix* B;
	TypedData_Get_Struct(self, struct matrix, &matrix_type, A);
	TypedData_Get_Struct(value, struct matrix, &matrix_type, B);

    if(A->n != B->n || A->m != B->m)
		return Qfalse;

    int n = A->n;
    int m = B->m;

    if(equal_d_arrays(n * m, A->data, B->data))
		return Qtrue;
	return Qfalse;
}

VALUE matrix_abs(VALUE self)
{
	struct matrix* A;
	TypedData_Get_Struct(self, struct matrix, &matrix_type, A);

    int m = A->m;
    int n = A->n;

    struct matrix* B;
    VALUE result = TypedData_Make_Struct(cMatrix, struct matrix, &matrix_type, B);

    c_matrix_init(B, m, n);
    abs_d_array(n * m, A->data, B->data);

    return result;
}

VALUE matrix_greater_or_equal(VALUE self, VALUE value)
{
	struct matrix* A;
    struct matrix* B;
	TypedData_Get_Struct(self, struct matrix, &matrix_type, A);
	TypedData_Get_Struct(value, struct matrix, &matrix_type, B);

    if(A->m != B->m && A->n != B->n)
        rb_raise(fm_eIndexError, "Different sizes matrices");

    int m = B->m;
    int n = A->n;

    if(greater_or_equal_d_array(n * m, A->data, B->data))
        return Qtrue;
    return Qfalse;
}

void init_fm_matrix()
{
    VALUE  mod = rb_define_module("FastMatrix");
	cMatrix = rb_define_class_under(mod, "Matrix", rb_cData);

	rb_define_alloc_func(cMatrix, matrix_alloc);

	rb_define_method(cMatrix, "initialize", matrix_initialize, 2);
	rb_define_method(cMatrix, "[]", matrix_get, 2);
	rb_define_method(cMatrix, "[]=", matrix_set, 3);
	rb_define_method(cMatrix, "*", matrix_multiply, 1);
	rb_define_method(cMatrix, "column_count", row_size, 0);
	rb_define_method(cMatrix, "row_count", column_size, 0);
	rb_define_method(cMatrix, "clone", matrix_copy, 0);
	rb_define_method(cMatrix, "transpose", transpose, 0);
	rb_define_method(cMatrix, "+", matrix_add_with, 1);
	rb_define_method(cMatrix, "+=", matrix_add_from, 1);
	rb_define_method(cMatrix, "-", matrix_sub_with, 1);
	rb_define_method(cMatrix, "-=", matrix_sub_from, 1);
	rb_define_method(cMatrix, "fill!", matrix_fill, 1);
    rb_define_method(cMatrix, "strassen", strassen, 1);
    rb_define_method(cMatrix, "abs", matrix_abs, 0);
    rb_define_method(cMatrix, ">=", matrix_greater_or_equal, 1);
    rb_define_method(cMatrix, "determinant", matrix_determinant, 0);
    rb_define_method(cMatrix, "eql?", matrix_equal, 1);
}
