#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <neorv32.h>


int8_t tanh_lut[64] ={-128,	-128	,-128	,-128,	-128,	-128,	-128,	-127,	-127	,-127	,-127,	-127	,-126,	-126,
	-125,	-124,	-123,	-122,	-120,	-118	,-115	,-111	,-107	,-101	,-95,	-87,	-77,	-66	,-53	,-39	,-24	,-8,	8,
  	24,	39,	53,	66,	77,	87,	95,	101,	107	,111	,115,	118,	120,	122,	123,	124,	125,	126	,126,	127	,127	,127,	127	,127	,127	,127	,127	,127,	127	,127,	127};


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
 uint64_t User_Time, End_Time, Begin_Time;

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


    input_val=100;
 { /* *****  NEORV32-SPECIFIC ***** */
    Begin_Time = (long)neorv32_cpu_get_systime();
  } /* ***** /NEORV32-SPECIFIC ***** */
    asm volatile(
            ".insn r 0b0001011, 0b001, 0b0000000,  %0, %1,%1\n\t"
            "sw   %0, 0(%2)\n\t"
    :
    :"r"(output_val), "r"(input_val), "r"(&output_val)
    : );
  { /* *****  NEORV32-SPECIFIC ***** */
    End_Time = (long)neorv32_cpu_get_systime();
  } /* ***** /NEORV32-SPECIFIC ***** */

    User_Time = End_Time - Begin_Time;
    neorv32_uart0_printf("%d \t %d\n",  (output_val), (input_val));
    neorv32_uart0_printf ("Microseconds for run vector: %u \n", (uint32_t)((User_Time * (1000 )) / NEORV32_SYSINFO.CLK));
    neorv32_uart0_printf ("Cycles: %u \n", (uint32_t)((User_Time ) ));
    neorv32_uart0_printf("NEORV32: Cycles per second: %u\n", (uint32_t)NEORV32_SYSINFO.CLK);


 { /* *****  NEORV32-SPECIFIC ***** */
    Begin_Time = (long)neorv32_cpu_get_systime();
  } /* ***** /NEORV32-SPECIFIC ***** */
    int32_t index_val = 0;
    index_val = input_val << 3;
    index_val = index_val + 0b00000000000000000011111100000000; //+31,5  F32.9
    index_val = index_val +  (1 <<8);

    index_val = index_val >> 9;
    if(index_val >63)
    {
      output_val=127;
    }
    else if(index_val < 0)
    {
      output_val= -128;
    }
    else{
    output_val = (int64_t)tanh_lut[index_val];
    }
  { /* *****  NEORV32-SPECIFIC ***** */
    End_Time = (long)neorv32_cpu_get_systime();
  } /* ***** /NEORV32-SPECIFIC ***** */

    User_Time = End_Time - Begin_Time;

    neorv32_uart0_printf("%d \t %d\n",  (output_val), (input_val));
    neorv32_uart0_printf ("Microseconds for run vector: %u \n", (uint32_t)((User_Time * (1000 )) / NEORV32_SYSINFO.CLK));
    neorv32_uart0_printf ("Cycles: %u \n", (uint32_t)((User_Time ) ));
    neorv32_uart0_printf("NEORV32: Cycles per second: %u\n", (uint32_t)NEORV32_SYSINFO.CLK);



  
  return 0;
}
