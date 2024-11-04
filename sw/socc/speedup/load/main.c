#include <complex.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <neorv32.h>
#include <sys/types.h>

#include "../../../vector_c_runtime/include/kernels.h"

#define DIM_I 2048

/**********************************************************************//**
 * @name User configuration
 **************************************************************************/
/**@{*/
/** UART BAUD rate */
#define BAUD_RATE 19200
/**@}*/
int main(void) {

    uint64_t User_Time = 0;
    uint64_t End_Time, Begin_Time;

    volatile int8_t* data = (int8_t*) malloc(DIM_I * sizeof(int8_t));

    // capture all exceptions and give debug info via UART
    // this is not required, but keeps us safe
    neorv32_rte_setup();

    // init UART at default baud rate, no parity bits, ho hw flow control
    neorv32_uart0_setup(BAUD_RATE, PARITY_NONE, FLOW_CONTROL_NONE);

    // check available hardware extensions and compare with compiler flags
    neorv32_rte_check_isa(0); // silent = 0 -> show message if isa mismatch



    { /* *****  NEORV32-SPECIFIC ***** */
        Begin_Time = (long)neorv32_cpu_get_systime();
    } /* ***** /NEORV32-SPECIFIC ***** */

    // VECTOR load
    uint32_t j = 0;
    uint32_t vl = MAX_VLEN_MAC;
    volatile int8_t* input = data;
    volatile int8_t* weights = data;

  uint32_t macs_performed = 0;
  uint32_t stride = DIM_I; //TODO
  uint32_t acc = 0;

  do {
    if((macs_performed+vl)<DIM_I){
      vl=MAX_VLEN_MAC;
    } else{
      vl=DIM_I-macs_performed;                  
    }

    Begin_Time = (long)neorv32_cpu_get_systime();
    // Configure VLEN
    asm volatile(".insn u 0b1011011, %3, 0b11000001001100000111\n\t"
    :
    :"r"(input), "r"(weights), "r"(acc), "r"(vl), "r"(&acc)
    : );

      // Load unit-strided row by row
      asm volatile(    
      ".insn i 0b1111011, 0b000,x24, %1,  0b000000000000\n\t"
      :
      :"r"(input), "r"(weights), "r"(acc), "r"(vl),  "r"(&acc)
      :  "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" ); 
    End_Time = (long)neorv32_cpu_get_systime();
    User_Time += (End_Time - Begin_Time);

              
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

  } while(macs_performed < DIM_I);   
    
    neorv32_uart0_printf ("Cycles vector: %u \n", (uint32_t)((User_Time ) ));
    User_Time = 0;


    // SCALAR data load of bytes
    int8_t tmp;
    Begin_Time = (long)neorv32_cpu_get_systime();
    for(int i = 0; i < DIM_I; i+=1){
        tmp = data[i];
    }
    End_Time = (long)neorv32_cpu_get_systime();
    User_Time += (End_Time - Begin_Time);

    neorv32_uart0_printf ("Cycles scalar: %u \n", (uint32_t)((User_Time ) ));

    return 0;
}


