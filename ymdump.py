import os.path
import sys

from ymreader import YmReader

def main():
    if len(sys.argv) != 2:
        print("Dump YM file to z88dk z80asm format.")
        print("Syntax is: {} <ym_filepath>".format(sys.argv[0]))
        exit(0)
    
    songname = os.path.splitext(os.path.basename(sys.argv[1]))[0]
    with open(sys.argv[1]) as fd:
        ym = YmReader(fd)
        print("._{}".format(songname))
        for s in ym.get_data():
            d = ", ".join(["${:02x}".format(ord(b)) for b in s[:14]])
            print("\tDEFB " + d)

main()
