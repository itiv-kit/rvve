#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <neorv32.h>
#include "model.h"
#include "matrix_operations.h"

#include "lstm_quantized.h"

#define BAUD_RATE 19200




int main()
{
   // capture all exceptions and give debug info via UART
   // this is not required, but keeps us safe
   neorv32_rte_setup();

   // init UART at default baud rate, no parity bits, ho hw flow control
   neorv32_uart0_setup(BAUD_RATE, PARITY_NONE, FLOW_CONTROL_NONE);

   // check available hardware extensions and compare with compiler flags
   neorv32_rte_check_isa(0); // silent = 0 -> show message if isa mismatch 

   quantization_parameters_t  X, B_input, B_output, B_cell, B_forget, Hidden_State, total_scaling_fc;
   quantization_parameters_t total_scaling_hidden_update, total_scaling_cell_update2,total_scaling_cell_update1,total_scaling_input, total_scaling_forget,total_scaling_cell,total_scaling_output, total_scaling_recurrent_input, total_scaling_recurrent_forget, total_scaling_recurrent_cell, total_scaling_recurrent_output;

   //quantization parameters, imported from tflite and converted to fixed point

   //input values
   X.scaling_factor.value = 0b11001101101111100010000000000000;
   X.scaling_factor.fraction_length = 27;  
   X.zero_point_shift = 6; 

   //bias
   B_input.scaling_factor.value = 0b00000000000010010111000101010010;
   B_input.scaling_factor.fraction_length = 31;  
   B_input.zero_point_shift = 0;

   B_forget.scaling_factor.value = 0b00000000000010010000111111111001;
   B_forget.scaling_factor.fraction_length = 31;  
   B_forget.zero_point_shift = 0;

   B_cell.scaling_factor.value = 0b00000000000001111110110011010111;
   B_cell.scaling_factor.fraction_length = 31;  
   B_cell.zero_point_shift = 0;

   B_output.scaling_factor.value = 0b00000000000010010000110110011010;
   B_output.scaling_factor.fraction_length = 31;  
   B_output.zero_point_shift = 0;

   //hidden
   Hidden_State.scaling_factor.value = 0b00000000111111111110100010010111;
   Hidden_State.scaling_factor.fraction_length = 31;  
   Hidden_State.zero_point_shift = 0;

   
   //computed scaling values in fixed point representation (needed for LSTM-Computation)
   // choose fraction length carefully so that no overflow is caused during the multiplication

   total_scaling_input.scaling_factor.value = 0b00000000000010010111000101010010; // X.scaling_factor.value*W_input.scaling_factor.value 
   total_scaling_input.scaling_factor.fraction_length = 31;                                         
   total_scaling_input.zero_point_shift = 0;

   total_scaling_forget.scaling_factor.value = 0b00000000000010010000111111111000; //X.scaling_factor.value*W_forget.scaling_factor.value
   total_scaling_forget.scaling_factor.fraction_length = 31;
   total_scaling_forget.zero_point_shift = 0;

   total_scaling_cell.scaling_factor.value = 0b00000000000001111110110011011000; //X.scaling_factor.value*W_cell.scaling_factor.value
   total_scaling_cell.scaling_factor.fraction_length = 31;
   total_scaling_cell.zero_point_shift = 0;

   total_scaling_output.scaling_factor.value = 0b00000000000010010000110110011011; //X.scaling_factor.value* W_output.scaling_factor.value
   total_scaling_output.scaling_factor.fraction_length = 31;
   total_scaling_output.zero_point_shift = 0;

   total_scaling_recurrent_input.scaling_factor.value = 0b00000000000000011000111110001111; //hidden.scaling_factor.value*W_rec_input.scaling_factor.value
   total_scaling_recurrent_input.scaling_factor.fraction_length = 31;
   total_scaling_recurrent_input.zero_point_shift = 0;

   total_scaling_recurrent_forget.scaling_factor.value = 0b00000000000000010011100111001001;//hidden.scaling_factor.value*W_rec_forget.scaling_factor.value 
   total_scaling_recurrent_forget.scaling_factor.fraction_length = 31;
   total_scaling_recurrent_forget.zero_point_shift = 0;

   total_scaling_recurrent_cell.scaling_factor.value = 0b00000000000000010001111111001110;//hidden.scaling_factor.value*W_rec_cell.scaling_factor.value 
   total_scaling_recurrent_cell.scaling_factor.fraction_length = 31;
   total_scaling_recurrent_cell.zero_point_shift = 0;

   total_scaling_recurrent_output.scaling_factor.value = 0b00000000000000010011101011111110;//hidden.scaling_factor.value*W_rec_output.scaling_factor.value 
   total_scaling_recurrent_output.scaling_factor.fraction_length = 31;
   total_scaling_recurrent_output.zero_point_shift = 0;

   total_scaling_cell_update1.scaling_factor.value = 0b00000000100000000000000000000000; //(LOGISTIC.scaling_factor.value*Cell_State.scaling_factor.value)/(Cell_State.scaling_factor.value) 
   total_scaling_cell_update1.scaling_factor.fraction_length = 31; 
   total_scaling_cell_update1.zero_point_shift  = 0;
                              
   total_scaling_cell_update2.scaling_factor.value = 0b00001000000000000000000000000000;// (TANH.scaling_factor.value*LOGISTIC.scaling_factor.value)/(Cell_State.scaling_factor.value)
   total_scaling_cell_update2.scaling_factor.fraction_length = 31;
   total_scaling_cell_update2.zero_point_shift = 0;

   total_scaling_hidden_update.scaling_factor.value = 0b00000000100001001010010101010110;// (cell_state.scaling factor * tanh.scaling_Factor)/hidden_state_scaling factor
   total_scaling_hidden_update.scaling_factor.fraction_length =  31;
   total_scaling_hidden_update.zero_point_shift = 0;
   
   //scaling factors of fc layer
   total_scaling_fc.scaling_factor.value = 0b00000000001110000000000001011110;
   total_scaling_fc.scaling_factor.fraction_length = 31;
   total_scaling_fc.zero_point_shift = 10;


   int32_t *input_result,*recurrent_result,*test, * input_weight_sum, * rec_input_weight_sum, *forget_weight_sum, *rec_forget_weight_sum,*cell_weight_sum, *rec_cell_weight_sum, *hidden_weight_sum,*rec_hidden_weight_sum;
   int64_t * Y, temp_output;
   lstm_state_t* state;
   int8_t *input_data;
   int32_t fc_weight_sum, output_result_fc, time_steps, in_length, out_length, fc_bias ;

   //bias of fully connected layer
   fc_bias =  1239;
   
   //lstm cell paramters
   in_length = 7;
   out_length = 32;
   time_steps = 120;


   //some memory allocation
   state = (lstm_state_t *)malloc(sizeof(lstm_state_t));

   input_result = (int32_t *)malloc(out_length * 4*sizeof(int32_t));
   recurrent_result = (int32_t *)malloc(out_length * 4* sizeof(int32_t));
   input_weight_sum = (int32_t *)malloc(out_length* sizeof(int32_t));
   rec_input_weight_sum = (int32_t *)malloc(out_length* sizeof(int32_t));
   forget_weight_sum = (int32_t *)malloc(out_length* sizeof(int32_t));
   rec_forget_weight_sum = (int32_t *)malloc(out_length* sizeof(int32_t));
   cell_weight_sum = (int32_t *)malloc(out_length* sizeof(int32_t));
   rec_cell_weight_sum = (int32_t *)malloc(out_length* sizeof(int32_t));
   hidden_weight_sum = (int32_t *)malloc(out_length* sizeof(int32_t));
   rec_hidden_weight_sum = (int32_t *)malloc(out_length* sizeof(int32_t));
   Y=(int64_t *)malloc(4*out_length* sizeof(int64_t));
   test=(int32_t *)malloc(4*out_length* sizeof(int32_t));
   init_lstm_state(state, out_length, 1);


 //cell initialization
 for(int32_t i = 0; i < out_length; i++)
    {
       state->carry[i]=0;
       state->hidden[i]=Hidden_State.zero_point_shift;
    } 

   //precomputations, values are needed later. For Information :https://leimao.github.io/article/Neural-Networks-Quantization/
    weight_redudction_row(&input_weights[0][0],input_weight_sum, out_length , in_length);
    weight_redudction_row(&rec_input_weights[0][0],rec_input_weight_sum ,out_length, out_length  );

    weight_redudction_row(&forget_weights[0][0],forget_weight_sum , out_length,in_length );
    weight_redudction_row(&rec_forget_weights[0][0],rec_forget_weight_sum ,out_length ,out_length  );

    weight_redudction_row(&cell_weights[0][0],cell_weight_sum , out_length,in_length  );
    weight_redudction_row(&rec_cell_weights[0][0],rec_cell_weight_sum ,out_length ,out_length  );

    weight_redudction_row(&output_weights[0][0],hidden_weight_sum , out_length ,in_length );
    weight_redudction_row(&rec_output_weights[0][0],rec_hidden_weight_sum ,out_length ,out_length );
    weight_redudction_column(&fc_weights[0],&fc_weight_sum ,out_length ,1 );




   //precomputations, values are needed later
    for(int32_t i = 0; i < (out_length); i++)
    {
      input_weight_sum[i]= input_weight_sum[i]*(-X.zero_point_shift);
      rec_input_weight_sum[i] = rec_input_weight_sum[i]*(-Hidden_State.zero_point_shift);

      forget_weight_sum[i]= forget_weight_sum[i]*(-X.zero_point_shift);
      rec_forget_weight_sum[i] = rec_forget_weight_sum[i]*(-Hidden_State.zero_point_shift);

      cell_weight_sum[i]= cell_weight_sum[i]*(-X.zero_point_shift);
      rec_cell_weight_sum[i] = rec_cell_weight_sum[i]*(-Hidden_State.zero_point_shift);

      hidden_weight_sum[i]= hidden_weight_sum[i]*(-X.zero_point_shift);
      rec_hidden_weight_sum[i] = rec_hidden_weight_sum[i]*(-Hidden_State.zero_point_shift);
    } 

   uint64_t User_Time, End_Time, Begin_Time;



          { /* *****  NEORV32-SPECIFIC ***** */
    Begin_Time = (long)neorv32_cpu_get_systime();
  } /* ***** /NEORV32-SPECIFIC ***** */

   for(int32_t k=0; k<time_steps; k++)
   {


      input_data= &input[k][0];

      //multiply input values and hidden state with weights
      mat_vec_mul_32_8( &input_weights[0][0] , input_data,  input_result,  out_length,in_length );
      mat_vec_mul_32_8(&forget_weights[0][0] ,input_data,    input_result+out_length,  out_length,in_length );
      mat_vec_mul_32_8( &cell_weights[0][0] , input_data,  input_result+out_length*2,  out_length,in_length );
      mat_vec_mul_32_8( &output_weights[0][0] ,input_data,   input_result+out_length*3,  out_length,in_length );

      mat_vec_mul_32_32(&rec_input_weights[0][0] , &state->hidden[0]  ,   recurrent_result, out_length ,out_length );
      mat_vec_mul_32_32( &rec_forget_weights[0][0]  ,&state->hidden[0],   recurrent_result+out_length,out_length  ,out_length );
      mat_vec_mul_32_32(&rec_cell_weights[0][0]  , &state->hidden[0],   recurrent_result+out_length*2, out_length,out_length );
      mat_vec_mul_32_32(&rec_output_weights[0][0]  ,&state->hidden[0],    recurrent_result+out_length*3, out_length,out_length );

      //scale results with quantization factors 
      scale_for_activation(Y,input_result,recurrent_result,input_weight_sum,rec_input_weight_sum,input_gate_bias,&total_scaling_input,&total_scaling_recurrent_input,&B_input,out_length);      
      scale_for_activation(Y+out_length,input_result+out_length,recurrent_result+out_length,forget_weight_sum,rec_forget_weight_sum,forget_gate_bias,&total_scaling_forget,&total_scaling_recurrent_forget,&B_forget,out_length);

      scale_for_activation(Y+2*out_length,input_result+2*out_length,recurrent_result+2*out_length,cell_weight_sum,rec_cell_weight_sum,cell_gate_bias,&total_scaling_cell,&total_scaling_recurrent_cell,&B_cell,out_length);

      scale_for_activation(Y+3*out_length,input_result+3*out_length,recurrent_result+3*out_length,hidden_weight_sum,rec_hidden_weight_sum,output_gate_bias,&total_scaling_output,&total_scaling_recurrent_output,&B_output,out_length);
            
 
      // apply activation functions
      logistic(&Y[0], out_length*2);
      logistic(&Y[3*out_length], out_length);
      tanh_activation(&Y[2*out_length], out_length);     

      //from 64-bit to 32-bit
      for(int32_t i =0; i<4*out_length;i++)
      {
            test[i] = Y[i];
      }  

     //update cell (hidden value and cell state)
     update_cell_value(&test[0], &test[out_length], &test[2*out_length],state,&total_scaling_cell_update1,&total_scaling_cell_update2,out_length);
     update_hidden_value( &test[out_length],&test[3*out_length], state,out_length,&total_scaling_hidden_update);


   }

   //fully conected layer
   mat_vec_mul( &fc_weights[0] ,&state->hidden[0],   &output_result_fc, 1,  out_length) ;

   output_result_fc= output_result_fc - total_scaling_hidden_update.zero_point_shift*fc_weight_sum;
   output_result_fc = output_result_fc + fc_bias;

   //scale for output
   temp_output=(int64_t)output_result_fc* (int64_t)total_scaling_fc.scaling_factor.value;
   //rounding shift
   temp_output = temp_output + (1 << (total_scaling_fc.scaling_factor.fraction_length-1));
   output_result_fc = (int32_t)(temp_output >> (total_scaling_fc.scaling_factor.fraction_length));
   //zero point shift
   output_result_fc = output_result_fc+total_scaling_fc.zero_point_shift;

   /* *****  NEORV32-SPECIFIC ***** */
    End_Time = (long)neorv32_cpu_get_systime();
   /* ***** /NEORV32-SPECIFIC ***** */              
    User_Time = End_Time - Begin_Time; 

   neorv32_uart0_print("final_res:");
   neorv32_uart0_printf("%d \n ",output_result_fc);
   neorv32_uart0_printf ("Microseconds for run: %u \n", (uint32_t)((User_Time * (1000 )) / NEORV32_SYSINFO.CLK));
   neorv32_uart0_printf ("Cycles: %u \n", (uint32_t)((User_Time ) ));
   neorv32_uart0_printf("NEORV32: Cycles per second: %u\n", (uint32_t)NEORV32_SYSINFO.CLK);

   return 0;
}
