#pragma once

#include "ap_int.h"

namespace hdmi {
namespace tmds {

//! \brief Data input to the TMDS channel
using ChannelDataInput = ap_uint<8>;

//! \brief Parallel output from a TMDS channel
using ChannelOutput = ap_uint<10>;

//! \brief A single TMDS channel
//!
//! It provides methods for encoding data, and it maintains the state needed for
//! the channel.
class Channel {
private:
  //! \brief State variable
  //!
  //! As specified, we need to keep track of the number of ones we've sent
  //! compared to the number of zeros. That can range between -8 and +8. It'll
  //! also always be an even number, so we store the half value which is between
  //! -4 and +4.
  ap_int<4> hcnt;

public:
  //! \brief Encode methods
  //!
  //! These two methods do the TMDS encoding algorithm, outputing 10-bits in
  //! parallel. We split into two methods based on whether DE is high or low.
  //!
  //! @{
  ChannelOutput encode_de(ChannelDataInput data);
  ChannelOutput encode_nde(bool c0, bool c1);
  //! @}
};

} // namespace tmds
} // namespace hdmi