#include "command.hpp"
#include "coordinate.hpp"

#include "ap_axi_sdata.h"
#include "hls_burst_maxi.h"
#include "hls_stream.h"

void top(
  hls::burst_maxi<uint32_t> framebuffer,
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

  // Keep track of the current row and column. The active region is generally
  // taken to be in the top-left corner, and we follow that convention.
  ap_uint<10> row;
  ap_uint<10> col;

  // Set up the burst. We know the screen size, and we read each pixel once, so
  // burst for that many words.
  framebuffer.read_request(0u, 640u * 480u);

  // Iterate over the active rows. Each row is split into an active region and a
  // blanking interval.
  VACTIVE_ROWS: for (row = 0u; row < 480u; row++) {

    // Active region
    HACTIVE_COLS: for (col = 0u; col < 640u; col++) {
    #pragma HLS PIPELINE II=1
      current_raw_coordinate = hdmi::Coordinate{fid, row, col}.raw();

      // Read the color from the framebuffer, then push it out. Remember to set
      // the data active too.
      ap_uint<32> color_data = framebuffer.read() & 0xffffffu;
      commands.write(hdmi::RawCommandPacket { .data = color_data | 0x04000000u });
    }

    // Horizontal blanking interval
    HBLANK_COLS: for (col = 640u; col < 800u; col++) {
    #pragma HLS PIPELINE II=1
      current_raw_coordinate = hdmi::Coordinate{fid, row, col}.raw();

      // Compute whether we should output HSYNC. This might be true since we're
      // in the horizontal blanking interval.
      bool hsync = col >= 656u && col < 752u;

      // Signal with the sync
      commands.write(hdmi::RawCommandPacket { .data = (hsync ? 0x02000000u : 0u) });
    }
  }

  // Finally, handle the blanking interval. All the columns are blanked here, so
  // there's no need to split up the inner loop.
  VBLANK_ROWS: for (row = 480u; row < 525u; row++) {
    VBLANK_COLS: for (col = 0u; col < 800u; col++) {
    #pragma HLS LOOP_FLATTEN
    #pragma HLS PIPELINE II=1
      current_raw_coordinate = hdmi::Coordinate{fid, row, col}.raw();

      // Compute both the horizontal and verital sync pulses. We could be in the
      // blanking interval for both, so we have to check if we're in the sync
      // interval.
      bool hsync = col >= 656u && col < 752u;
      bool vsync = row >= 490u && row < 492u;

      // Signal with the syncs
      commands.write(hdmi::RawCommandPacket { .data = (hsync ? 0x02000000u : 0u) | (vsync ? 0x01000000u : 0u) });
    }
  }
}