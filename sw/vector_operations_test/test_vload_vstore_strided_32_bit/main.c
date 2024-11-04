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

 uint32_t stride =40;
 uint32_t vl = 4;
 uint32_t size = 10;

 uint32_t* array  = (uint32_t *) malloc(size * 12* sizeof(uint32_t));
 uint32_t* test_array  = (uint32_t *) malloc(size *12*  sizeof(uint32_t));


for(int i = 0; i<12; i++)
{
  for (int k = 0; k < size; k++)
    {
      *(array+i*size+k)= (uint32_t )(k+i);
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

  //test
  //load vector with stride
  //store vector width stride on different location
  //print and check result

  neorv32_uart0_print("This is a test! :)\n");	

  asm volatile(".insn u 0b1011011, %2, 0b11000001001100000111\n\t"
              ".insn r 0b1111011, 0b010, 0b0000110,  x21, %0,%3\n\t"
              ".insn r 0b0101011, 0b010, 0b0000110,  %3,%1, x21\n\t" 

  :
  :"r"(array+1), "r"(test_array+3), "r"(vl), "r"(stride)
  :"x9","x10","x11", "x12", "x13","x14", "x15", "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );

  //print
  for(int i = 0; i<12; i++)
  {
    for (int k = 0; k < size; k++)
            {
              //read 3rd row of Matrix
              if(k == 3  )
              {
              neorv32_uart0_printf("%d\n", *(test_array+i*size+k));
              neorv32_uart0_printf("%x\n",(test_array+i*size+k));
              }
            }
  }

  return 0;
}
