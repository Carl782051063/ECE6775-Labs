//==========================================================================
// digitrec.cpp
//==========================================================================
// @brief: A k-nearest-neighbors implementation for digit recognition

#include "digitrec.h"

//----------------------------------------------------------
// Top function
//----------------------------------------------------------
// @param[in] : input - the testing instance
// @return : the recognized digit (0~9)

bit4 digitrec(digit input) {
#include "training_data.h"
  
  // This array stores K minimum distances per training set
  //#pragma HLS array_partition variable=knn_set 
  bit6 knn_set[10][K_CONST];
  // Initialize the knn set
  for (int i = 0; i < 10; ++i)
    for (int k = 0; k < K_CONST; ++k)
      // Note that the max distance is 49
      knn_set[i][k] = 50;

  for (int i = 0; i < TRAINING_SIZE; ++i) {
    //pragma HLS unroll
    LOOP_dig_1:
    for (int j = 0; j < 10; j++) {
      // Read a new instance from the training set
      digit training_instance = training_data[j * TRAINING_SIZE + i];
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
                bit6 min_distances[K_CONST]) {
  // Compute the bitwise difference between two digits using XOR
  digit diff = test_inst ^ train_inst;
  bit6 dist = 0;
  // Calculate the hamming distance by counting the number of 1s
  LOOP_cal:
  //pragma HLS unroll
  for (int i = 0; i < 49; ++i) {
    dist += diff[i];
  }
  // ----------------------------- 
  //max_value records the largest distance in the array min_distance[K_CONST] 
  bit6 max_value=min_distances[0];
  //max_index records the index of the largest distance value in the array min_distance[K_CONST] 
  bit4 max_index=0;
  //Find the current largest distance value and record its position index.
  LOOP_update_1:
  //pragma HLS unroll
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
// @return : the recognized digit (0-9)
//
bit4 knn_vote(bit6 knn_set[10][K_CONST]) {
  // -----------------------------
  //max_value records the largest distance in the array min[K_CONST]
  bit6 max_value;
  //max_index records the index of the largest distance value in the array min[K_CONST]
  bit4 max_index=0;
  //min[K_CONST] stores the K nearest neighbors
  bit6 min[K_CONST];
  //num[K_CONST] stores the the identification number of the K nearest neighbors
  bit4 num[K_CONST];
  //count[10] stores the number of occurrences of each recognition result
  bit4 count[10];
  //maxcount records the majority of the K neighbors with the shortest distance  
  bit4 maxcount=0;
  //The final output recognized digit 
  bit4 maxfrequencynum=0;

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
  LOOP_vote1:
  //pragma HLS unroll
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