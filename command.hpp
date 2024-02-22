#pragma once

#include "ap_axi_sdata.h"
#include "ap_int.h"

namespace hdmi {

//! \brief The commands output from this module
//!
//! Individual pixels are output in order over AXI4 Stream. This way, we don't
//! have to worry about timing as long as we can keep the FIFO saturated.
using RawCommand = ap_uint<32>;

//! \brief A RawCommand we can send over AXI4-Stream
using RawCommandPacket = hls::axis_data<RawCommand>;

} // namespace hdmi