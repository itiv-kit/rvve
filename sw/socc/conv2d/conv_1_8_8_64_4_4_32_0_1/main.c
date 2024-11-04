#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <neorv32.h>
#include <sys/types.h>

#include "../../../vector_c_runtime/include/kernels.h"

#include "sizes.h"


/**********************************************************************//**
 * @name User configuration
 **************************************************************************/
/**@{*/
/** UART BAUD rate */
#define BAUD_RATE 19200
/**@}*/
int main(void) {

    uint64_t User_Time, End_Time, Begin_Time;

    neorv32_uart0_printf("START:\n");
    int32_t* mat_res = (int32_t *) malloc(BATCHES*OUT_ROWS*OUT_COLS*OUT_CHANNELS * sizeof(int32_t));
    int8_t* mat_a  = (int8_t *) malloc(BATCHES*IN_ROWS*IN_COLS*IN_CHANNELS * sizeof(int8_t));
    int8_t* mat_b  = (int8_t *) malloc(KERNEL_ROWS*KERNEL_COLS*OUT_CHANNELS*IN_CHANNELS * sizeof(int8_t));
    neorv32_uart0_printf("ALLOC\n");


    // capture all exceptions and give debug info via UART
    // this is not required, but keeps us safe
    neorv32_rte_setup();

    // init UART at default baud rate, no parity bits, ho hw flow control
    neorv32_uart0_setup(BAUD_RATE, PARITY_NONE, FLOW_CONTROL_NONE);

    // check available hardware extensions and compare with compiler flags
    neorv32_rte_check_isa(0); // silent = 0 -> show message if isa mismatch


    //initialize Matrices
    for(uint32_t n = 0; n < BATCHES; n++){
        for(uint32_t h = 0; h < IN_ROWS; h++){
            for(uint32_t w = 0; w < IN_COLS; w++){
                for(uint32_t c = 0; c < IN_CHANNELS; c++){
                    mat_a[n*IN_ROWS*IN_COLS*IN_CHANNELS + h*IN_COLS*IN_CHANNELS + w*IN_CHANNELS + c] = (int8_t) h;
                }
            }
        }
    }
    for(uint32_t kh = 0; kh < KERNEL_ROWS; kh++){
        for(uint32_t kw = 0; kw < KERNEL_COLS; kw++){
            for(uint32_t oc = 0; oc < OUT_CHANNELS; oc++){
                for(uint32_t ic = 0; ic < IN_CHANNELS; ic++){
                    mat_b[kh*KERNEL_COLS*OUT_CHANNELS*IN_CHANNELS + kw*OUT_CHANNELS*IN_CHANNELS + oc*IN_CHANNELS + ic] = (int8_t) kw;
                }
            }
        }
    }
    neorv32_uart0_printf("Init done\n");


    { /* *****  NEORV32-SPECIFIC ***** */
        Begin_Time = (long)neorv32_cpu_get_systime();
    } /* ***** /NEORV32-SPECIFIC ***** */

    // VECTOR Matrix Multiplication mat_a*mat_b = mat_res
    tiled_conv(mat_a, mat_b, mat_res,
                 BATCHES, IN_ROWS, IN_COLS, IN_CHANNELS, 
                 OUT_ROWS, OUT_COLS, OUT_CHANNELS, 
                 KERNEL_ROWS, KERNEL_COLS, 
                 PADDING, STRIDE, 0);

    { /* *****  NEORV32-SPECIFIC ***** */
        End_Time = (long)neorv32_cpu_get_systime();
    } /* ***** /NEORV32-SPECIFIC ***** */

    User_Time = End_Time - Begin_Time;

    // output of mat_res
    neorv32_uart0_printf("mat_res_vector:\n");
    for(uint32_t n = 0; n < BATCHES; n++){
        for(uint32_t h = 0; h < OUT_ROWS; h++){
            for(uint32_t w = 0; w < OUT_COLS; w++){
                for(uint32_t c = 0; c < OUT_CHANNELS; c++){
                    neorv32_uart0_printf("%d\t", mat_res[n*OUT_ROWS*OUT_COLS*OUT_CHANNELS + h*OUT_COLS*OUT_CHANNELS + w*OUT_CHANNELS + c] );
                }
                neorv32_uart0_printf("\n");
            }
        }
    }


    neorv32_uart0_printf ("Cycles vector: %u \n", (uint32_t)((User_Time ) ));

    { /* *****  NEORV32-SPECIFIC ***** */
        Begin_Time = (long)neorv32_cpu_get_systime();
    } /* ***** /NEORV32-SPECIFIC ***** */

    // SCALAR Matrix Multiplication mat_a*mat_b = mat_res
    conv_cpu(mat_a, mat_b, mat_res, 
            BATCHES, IN_ROWS, IN_COLS, IN_CHANNELS, 
            OUT_ROWS, OUT_COLS, OUT_CHANNELS, 
            KERNEL_ROWS, KERNEL_COLS, 
            PADDING, STRIDE);

    { /* *****  NEORV32-SPECIFIC ***** */
        End_Time = (long)neorv32_cpu_get_systime();
    } /* ***** /NEORV32-SPECIFIC ***** */

    User_Time = End_Time - Begin_Time;

    // output of mat_res
    neorv32_uart0_printf("mat_res scalar:\n");
    for(uint32_t n = 0; n < BATCHES; n++){
        for(uint32_t h = 0; h < OUT_ROWS; h++){
            for(uint32_t w = 0; w < OUT_COLS; w++){
                for(uint32_t c = 0; c < OUT_CHANNELS; c++){
                    neorv32_uart0_printf("%d\t", mat_res[n*OUT_ROWS*OUT_COLS*OUT_CHANNELS + h*OUT_COLS*OUT_CHANNELS + w*OUT_CHANNELS + c] );
                }
                neorv32_uart0_printf("\n");
            }
        }
    }

    neorv32_uart0_printf ("Cycles scalar:: %u \n", (uint32_t)((User_Time ) ));

    return 0;
}


