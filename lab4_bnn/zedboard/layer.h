//===========================================================================
// layer.h
//===========================================================================
// @brief: This header file defines the interface for the core functions.

#ifndef LAYER_H
#define LAYER_H

#include "model.h"
#include "typedefs.h"
 // optimized deisgn with line buffer   achieve 118x in hls report
#include <cassert>
typedef ap_uint<32> HLS_SIZE_T;
#include "hls/hls_video_mem.h"

//----------------------------------------------------------
// Padding
//----------------------------------------------------------
// @param[in] : input - input fmaps
//              M - number of input fmaps
//              I - width of input fmaps
// @param[out] : output - output fmaps
template <int M, int I>
void pad(bit input[M][I][I], bit output[M][I + F_PAD][I + F_PAD]) {
#pragma HLS array_reshape variable=input complete dim=1   
    for (int x = 0; x < I; x++) {
      for (int y = 0; y < I; y++) {
               #pragma HLS pipeline  
          for (int m = 0; m < M; m++) {
                #pragma HLS unroll  
        output[m][y + F_PAD / 2][x + F_PAD / 2] = input[m][y][x];
      }
    }
  }
}

//----------------------------------------------------------
// Initialize Padded Memory with Constant
//----------------------------------------------------------
// @param[in] : input - input fmaps to be initialized
// @param[out] : output - output fmaps
template <int M, int I, int C>
void initialize_padded_memory(bit input[M][I][I]) {
#pragma HLS array_reshape variable=input complete dim=1   
  for (int y = 0; y < I; y++) { 
    for (int x = 0; x < I; x++) {
         #pragma HLS pipeline
          for (int m = 0; m < M; m++) { 
          #pragma HLS unroll
        input[m][x][y] = C;
      }
    }
  }
}

// Perform Convolution Layer
//----------------------------------------------------------
// @param[in] : input - input fmaps
//              threshold - threshold for batchnorm operation
//              M - number of input fmaps
//              N - number of output fmaps
//              I - width of input fmaps
//              weight - layer weights
// @param[out] : output - output fmaps
template <int M, int N, int I>
void conv(bit input[M][I][I], bit output[N][I - F + 1][I - F + 1],
          const bit8_t threshold[N], const bit weight[M][N][F][F]) {
         #pragma HLS array_reshape variable=input complete dim=1   
          #pragma HLS array_reshape variable=weight complete dim=1

  hls::LineBuffer <I*2+F,M,bit> buffer;
  hls::LineBuffer <I*2+F,M,bit> buffer2;

      for(int j=0;j<M;j++){
           #pragma HLS pipeline
        for(int z1=0; z1<I;z1++){
      buffer.shift_pixels_up(j);
      buffer.insert_bottom_row(input[j][0][z1],j);
      }
        for(int z2=0;z2<I;z2++){
      buffer.shift_pixels_up(j);
      buffer.insert_bottom_row(input[j][1][z2],j);
      }
        for(int z3=0;z3<F;z3++){
      buffer.shift_pixels_up(j);
      buffer.insert_bottom_row(input[j][2][z3],j);
      }
    }

  int num_accum = F * F * M;
   for (int n = 0; n < N; n++) {
        buffer2=buffer;
     for (int x = 0; x < I - F + 1; x++) {
      for (int y = 0; y < I - F + 1; y++) {
        bit16_t accum = 0;
        for (int c = 0; c < F; c++) {
          for (int r = 0; r < F; r++) {
            #pragma HLS pipeline
              for (int m = 0; m < M; m++) {   // unroll this reshape m
                  #pragma HLS unroll
              accum += buffer2.getval(c+r*I,m) == weight[m][n][r][c]; //input[m][y + r][x + c]
            }
          }
        }
        accum = (accum << 1) - num_accum;
        output[n][x][y] = accum > threshold[n] ? 1 : 0;

        if(y<I-F){
          #pragma HLS pipeline  
        for(int mi=0;mi<M;mi++){
        buffer2.shift_pixels_up(mi);
        buffer2.insert_bottom_row(input[mi][x+2][y+3],mi);
        }
        }
      }
        //row change update
        if(x<I-F){
           #pragma HLS pipeline      
        for(int mii=0;mii<M;mii++){   
       for(int i=0;i<F;i++){
       buffer2.shift_pixels_up(mii);
       buffer2.insert_bottom_row(input[mii][x+3][i],mii);
      }    
      }
     }
    }
  }
}

