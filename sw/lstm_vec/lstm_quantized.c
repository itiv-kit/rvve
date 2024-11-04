#include "lstm_quantized.h"
#include <stdlib.h>




void init_lstm_state(lstm_state_t* state, int32_t inputs, int32_t n_cells)
{
     state->hidden = (int8_t*)malloc(inputs * n_cells * sizeof(int8_t));
     state->carry = (int32_t*)malloc(inputs * n_cells * sizeof(int32_t));
}


// this function assumes, that the fraction length of both scaling values is the same
void scale_for_activation(int64_t * output,int32_t *gate_result,int32_t *gate_recurrent_result,int32_t *input_weight_sum,int32_t * rec_weight_sum,int32_t * bias,quantization_parameters_t * total_scaling_input,quantization_parameters_t * total_scaling_recurrent_input,quantization_parameters_t * B_input,uint32_t length)
{
      //used for later rounding shift
      int32_t rounding_add_val = (1 << (total_scaling_input->scaling_factor.fraction_length-10));
      //temp
      int64_t temp_val1, temp_val2,temp_val3  = 0;

      inplace_point32_twise_add(gate_recurrent_result, rec_weight_sum ,length);
      inplace_point32_twise_add(gate_result, input_weight_sum ,length);

      //actual scaling
      for(int32_t  i=0; i<length; i++ )
      {    
            temp_val1 = (int64_t)gate_result[i] * (int64_t) total_scaling_input->scaling_factor.value;
            temp_val2 = (int64_t)gate_recurrent_result[i] *(int64_t)total_scaling_recurrent_input->scaling_factor.value;
            temp_val3 = (int64_t)bias[i] *(int64_t)B_input->scaling_factor.value;
            
            temp_val1 = temp_val1+temp_val2+temp_val3;

            temp_val1 = (int64_t)rounding_add_val+temp_val1;
            output[i] = temp_val1 >> (total_scaling_input->scaling_factor.fraction_length-9);// a fraction length of 9 is required for tanh/sig activation             

      }

}

// this function assumes, that the fraction length of both scaling values is the same
void update_cell_value(int32_t * input_gate_data,int32_t * forget_gate_data, int32_t * cell_gate_data, lstm_state_t * state,quantization_parameters_t *total_scaling_cell_update1,quantization_parameters_t *total_scaling_cell_update2,uint32_t length   )
{
      int32_t rounding_add_val;
      int32_t sigmoid_zero_shift=128;
      int64_t temp_val1, temp_val2,temp_val3  = 0;

      //value for later rounding shift
      rounding_add_val = (1 << (total_scaling_cell_update1->scaling_factor.fraction_length-1));

      // add zero shift before multiplying
      inplace_point32_twise_add_scalar(forget_gate_data, &sigmoid_zero_shift ,length);
      inplace_point32_twise_add_scalar(input_gate_data, &sigmoid_zero_shift ,length);

      // multiply
      inplace_point32_twise_multiplication_32(forget_gate_data,&state->carry[0],length);
      inplace_point32_twise_multiplication_32(input_gate_data,cell_gate_data,length);

      for(int32_t  i=0; i<length; i++ )
      {     

            temp_val1 = (int64_t)forget_gate_data[i] * (int64_t) total_scaling_cell_update1->scaling_factor.value;
            temp_val2 = (int64_t)input_gate_data[i] *(int64_t)total_scaling_cell_update2->scaling_factor.value;
            temp_val3 = temp_val1+temp_val2;
            temp_val1 = (int64_t)rounding_add_val+temp_val3;
            state->carry[i]= (int16_t)(temp_val1 >> total_scaling_cell_update1->scaling_factor.fraction_length);            

      }

}

//
void update_hidden_value( int32_t * temp_data,int32_t * output_gate_data,lstm_state_t * state,uint32_t length,quantization_parameters_t * total_scaling_hidden_update)
{
      int32_t cell_result_temp = 0;
      int64_t hidden_state_intermediate =0;
      int32_t sigmoid_zero_shift=128;
      int64_t test = 0;

      //prepare for acitvation function. Therefore scale to fraction_lenght of 9. state_carry is scaled (1/2^11) so shift two times right. Then apply tanh
      for(uint32_t i = 0; i<length; i++)
      {  
         cell_result_temp = state->carry[i];
         cell_result_temp = cell_result_temp+ (1<<1);
         cell_result_temp = cell_result_temp >> 2;
         test       = (int64_t) cell_result_temp ;
         tanh_activation(&test,1);     
         temp_data[i] = (int32_t) test ;
      }    
      // zero shift
      inplace_point32_twise_add_scalar(output_gate_data, &sigmoid_zero_shift ,length);
      //multiplication
      inplace_point32_twise_multiplication_32(temp_data,output_gate_data,length);
      //scale and store value
      for(uint32_t i = 0; i<length; i++)
      {  hidden_state_intermediate = (int64_t)temp_data[i] * (int64_t)total_scaling_hidden_update->scaling_factor.value;
         hidden_state_intermediate = hidden_state_intermediate + (1 <<  (total_scaling_hidden_update->scaling_factor.fraction_length-1));
         hidden_state_intermediate= (hidden_state_intermediate >> total_scaling_hidden_update->scaling_factor.fraction_length);      
         state->hidden[i] =(int8_t) (hidden_state_intermediate+ total_scaling_hidden_update->zero_point_shift);           
      }    

}
