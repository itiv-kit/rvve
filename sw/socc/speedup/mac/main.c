#include <complex.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <neorv32.h>
#include <sys/types.h>

#include "../../../vector_c_runtime/include/kernels.h"

#define DIM_I 32
#define ITERATIONS 10000000
#define MAX_VL MAX_VLEN_MAC

typedef int dtype ;

/**********************************************************************//**
 * @name User configuration
 **************************************************************************/
/**@{*/
/** UART BAUD rate */
#define BAUD_RATE 19200
/**@}*/
int main(void) {

    uint64_t User_Time, End_Time, Begin_Time;

    dtype* mat_a = (dtype*) malloc(DIM_I * sizeof(dtype));
    dtype* mat_b = (dtype*) malloc(DIM_I * sizeof(dtype));
                                                             
    // capture all exceptions and give debug info via UART
    // this is not required, but keeps us safe
    neorv32_rte_setup();

    // init UART at default baud rate, no parity bits, ho hw flow control
    neorv32_uart0_setup(BAUD_RATE, PARITY_NONE, FLOW_CONTROL_NONE);

    // check available hardware extensions and compare with compiler flags
    neorv32_rte_check_isa(0); // silent = 0 -> show message if isa mismatch


    // Helpers
    uint32_t vl = MAX_VL;
    uint32_t col_dim = 0;
    int32_t acc = 0;
    dtype * mat_b_help = mat_b;
    dtype * mat_a_help = mat_a;

    neorv32_uart0_printf ("mat_b_help: %p \n", mat_b_help );
    neorv32_uart0_printf ("mat_a_help: %p \n", mat_a_help );

    Begin_Time = (long)neorv32_cpu_get_systime();
    do {
      if((col_dim+vl)<ITERATIONS){
        vl=MAX_VL;
      } else{
        vl=ITERATIONS-col_dim;                  
      }
      // Configure VLEN
      asm volatile(".insn u 0b1011011, %3, 0b11000001001100000111\n\t"
      :
      :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl), "r"(&acc)
      : );

        // Load tile of matrix_a
        asm volatile(
        ".insn i 0b1111011, 0b000,x16, %0,  0b000000000000\n\t"    
        :
        :"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
        :   "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" );

      // Perform reduction VMAC
      //asm volatile(
      //".insn r 0b1011011, 0b011, 0x00, x16, x24, x16\n\t"         
      //:
      //:"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      //: "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );

      // Execute multiplication
      //asm volatile(
      //".insn r 0b1011011, 0b100, 0x00, x16, x24, x16\n\t"         
      //:
      //:"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      //: "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31", "memory" );

      // Perform inplace VADD
      //asm volatile(
      //".insn r 0b1011011, 0b010, 0x00, x16, x24, x16\n\t"         
      //:
      //:"r"(mat_a_help), "r"(mat_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      //: "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );


      col_dim=col_dim+vl;
      //mat_a_help=mat_a_help+vl;
      //mat_b_help=mat_b_help+vl;

    } while(col_dim < ITERATIONS);   

    End_Time = (long)neorv32_cpu_get_systime();
    User_Time = (End_Time - Begin_Time);
    

    neorv32_uart0_printf ("Cycles vector: %u \n", (uint32_t)((User_Time ) ));
    User_Time = 0;

    asm volatile( 
      "li x8, 1\n\t"
      "li x9, 1\n\t"
      "li x10, 2\n\t"
      "sw x11, %0"
      :
      : "m"(mat_a_help)
      :"x9", "x10");

    // SCALAR data load of bytes
    uint32_t tmp = 0;
    Begin_Time = (long)neorv32_cpu_get_systime();
      //"lb x9, 0(x11)\n\t"
    for(int i = 0; i < ITERATIONS; i+=1){
      asm volatile(          
      "lb x9, 0(x11)\n\t"
      "lb x10, 0(x11)\n\t"
      :
      :"r"(mat_a_help)
      :"x8", "x9", "x10");

    }
    End_Time = (long)neorv32_cpu_get_systime();
    User_Time = (End_Time - Begin_Time);

    neorv32_uart0_printf ("Cycles scalar: %u \n", (uint32_t)((User_Time ) ));

    return 0;
}


