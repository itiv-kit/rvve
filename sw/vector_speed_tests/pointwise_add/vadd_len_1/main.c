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
 uint32_t vl = 1;
 uint32_t size = 12;

 uint64_t User_Time, End_Time, Begin_Time;

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

  //Start and measure cycles

   { /* *****  NEORV32-SPECIFIC ***** */
    Begin_Time = (long)neorv32_cpu_get_systime();
  } /* ***** /NEORV32-SPECIFIC ***** */
  asm volatile(".insn u 0b1011011, %2, 0b11000000001100000111\n\t"
              ".insn i 0b1111011, 0b010,x30, %0,  0b000000000100\n\t"
              ".insn i 0b1111011, 0b010,x31, %1,  0b000000000100\n\t"
              ".insn r 0b1011011, 0b010, 0x00, x31, x30, x31\n\t"   
              ".insn s 0b0101011, 0b010, x31, 0x000(%3)\n\t"         
  :
  :"r"(array), "r"(test_array), "r"(vl), "r"(result_array)
  : "x30", "x31","memory" );

  { /* *****  NEORV32-SPECIFIC ***** */
    End_Time = (long)neorv32_cpu_get_systime();
  } /* ***** /NEORV32-SPECIFIC ***** */

    User_Time = End_Time - Begin_Time;
  
  for(int i=0; i<size; i++)
  {    
    neorv32_uart0_printf("%d\n", *(result_array+i));
    neorv32_uart0_printf("%x\n",((result_array+i)));
  }


  neorv32_uart0_printf ("Microseconds for run vector: %u \n", (uint32_t)((User_Time * (1000 )) / NEORV32_SYSINFO.CLK));
  neorv32_uart0_printf ("Cycles: %u \n", (uint32_t)((User_Time ) ));
  neorv32_uart0_printf("NEORV32: Cycles per second: %u\n", (uint32_t)NEORV32_SYSINFO.CLK);



  //Start and measure cycles


   { /* *****  NEORV32-SPECIFIC ***** */
    Begin_Time = (long)neorv32_cpu_get_systime();
  } /* ***** /NEORV32-SPECIFIC ***** */

  for (int32_t i = 0; i < vl; i++)
      array[i] += test_array[i];

  { /* *****  NEORV32-SPECIFIC ***** */
    End_Time = (long)neorv32_cpu_get_systime();
  } /* ***** /NEORV32-SPECIFIC ***** */

    User_Time = End_Time - Begin_Time;



  for(int i=0; i<size; i++)
  {    
    neorv32_uart0_printf("%d\n", *(array+i));
    neorv32_uart0_printf("%x\n",((array+i)));
  }

 neorv32_uart0_printf ("Microseconds for run scalar: %u \n", (uint32_t)((User_Time * (1000 )) / NEORV32_SYSINFO.CLK));
 neorv32_uart0_printf ("Cycles: %u \n", (uint32_t)((User_Time ) ));
 neorv32_uart0_printf("NEORV32: Cycles per second: %u\n", (uint32_t)NEORV32_SYSINFO.CLK);

  return 0;
}
