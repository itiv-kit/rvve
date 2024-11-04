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

int32_t result_mac= 0;
int32_t result_normal= 0;

uint32_t vl = 8;
uint32_t size = 45;

int8_t* array  = (int8_t *) malloc(size * sizeof(int8_t));
int8_t* test_array  = (int8_t *) malloc(size * sizeof(int8_t));




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
  //1.load two vectors into registers
  //2.use mac-operation on the vectors
  //3.store result in memory
  //4. print result and compare

  // say hello
  neorv32_uart0_print("This is a test! :)\n");
array=array+3;
test_array=test_array+3;

for(uint32_t k = 0; k<45; k++)
{
  vl = k;
  result_normal=0;
    for(i=0; i<45; i++)
  {
    *(array+i)=(i);
    *(test_array+i)=(i);

    if(i<vl){
      //compute mac result without mac-instruction
      result_normal += ((int32_t)*(array+i)) * ((int32_t) (*(test_array+i)));
    }
  }


    
    asm volatile(".insn u 0b1011011, %2, 0b11000001001100000111\n\t"
                ".insn i 0b1111011, 0b000, x9, %0, 0x004\n\t"
                ".insn i 0b1111011, 0b000, x21, %1, 0x004\n\t"
                ".insn r 0b1011011, 0b011, 0x00, x21, x21, x9\n\t"            
                "sw x21, 0(%3)\n\t"
    :
    :"r"(array), "r"(test_array), "r"(vl), "r"(&result_mac)
    :"x9","x10","x11", "x12", "x13","x14", "x15", "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );


    //print result with and without mac operation
    neorv32_uart0_printf("result vector mac:\n");
    neorv32_uart0_printf("%d\n", result_mac);
    neorv32_uart0_printf("%x\n",((&result_mac)));
    neorv32_uart0_printf("result normal mac:\n");
    neorv32_uart0_printf("%d\n", result_normal);
    neorv32_uart0_printf("%x\n",((&result_normal)));
    neorv32_uart0_printf("vl:\n");
    neorv32_uart0_printf("%d\n", vl);
    neorv32_uart0_printf("\n");

}
  return 0;
  }
