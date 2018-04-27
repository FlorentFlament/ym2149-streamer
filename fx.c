#include "uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#include "fx.h"
#include "ym2149.h"

//Timer
const unsigned char *sampleOffset;
char sampleChannel;
int sampleCounter = 0;
int sampleLength = 0;

unsigned int dd_size;
unsigned char * dd_buffer;
int dd_index;
int dd_offsets[32], dd_lengths[32];

// ISR(TIMER1_COMPA_vect)
// {
//     if (sampleCounter < sampleLength) {
//         send_data(sampleChannel, pgm_read_byte_near(sampleOffset + sampleCounter++));
//     }
// }

/*
  readYm6Effect(pReg, code, prediv, count) {
    let voice;
    let ndrum;

    code = pReg[code] & 0xf0;
    prediv = (pReg[prediv] >> 5) & 7;
    count = pReg[count];

    if (code & 0x30) {
      let tmpFreq;

      voice = ((code & 0x30) >> 4) - 1; // Voice 0,1,2 = A,B,C
      switch (code & 0xc0) {
        case 0x00:		// SID
        case 0x80:		// Sinus-SID
          prediv = mfpPrediv[prediv];
          prediv *= count;
          tmpFreq = 0;
          if (prediv) {
            tmpFreq = MFP_CLOCK / prediv;
            if ((code & 0xc0) == 0x00)
              // ymChip.sidStart(voice, tmpFreq, pReg[voice + 8] & 15);
              console.log(`==> sidStart(${voice}, ${tmpFreq}, ${pReg[voice + 8] & 15})`);
              else
              // ymChip.sidSinStart(voice, tmpFreq, pReg[voice + 8] & 15);
              console.log(`==> sidSinStart(${voice}, ${tmpFreq}, ${pReg[voice + 8] & 15})`);
            }
          break;

        case 0x40:		// DigiDrum
          ndrum = pReg[voice + 8] & 31;
          if ((ndrum >= 0) && (ndrum < this.header.nbDigidrumsSample)) {
            prediv = mfpPrediv[prediv];
            prediv *= count;
            if (prediv > 0) {
              tmpFreq = MFP_CLOCK / prediv;
              // ymChip.drumStart(voice, pDrumTab[ndrum].pData, pDrumTab[ndrum].size, tmpFreq);
              console.log(`==> drumStart(${voice}, ${ndrum}, ${this.digiDrums[ndrum].length}, ${sampleFrq})`);
            }
          }
          break;

        case 0xc0:		// Sync-Buzzer. (https://youtu.be/5As0eMhajp4)
          prediv = mfpPrediv[prediv];
          prediv *= count;
          tmpFreq = 0;
          if (prediv) {
            tmpFreq = MFP_CLOCK / prediv;
            // ymChip.syncBuzzerStart(tmpFreq, pReg[voice + 8] & 15);
            console.log(`==> syncBuzzerStart(${tmpFreq}, ${pReg[voice + 8] & 15})`);
          }
          break;
      }
    }
  }
*/
void fx_loadDigidrum() {
    int n, length;

    length = getByte();
    length = (length << 8) & getByte();

    if (1500 - dd_size < (unsigned int)length) {  // If we're out of memory, repeat last sample
        dd_offsets[dd_index] = dd_size;
        dd_lengths[dd_index] = dd_lengths[dd_index - 1];
        dd_index ++;
        // We still have to flush the cmd
        for (n = 0; n < length; n++) {
            getByte();
        }
        return;
    }
    // Else load sample normally  
    dd_offsets[dd_index] = dd_size;
    dd_lengths[dd_index ++] = length;
    for (n = 0; n < length; n++) {
        dd_buffer[dd_size++] = getByte();
    }
}

void fx_playDigidrum(char index, char channel, unsigned int divider)
{
    cli();
    TCCR1A = 0;              //timer reset
    TCCR1B = 0;              //timer reset
    OCR1A = divider;
    TCCR1B |= (1 << WGM12);  //CTC mode
    TCCR1B |= (1 << CS10);   // timer ticks = clock ticks
    TIMSK1 |= (1 << OCIE1A); // enable compare

    sampleCounter = 0;
    sampleChannel = channel;
    sampleOffset = dd_buffer + dd_offsets[(int)index];
    sampleLength = dd_lengths[(int)index];
    sei();
}
