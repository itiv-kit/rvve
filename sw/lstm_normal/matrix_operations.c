#include "matrix_operations.h"

#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>

void point32_twise_multiplication(int8_t *a, int8_t *b, int8_t *c, int32_t n)
{
    int32_t i;
    for(i = 0; i < n; i++)
        c[i] = a[i] * b[i];
}

int32_t reduction_row(int8_t *a,  int32_t n)
{
    int32_t res = 0;
    for(int32_t i = 0; i < n; i++)
    {
        res += (int32_t) a[i];
    }
    return res;
}

void weight_redudction_row(int8_t *a, int32_t *b, int32_t a_height, int32_t a_width)
{
    for(int32_t i=0; i<a_height;i++)
    {
     b[i]=reduction_row( a+(i*a_width), a_width); 

    }

}


int32_t reduction_column(int8_t *a,int32_t a_height , int32_t a_width)
{
    int32_t res = 0;
    for(int32_t i = 0; i < a_height; i++)
    {
        res += (int32_t) a[i*a_width];
    }
    return res;
}

void inplace_point32_twise_multiplication(int32_t *a, int32_t *b, int32_t n)
{
    int32_t i;
    for (i = 0; i < n; i++)
        a[i] *= b[i];
}

// target is a
void inplace_point32_twise_add(int32_t *a, int32_t *b, int32_t n)
{
    int32_t i;
    for (i = 0; i < n; i++)
        a[i] += b[i];
}

void inplace_add_bias(int32_t *a, int32_t *bias, int32_t a_height, int32_t a_width)
{
    for (int32_t n = 0; n < a_height; n++)
    {
        for (int32_t m = 0; m < a_width; m++)
        {
            a[(n * a_width) + m] += bias[m];
        }
    }
}

void weight_redudction_column(int8_t *a, int32_t *b, int32_t a_height, int32_t a_width)
{
    for(int32_t i=0; i<a_width;i++)
    {
     b[i]=reduction_column( a+i, a_height, a_width); 

    }

}

void inplace_multiplication_vector_scalar(int32_t *a_vector, int32_t *b_scalar, int32_t n)
{

    int32_t i;
    for (i = 0; i < n; i++)
        a_vector[i] *= *b_scalar;


}

void inplace_point32_twise_add_scalar(int32_t *a_vector, int32_t *b_scalar, int32_t n)
{
        int32_t i;
    for (i = 0; i < n; i++)
        a_vector[i] += *b_scalar;
}

//multiplication of matrix and vector
void mat_vec_mul(int8_t *a_matrix, int8_t *b_vector, int32_t* c, int32_t a_height, int32_t a_width_b_height)
{
    int32_t acc = 0;

    for (int32_t i = 0; i < a_height; i++)
    {
  
        acc = 0;
        for (int32_t j = 0; j < a_width_b_height; j++)
        {
            acc +=  (int32_t) a_matrix[(i * a_width_b_height) + j] * (int32_t) b_vector[j];

        }
        c[i] = acc;
        
    }
}