//----------------------------------------------------------
// Max pooling
//----------------------------------------------------------
// @param[in] : input - input fmaps
//              M - number of input fmaps
//              I - width of input fmaps
// @param[out] : output - output fmaps
template <int M, int I>
void max_pool(bit input[M][I][I], bit output[M][I / 2][I / 2]) {
       #pragma HLS array_reshape variable=input complete dim=1              
  for (int m = 0; m < M; m++) {
    for (int x = 0; x < I / 2; x++) {
      for (int y = 0; y < I / 2; y++) {
        bit max = 0;
        for (int c = 0; c < 2; c++) {
          #pragma HLS pipeline  //II=1
          for (int r = 0; r < 2; r++) {
            #pragma HLS unroll
            if (input[m][2 * y + r][2 * x + c])
              max = 1;
          }
        }
        output[m][y][x] = max;
      }
    }
  }
}

//----------------------------------------------------------
// Flatten the Output from Conv Layer
//----------------------------------------------------------
// @param[in] : input - output fmaps from the last conv layer
// @param[out] : output - input famps of the first dense layer

void flatten(bit input[O_CHANNEL2][O_WIDTH][O_WIDTH], bit output[I_UNITS1]) {
#pragma HLS array_reshape variable=input complete dim=1
 for (int x = 0; x < O_WIDTH; x++) {
    for (int y = 0; y < O_WIDTH; y++) {
           #pragma HLS pipeline
       for (int c = 0; c < O_CHANNEL2; c++) {
             #pragma HLS unroll
        int o_index = c + (x + y * O_WIDTH) * O_CHANNEL2;
        output[o_index] = input[c][y][x];
      }
    }
  }
}

//----------------------------------------------------------
// Perform Sign Layer
//----------------------------------------------------------
// @param[in] : input - input fmaps
//              M - number of input and output channels
// @param[out] : output - output fmaps

template <int M> void sign(bit16_t input[M], bit output[M]) {
  for (int m = 0; m < M; m++) {
    output[m] = (input[m] > 0) ? 1 : 0;
  }
}

//----------------------------------------------------------
// Perform Argmax Layer
//----------------------------------------------------------
// @param[in] : input - input channels
// @param[out] : output - argmax of the inputs

bit4_t argmax(bit16_t input[NUM_DIGITS]) {
  bit16_t max = input[0];
  bit4_t max_id = 0;
  for (int i = 1; i < NUM_DIGITS; i++) {
    if (input[i] > max) {
      max = input[i];
      max_id = i;
    }
  }
  return max_id;
}

//----------------------------------------------------------
// Perform Dense Layer
//----------------------------------------------------------
// @param[in] : input - input fmaps
//              M - number of input fmaps
//              N - number of output fmaps
//              weight - layer weights
// @param[out] : output - output fmaps

template <int M, int N>
void dense(bit input[M], bit16_t output[N], const bit weight[M][N]) {
              #pragma HLS array_reshape variable=input complete dim=1
            #pragma HLS array_reshape variable=weight complete dim=1
  for (int n = 0; n < N; n++) {
    bit16_t accum = 0; 
     #pragma HLS pipeline //II=1
    for (int m = 0; m < M; m++) {
      #pragma HLS unroll
      int w_index = m * N + n;
      accum += input[m] == weight[m][n]; // XNOR
    }
    output[n] = (accum << 1) - M;
  }
}

#endif

