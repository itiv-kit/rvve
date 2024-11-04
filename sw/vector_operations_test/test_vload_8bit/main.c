#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <neorv32.h>


/**********************************************************************//**
 * @name User configuration
 **************************************************************************/
/**@{*/
/** UART BAUD rate */
#define BAUD_RATE 19200
/**@}*/
int main(void)
{


 uint32_t i =0;

 uint32_t vl = 9;
 uint32_t vl_store;
 uint32_t size = 20;
 uint8_t* array1  = (uint8_t *) malloc(size * sizeof(uint8_t));
 uint8_t* array2  = (uint8_t *) malloc(size * sizeof(uint8_t));

 uint8_t* array2_test;


if(vl%4>0)
{
vl_store=(vl/4)+1;
}
else
{
vl_store=vl/4;
}

for(i=0; i<size; i++)
{
    *(array1+i)=(i);
    *(array2+i)=(i);
}

array1 = array1+3;
array2_test = array2;
array2 = array2;


  // capture all exceptions and give debug info via UART
  // this is not required, but keeps us safe
  neorv32_rte_setup();

  // init UART at default baud rate, no parity bits, ho hw flow control
  neorv32_uart0_setup(BAUD_RATE, PARITY_NONE, FLOW_CONTROL_NONE);

  // check available hardware extensions and compare with compiler flags
  neorv32_rte_check_isa(0); // silent = 0 -> show message if isa mismatch

  // say hello
  neorv32_uart0_print("Hello world! :)\n");	


// test:
//load vector into registers
//store  vector to different location
//display results via printf

asm volatile(".insn u 0b1011011, %2, 0b00000001000000000111\n\t"
             ".insn i 0b1111011, 0b000,x9, %0,  0b000000000100\n\t"
             ".insn i 0b1111011, 0b000,x19, %0,  0b000000000100\n\t"
             ".insn u 0b1011011, %3, 0b00000001000000000111\n\t"
	           ".insn s 0b0101011, 0b010, x19, 0x000(%1)\n\t"
:
:"r"(array1), "r"(array2), "r"(vl),"r"(vl_store)
:"x9","x10","x11", "x12", "x13","x14", "x15", "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );




for(i=0; i<size; i++)
{    
  neorv32_uart0_printf("%d\n", *(array2_test+i));
  neorv32_uart0_printf("%x\n",((array2_test+i)));
}


return 0;
}
