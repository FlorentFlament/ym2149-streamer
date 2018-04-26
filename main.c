#include <string.h>
#include "uart.h"
#include "ym2149.h"
#include "fx.h"

#define MFP2AVR (16/2.4576)    // Arduino@16MHz / MFP@2.4576MHz
// MFP predivisor
static const int mfpPrediv[8] = {0, 4, 10, 16, 50, 64, 100, 200};

typedef struct _s_frame {
  uint16_t mask;
  uint8_t regs[16];
} FRAME;

// These are shared between the interrupt vector and the main loop,
// so we mark them volatile.
FRAME frame;  // well, expect for this one as it is "read-only"
volatile uint8_t nbFrames = 0;
volatile uint8_t syncFlag = 0;

// Song vector
ISR(TIMER1_COMPA_vect) {
  if (nbFrames) {
    set_registers(frame.regs, frame.mask);
    nbFrames --;
  }

  syncFlag = 0;
}

void sync() {
  syncFlag = 1;
  while (syncFlag != 0);
}

void setupSongTimer(uint16_t counter) {
    //timer1 : song interrupt
    cli();
    TCCR1A = 0;              // timer reset
    TCCR1B = 0;              // timer reset
    OCR1A = counter;         // counter = ((16Mhz / 64) / freqHz) - 1
    TCCR1B |= (1 << WGM12);  //CTC mode
    TCCR1B |= ((1 << CS10) | (1 << CS11));   // timer ticks = clock ticks / 64
    TIMSK1 |= (1 << OCIE1A); // enable compare
    sei();
}

void clear_frame(FRAME *f)
{
  uint8_t rclear[] = {0, 0, 0, 0, 0, 0, 0, 0x40,
                      0, 0, 0, 0, 0, 0, 0, 0};
  f->mask = 0x7fff;
  memcpy(f->regs, rclear, 16);
}

void set_leds(uint8_t value, FRAME *f) {
  f->regs[7] |= 0x40; // Setup IOA as output
  f->regs[7] &= 0x7F; // Setup IOB as input

  f->regs[14] = value;
  // Set regs 7 & 14 for writing
  f->mask |= 0x4080;
}

void refresh_leds(FRAME *f) {
  uint8_t leds;

  // Have LED blink with noise (drums)
  leds = (~f->regs[7] & 0x38) ? 0x01 : 0x00;
  // Add ABC volume LEDs
  leds |= ((f->regs[8] & 0x08) >> 2) | ((f->regs[9] & 0x08) >> 1) | (f->regs[10] & 0x08);
  set_leds(leds, f);
}

int main()
{
  uint16_t i;
  uint8_t cmd;

  set_bus_ctl();
  initUART();
  setupSongTimer(24999); // Start timer @ 10Hz by default (idle)
  // Mute and turn off leds
  clear_frame(&frame);  
  nbFrames = 1;

main_loop: 
  // sync to song interrupt
  sync();

  putByte(0);       // Tell the arduino we are Ready to RCV
  cmd = getByte();  // First byte is the CMD

  switch(cmd) {
    case 0:         // NOP
      break;
    case 1:         // recvCounter
      setupSongTimer(getUShort());
      break;
    case 2:         // recvFrame
      // if (nbFrames == 1) {
      //   // TODO: error handling
      //   // Buffer overflow
      // }
      frame.mask = getUShort();
      for (i = 0; i < 16; i++) {      // Read masked registers, leave others unchanged
        if (frame.mask & (1 << i))
          frame.regs[i] = getByte();
      }
      refresh_leds(&frame);
      nbFrames = 1;
      break;
    case 3:       // clearFrame
      clear_frame(&frame);
      nbFrames = 1;
      break;
    default:
      // TODO: error handling
      // unknown CMD
      break;  
  }

  goto main_loop; // "Don't tell me what I can't do" - John L.

  return 0;
}
