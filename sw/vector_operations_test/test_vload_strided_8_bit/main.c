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

 uint32_t stride =41;
 uint32_t vl_load = 9;
 uint32_t vl_store = 0;
 uint32_t size = 41;
 uint8_t* array  = (uint8_t *) malloc(size * 12* sizeof(uint8_t));
 uint8_t* test_array  = (uint8_t *) malloc(size *12*  sizeof(uint8_t));


if(vl_load%4== 0)
vl_store=vl_load/4;
else 
vl_store=(vl_load/4)+1;

for(int i = 0; i<12; i++)
{
  for (int k = 0; k < size; k++)
          {
            *(array+i*size+k)= (uint8_t )(k+i);
            *(test_array+i*size+k)= 0; 

          }
}




  // capture all exceptions and give debug info via UART
  // this is not required, but keeps us safe
  neorv32_rte_setup();

  // init UART at default baud rate, no parity bits, ho hw flow control
  neorv32_uart0_setup(BAUD_RATE, PARITY_NONE, FLOW_CONTROL_NONE);

  // check available hardware extensions and compare with compiler flags
  neorv32_rte_check_isa(0); // silent = 0 -> show message if isa mismatch

  // say hello   	         
  neorv32_uart0_print("Hello world! :)\n");	

//test:
//1. load some Elements into Register File
//2. Store to different location with 32-bit Storage Command
//3. Print storage
//4. Check if storage ist correct

asm volatile(".insn u 0b1011011, %2, 0b11000001001100000111\n\t"
             ".insn r 0b1111011, 0b000, 0b0000110,  x21, %0,%3\n\t"
             ".insn u 0b1011011, %4, 0b11000001001100000111\n\t"
             ".insn s 0b0101011, 0b010, x21, 0x000(%1)\n\t"         

:
:"r"(array+2), "r"(test_array+2), "r"(vl_load), "r"(stride),"r"(vl_store)
:"x9","x10","x11", "x12", "x13","x14", "x15", "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );


neorv32_uart0_print("This is a test! :)\n");	


for(int i = 0; i<12; i++)
{
      neorv32_uart0_printf("%d\n", *(test_array+i));
      neorv32_uart0_printf("%x\n",(test_array+i));
}

return 0;
}
