import os.path
import sys

from ymreader import YmReader

def divide_by2(l):
    val = l[0] + l[1]*256
    val /= 2
    return (val%256, val/256)

def main():
    if len(sys.argv) not in (2, 3):
        print("Dump YM file to z88dk z80asm format.")
        print("Syntax is: {} <ym_filepath> [max_frames_count]".format(sys.argv[0]))
        exit(0)


    songname = os.path.splitext(os.path.basename(sys.argv[1]))[0]
    with open(sys.argv[1]) as fd:
        ym = YmReader(fd)
        atari_clock = ym.get_header()['chip_clock'] == 2000000

        data = ym.get_data()
        # Possibly capping number of frames to dump
        if len(sys.argv) == 3:
            max_frames = int(sys.argv[2])
            data = data[:max_frames]

        # Printing out stuff
        print("\tPUBLIC _{}".format(songname))
        print("._{}".format(songname))
        for s in data:
            d = [ord(c) for c in s]
            if atari_clock: # Ajust notes to the 1000000 MHz Amstrad CPC clock
                d[0:2] = divide_by2(d[0:2])
                d[2:4] = divide_by2(d[2:4])
                d[4:6] = divide_by2(d[4:6])
            l = ", ".join(["${:02x}".format(x) for x in d[:14]])
            print("\tDEFB " + l)

main()
