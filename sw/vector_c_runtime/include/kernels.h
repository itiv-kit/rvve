#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <neorv32.h>

// TODO:  The clobbering marks are probably all a bit overly cautious, maybe those could be simplified
//        Clean up the ASM code
//        Simplify the while loops with less control flow

// Architecture constraints
#define MAX_VLEN_MAC 32
#define MAX_VLEN_ADD 8

// Helper functions
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// Structs for fixed point representation of quantization parameters
typedef struct {
      int64_t value;
      uint8_t fraction_length;
      uint8_t word_length;
      uint8_t int_length;
      int32_t two_power_frac;
} fixed_point_t;

typedef struct {
   fixed_point_t scale;
   int32_t zero_point;
} quantization_parameters_t;


// Perform tiled elementwise multiplication of two vector that must be of equal len
// After this operation, vector_a will contain the results
void tiled_inplace_elementwise_mul(int32_t* vector_a, int32_t* vector_b, uint32_t len){
    int32_t * vec_b_help = vector_b;
    int32_t * vec_a_help = vector_a;
    uint32_t vl = MAX_VLEN_ADD;
    int32_t acc = 0;
    uint32_t j = 0;
    do {
      if((j+vl)<len){
        vl=MAX_VLEN_ADD;
      } else {
        vl=len-j;                  
      }

      // Configure vector length
      asm volatile(".insn u 0b1011011, %3, 0b11000000001100000111\n\t"
      :
      :"r"(vec_a_help), "r"(vec_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      : );

      // Load elements of vector_b
      asm volatile(    
      ".insn i 0b1111011, 0b010,x24, %1,  0b000000000100\n\t"
      :
      :"r"(vec_a_help), "r"(vec_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      :  "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" ); 

      // Load elements of vector_a
      asm volatile(
     ".insn i 0b1111011, 0b010,x16, %0,  0b000000000100\n\t"
      :
      :"r"(vec_a_help), "r"(vec_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      :   "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" );

      // Execute multiplication
      asm volatile(
      ".insn r 0b1011011, 0b100, 0x00, x16, x24, x16\n\t"         
      :
      :"r"(vec_a_help), "r"(vec_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      : "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );

      // Store back results
      asm volatile(        
      ".insn s 0b0101011, 0b010, x16 ,  0b000000000000(%0)\n\t"   
      :
      :"r"(vec_a_help), "r"(vec_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      :"x16", "x17", "x18","x19","x20", "x21", "x22","x23","memory");

      j=j+vl;
      vec_a_help = vec_a_help+vl;
      vec_b_help = vec_b_help+vl;
    } while( j < len );
}


// Perform tiled elementwise addition of two vectors that must be of equal len
// After this operation, vector_a will contain the results
void tiled_inplace_elementwise_add(int32_t* vector_a, int32_t* vector_b, uint32_t len){
    int32_t * vec_b_help = vector_b;
    int32_t * vec_a_help = vector_a;
    uint32_t vl = MAX_VLEN_ADD;
    int32_t acc = 0;
    uint32_t j = 0;
    do {
      if((j+vl)<len){
        vl=MAX_VLEN_ADD;
      } else {
        vl=len-j;                  
      }

      // Configure VLEN
      asm volatile(".insn u 0b1011011, %3, 0b11000000001100000111\n\t"
      :
      :"r"(vec_a_help), "r"(vec_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      : );

      // Load vector_b
      asm volatile(    
      ".insn i 0b1111011, 0b010,x24, %1,  0b000000000100\n\t"
      :
      :"r"(vec_a_help), "r"(vec_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      :  "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" ); 

      // Load vector_a
      asm volatile(
      ".insn i 0b1111011, 0b010,x16, %0,  0b000000000100\n\t"
      :
      :"r"(vec_a_help), "r"(vec_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      :   "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" );

      // Perform inplace VADD
      asm volatile(
      ".insn r 0b1011011, 0b010, 0x00, x16, x24, x16\n\t"         
      :
      :"r"(vec_a_help), "r"(vec_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      : "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );

      // Store results
      asm volatile(        
      ".insn s 0b0101011, 0b010,  x16 ,0b000000000000(%0)\n\t"
      :
      :"r"(vec_a_help), "r"(vec_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      :"x16", "x17", "x18","x19","x20", "x21", "x22","x23","memory");

      j=j+vl;
      vec_a_help=vec_a_help+vl;
      vec_b_help=vec_b_help+vl;

    } while( j < len );

}

void tiled_inplace_mul_scalar(int32_t* vector_a, int32_t* scalar_b, uint32_t len){
    int32_t * vec_b_help = scalar_b;
    int32_t * vec_a_help = vector_a;
    uint32_t vl = 8;
    uint32_t j = 0;
    uint32_t stride = 0;

   do {
    if((j+vl)<len){
      vl=MAX_VLEN_ADD;
    } else {
      vl=len-j;                  
    }

    // Configure VLEN
    asm volatile(".insn u 0b1011011, %2, 0b11000000001100000111\n\t"
    :
    :"r"(vec_a_help), "r"(vec_b_help), "r"(vl)
    : );

    // Load b with stride = 0
    asm volatile(    
    ".insn r 0b1111011, 0b010, 0b0000110,  x24, %1, %3\n\t"
    :
    :"r"(vec_a_help), "r"(vec_b_help), "r"(vl), "r"(stride)
    :  "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" ); 

    // Load vector_a
    asm volatile(
    ".insn i 0b1111011, 0b010,x16, %0,  0b000000000100\n\t"
    :
    :"r"(vec_a_help), "r"(vec_b_help), "r"(vl)
    :   "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" );

    // Perform VMUL operation
    asm volatile(
    ".insn r 0b1011011, 0b100, 0x00, x16, x24, x16\n\t"         
    :
    :"r"(vec_a_help), "r"(vec_b_help), "r"(vl)
    : "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );

    // Store result
    asm volatile(        
    ".insn s 0b0101011, 0b010, x16, 0b000000000000(%0)\n\t"
    :
    :"r"(vec_a_help), "r"(vec_b_help), "r"(vl)
    :"x16", "x17", "x18","x19","x20", "x21", "x22","x23","memory");

    j=j+vl;
    vec_a_help=vec_a_help+vl;

  } while( j < len );
}

void tiled_inplace_add_scalar(int32_t* vector_a, int32_t* scalar_b, uint32_t len){
    int32_t * vec_b_help = scalar_b;
    int32_t * vec_a_help = vector_a;
    uint32_t vl = 8;
    uint32_t j = 0;
    uint32_t stride = 0;

   do {
    if((j+vl)<len){
      vl=MAX_VLEN_ADD;
    } else {
      vl=len-j;                  
    }

    // Configure VLEN
    asm volatile(".insn u 0b1011011, %2, 0b11000000001100000111\n\t"
    :
    :"r"(vec_a_help), "r"(vec_b_help), "r"(vl)
    : );

    // Load b with stride = 0
    asm volatile(    
    ".insn r 0b1111011, 0b010, 0b0000110,  x24, %1, %3\n\t"
    :
    :"r"(vec_a_help), "r"(vec_b_help), "r"(vl), "r"(stride)
    :  "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" ); 

    // Load vector_a
    asm volatile(
    ".insn i 0b1111011, 0b010,x16, %0,  0b000000000100\n\t"
    :
    :"r"(vec_a_help), "r"(vec_b_help), "r"(vl)
    :   "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" );

    // Perform VADD operation
    asm volatile(
    ".insn r 0b1011011, 0b010, 0x00, x16, x24, x16\n\t"         
    :
    :"r"(vec_a_help), "r"(vec_b_help), "r"(vl)
    : "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );

    // Store result
    asm volatile(        
    ".insn s 0b0101011, 0b010, x16, 0b000000000000(%0)\n\t"
    :
    :"r"(vec_a_help), "r"(vec_b_help), "r"(vl)
    :"x16", "x17", "x18","x19","x20", "x21", "x22","x23","memory");

    j=j+vl;
    vec_a_help=vec_a_help+vl;

  } while( j < len );
}

// Perform inner product reduction operation along a complete, untiled axis
// Results are stored to acc and must later be written back to memory
void tiled_inner_product(int8_t* input, int8_t* weights, int32_t* acc, uint32_t len, bool weights_transposed){
  uint32_t macs_performed = 0;
  uint32_t vl = MAX_VLEN_MAC;

  uint32_t stride = len;

  do {
    if((macs_performed+vl)<len){
      vl=MAX_VLEN_MAC;
    } else{
      vl=len-macs_performed;                  
    }

    // Configure VLEN
    asm volatile(".insn u 0b1011011, %3, 0b11000001001100000111\n\t"
    :
    :"r"(input), "r"(weights), "r"(acc), "r"(vl), "r"(&acc)
    : );

    // Load tile of vector_b
    if(weights_transposed){
      // Load unit-strided row by row
      asm volatile(    
      ".insn i 0b1111011, 0b000,x24, %1,  0b000000000000\n\t"
      :
      :"r"(input), "r"(weights), "r"(acc), "r"(vl),  "r"(&acc)
      :  "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" ); 

    } else {
      // Strided access pattern 
      asm volatile(    
      ".insn r 0b1111011, 0b010, 0b0000110,  x24, %1,%5\n\t"
      :
      :"r"(input), "r"(weights), "r"(acc), "r"(vl),  "r"(&acc),"r"(stride)
      :  "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" ); 
    }

              
    // Load tile of matrix_a
    asm volatile(
    ".insn i 0b1111011, 0b000,x16, %0,  0b000000000000\n\t"    
    :
    :"r"(input), "r"(weights), "r"(acc), "r"(vl),  "r"(&acc)
    :   "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" );

    // Perform reduction VMAC
    asm volatile(
    ".insn r 0b1011011, 0b011, 0x00, x16, x24, x16\n\t"         
    :
    :"r"(input), "r"(weights), "r"(acc), "r"(vl),  "r"(&acc)
    : "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );

    // Accumulate partial result and store to memory
    asm volatile(          
    "add %2,x16,%2\n\t"
    "sw %2,0(%4)\n\t"
    :
    :"r"(input), "r"(weights), "r"(acc), "r"(vl),  "r"(&acc)
    :"x16","memory");

    macs_performed=macs_performed+vl;

  } while(macs_performed < len);   
}


// Perform A*x operation and store result to memory
// We assume that A is of size a_rows x a_cols and b is a vector of length a_cols
// Here, a_cols is the reduction axis
void tiled_matvec_mul(int8_t* matrix_a, int8_t* vector_b, int32_t* result, uint32_t a_rows, uint32_t a_cols){
    int8_t * mat_b_help = vector_b;
    int8_t * mat_a_help = matrix_a;

    uint32_t vl = MAX_VLEN_MAC;
    uint32_t col_dim = 0;
    int32_t acc = 0;

  for (uint32_t i = 0; i < a_rows; i++){
    acc = 0;
    col_dim = 0;
    
    mat_b_help = vector_b;
     
    do {
      if((col_dim+vl)<a_cols){
        vl=MAX_VLEN_MAC;
      } else{
        vl=a_cols-col_dim;                  
      }

      // Configure VLEN
      asm volatile(".insn u 0b1011011, %3, 0b11000001001100000111\n\t"
      :
      :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl), "r"(&acc)
      : );

      // Load tile of vector_b
      asm volatile(    
      ".insn i 0b1111011, 0b000,x24, %1,  0b000000000000\n\t"
      :
      :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      :  "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" ); 

                
      // Load tile of matrix_a
      asm volatile(
      ".insn i 0b1111011, 0b000,x16, %0,  0b000000000000\n\t"    
      :
      :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      :   "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" );

      // Perform reduction VMAC
      asm volatile(
      ".insn r 0b1011011, 0b011, 0x00, x16, x24, x16\n\t"         
      :
      :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      : "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );

      // Accumulate partial result and store to memory
      asm volatile(          
      "add %2,x16,%2\n\t"
      "sw %2,0(%4)\n\t"
      :
      :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      :"x16","memory");

      col_dim=col_dim+vl;
      mat_a_help=mat_a_help+vl;
      mat_b_help=mat_b_help+vl;

    } while(col_dim < a_cols);   

    asm volatile(
    "sw %2,0(%4)\n\t"
    :
    :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(result+i)
    :"memory");     
  }

}

// Perform matmul operation and store result to memory
// A is of size (a_rows x reduction_dim)
// B is of size (reduction_dim x b_cols)
void tiled_matmul(int8_t* matrix_a, int8_t* matrix_b, int32_t* result, 
                  uint32_t a_rows, uint32_t reduction_dim, uint32_t b_cols,
                  bool b_is_unit_strided){
  // internal convention:
  //  i := a_rows
  //  k := reduction_dim
  //  j := b_cols

  int8_t * mat_b_help = matrix_b;
  int8_t * mat_a_help = matrix_a;

  uint32_t stride = reduction_dim;
  uint32_t vl = MAX_VLEN_MAC;
  uint32_t red_dim = 0;
  int32_t acc = 0;
  for(uint32_t i = 0; i < a_rows; i++){
    for(uint32_t j = 0; j < b_cols; j++){
      acc = 0;
      red_dim = 0;

      // Address calculation
      mat_a_help = matrix_a + i*reduction_dim;

      if(b_is_unit_strided){
        mat_b_help = matrix_b + reduction_dim*j;
      } else {
        mat_b_help = matrix_b + j;
      }

      // Reduction loop
      do {
        if((red_dim+vl)<reduction_dim){
          vl=MAX_VLEN_MAC;
        } else{
          vl=reduction_dim-red_dim;                  
        }

        // Configure VLEN
        asm volatile(".insn u 0b1011011, %3, 0b11000001001100000111\n\t"
        :
        :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl), "r"(&acc)
        : );

        // Load tile of vector_b
        if(b_is_unit_strided){
          //Load unit-strided row by row
          asm volatile(    
          ".insn i 0b1111011, 0b000,x24, %1,  0b000000000000\n\t"
          :
          :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
          :  "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" ); 

        } else {
          // Strided access pattern 
          asm volatile(    
          ".insn r 0b1111011, 0b010, 0b0000110,  x24, %1,%5\n\t"
          :
          :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc),"r"(stride)
          :  "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" ); 
        }

        // Load tile of matrix_a
        asm volatile(
        ".insn i 0b1111011, 0b000,x16, %0,  0b000000000000\n\t"    
        :
        :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
        :   "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" );

        // Perform reduction VMAC
        asm volatile(
        ".insn r 0b1011011, 0b011, 0x00, x16, x24, x16\n\t"         
        :
        :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
        : "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );

        // Accumulate partial result and store to memory
        asm volatile(          
        "add %2,x16,%2\n\t"
        "sw %2,0(%4)\n\t"
        :
        :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
        :"x16","memory");

        red_dim=red_dim+vl;
        mat_a_help=mat_a_help+vl;
        mat_b_help=mat_b_help+vl;

      } while(red_dim < reduction_dim);   

      asm volatile(
      "sw %2,0(%4)\n\t"
      :
      :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(result+i*b_cols+j)
      :"memory");     

    }
  }
}

// Convolution operation over input and kernel
// Input is assumed to be in NHWC format, kernel in HWOI
void tiled_conv(int8_t* input, int8_t* kernel, int32_t* result,
              uint32_t batch_size, uint32_t in_rows, uint32_t in_cols, uint32_t in_channels,
              uint32_t out_rows, uint32_t out_cols, uint32_t out_channels,
              uint32_t kernel_rows, uint32_t kernel_cols,
              uint32_t padding, uint32_t stride,
              int32_t input_zero_point){

  int32_t acc;
  uint32_t red_dim;
  uint32_t vl = MAX_VLEN_MAC;

  int8_t* input_help = input;
  int8_t *kernel_help = kernel;
  int32_t* result_help = result;

  uint32_t input_col_idx, input_row_idx;
  bool a_load_zero_point = false;
  uint32_t a_stride = 0;

  for(int n = 0; n < batch_size; n++){
    for(int p = 0; p < out_rows; p++){
      for(int q = 0; q < out_cols; q++){
        for(int k = 0; k < out_channels; k++){
         
          result_help = result +
                        (n*out_rows*out_cols*out_channels) +
                        (p*out_cols*out_channels) +
                        (q*out_channels) +
                        (k);
          acc = 0; 

          for(int r = 0; r < kernel_rows; r++){
            for(int s = 0; s < kernel_cols; s++){

              input_row_idx = (p*stride) + r - padding;
              input_col_idx = (q*stride) + s - padding;

              // Are we in a padded dimension?
              if( (input_col_idx < 0 || input_row_idx < 0) || (input_col_idx > in_cols || input_row_idx > in_rows) ){
                if(input_zero_point != 0){
                  // Load input zero point
                  input_help = (int8_t*) &input_zero_point;
                  a_load_zero_point = true;
                } else {
                  // zero_point is zero, so we can just skip the computation
                  a_load_zero_point = false;
                  continue;
                }
              }

              // Address calculation, set in_channel offset to 0 as the inner loop will perform the full reduction along the axis
              input_help = input + 
                          (n            *in_rows*in_cols*in_channels) + 
                          (input_row_idx*in_cols*in_channels) +
                          (input_col_idx*in_channels) +
                          (0);
              kernel_help = kernel +
                          (r*kernel_cols*out_channels*in_channels) +
                          (s*out_channels*in_channels) +
                          (k*in_channels) +
                          (0);
              red_dim = 0;

              // vectorized reduction over in_channels
              do {
                if((red_dim+vl)<in_channels){
                  vl=MAX_VLEN_MAC;
                } else{
                  vl=in_channels-red_dim;                  
                }

                // Configure VLEN
                asm volatile(".insn u 0b1011011, %3, 0b11000001001100000111\n\t"
                :
                :"r"(input_help), "r"(kernel_help), "r"(acc), "r"(vl), "r"(&acc)
                : );

                // Load unit-strided row by row
                asm volatile(    
                ".insn i 0b1111011, 0b000,x24, %1,  0b000000000000\n\t"
                :
                :"r"(input_help), "r"(kernel_help), "r"(acc), "r"(vl),  "r"(&acc)
                :  "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" ); 
                          
                // Load tile of matrix_a
                if (!a_load_zero_point) {
                  asm volatile(
                  ".insn i 0b1111011, 0b000,x16, %0,  0b000000000000\n\t"    
                  :
                  :"r"(input_help), "r"(kernel_help), "r"(acc), "r"(vl),  "r"(&acc)
                  :   "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" );
                } else {
                  // Load scalar zero point by setting stride=0
                  asm volatile(    
                  ".insn r 0b1111011, 0b010, 0b0000110,  x24, %0, %5\n\t"
                  :
                  :"r"(input_help), "r"(kernel_help), "r"(acc), "r"(vl),  "r"(&acc),"r"(a_stride)
                  :   "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" );
                }

                // Perform reduction VMAC
                asm volatile(
                ".insn r 0b1011011, 0b011, 0x00, x16, x24, x16\n\t"         
                :
                :"r"(input_help), "r"(kernel_help), "r"(acc), "r"(vl),  "r"(&acc)
                : "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );

                // Accumulate partial result and store to memory
                asm volatile(          
                "add %2,x16,%2\n\t"
                "sw %2,0(%4)\n\t"
                :
                :"r"(input_help), "r"(kernel_help), "r"(acc), "r"(vl),  "r"(&acc)
                :"x16","memory");

                red_dim=red_dim+vl;
                input_help=input_help+vl;
                kernel_help=kernel_help+vl;

              } while(red_dim < in_channels);   
            }
          } // reduction complete

          // store back result
          asm volatile(
          "sw %2,0(%3)\n\t"
          :
          :"r"(input_help), "r"(kernel_help), "r"(acc), "r"(result_help)
          :"memory");     

        }
      }
    }
  }

}


void matvec_mul_cpu(int8_t* matrix_a, int8_t* vector_b, int32_t* result, 
                    uint32_t a_rows, uint32_t a_cols){
  
  int32_t acc = 0;

  for(int32_t i = 0; i < a_rows; i++){
    acc = 0;
    for(int32_t k = 0; k < a_cols; k++){
      acc += vector_b[k] * matrix_a[(i*a_cols) + k];
    }
    result[i] = acc;
  }
}


void matmul_cpu(int8_t* matrix_a, int8_t* matrix_b, int32_t* result,
                uint32_t a_rows, uint32_t reduction_dim, uint32_t b_cols,
                bool b_is_unit_strided){
  int32_t acc = 0;

  for(int32_t i = 0; i < a_rows; i++){
    for(int32_t j = 0; j < b_cols; j++){

      acc = 0;
      for(int32_t k = 0; k < reduction_dim; k++){
        if(!b_is_unit_strided){
          acc += matrix_a[i*reduction_dim + k] * matrix_b[k*b_cols + j];
        } else {
          acc += matrix_a[i*reduction_dim + k] * matrix_b[j*reduction_dim + k];
        }
      }
      result[i*b_cols + j] = acc;
    }
  }
}


void conv_cpu(int8_t* input, int8_t* kernel, int32_t* result,
              uint32_t batch_size, uint32_t in_rows, uint32_t in_cols, uint32_t in_channels,
              uint32_t out_rows, uint32_t out_cols, uint32_t out_channels,
              uint32_t kernel_rows, uint32_t kernel_cols,
              uint32_t padding, uint32_t stride){

  int32_t acc = 0;
  int8_t input_value = 0;
  int8_t weight_value = 0;

  int32_t input_row_idx, input_col_idx;

  for(int n = 0; n < batch_size; n++){
    for(int p = 0; p < out_rows; p++){
      for(int q = 0; q < out_cols; q++){
        for(int k = 0; k < out_channels; k++){

         acc = 0; 

          for(int r = 0; r < kernel_rows; r++){
            for(int s = 0; s < kernel_cols; s++){
              for(int ic = 0; ic < in_channels; ic++){

                input_row_idx = (p*stride) + r - padding;
                input_col_idx = (q*stride) + s - padding;

                if( (input_col_idx < 0 || input_row_idx < 0) || (input_col_idx > in_cols || input_row_idx > in_rows)){
                  input_value = 0;
                } else {
                  input_value = input[n*in_rows*in_cols*in_channels + input_row_idx*in_cols*in_channels + input_col_idx*in_channels + ic];
                }

                weight_value = kernel[r*kernel_cols*out_channels*in_channels + s*out_channels*in_channels + k*in_channels + ic];
                acc += input_value*weight_value;
              }
            }
          }

          result[n*out_rows*out_cols*out_channels + p*out_cols*out_channels + q*out_channels + k] = acc;
        }
      }
    }
  }

}


// 2D and 1D pooling operation executed on scalar units
void maxpool(int8_t* input, int8_t* result, 
            uint32_t batches,
            uint32_t row_out_dim, uint32_t col_out_dim, uint32_t channels,
            uint32_t pool_rows, uint32_t pool_cols, uint32_t pool_pad, uint32_t stride){

  int8_t max;
  size_t col_idx, row_idx;

  for(int c = 0; c < channels; c++){
    for(int n = 0; n < batches; n++){
      for(int p = 0; p < row_out_dim; p++){
        for(int q = 0; q < col_out_dim; q++){
          
          max = INT8_MIN;
          for(int r = 0; r < pool_rows; r++){
            for(int s = 0; s < pool_cols; s++){

              col_idx = (q*stride) + s - pool_pad;
              row_idx = (p*stride) + r - pool_pad;
              if(col_idx < 0 || (row_idx < 0) ){
                continue;
              }
              if(col_idx > col_out_dim || row_idx > row_out_dim){
                continue;
              }

              if (input[n*row_out_dim*col_out_dim*channels + row_idx*col_out_dim*channels + col_idx*channels + c] > max){
                max = input[n*row_out_dim*col_out_dim*channels + row_idx*col_out_dim*channels + col_idx*channels + c];
              }

            }
          }
          result[n*row_out_dim*col_out_dim*channels + p*col_out_dim*channels + q*channels + c] = max;

        }
      }
    }

  }
}

void relu(int32_t* data, uint32_t len){
  for(int32_t i = 0; i < len; i++){
    data[i] = (data[i] < 0) ? 0 : data[i];  
  }
}


void elementwise_add_cpu(int32_t* vec_a, int32_t* vec_b, uint32_t vlen){
  for(int i = 0; i < vlen; i++){
    vec_a[i] += vec_b[i];
  }
}


void elementwise_mul_cpu(int32_t* vec_a, int32_t* vec_b, uint32_t vlen){
  for(int i = 0; i < vlen; i++){
    vec_a[i] *= vec_b[i];
  }
}


//// Matmul kernel for quantized layers. Performs matmul, bias add and scale
//// This assumes that the bias value has already been adapted to respect scale and zero point
//void tiled_matmul_qnn(int8_t* matrix_a, int8_t* matrix_b, int8_t* result, int32_t* bias,
//                  uint32_t a_rows, uint32_t reduction_dim, uint32_t b_cols,
//                  fixed_point_t scale,
//                  bool b_is_unit_strided){
//  
//  //tiled_matmul(matrix_a, matrix_b, result, a_rows, reduction_dim, b_cols, b_is_unit_strided);
//  int8_t * mat_b_help = matrix_b;
//  int8_t * mat_a_help = matrix_a;
//
//  uint32_t stride = reduction_dim;
//  uint32_t vl = MAX_VLEN_MAC;
//  uint32_t red_dim = 0;
//  int32_t acc = 0;
//  int8_t acc_scale = 0;
//
//  for(uint32_t i = 0; i < a_rows; i++){
//    for(uint32_t j = 0; j < b_cols; j++){
//      acc = 0;
//      red_dim = 0;
//
//      // Address calculation
//      mat_a_help = matrix_a + i*reduction_dim;
//
//      if(b_is_unit_strided){
//        mat_b_help = matrix_b + reduction_dim*j;
//      } else {
//        mat_b_help = matrix_b + j;
//      }
//
//      // Reduction loop
//      do {
//        if((red_dim+vl)<reduction_dim){
//          vl=MAX_VLEN_MAC;
//        } else{
//          vl=reduction_dim-red_dim;                  
//        }
//
//        // Configure VLEN
//        asm volatile(".insn u 0b1011011, %3, 0b11000001001100000111\n\t"
//        :
//        :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl), "r"(&acc)
//        : );
//
//        // Load tile of vector_b
//        if(b_is_unit_strided){
//          //Load unit-strided row by row
//          asm volatile(    
//          ".insn i 0b1111011, 0b000,x24, %1,  0b000000000000\n\t"
//          :
//          :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
//          :  "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" ); 
//
//        } else {
//          // Strided access pattern 
//          asm volatile(    
//          ".insn r 0b1111011, 0b010, 0b0000110,  x24, %1,%5\n\t"
//          :
//          :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc),"r"(stride)
//          :  "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" ); 
//        }
//
//        // Load tile of matrix_a
//        asm volatile(
//        ".insn i 0b1111011, 0b000,x16, %0,  0b000000000000\n\t"    
//        :
//        :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
//        :   "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" );
//
//        // Perform reduction VMAC
//        asm volatile(
//        ".insn r 0b1011011, 0b011, 0x00, x16, x24, x16\n\t"         
//        :
//        :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
//        : "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );
//
//        // Accumulate partial result and store to memory
//        asm volatile(          
//        "add %2,x16,%2\n\t"
//        "sw %2,0(%4)\n\t"
//        :
//        :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
//        :"x16","memory");
//
//        red_dim=red_dim+vl;
//        mat_a_help=mat_a_help+vl;
//        mat_b_help=mat_b_help+vl;
//
//      } while(red_dim < reduction_dim);   
//
//      // Fixed point arithmetic. Input is of format S(63.0), Weight is S(IntLength.FractionLength)
//      // Output is then S(63+IntLength.FractionLength)
//      // We truncate fractional bits by shifting
//      acc = (int32_t) (( (int64_t) acc * scale.value) / scale.two_power_frac);
//      acc = acc + bias[j];
//      acc = (acc < INT8_MIN) ? INT8_MIN : acc;
//      acc = (acc > INT8_MAX) ? INT8_MAX : acc; 
//      acc_scale = (int8_t) acc;
//
//      asm volatile(
//      "sb %2,0(%4)\n\t"
//      :
//      :"r"(mat_a_help), "r"(mat_b_help), "r"(acc_scale), "r"(vl),  "r"(result+i*b_cols+j)
//      :"memory");     
//
//    }
//  } // end matmul
//}
//
//void matmul_qnn_cpu(int8_t* matrix_a, int8_t* matrix_b, int8_t* result, int32_t* bias,
//                  uint32_t a_rows, uint32_t reduction_dim, uint32_t b_cols,
//                  fixed_point_t scale,
//                  bool b_is_unit_strided){
//  int32_t acc = 0;
//
//  for(int32_t i = 0; i < a_rows; i++){
//    for(int32_t j = 0; j < b_cols; j++){
//
//      acc = 0;
//      for(int32_t k = 0; k < reduction_dim; k++){
//        if(!b_is_unit_strided){
//          acc += matrix_a[i*reduction_dim + k] * matrix_b[k*b_cols + j];
//        } else {
//          acc += matrix_a[i*reduction_dim + k] * matrix_b[j*reduction_dim + k];
//        }
//      }
//
//      // Handle bias and scale
//      acc = (int32_t) (( (int64_t) acc * scale.value) / scale.two_power_frac);
//      acc = acc + bias[j];
//      acc = (acc < INT8_MIN) ? INT8_MIN : acc;
//      acc = (acc > INT8_MAX) ? INT8_MAX : acc; 
//      result[i*b_cols + j] = (int8_t) acc;
//    }
//  }
//
//}
//
//void tiled_conv_qnn(int8_t* input, int8_t* kernel, int8_t* result, int32_t* bias,
//              uint32_t batch_size, uint32_t in_rows, uint32_t in_cols, uint32_t in_channels,
//              uint32_t out_rows, uint32_t out_cols, uint32_t out_channels,
//              uint32_t kernel_rows, uint32_t kernel_cols,
//              uint32_t padding, uint32_t stride,
//              fixed_point_t scale, 
//              quantization_parameters_t input_quant, quantization_parameters_t output_quant){
//
//  int32_t acc;
//  int8_t acc_scale;
//  uint32_t red_dim;
//  uint32_t vl = MAX_VLEN_MAC;
//
//  int8_t* input_help = input;
//  int8_t *kernel_help = kernel;
//  int32_t* result_help = result;
//
//  uint32_t input_col_idx, input_row_idx;
//  bool a_load_zero_point = false;
//  uint32_t a_stride = 0;
//
//  for(int n = 0; n < batch_size; n++){
//    for(int p = 0; p < out_rows; p++){
//      for(int q = 0; q < out_cols; q++){
//        for(int k = 0; k < out_channels; k++){
//         
//          result_help = result +
//                        (n*out_rows*out_cols*out_channels) +
//                        (p*out_cols*out_channels) +
//                        (q*out_channels) +
//                        (k);
//          acc = 0; 
//          acc_scale = 0;
//
//          for(int r = 0; r < kernel_rows; r++){
//            for(int s = 0; s < kernel_cols; s++){
//
//              input_row_idx = (p*stride) + r - padding;
//              input_col_idx = (q*stride) + s - padding;
//
//              // Are we in a padded dimension?
//              if( (input_col_idx < 0 || input_row_idx < 0) || (input_col_idx > in_cols || input_row_idx > in_rows) ){
//                if(input_zero_point != 0){
//                  // Load input zero point
//                  input_help = (int8_t*) &input_quant.zero_point;
//                  a_load_zero_point = true;
//                } else {
//                  // zero_point is zero, so we can just skip the computation
//                  a_load_zero_point = false;
//                  continue;
//                }
//              }
//
//              // Address calculation, set in_channel offset to 0 as the inner loop will perform the full reduction along the axis
//              input_help = input + 
//                          (n            *in_rows*in_cols*in_channels) + 
//                          (input_row_idx*in_cols*in_channels) +
//                          (input_col_idx*in_channels) +
//                          (0);
//              kernel_help = kernel +
//                          (r*kernel_cols*out_channels*in_channels) +
//                          (s*out_channels*in_channels) +
//                          (k*in_channels) +
//                          (0);
//              red_dim = 0;
//
//              // vectorized reduction over in_channels
//              do {
//                if((red_dim+vl)<in_channels){
//                  vl=MAX_VLEN_MAC;
//                } else{
//                  vl=in_channels-red_dim;                  
//                }
//
//                // Configure VLEN
//                asm volatile(".insn u 0b1011011, %3, 0b11000001001100000111\n\t"
//                :
//                :"r"(input_help), "r"(kernel_help), "r"(acc), "r"(vl), "r"(&acc)
//                : );
//
//                // Load unit-strided row by row
//                asm volatile(    
//                ".insn i 0b1111011, 0b000,x24, %1,  0b000000000000\n\t"
//                :
//                :"r"(input_help), "r"(kernel_help), "r"(acc), "r"(vl),  "r"(&acc)
//                :  "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" ); 
//                          
//                // Load tile of matrix_a
//                if (!a_load_zero_point) {
//                  asm volatile(
//                  ".insn i 0b1111011, 0b000,x16, %0,  0b000000000000\n\t"    
//                  :
//                  :"r"(input_help), "r"(kernel_help), "r"(acc), "r"(vl),  "r"(&acc)
//                  :   "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" );
//                } else {
//                  // Load scalar zero point by setting stride=0
//                  asm volatile(    
//                  ".insn r 0b1111011, 0b010, 0b0000110,  x24, %0, %5\n\t"
//                  :
//                  :"r"(input_help), "r"(kernel_help), "r"(acc), "r"(vl),  "r"(&acc),"r"(a_stride)
//                  :   "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" );
//                }
//
//                // Perform reduction VMAC
//                asm volatile(
//                ".insn r 0b1011011, 0b011, 0x00, x16, x24, x16\n\t"         
//                :
//                :"r"(input_help), "r"(kernel_help), "r"(acc), "r"(vl),  "r"(&acc)
//                : "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );
//
//                // Accumulate partial result and store to memory
//                asm volatile(          
//                "add %2,x16,%2\n\t"
//                "sw %2,0(%4)\n\t"
//                :
//                :"r"(input_help), "r"(kernel_help), "r"(acc), "r"(vl),  "r"(&acc)
//                :"x16","memory");
//
//                red_dim=red_dim+vl;
//                input_help=input_help+vl;
//                kernel_help=kernel_help+vl;
//
//              } while(red_dim < in_channels);   
//            }
//          } // reduction complete
//
//          // Fixed point arithmetic. Input is of format S(63.0), Weight is S(IntLength.FractionLength)
//          // Output is then S(63+IntLength.FractionLength)
//          // We truncate fractional bits by shifting
//          acc = (int32_t) (( (int64_t) acc * scale.value) / scale.two_power_frac);
//          acc = acc + bias[j];
//          acc = (acc < INT8_MIN) ? INT8_MIN : acc;
//          acc = (acc > INT8_MAX) ? INT8_MAX : acc; 
//          acc_scale = (int8_t) acc;
//
//          // store back result
//          asm volatile(
//          "sb %2,0(%3)\n\t"
//          :
//          :"r"(input_help), "r"(kernel_help), "r"(acc_scale), "r"(result_help)
//          :"memory");     
//
//        }
//      }
//    }
//  }
//
//}
//
//
//void conv_qnn_cpu(int8_t* input, int8_t* kernel, int32_t* result, int32_t* bias,
//              uint32_t batch_size, uint32_t in_rows, uint32_t in_cols, uint32_t in_channels,
//              uint32_t out_rows, uint32_t out_cols, uint32_t out_channels,
//              uint32_t kernel_rows, uint32_t kernel_cols,
//              uint32_t padding, uint32_t stride,
//              fixed_point_t scale, int32_t input_zero_point){
//
//  int32_t acc = 0;
//  int8_t input_value = 0;
//  int8_t weight_value = 0;
//
//  int32_t input_row_idx, input_col_idx;
//
//  for(int n = 0; n < batch_size; n++){
//    for(int p = 0; p < out_rows; p++){
//      for(int q = 0; q < out_cols; q++){
//        for(int k = 0; k < out_channels; k++){
//
//         acc = 0; 
//
//          for(int r = 0; r < kernel_rows; r++){
//            for(int s = 0; s < kernel_cols; s++){
//              for(int ic = 0; ic < in_channels; ic++){
//
//                input_row_idx = (p*stride) + r - padding;
//                input_col_idx = (q*stride) + s - padding;
//
//                if( (input_col_idx < 0 || input_row_idx < 0) || (input_col_idx > in_cols || input_row_idx > in_rows)){
//                  //input_value = 0;
//                  continue;
//                } else {
//                  input_value = input[n*in_rows*in_cols*in_channels + input_row_idx*in_cols*in_channels + input_col_idx*in_channels + ic];
//                }
//
//                weight_value = kernel[r*kernel_cols*out_channels*in_channels + s*out_channels*in_channels + k*in_channels + ic];
//                acc += input_value*weight_value;
//              }
//            }
//          }
//
//          acc = (int32_t) (( (int64_t) acc * scale.value) / scale.two_power_frac);
//          acc = acc + bias[j];
//          acc = (acc < INT8_MIN) ? INT8_MIN : acc;
//          acc = (acc > INT8_MAX) ? INT8_MAX : acc; 
//          result[n*out_rows*out_cols*out_channels + p*out_cols*out_channels + q*out_channels + k] = (int8_t) acc;
//        }
//      }
//    }
//  }
//              
//}