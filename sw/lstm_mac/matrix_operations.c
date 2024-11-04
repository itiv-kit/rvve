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

int8_t * mat_b_help = b_vector;
int8_t * mat_a_help = a_matrix;
uint32_t vl = 0;
int32_t acc = 0;
uint32_t j = 0;

  
  vl=a_width_b_height;


  for (uint32_t i = 0; i < a_height; i++){
    acc = 0;
    j = 0;
    
    mat_b_help = b_vector;


     
    do 
    {
      if((j+vl)<a_width_b_height){
          vl=32;
        }
      else{
          vl=a_width_b_height-j;                  
        }


      asm volatile(".insn u 0b1011011, %3, 0b11000001001100000111\n\t"
      :
      :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl), "r"(&acc)
      : );

         asm volatile(    
      ".insn i 0b1111011, 0b000,x24, %1,  0b000000000000\n\t"
     :
     :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
     :  "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" ); 

                
      asm volatile(
      ".insn i 0b1111011, 0b000,x16, %0,  0b000000000000\n\t"    
      :
      :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      :   "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" );

      asm volatile(
      ".insn r 0b1011011, 0b011, 0x00, x16, x24, x16\n\t"         
      :
      :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      : "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );

      asm volatile(          
      "add %2,x16,%2\n\t"
      "sw %2,0(%4)\n\t"

      :
      :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      :"x16","memory");

      j=j+vl;
      mat_a_help=mat_a_help+vl;
      mat_b_help=mat_b_help+vl;



    }while( j < a_width_b_height );
   

    asm volatile(
    "sw %2,0(%4)\n\t"
    :
    :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(c+i)
    :);     
    }
    
}
void mat_vec_mul_32_32(int8_t *a_matrix, int8_t *b_vector, int32_t* c, int32_t a_height, int32_t a_width_b_height)
{

    int8_t * mat_b_help = b_vector;
    int8_t * mat_a_help = a_matrix;
    uint32_t vl;
    vl= a_width_b_height;
    asm volatile(".insn u 0b1011011, %2, 0b11000001001100000111\n\t"
    :
    :"r"(mat_a_help), "r"(mat_b_help), "r"(vl)
    : );

    for (uint32_t i = 0; i < a_height; i++){

            
      
          asm volatile(    
        ".insn i 0b1111011, 0b000,x24, %1,  0b000000000000\n\t"
      :
      :"r"(mat_a_help), "r"(mat_b_help),  "r"(vl)
      :  "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" ); 

                  
        asm volatile(
        ".insn i 0b1111011, 0b000,x16, %0,  0b000000000000\n\t"    
        :
        :"r"(mat_a_help), "r"(mat_b_help)
        :   "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" );

        asm volatile(
        ".insn r 0b1011011, 0b011, 0x00, x16, x24, x16\n\t"         
        :
        :"r"(mat_a_help), "r"(mat_b_help)
        : "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" );

      

        asm volatile(
        "sw x16,0(%0)\n\t"
        :
        :  "r"(c+i)
        :"x16","memory"); 

        mat_a_help = mat_a_help+a_width_b_height;  
    
    }
    
}

void mat_vec_mul_32_8(int8_t *a_matrix, int8_t *b_vector, int32_t* c, int32_t a_height, int32_t a_width_b_height)
{

    int8_t * mat_b_help = b_vector;
    int8_t * mat_a_help = a_matrix;
    uint32_t vl = 0;

    vl = a_width_b_height;

    asm volatile(".insn u 0b1011011, %2, 0b11000001001100000111\n\t"
    :
    :"r"(mat_a_help), "r"(mat_b_help), "r"(vl)
    : );

    for (uint32_t i = 0; i < a_height; i++){

            
      
          asm volatile(    
        ".insn i 0b1111011, 0b000,x28, %1,  0b000000000000\n\t"
      :
      :"r"(mat_a_help), "r"(mat_b_help),  "r"(vl)
      :  "x28","x29","x30", "x31" ); 

                  
        asm volatile(
        ".insn i 0b1111011, 0b000,x30, %0,  0b000000000000\n\t"    
        :
        :"r"(mat_a_help), "r"(mat_b_help)
        :    "x28","x29","x30", "x31" );

        asm volatile(
        ".insn r 0b1011011, 0b011, 0x00, x28, x30, x28\n\t"         
        :
        :"r"(mat_a_help), "r"(mat_b_help)
        :  "x28","x29","x30", "x31");

      

        asm volatile(
        "sw x28,0(%0)\n\t"
        :
        :  "r"(c+i)
        :"x28","memory"); 

        mat_a_help = mat_a_help+a_width_b_height;  
    
    }
    
}