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
  int32_t i =0;
  int32_t input_val = 0;
  int32_t output_val = 0;

  // capture all exceptions and give debug info via UART
  // this is not required, but keeps us safe
  neorv32_rte_setup();

  // init UART at default baud rate, no parity bits, ho hw flow control
  neorv32_uart0_setup(BAUD_RATE, PARITY_NONE, FLOW_CONTROL_NONE);

  // check available hardware extensions and compare with compiler flags
  neorv32_rte_check_isa(0); // silent = 0 -> show message if isa mismatch

  // say hello
  neorv32_uart0_print("Hello world! :)\n");

  neorv32_uart0_print("output values \t input_values\n");
 
  // input value is 32-bit signed fixed point with fraction length of 9
  // output is 8-bit signed fixed point number with fraction length of 7  scaling factor of (1/128)

  //test
  //input values into function
  //print and check output

  for(i=-2000; i<2000; i++)
  {
    input_val=i;

    asm volatile(
            ".insn r 0b0001011, 0b001, 0b0000000,  %0, %1,%1\n\t"
            "sw   %0, 0(%2)\n\t"
    :
    :"r"(output_val), "r"(input_val), "r"(&output_val)
    : );

    neorv32_uart0_printf("%d \t %d\n",  (output_val), (input_val));

  }
  return 0;
}