// optimized deisgn without line buffer (below)  achieve 87x in hls report
/*  
//===========================================================================
// layer.h
//===========================================================================
// @brief: This header file defines the interface for the core functions.

#ifndef LAYER_H
#define LAYER_H

#include "model.h"
#include "typedefs.h"
#include <algorithm>

//----------------------------------------------------------
// Padding
//----------------------------------------------------------
// @param[in] : input - input fmaps
//              M - number of input fmaps
//              I - width of input fmaps
// @param[out] : output - output fmaps
template <int M, int I>
void pad(bit input[M][I][I], bit output[M][I + F_PAD][I + F_PAD]) {

  for (int m = 0; m < M; m++) {
    for (int x = 0; x < I; x++) {
      for (int y = 0; y < I; y++) {
        output[m][y + F_PAD / 2][x + F_PAD / 2] = input[m][y][x];
      }
    }
  }
}

//----------------------------------------------------------
// Initialize Padded Memory with Constant
//----------------------------------------------------------
// @param[in] : input - input fmaps to be initialized
// @param[out] : output - output fmaps
template <int M, int I, int C>
void initialize_padded_memory(bit input[M][I][I]) {
  for (int m = 0; m < M; m++) {
    for (int x = 0; x < I; x++) {
      for (int y = 0; y < I; y++) {
        input[m][x][y] = C;
      }
    }
  }
}

//----------------------------------------------------------
// Perform Convolution Layer
//----------------------------------------------------------
// @param[in] : input - input fmaps
//              threshold - threshold for batchnorm operation
//              M - number of input fmaps
//              N - number of output fmaps
//              I - width of input fmaps
//              weight - layer weights
// @param[out] : output - output fmaps
template <int M, int N, int I>
void conv(bit input[M][I][I], bit output[N][I - F + 1][I - F + 1],
          const bit8_t threshold[N], const bit weight[M][N][F][F]) {
            #pragma HLS array_reshape variable=input complete dim=1
            #pragma HLS array_reshape variable=input complete dim=2
            #pragma HLS array_reshape variable=weight complete dim=1
            #pragma HLS array_reshape variable=weight complete dim=3
            //#pragma HLS array_reshape variable=threshold complete dim=1

  int num_accum = F * F * M;
   for (int n = 0; n < N; n++) {
     for (int x = 0; x < I - F + 1; x++) {
      #pragma HLS pipeline
      for (int y = 0; y < I - F + 1; y++) {
        bit16_t accum = 0;
        for (int c = 0; c < F; c++) {
          for (int r = 0; r < F; r++) {
            for (int m = 0; m < M; m++) {   // unroll this reshape m
            //#pragma HLS unroll
              accum += input[m][y + r][x + c] == weight[m][n][r][c];
            }
          }
        }
        accum = (accum << 1) - num_accum;
        output[n][y][x] = accum > threshold[n] ? 1 : 0;
      }
    }
  }
}


//----------------------------------------------------------
// Max pooling
//----------------------------------------------------------
// @param[in] : input - input fmaps
//              M - number of input fmaps
//              I - width of input fmaps
// @param[out] : output - output fmaps
template <int M, int I>
void max_pool(bit input[M][I][I], bit output[M][I / 2][I / 2]) {
  #pragma HLS array_reshape variable=input complete dim=1
  for (int m = 0; m < M; m++) {
    #pragma HLS pipeline
    for (int x = 0; x < I / 2; x++) {
      for (int y = 0; y < I / 2; y++) {
        bit max = 0;
        for (int c = 0; c < 2; c++) {
          for (int r = 0; r < 2; r++) {
            if (input[m][2 * y + r][2 * x + c])
              max = 1;
          }
        }
        output[m][y][x] = max;
      }
    }
  }
}

//----------------------------------------------------------
// Flatten the Output from Conv Layer
//----------------------------------------------------------
// @param[in] : input - output fmaps from the last conv layer
// @param[out] : output - input famps of the first dense layer

void flatten(bit input[O_CHANNEL2][O_WIDTH][O_WIDTH], bit output[I_UNITS1]) {
  for (int c = 0; c < O_CHANNEL2; c++) {
    for (int y = 0; y < O_WIDTH; y++) {
      for (int x = 0; x < O_WIDTH; x++) {
        int o_index = c + (x + y * O_WIDTH) * O_CHANNEL2;
        output[o_index] = input[c][y][x];
      }
    }
  }
}

//----------------------------------------------------------
// Perform Sign Layer
//----------------------------------------------------------
// @param[in] : input - input fmaps
//              M - number of input and output channels
// @param[out] : output - output fmaps

template <int M> void sign(bit16_t input[M], bit output[M]) {
  for (int m = 0; m < M; m++) {
    output[m] = (input[m] > 0) ? 1 : 0;
  }
}

//----------------------------------------------------------
// Perform Argmax Layer
//----------------------------------------------------------
// @param[in] : input - input channels
// @param[out] : output - argmax of the inputs

bit4_t argmax(bit16_t input[NUM_DIGITS]) {
  bit16_t max = input[0];
  bit4_t max_id = 0;
  for (int i = 1; i < NUM_DIGITS; i++) {
    if (input[i] > max) {
      max = input[i];
      max_id = i;
    }
  }
  return max_id;
}

//----------------------------------------------------------
// Perform Dense Layer
//----------------------------------------------------------
// @param[in] : input - input fmaps
//              M - number of input fmaps
//              N - number of output fmaps
//              weight - layer weights
// @param[out] : output - output fmaps

template <int M, int N>
void dense(bit input[M], bit16_t output[N], const bit weight[M][N]) {
  #pragma HLS array_reshape variable=input complete dim=1
  #pragma HLS array_reshape variable=weight complete dim=1
  for (int n = 0; n < N; n++) {
    bit16_t accum = 0;
    #pragma HLS pipeline
    for (int m = 0; m < M; m++) {
    //#pragma HLS unroll
      int w_index = m * N + n;
      accum += input[m] == weight[m][n]; // XNOR
    }
    output[n] = (accum << 1) - M;
  }
}

#endif
*/

