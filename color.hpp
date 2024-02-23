#pragma once

#include "ap_int.h"

namespace hdmi {

//! \brief A color as stored in memory
using RawColor = ap_uint<32>;

//! \brief Parse a color's components
//!
//! Colors are stored in memory as 32-bit integers. This class parses the
//! the components of that integer into red, green, and blue values.
struct Color {
  ap_uint<8> r;
  ap_uint<8> g;
  ap_uint<8> b;

  Color(RawColor raw): r{raw(23, 16)}, g{raw(15, 8)}, b{raw(7, 0)} {}
};

} // namespace hdmi