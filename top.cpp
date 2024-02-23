#include "color.hpp"
#include "command.hpp"
#include "coordinate.hpp"

#include "ap_axi_sdata.h"
#include "hls_burst_maxi.h"
#include "hls_stream.h"

//! \brief Entrypoint of the peripheral
//!
//! The peripheral can be configured over the AXI4-Lite interface. It's intended
//! to be run in continuous mode, with each run producing the commands for one
//! frame of video. The commands start immediately with active video, with the
//! horizontal/vertical blanking intervals being on the right/bottom edges of
//! the screen.
//!
//! The framebuffer register is copied to the device on start, so you have to
//! wait until the end of the frame to free the buffer.
//!
//! Also, the current raw coordinate only goes valid once we start rasterizing,
//! which doesn't happen for 20 or so bus cycles. Make sure to wait for the data
//! to go valid before trusting it, and make sure to clear the valid bit from
//! the last run if needed.
//!
//! \param[in] framebuffer The physical address to read pixels from
//! \param[out] current_raw_coordinate An encoded representation of where we are
//!                                    currently rasterizing
//! \param[out] commands The HDMI commands to draw the screen
void top(
  hls::burst_maxi<hdmi::RawColor> framebuffer,
  volatile hdmi::RawCoordinate &current_raw_coordinate,
  hls::stream<hdmi::RawCommandPacket> &commands
) {
#pragma HLS INTERFACE mode=s_axilite port=return
#pragma HLS INTERFACE mode=m_axi port=framebuffer offset=slave
#pragma HLS INTERFACE mode=s_axilite port=current_raw_coordinate
#pragma HLS INTERFACE mode=axis port=commands

  // Keep a frame id between iterations. At the start of the function, we
  // increment it by one.
  static ap_uint<12> fid = 0u;
  fid++;

  // Set up the burst. We know the screen size, and we read each pixel once, so
  // burst for that many words.
  framebuffer.read_request(0u, 640u * 480u);

  // Use this encoder to generate commands. We need a single encoder because we
  // need to persist state between pixels.
  hdmi::CommandEncoder encoder;

  // Iterate over the rows and columns. The active region is in the top-left
  // corner, as is convention.
  ROWS: for (ap_uint<10> row = 0u; row < 525u; row++) {
    COLS: for (ap_uint<10> col = 0u; col < 800u; col++) {
    #pragma HLS LOOP_FLATTEN
    #pragma HLS PIPELINE II=1

      // Send the current coordinate back
      current_raw_coordinate = hdmi::Coordinate{fid, row, col}.raw();

      // Compute whether we are in the active region, as well as the sync pulses
      // during the blanking interval.
      bool de = col < 640u && row < 480u;
      bool hsync = col >= 656u && col < 752u;
      bool vsync = row >= 490u && row < 492u;

      // Compute the output packet
      hdmi::RawCommandPacket packet;
      if (de) {
        // If we're in the active region, read a pixel
        hdmi::Color color{framebuffer.read()};
        packet = encoder.encode_active(color);
      } else {
        // Otherwise, send sync pulses as needed
        packet = encoder.encode_blanking(hsync, vsync);
      }

      // Write the packet
      commands.write(packet);
    }
  }
}