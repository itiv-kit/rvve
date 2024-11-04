#ifndef _LSTM_H
#define _LSTM_H
#include <stdint.h>

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

extern void init_lstm_state(lstm_state_t* state, int32_t inputs, int32_t n_cells);

extern void split_lstm_input_result(int8_t *data, int8_t *i, int8_t *f, int8_t *c,
    int8_t *o, int32_t n_cells, int32_t input_dim);

// works only on batch size = 1
// Output shape = time_steps x n_cells
extern void compute_lstm_layer(int8_t* input_data, int8_t* result, lstm_state_t* state, int32_t in_length, int32_t out_length,
    int8_t* weights, int8_t* recurrent_weights, int8_t* biases, int16_t multiplier, int32_t x_zero_shift, int32_t w_zero_shift);

// Reversed hidden and cell states, required for bidirectional lstms 
extern void reversed_lstm(int8_t* input_data, int8_t* result, lstm_state_t* state,
    int32_t time_steps, int32_t in_length, int32_t n_cells,
    int8_t* weights, int8_t* recurrent_weights, int8_t* biases);

#endif
