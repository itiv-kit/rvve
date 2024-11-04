#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <neorv32.h>
#include <assert.h>
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

    uint32_t a_height = DIM_I;
    uint32_t a_width_b_height = DIM_K;

    uint32_t vl = DIM_K;

    // ALlocate data structures. None of these malloc operations is allowed to fail
    int32_t* mat_res = (int32_t *) malloc(DIM_I * sizeof(int32_t));
    assert(mat_res);
    int8_t* mat_a  = (int8_t *) malloc(DIM_I*DIM_K * sizeof(int8_t));
    assert(mat_a);
    int8_t* vec_b  = (int8_t *) malloc(DIM_K * sizeof(int8_t));
    assert(vec_b);

    // capture all exceptions and give debug info via UART
    // this is not required, but keeps us safe
    neorv32_rte_setup();

    // init UART at default baud rate, no parity bits, ho hw flow control
    neorv32_uart0_setup(BAUD_RATE, PARITY_NONE, FLOW_CONTROL_NONE);

    // check available hardware extensions and compare with compiler flags
    neorv32_rte_check_isa(0); // silent = 0 -> show message if isa mismatch


    //initialize Matrices
    for(uint32_t i = 0; i < a_height; i++) {
        for (uint32_t k = 0; k < a_width_b_height; k++){
            //*(mat_a+i*a_width_b_height+k)= (int8_t)k;  
            mat_a[i*a_width_b_height+k] = (int8_t)i;
        }
    }
    for(uint32_t k = 0; k < DIM_K; k++){
        vec_b[k] = k;
    }


    { /* *****  NEORV32-SPECIFIC ***** */
        Begin_Time = (long)neorv32_cpu_get_systime();
    } /* ***** /NEORV32-SPECIFIC ***** */

    // VECTOR Matrix Multiplication mat_a*mat_b = mat_res
    tiled_matvec_mul(mat_a, vec_b, mat_res, DIM_I, DIM_K);

    { /* *****  NEORV32-SPECIFIC ***** */
        End_Time = (long)neorv32_cpu_get_systime();
    } /* ***** /NEORV32-SPECIFIC ***** */

    User_Time = End_Time - Begin_Time;

    // output of mat_res
    neorv32_uart0_printf("mat_res_vector:\n");

    for(int i = 0; i<a_height; i++) {
        neorv32_uart0_printf("%d\t", mat_res[i] );
    }

    neorv32_uart0_printf ("Cycles vector: %u \n", (uint32_t)((User_Time ) ));

    { /* *****  NEORV32-SPECIFIC ***** */
        Begin_Time = (long)neorv32_cpu_get_systime();
    } /* ***** /NEORV32-SPECIFIC ***** */

    // SCALAR Matrix Multiplication mat_a*mat_b = mat_res
    matvec_mul_cpu(mat_a, vec_b, mat_res, DIM_I, DIM_K);

    { /* *****  NEORV32-SPECIFIC ***** */
        End_Time = (long)neorv32_cpu_get_systime();
    } /* ***** /NEORV32-SPECIFIC ***** */

    User_Time = End_Time - Begin_Time;

    // output of mat_res
    neorv32_uart0_printf("mat_res_scalar:\n");
    for(int i = 0; i<a_height; i++) {
        neorv32_uart0_printf("%d\t",  mat_res[i]);
    }
    neorv32_uart0_printf("\n");

    neorv32_uart0_printf ("Cycles scalar:: %u \n", (uint32_t)((User_Time ) ));

    return 0;
}


