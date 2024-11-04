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
    // NOTE:
    //  This kernel assumes that the weights matrix is transposed
    //  I.e. A(i, k) @ B(j, k) = C(i, j)

    uint64_t User_Time, End_Time, Begin_Time;

    uint32_t a_height = DIM_I;
    uint32_t a_width_b_height = DIM_K;

    uint32_t vl = DIM_K;

    int32_t* mat_res = (int32_t *) malloc(DIM_I * DIM_J * sizeof(int32_t));
    int8_t* mat_a = (int8_t *) malloc(DIM_I * DIM_K * sizeof(int8_t));
    int8_t* mat_b = (int8_t *) malloc(DIM_J * DIM_K * sizeof(int8_t));
    neorv32_uart0_printf("%p\n", mat_res);
    neorv32_uart0_printf("%p\n", mat_a);
    neorv32_uart0_printf("%p\n", mat_b);

    // capture all exceptions and give debug info via UART
    // this is not required, but keeps us safe
    neorv32_rte_setup();

    // init UART at default baud rate, no parity bits, ho hw flow control
    neorv32_uart0_setup(BAUD_RATE, PARITY_NONE, FLOW_CONTROL_NONE);

    // check available hardware extensions and compare with compiler flags
    neorv32_rte_check_isa(0); // silent = 0 -> show message if isa mismatch

    //initialize Matrices
    for(uint32_t i = 0; i < DIM_I; i++) {
        for (uint32_t k = 0; k < DIM_K; k++){
            mat_a[i*a_width_b_height+k] = (int8_t)i;
        }
    }
    for(uint32_t j = 0; j < DIM_J; j++){
        for(uint32_t k = 0; k < DIM_K; k++){
            mat_b[j*DIM_K+k] = (int8_t)k;
        }
    }

    neorv32_uart0_printf("INIT DONE\n");

    { /* *****  NEORV32-SPECIFIC ***** */
        Begin_Time = (long)neorv32_cpu_get_systime();
    } /* ***** /NEORV32-SPECIFIC ***** */

    // VECTOR Matrix Multiplication mat_a*mat_b = mat_res
    tiled_matmul(mat_a, mat_b, mat_res, DIM_I, DIM_K, DIM_J, true);

    { /* *****  NEORV32-SPECIFIC ***** */
        End_Time = (long)neorv32_cpu_get_systime();
    } /* ***** /NEORV32-SPECIFIC ***** */

    User_Time = End_Time - Begin_Time;

    // output of mat_res
    neorv32_uart0_printf("mat_res_vector:\n");

    for(int i = 0; i<a_height; i++) {
        for(int j = 0; j < DIM_J; j++){
            neorv32_uart0_printf("%d\t", mat_res[i*DIM_J+j] );
        }
        neorv32_uart0_printf("\n");
    }

    neorv32_uart0_printf ("Cycles vector: %u \n", (uint32_t)((User_Time ) ));

    { /* *****  NEORV32-SPECIFIC ***** */
        Begin_Time = (long)neorv32_cpu_get_systime();
    } /* ***** /NEORV32-SPECIFIC ***** */

    // SCALAR Matrix Multiplication mat_a*mat_b = mat_res
    matmul_cpu(mat_a, mat_b, mat_res, DIM_I, DIM_K, DIM_J, true);

    { /* *****  NEORV32-SPECIFIC ***** */
        End_Time = (long)neorv32_cpu_get_systime();
    } /* ***** /NEORV32-SPECIFIC ***** */

    User_Time = End_Time - Begin_Time;

    // output of mat_res
    for(int i = 0; i<a_height; i++) {
        for(int j = 0; j < DIM_J; j++){
            neorv32_uart0_printf("%d\t", mat_res[i*DIM_J+j] );
        }
        neorv32_uart0_printf("\n");
    }

    neorv32_uart0_printf ("Cycles scalar: %u \n", (uint32_t)((User_Time ) ));

    return 0;
}


