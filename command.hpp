#pragma once

#include "channel.hpp"
#include "color.hpp"

#include "ap_axi_sdata.h"
#include "ap_int.h"

namespace hdmi {

//! \brief The commands output from this module
//!
//! Individual pixels are output in order over AXI4 Stream. This way, we don't
//! have to worry about timing as long as we can keep the FIFO saturated.
//!
//! The format is:
//! * [ 9: 0] Channel 0 parallel data
//! * [19:10] Channel 1 parallel data
//! * [29:20] Channel 2 parallel data
//! * [31:30] Zeros
//! All data has the MSB on the left.
using RawCommand = ap_uint<32>;

//! \brief A RawCommand we can send over AXI4-Stream
using RawCommandPacket = hls::axis_data<RawCommand>;

//! \brief Generate commands
//!
//! This class generates the commands we should send to the output. It can
//! be called during the active video region with the color for the next pixel,
//! or it can be called during the blanking interval.
class CommandEncoder {
private:
  //! \brief TMDS channels
  //!
  //! The encoder encapsulates the three TMDS channels. It delegates encoding
  //! to them.
  //!
  //! @{
  tmds::Channel chan0;
  tmds::Channel chan1;
  tmds::Channel chan2;
  //! @}

public:
  //! \brief Encode the next color for the active region
  RawCommandPacket encode_active(Color color) {
    return RawCommandPacket{(
      ap_uint<2>(0x0u),
      chan2.encode_de(color.r),
      chan1.encode_de(color.g),
      chan0.encode_de(color.b)
    )};
  };
  //! \brief Encode the next blanking command
  RawCommandPacket encode_blanking(bool hsync, bool vsync) {
    return RawCommandPacket{(
      ap_uint<2>(0x0u),
      chan2.encode_nde(false, false),
      chan1.encode_nde(false, false),
      chan0.encode_nde(hsync, vsync)
    )};
  }
};

} // namespace hdmi