#include "matrix.h"
#include "c_array_operations.h"

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

	return TypedData_Wrap_Struct(self, &matrix_type, mtx);
}

double raise_rb_value_to_double(VALUE v)
{
    if(RB_FLOAT_TYPE_P(v) || FIXNUM_P(v)
        || RB_TYPE_P(v, T_BIGNUM))
        return NUM2DBL(v);

    rb_raise(matrix_eTypeError, "Value is not number");
    return 0;
}

int raise_rb_value_to_int(VALUE v)
{
    if(FIXNUM_P(v))
        return NUM2INT(v);

    rb_raise(matrix_eTypeError, "Index is not integer");
    return 0;
}

void raise_check_range(int v, int min, int max)
{
    if(v < min || v >= max)
        rb_raise(matrix_eIndexError, "Index out of range");
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
        rb_raise(matrix_eIndexError, "Size cannot be negative or zero");

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

    raise_check_range(m, 0, data->m);
    raise_check_range(n, 0, data->n);

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
            {
                p_c[i] += d_a * p_b[i];
            }
        }
    }
}

VALUE matrix_multiply_mm(VALUE self, VALUE other)
{
	struct matrix* A;
    struct matrix* B;
	TypedData_Get_Struct(self, struct matrix, &matrix_type, A);
	TypedData_Get_Struct(other, struct matrix, &matrix_type, B);

    if(A->m != B->n)
        rb_raise(matrix_eIndexError, "First columns differs from second rows");

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
    return matrix_multiply_mm(self, v);
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

void init_fm_matrix()
{
    VALUE  mod = rb_define_module("FastMatrix");
	cMatrix = rb_define_class_under(mod, "Matrix", rb_cData);

    matrix_eTypeError  = rb_define_class_under(cMatrix, "TypeError",  rb_eTypeError);
    matrix_eIndexError = rb_define_class_under(cMatrix, "IndexError", rb_eIndexError);


	rb_define_alloc_func(cMatrix, matrix_alloc);

	rb_define_method(cMatrix, "initialize", matrix_initialize, 2);
	rb_define_method(cMatrix, "[]", matrix_get, 2);
	rb_define_method(cMatrix, "[]=", matrix_set, 3);
	rb_define_method(cMatrix, "*", matrix_multiply, 1);
	rb_define_method(cMatrix, "column_count", row_size, 0);
	rb_define_method(cMatrix, "row_count", column_size, 0);
	rb_define_method(cMatrix, "copy", matrix_copy, 0);
	rb_define_method(cMatrix, "transpose", transpose, 0);
}