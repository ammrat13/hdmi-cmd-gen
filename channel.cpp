#include "channel.hpp"

using namespace hdmi::tmds;

//! \brief Cumulative XOR
//!
//! This is the first part of the encoding algorithm. Cumulative XNOR can be
//! made by XORing the result of this with 0b10101010.
static ap_uint<8> cumxor(ap_uint<8> x) {
  // Initialize the return value to zero
  ap_uint<8> y = 0u;
  // Each bit of the return value is the XOR of everyting under and including
  for (unsigned i = 0u; i < 8u; i++) {
  #pragma HLS UNROLL
    for (unsigned j = 0u; j <= i; j++) {
    #pragma HLS UNROLL
      y[i] ^= x[j];
    }
  }
  // Return
  return y;
}

//! \brief Subtract the number of ones from the number of zeros
//!
//! The result will always be even, so we chop off the last bit. Still, if the
//! result is positive we have more ones, and if the result is negative we have
//! more zeros.
static ap_int<4> popdiff(ap_uint<8> x) {
  // Create variables to hold each count
  ap_int<5> ones_count;
  ap_int<5> zero_count;
  // Do the counting
  for (unsigned i = 0u; i < 8u; i++) {
  #pragma HLS UNROLL
    if (x[i])
      ones_count++;
    else
      zero_count++;
  }
  // Return the half difference
  return (ones_count - zero_count)(4, 1);
}

ChannelOutput Channel::encode_de(ChannelDataInput data) {

  // Phase 1: Cumulative XOR
  ap_uint<9> q_m;
  {
    // Initially, the output data is the rolling XOR of the input, where we
    // didn't use XNOR.
    q_m = (ap_uint<1>(1u), cumxor(data));
    // Based on the number of ones in the input data, we may use XNOR instead
    ap_int<4> d = popdiff(data);
    if (d > 0 || (d == 0 && !data[0])) {
      q_m ^= 0b110101010u;
    }
  }

  // Phase 2: Inversion
  ap_uint<10> q_out;
  {
    // Compute the difference in the output from the first phase. We use this a
    // lot, so it's good to keep it around.
    ap_int<4> d = popdiff(q_m(7, 0));

    if (this->hcnt == 0 || d == 0) {
      // This is the right branch of the diagram
      q_out[9] = ~q_m[8];
      q_out[8] = q_m[8];
      q_out(7, 0) = q_m[8] ? q_m(7, 0) : ~q_m(7, 0);
      this->hcnt += q_m[8] ? ap_int<4>(d) : ap_int<4>(-d);

    } else if ((this->hcnt > 0 && d > 0) || (this->hcnt < 0 && d < 0)) {
      // Bottom branch right case. Remember that we've halved the counter, so we
      // should only increment by 1 if q_m[8] is high, not by 2.
      q_out[9] = 1u;
      q_out[8] = q_m[8];
      q_out(7, 0) = ~q_m(7, 0);
      this->hcnt += ap_int<4>(q_m[8] ? 1 : 0) - ap_int<4>(d);

    } else {
      // Bottom branch bottom case. Remember that we've halved the counter, so
      // we should only decrement by 1 if q_m[8] is low, not by 2.
      q_out[9] = 0u;
      q_out[8] = q_m[8];
      q_out(7, 0) = q_m(7, 0);
      this->hcnt += ap_int<4>(q_m[8] ? 0 : -1) + ap_int<4>(d);
    }
  }

  return q_out;
}

ChannelOutput Channel::encode_nde(bool c0, bool c1) {
  this->hcnt = 0;
  return c1
    ? (c0 ? 0b1010101011 : 0b0101010100)
    : (c0 ? 0b0010101011 : 0b1101010100);
}