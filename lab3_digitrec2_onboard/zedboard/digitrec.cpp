//==========================================================================
// digitrec.cpp
//==========================================================================
// @brief: A k-nearest-neighbor implementation for digit recognition (k=1)

#include "digitrec.h"
//----------------------------------------------------------
// Top function
//----------------------------------------------------------

void dut(hls::stream<bit32_t> &strm_in, hls::stream<bit32_t> &strm_out) {
  // -----------------------------
  bit4_t result_digit;
  digit input_test;
  // ------------------------------------------------------
  // Input processing
  // ------------------------------------------------------
  // Read the two input 32-bit words (low word first)
  bit32_t input_lo = strm_in.read();
  bit32_t input_hi = strm_in.read();

  input_test(31, 0) = input_lo;
  input_test(input_test.length()-1, 32) = input_hi;

  // ------------------------------------------------------
  // Call Digitrec
  // ------------------------------------------------------
  result_digit=digitrec(input_test);
  // ------------------------------------------------------
  // Output processing
  // ------------------------------------------------------
  // Write out the result_digit
  strm_out.write( result_digit(3, 0) );
}

//----------------------------------------------------------
// Digitrec
//----------------------------------------------------------
// @param[in] : input - the testing instance
// @return : the recognized digit (0~9)

bit4_t digitrec(digit input) {
#include "training_data.h"

  // This array stores K minimum distances per training set
  bit6_t knn_set[10][K_CONST];

  // Initialize the knn set
  for (int i = 0; i < 10; ++i)
    for (int k = 0; k < K_CONST; ++k)
      // Note that the max distance is 49
      knn_set[i][k] = 50;

LOOP_I_1800:
  for (int i = 0; i < TRAINING_SIZE; ++i) {
  LOOP_J_10:
    for (int j = 0; j < 10; j++) {
      // Read a new instance from the training set
      digit training_instance = training_data[j][i];
      // Update the KNN set
      update_knn(input, training_instance, knn_set[j]);
    }
  }

  // Compute the final output
  return knn_vote(knn_set);
}

//-----------------------------------------------------------------------
// update_knn function
//-----------------------------------------------------------------------
// Given the test instance and a (new) training instance, this
// function maintains/updates an array of K minimum
// distances per training set.

// @param[in] : test_inst - the testing instance
// @param[in] : train_inst - the training instance
// @param[in/out] : min_distances[K_CONST] - the array that stores the current
//                  K_CONST minimum distance values per training set

void update_knn(digit test_inst, digit train_inst,
                bit6_t min_distances[K_CONST]) {
  // Compute the bitwise difference between two digits using XOR
  digit diff = test_inst ^ train_inst;
  bit6_t dist = 0;
  // Calculate the hamming distance by counting the number of 1s
  for (int i = 0; i < 49; ++i) {
    dist += diff[i];
  }

// ----------------------------- 
  //max_value records the largest distance in the array min_distance[K_CONST] 
  bit6_t max_value=min_distances[0];
  //max_index records the index of the largest distance value in the array min_distance[K_CONST] 
  bit4_t max_index=0;
  //Find the current largest distance value and record its position index.
  for(int j=0;j<K_CONST;j++){
      if(min_distances[j]>max_value){
        max_value=min_distances[j];
        max_index=j;
      }
  }
  //If new calculated distance is less than the current largest one, then replace it.
  if(dist<min_distances[max_index])
  {
    min_distances[max_index]=dist;
  }
  // -----------------------------
}

//-----------------------------------------------------------------------
// knn_vote function
//-----------------------------------------------------------------------
// Given 10xK minimum distance values, this function
// finds the actual K nearest neighbors and determines the
// final output based on the most common digit represented by
// these nearest neighbors (i.e., a vote among KNNs).
//
// @param[in] : knn_set - 10xK_CONST min distance values
// @return : the recognized digit
//

bit4_t knn_vote(bit6_t knn_set[10][K_CONST]) {
  // -----------------------------
//max_value records the largest distance in the array min[K_CONST]
  bit6_t max_value;
  //max_index records the index of the largest distance value in the array min[K_CONST]
  bit4_t max_index=0;
  //min[K_CONST] stores the K nearest neighbors
  bit6_t min[K_CONST];
  //num[K_CONST] stores the the identification number of the K nearest neighbors
  bit4_t num[K_CONST];
  //count[10] stores the number of occurrences of each recognition result
  bit4_t count[10];
  //maxcount records the majority of the K neighbors with the shortest distance  
  bit4_t maxcount=0;
  //The final output recognized digit 
  bit4_t maxfrequencynum=0;

  //initialize array
  for(int z=0;z<10;z++)
  {
    count[z]=0;
  }
  for(int z=0;z<K_CONST;z++){
    num[z]=0;
    min[z]=49;
  }

  //Finds the actual K nearest neighbors
  for(int i=0;i<10;i++){
    for(int j=0;j<K_CONST;j++){
        max_value=min[0];
        max_index=0;
        for(int z=0;z<K_CONST;z++){
            if(min[z]>max_value){
            max_value=min[z];
            max_index=z;
          }
        }
        if(knn_set[i][j]<min[max_index])
      {
        min[max_index]=knn_set[i][j];
        num[max_index]=i;
      }
    }
  }
  // determines the final output based on the most common digit represented by these nearest neighbors 
  for(int z=0;z<K_CONST;z++){
    count[num[z]]++;
  }
  for(int z=0;z<10;z++){
    if(count[z]>=maxcount){
      maxcount=count[z];
      maxfrequencynum=z;
    }
  }
return maxfrequencynum;
  // -----------------------------
}
