#include "uart.h"
#include "ym2149.h"
#include "fx.h"

#define MFP2AVR (625/96)    // 16MHz / 2.4576MHz === 625/96
// ATARI-ST MFP chip predivisor
static const int mfpPrediv[8] = {0, 4, 10, 16, 50, 64, 100, 200};

void clear_registers(void)
{
  unsigned char rclear[] = {0, 0, 0, 0, 0, 0, 0, 0x40,
                            0, 0, 0, 0, 0, 0, 0, 0};

  set_registers(rclear, 0xffff);
}

int main()
{
  unsigned int i;
  unsigned char cmd;
  unsigned int mask;
  unsigned char r[16];
  unsigned char dd_cmd;

  set_bus_ctl();
  initUART();

  clear_registers();

  for/*ever*/(;;) {
    mask = 0;

    cmd = getByte();
    switch(cmd) {
      case 0: // masked registers
        mask = getByte();               // Read mask MSB
        mask = (mask << 8) | getByte(); // Read mask LSB
        for (i = 0; i < 16; i++) {      // Read masked registers, leave others unchanged
          if (mask & (1 << i))
            r[i] = getByte();
        }
        break;
      case 1: // send digidrum
        fx_loadDigidrum();
        break;  
    }

    // r3 free bits are used to code a DD start.
    // r3 b5-b4 is a 2bits code wich means:
    dd_cmd = (r[3] & 0x30) >> 4;

    // 00:     No DD
    // 01:     DD starts on voice A
    // 10:     DD starts on voice B
    // 11:     DD starts on voice C
    if (dd_cmd)
    {
      char dd_sample, dd_voice;
      char TP, TC;
      unsigned int dd_div;
      // WARNING:
      // If a DD starts on voice V, the volume register corresponding to V (Ex r8 for voice A,
      // r9 for B and r10 for C) contains the sample number in 5 low bits (That mean you have
      // 32 digiDrum max in a song).
      dd_voice = 7 + dd_cmd;
      dd_sample = r[(int)dd_voice] & 0x1f;
      TP = mfpPrediv[(r[8] >> 5) & 0x07];
			TC = r[15];
      dd_div = MFP2AVR * (TP * TC);

      fx_playDigidrum(dd_sample, dd_voice, dd_div);
    }

    // YM Nano hardware I/O on port A and B of the  PSG
    r[7] |= 0x40; // Setup IOA as output
    r[7] &= 0x7F; // Setup IOB as input

    // Have LED blink with noise (drums)
    r[14] = (~r[7] & 0x38) ? 0x01 : 0x00;
    // Add ABC volume LEDs
    r[14] |= ((r[8] & 0x08) >> 2) | ((r[9] & 0x08) >> 1) | (r[10] & 0x08);
    // Set r[14]
    mask |= 0x4000;

    set_registers(r, mask);
  }

  return 0;
}
