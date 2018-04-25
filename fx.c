#include "uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#include "fx.h"
#include "samples.h"
#include "ym2149.h"

//Timer
const unsigned char *sampleOffset;
char sampleChannel;
int sampleCounter = 0;
int sampleLength = 0;

// ISR(TIMER1_COMPA_vect)
// {
//     if (sampleCounter < sampleLength) {
//         send_data(sampleChannel, pgm_read_byte_near(sampleOffset + sampleCounter++));
//     }
// }

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
