# HDMI Peripheral: Command Generator

This is the "Command Generator" component of an HDMI peripheral for the Zynq
7000. A command consists of the current pixel's color, whether the color is
valid, as well as the vertical and horizontal synchronization pulses. These
commands are generated and encoded by this component and streamed in parallel on
the output.

This component outputs commands for 480p video. This cannot be configured at
runtime.

For more information, see the DVI Specification. A copy is in the `docs/`
directory of this repository.

## Usage

```
0x00 : Control signals
       bit 0  - ap_start (Read/Write/COH)
       bit 1  - ap_done (Read/COR)
       bit 2  - ap_idle (Read)
       bit 3  - ap_ready (Read/COR)
       bit 7  - auto_restart (Read/Write)
       bit 9  - interrupt (Read)
       others - reserved
0x04 : Global Interrupt Enable Register
       bit 0  - Global Interrupt Enable (Read/Write)
       others - reserved
0x08 : IP Interrupt Enable Register (Read/Write)
       bit 0 - enable ap_done interrupt (Read/Write)
       bit 1 - enable ap_ready interrupt (Read/Write)
       others - reserved
0x0c : IP Interrupt Status Register (Read/TOW)
       bit 0 - ap_done (Read/TOW)
       bit 1 - ap_ready (Read/TOW)
       others - reserved
0x10 : Data signal of framebuffer
       bit 31~0 - framebuffer[31:0] (Read/Write)
0x14 : reserved
0x18 : Data signal of current_raw_coordinate
       bit 31~0 - current_raw_coordinate[31:0] (Read)
0x1c : Control signal of current_raw_coordinate
       bit 0  - current_raw_coordinate_ap_vld (Read/COR)
       others - reserved
(SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)
```

The registers above are exposed on the AXI4-Lite slave interface. This component
is intended to be run with `auto_restart` enabled, so it starts streaming the
next frame's commands just after it's done with this one. At the start of each
frame, the component stores the `framebuffer`'s address and starts fetching
pixels from it. The `current_raw_coordinate` register reports how much progress
the component has made on the current frame.

Framebuffer accesses are done over the AXI4 master interface. The framebuffer is
taken to be an array of `640 * 480 = 307200` 32-bit words. Each word stores an
RGB color, with:

* `R = Word[23:16]`
* `G = Word[15: 8]`
* `B = Word[ 7: 0]`

The commands are streamed out over the AXI4-Stream master interface. Normally,
this output is connected to a FIFO to buffer commands between frames. If that is
done, this component should be clocked faster than the pixel clock to ensure the
FIFO never goes empty. Each command is 30 bits wide, with:

* `Channel0 = Command[ 9: 0]`
* `Channel1 = Command[19:10]`
* `Channel2 = Command[29:20]`

The least-significant bit of each channel's output corresponds to the first bit
that should be serialized.

Finally, the format of the current raw coordinate is given below. The row and
column numbers are absolute, but the frame identifier is relative. The intent is
that drivers may use that field to determine how many frames have elapsed since
the last read.

* `Col = Coordinate[ 9: 0]`
* `Row = Coordinate[19:10]`
* `FrameID = Coordinate[31:20]`
