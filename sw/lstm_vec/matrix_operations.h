#ifndef _MATRIX_OPERATIONS_H
#define _MATRIX_OPERATIONS_H

#include <stdint.h>

extern void point32_twise_multiplication(int8_t *a, int8_t *b, int8_t *c, int32_t n);
extern void inplace_point32_twise_multiplication(int32_t *a, int32_t *b, int32_t n);
extern void inplace_point32_twise_add(int32_t *a, int32_t *b, int32_t n);
void mat_vec_mul(int8_t *a_matrix, int8_t *b_vector, int32_t* c, int32_t a_height, int32_t a_width_b_height);
void mat_vec_mul_32_32(int8_t *a_matrix, int8_t *b_vector, int32_t* c, int32_t a_height, int32_t a_width_b_height);
void mat_vec_mul_32_8(int8_t *a_matrix, int8_t *b_vector, int32_t* c, int32_t a_height, int32_t a_width_b_height);
extern void inplace_add_bias(int32_t *a, int32_t *bias, int32_t a_height, int32_t a_width);
extern void inplace_point32_twise_add_scalar(int32_t *a, int32_t *b, int32_t n);
extern void inplace_multiplication_vector_scalar(int32_t *a, int32_t *b, int32_t n);
extern int32_t reduction(int8_t *a,  int32_t n);
void weight_redudction_row(int8_t *a, int32_t *b, int32_t a_height, int32_t a_width);
void weight_redudction_column(int8_t *a, int32_t *b, int32_t a_height, int32_t a_width);
void inplace_point32_twise_multiplication_32(int32_t *a, int32_t *b, int32_t n);


#endif