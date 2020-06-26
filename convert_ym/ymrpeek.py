import sys
from lhafile import LhaFile
import struct
import os

def to_string(ymdata):
    return ymdata.decode('cp1252')

def to_byte(ymdata):
    assert(isinstance(ymdata, int))
    return ymdata

def to_word(ymdata):
    assert(len(ymdata) == 2)
    return struct.unpack('<H', ymdata)[0]

def to_dword(ymdata):
    assert(len(ymdata) == 4)
    return struct.unpack('<L', ymdata)[0]

def from_byte(b):
    return struct.pack('<B', b)

def usage():
    print("YM format parser for hardware player")
    print("ymparser.py YM_FILE YMR_FILE")
    exit(1)

def peek(filename):
    with open(filename, 'rb') as f:
        ymdata = f.read()
        print(ymdata[0:200])
        ym = {}
        ym['tag'] = to_string(ymdata[0:4])
        ym['frames'] = to_dword(ymdata[4:8])
        ym['clock'] = to_dword(ymdata[8:12])
        ym['rate'] = to_word(ymdata[12:14])
        song_name_end = ymdata.index(b'\0', 14)
        author_name_end = ymdata.index(b'\0', song_name_end + 1)
        song_comment_end = ymdata.index(b'\0', author_name_end + 1)
        ym['song_name'] = to_string(ymdata[14:song_name_end])
        ym['author_name'] = to_string(ymdata[song_name_end + 1:author_name_end])
        ym['song_comment'] = to_string(ymdata[author_name_end + 1:song_comment_end])
        n = ym['frames']
        ym['tagend'] = to_string(ymdata[song_comment_end + 1 + 14 * n:])
        print(ym)

if __name__ == '__main__':
    if len(sys.argv) != 2:
        usage()
    if not os.path.isfile(sys.argv[1]):
        usage()

    peek(sys.argv[1])
