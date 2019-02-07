#!/usr/bin/env python

import sys
import time

from ymreader import YmReader

def to_minsec(frames, frames_rate):
    secs = frames / frames_rate
    mins = secs / 60
    secs = secs % 60
    return (mins, secs)

def main():
    header = None
    data = None

    if len(sys.argv) != 3:
        print("Syntax is: {} <output_device> <ym_filepath>".format(sys.argv[0]))
        exit(0)

    with open(sys.argv[2]) as fd:
        ym = YmReader(fd)
        ym.dump_header()
        header = ym.get_header()
        data = ym.get_data()

    song_min, song_sec = to_minsec(header['nb_frames'], header['frames_rate'])
    print("")
    with open(sys.argv[1], 'w') as fd:
        time.sleep(2) # Wait for Arduino reset
        frame_t = time.time()
        for i in range(header['nb_frames']):
            # Substract time spent in code
            time.sleep(1./header['frames_rate'] - (time.time() - frame_t))
            frame_t = time.time()
            fd.write(data[i])
            fd.flush()
            i+= 1

            # Additionnal processing
            cur_min, cur_sec = to_minsec(i, header['frames_rate'])
            sys.stdout.write(
                "\x1b[2K\rPlaying {0:02}:{1:02} / {2:02}:{3:02}".format(
                cur_min, cur_sec, song_min, song_sec))
            sys.stdout.flush()

        # Clear YM2149 registers
        fd.write('\x00'*16)
        fd.flush()
        print("")

if __name__ == '__main__':
    main()
