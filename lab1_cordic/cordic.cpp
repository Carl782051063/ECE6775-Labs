//=========================================================================
// cordic.cpp  fixed-point design
//=========================================================================
// @brief : A CORDIC implementation of sine and cosine functions.

#include "cordic.h"
#include <math.h>

#include <iostream>
//-----------------------------------
// cordic function
//-----------------------------------
// @param[in]  : theta - input angle
// @param[out] : s - sine output
// @param[out] : c - cosine output
void cordic(theta_type theta, cos_sin_type &s, cos_sin_type &c){

#ifdef FIXED_TYPE // fixed-point design
// -----------------------------
cos_sin_type current_cos=0.60735;            
//current_cos = 1 / CORDIC Gain
cos_sin_type current_sin=0.0;
//current_sin = 0
// -----------------------------
FIXED_STEP_LOOP:
  for (int step = 0; step < 20; step++) {
    // operate (>>i)  =  current_cos* 2^(-i)
    cos_sin_type cos_shift = current_cos>>step;
    cos_sin_type sin_shift = current_sin>>step;
    //Determine if we are rotating by a positive or negtive angle
    if(theta>=0){
    //perform rotation
    current_cos=current_cos-sin_shift;
    current_sin=current_sin+cos_shift;
    theta=theta-cordic_ctab[step];
    }
    else{
    //perform rotation
    current_cos=current_cos+sin_shift;
    current_sin=current_sin-cos_shift;
    //Determine the new theta
    theta=theta+cordic_ctab[step];
    }
}
// Return the results
s=current_sin;
c=current_cos;
#else // floating point design
// -----------------------------
cos_sin_type current_cos=0.60735;            
//current_cos = 1 / CORDIC Gain
cos_sin_type current_sin=0.0;
//current_sin = 0
// -----------------------------
cos_sin_type factor=1.0;
FLOAT_STEP_LOOP:
  for (int step = 0; step < NUM_ITER; step++){
  int sigma =(theta<0)? -1:1;
    //Multiply previous iteration by 2^(-j)
    cos_sin_type  cos_shift=current_cos*sigma*factor;
    cos_sin_type  sin_shift=current_sin*sigma*factor;
    //Perform the rotation
    current_cos=current_cos-sin_shift;
    current_sin=current_sin+cos_shift;
    //Determine the new theta
    theta =theta-sigma*cordic_ctab[step];
    factor=factor/2;
}
// Return the results
s=current_sin; 
c=current_cos;
#endif
}

