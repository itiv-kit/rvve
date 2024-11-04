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

  uint64_t User_Time, End_Time, Begin_Time;
  uint32_t a_height = 36;
  uint32_t b_width = 36;
  uint32_t a_width_b_height = 36;

  uint32_t vl = 36;
  uint32_t acc = 0;

  uint32_t* mat_res = (uint32_t *) malloc(a_height*b_width * sizeof(uint32_t));
  uint8_t* mat_a  = (uint8_t *) malloc(a_height*a_width_b_height * sizeof(uint8_t));
  uint8_t* mat_b  = (uint8_t *) malloc(b_width*a_width_b_height * sizeof(uint8_t));

  uint8_t* mat_b_help = NULL; 
  uint8_t* mat_a_help = NULL;
 
        

  // capture all exceptions and give debug info via UART
  // this is not required, but keeps us safe
  neorv32_rte_setup();

  // init UART at default baud rate, no parity bits, ho hw flow control
  neorv32_uart0_setup(BAUD_RATE, PARITY_NONE, FLOW_CONTROL_NONE);

  // check available hardware extensions and compare with compiler flags
  neorv32_rte_check_isa(0); // silent = 0 -> show message if isa mismatch

  // say hello




  //initialize Matrices
  for(uint32_t i = 0; i<a_height; i++)
  {
    for (uint32_t k = 0; k < a_width_b_height; k++)    {
      *(mat_a+i*a_width_b_height+k)= (uint8_t )k;  
    }
  }

  for(uint32_t i = 0; i<a_width_b_height; i++)
  {
    for (uint32_t k = 0; k < b_width; k++) {
      *(mat_b+i*b_width+k)= (uint8_t )i; 
    }
  }

    for(uint32_t i = 0; i<a_height; i++)
  {
    for (uint32_t k = 0; k < b_width; k++) {
      *(mat_res+i*a_height+k)= 0; 
    }
  }

  { /* *****  NEORV32-SPECIFIC ***** */
    Begin_Time = (long)neorv32_cpu_get_systime();
  } /* ***** /NEORV32-SPECIFIC ***** */

// VECTOR Matrix Multiplication mat_a*mat_b = mat_res
mat_b_help = mat_b;
mat_a_help = mat_a;

asm volatile(".insn u 0b1011011, %2, 0b11000001001100000111\n\t"
:
:"r"(mat_a_help), "r"(mat_b_help), "r"(vl)
:);
for (uint32_t k = 0; k < b_width; k++)  {

  mat_a_help = mat_a;
  mat_b_help = mat_b;

  for (uint32_t i = 0; i < a_height; i++){
    mat_b_help=mat_b+b_width*i;
                        
    asm volatile(
    ".insn i 0b1111011, 0b000,x14, %0,  0b000000000100\n\t"
    ".insn i 0b1111011, 0b000,x23, %1,  0b000000000100\n\t"
    :
    :"r"(mat_a_help), "r"(mat_b_help)
    :  "x14","x15","x16","x17","x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" );

    asm volatile(
    ".insn r 0b1011011, 0b011, 0x00, x14, x23, x14\n\t"         
    :
    :"r"(mat_a_help), "r"(mat_b_help)
    : "x14","x15","x16","x17","x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31"  );

    asm volatile(      
    "sw x14,0(%0)\n\t"
    :
    : "r"(mat_res+i+k*b_width)
    :"x14","memory");     
    }
}


  { /* *****  NEORV32-SPECIFIC ***** */
    End_Time = (long)neorv32_cpu_get_systime();
  } /* ***** /NEORV32-SPECIFIC ***** */

    User_Time = End_Time - Begin_Time;



// output of mat_res
neorv32_uart0_printf("mat_res_vector:\n");

for(int i = 0; i<a_height; i++)
{
  for (int k = 0; k < b_width; k++)
  {
    neorv32_uart0_printf("%d\t", *(mat_res+i*b_width+k));
  }
  neorv32_uart0_printf("\n");
}

 neorv32_uart0_printf ("Microseconds for run vector: %u \n", (uint32_t)((User_Time * (1000 )) / NEORV32_SYSINFO.CLK));
 neorv32_uart0_printf ("Cycles: %u \n", (uint32_t)((User_Time ) ));
 neorv32_uart0_printf("NEORV32: Cycles per second: %u\n", (uint32_t)NEORV32_SYSINFO.CLK);

   { /* *****  NEORV32-SPECIFIC ***** */
    Begin_Time = (long)neorv32_cpu_get_systime();
  } /* ***** /NEORV32-SPECIFIC ***** */

// SCALAR Matrix Multiplication mat_a*mat_b = mat_res

for (uint32_t k = 0; k < a_height; k++)  {
  
  for (uint32_t i = 0; i < b_width; i++){

    acc = 0;
      for (uint32_t m = 0; m< a_width_b_height; m++)
      {
         acc+= mat_a[k*a_width_b_height+m] * mat_b[m+i*b_width];
      }
      mat_res[k*b_width+i] = acc;
    }
}

  { /* *****  NEORV32-SPECIFIC ***** */
    End_Time = (long)neorv32_cpu_get_systime();
  } /* ***** /NEORV32-SPECIFIC ***** */

    User_Time = End_Time - Begin_Time;



// output of mat_res
neorv32_uart0_printf("mat_res_scalar:\n");

for(int i = 0; i<a_height; i++)
{
  for (int k = 0; k < b_width; k++)
  {
    neorv32_uart0_printf("%d\t", *(mat_res+i*b_width+k));
  }
  neorv32_uart0_printf("\n");
}

 neorv32_uart0_printf ("Microseconds for run scalar: %u \n", (uint32_t)((User_Time * (1000 )) / NEORV32_SYSINFO.CLK));
 neorv32_uart0_printf ("Cycles: %u \n", (uint32_t)((User_Time ) ));
 neorv32_uart0_printf("NEORV32: Cycles per second: %u\n", (uint32_t)NEORV32_SYSINFO.CLK);

return 0;
}

