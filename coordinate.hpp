#pragma once

#include "ap_int.h"

namespace hdmi {

//! \brief Reverse data about where we are on the screen
//!
//! The coordinate contains data about where we are in the current frame, as
//! well as what frame we're currently on. The format is:
//! * [ 9: 0] column
//! * [19:10] row
//! * [31:20] frame
//! The row and column are absolute, but the frame is relative. We don't
//! necessarily start on frame zero, but we do increment by one each time.
using RawCoordinate = ap_uint<32>;

//! \brief Parsed version of RawCoordinates
//!
//! Again, coordinates contain data not only asbout where we are in the current
//! frame, but also the relative frame ID. The frame ID doesn't start at zero,
//! so only offsets are meaningful.
struct Coordinate {
  ap_uint<12> frame_id;
  ap_uint<10> row;
  ap_uint<10> col;

  //! \brief Forward struct fields
  Coordinate(ap_uint<12> frame_id, ap_uint<10> row, ap_uint<10> col)
    : frame_id(frame_id), row(row), col(col) {}

  //! \brief Convert to the raw version
  RawCoordinate raw() const {
    return (this->frame_id, this->row, this->col);
  }
};

} // namespace hdmi