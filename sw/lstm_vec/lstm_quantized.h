#ifndef LSTM_QUANTIZED_H
#define LSTM_QUANTIZED_H

#include <stdint.h>
#include "matrix_operations.h"
#include "activation.h"

typedef struct 
   {
      int32_t value;
      uint32_t fraction_length;

   }fixed_point_t;



typedef struct 
{
   fixed_point_t scaling_factor;
   int32_t zero_point_shift;

}quantization_parameters_t;


typedef struct _lstm_state{
    int8_t* hidden; // recurrent input
    int32_t* carry; // cell weight value
} lstm_state_t;

extern void scale_for_activation(int64_t * output,int32_t *gate_result,int32_t *gate_recurrent_result,int32_t *input_weight_sum,int32_t * rec_weight_sum,int32_t * bias,quantization_parameters_t * total_scaling_input,quantization_parameters_t * total_scaling_recurrent_input,quantization_parameters_t * B_input,uint32_t length);
extern void update_cell_value(int32_t * input_gate_data,int32_t * forget_gate_data, int32_t * cell_gate_data, lstm_state_t * state,quantization_parameters_t *total_scaling_cell_update1,quantization_parameters_t *total_scaling_cell_update2,uint32_t length   );
extern void update_hidden_value( int32_t * temp_data,int32_t * output_gate_data,lstm_state_t * state,uint32_t length,quantization_parameters_t * total_scaling_hidden_update);
extern void init_lstm_state(lstm_state_t* state, int32_t inputs, int32_t n_cells);


#endif

