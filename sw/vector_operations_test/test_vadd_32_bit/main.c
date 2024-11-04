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
uint32_t vl = 8;
 uint32_t size = 20;

 int32_t* array  = (int32_t *) malloc(size * sizeof(int32_t));
 int32_t* test_array  = (int32_t *) malloc(size * sizeof(int32_t));
 int32_t* result_array =  (int32_t*) malloc(size * sizeof(int32_t));
  for(int i=0; i<size; i++)
  {
      *(array+i)=(i);
      *(test_array+i)=(i);
      *(result_array+i)=0;
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
  //1.load two vectors
  //2.add them
  //3.store solution
  //4.print solution and check if it is correct

  // say hello
  neorv32_uart0_print("This is a Test! :)\n");	

  asm volatile(".insn u 0b1011011, %2, 0b11000000001100000111\n\t"
              ".insn i 0b1111011, 0b010,x9, %0,  0b000000000100\n\t"
              ".insn i 0b1111011, 0b010,x19, %1,  0b000000000100\n\t"
              ".insn r 0b1011011, 0b010, 0x00, x19, x19, x9\n\t"   
              ".insn s 0b0101011, 0b010, x19, 0x000(%3)\n\t"         
  :
  :"r"(array), "r"(test_array), "r"(vl), "r"(result_array)
  :"x9","x10","x11", "x12", "x13","x14", "x15", "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );

  for(int i=0; i<size; i++)
  {    
    neorv32_uart0_printf("%d\n", *(result_array+i));
    neorv32_uart0_printf("%x\n",((result_array+i)));
  }

  return 0;
}
