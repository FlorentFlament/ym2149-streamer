import functools
import itertools
import logging
import struct

class YmReader(object):

    def __parse_extra_infos(self):
        # Thanks http://stackoverflow.com/questions/32774910/clean-way-to-read-a-null-terminated-c-style-string-from-a-file
        toeof = iter(functools.partial(self.__fd.read, 1), '')
        def readcstr():
            return ''.join(itertools.takewhile('\0'.__ne__, toeof))
        self.__header['song_name'] = readcstr()
        self.__header['author_name'] = readcstr()
        self.__header['song_comment'] = readcstr()

    def __parse_header(self):
        # See:
        # http://leonard.oxg.free.fr/ymformat.html
        # ftp://ftp.modland.com/pub/documents/format_documentation/Atari%20ST%20Sound%20Chip%20Emulator%20YM1-6%20(.ay,%20.ym).txt
        ym_header = '> 4s 8s I I H I H I H'
        s = self.__fd.read(struct.calcsize(ym_header))
        d = {}
        (d['id'],
         d['check_string'],
         d['nb_frames'],
         d['song_attributes'],
         d['nb_digidrums'],
         d['chip_clock'],
         d['frames_rate'],
         d['loop_frame'],
         d['extra_data'],
        ) = struct.unpack(ym_header, s)

        if d['check_string'] != 'LeOnArD!':
            raise Exception('Unsupported file format: Bad check string: {}'.format(d['check_string']))
        if d['id'] not in ('YM5!', 'YM6!'):
            raise Exception('Unsupported file format: Only YM5 supported (got {})'.format(d['id']))
        if d['nb_digidrums'] != 0:
            raise Exception('Unsupported file format: Digidrums are not supported')
        if d['chip_clock'] not in (1000000, 2000000):
            raise Exception('Unsupported file format: Invalid chip clock: {}'.format(d['chip_clock']))

        d['interleaved'] = d['song_attributes'] & 0x01 != 0
        self.__header = d

        self.__parse_extra_infos()

    def __read_data_interleaved(self):
        cnt  = self.__header['nb_frames']
        regs = [self.__fd.read(cnt) for i in range(16)]
        self.__data=[''.join(f) for f in zip(*regs)]

    def __read_data(self):
        if not self.__header['interleaved']:
            raise Exception(
                'Unsupported file format: Only interleaved data are supported')
        self.__read_data_interleaved()

    def __check_eof(self):
        if self.__fd.read(4) != 'End!':
            logging.warning("*Warning* End! marker not found after frames")

    def __init__(self, fd):
        self.__fd = fd
        self.__parse_header()
        self.__data = []

    def dump_header(self):
        for k in ('id','check_string', 'nb_frames', 'song_attributes',
                  'nb_digidrums', 'chip_clock', 'frames_rate', 'loop_frame',
                  'extra_data', 'song_name', 'author_name', 'song_comment'):
            print("{}: {}".format(k, self.__header[k]))

    def get_header(self):
        return self.__header

    def get_data(self):
        if not self.__data:
            self.__read_data()
            self.__check_eof()
        return self.__data
