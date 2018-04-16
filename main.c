#include "uart.h"
#include "ym2149.h"
#include "fx.h"

void clear_registers(void)
{
  unsigned char rclear[] = {0, 0, 0, 0, 0, 0, 0, 0x40,
                           0, 0, 0, 0, 0, 0, 0, 0};

  set_registers(rclear, 16);
}

int main()
{
  unsigned int i;
  unsigned char r[16];
  unsigned char cban_leds;
  // unsigned char dd_cmd, dd_sample;

  set_ym_clock();
  set_bus_ctl();
  initUART();
  //fx_setupTimer();

  clear_registers();

  for/*ever*/(;;)
  {
    for (i = 0; i < 14; i++)
    {
      r[i] = getByte();
    }

    // // r3 free bits are used to code a DD start.
    // // r3 b5-b4 is a 2bits code wich means:
    // dd_cmd = (r[3] & 0x30) >> 4;

    // // 00:     No DD
    // // 01:     DD starts on voice A
    // // 10:     DD starts on voice B
    // // 11:     DD starts on voice C
    // if (dd_cmd)
    // {
    //   const char voice[4] = {0, 8, 9, 10};
    //   char dd_voice = voice[dd_cmd];

    //   // WARNING:
    //   // If a DD starts on voice V, the volume register corresponding to V (Ex r8 for voice A,
    //   // r9 for B and r10 for C) contains the sample number in 5 low bits (That mean you have
    //   // 32 digiDrum max in a song).
    //   dd_sample = r[(int)dd_voice] & 0x1f;

    //   fx_playDigidrum(dd_sample, dd_voice);
    // }

    // Set IOA port as output
    r[7] |= 0x40;

    // As we've seen, r13 has a particular status. If the value stored in the file is 0xff,
    // YM emulator will not reset the waveform position.
    set_registers(r, r[13] == 0xff ? 12 : 13);

    // Have LED blink with noise (drums)
    cban_leds = (~r[7] & 0x38) ? 0x01 : 0x00;
    // Add ABC volume LEDs
    cban_leds |= ((r[8] & 0x08) >> 2) | ((r[9] & 0x08) >> 1) | (r[10] & 0x08);
    // Set IOA
    send_data(14, cban_leds);
  }

  return 0;
}
