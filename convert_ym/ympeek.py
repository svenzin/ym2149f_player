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
    return struct.unpack('>H', ymdata)[0]

def to_dword(ymdata):
    assert(len(ymdata) == 4)
    return struct.unpack('>L', ymdata)[0]

def to_dword_littleendian(ymdata):
    assert(len(ymdata) == 4)
    return struct.unpack('<L', ymdata)[0]

def from_byte(b):
    return struct.pack('<B', b)

def usage():
    print("YM format parser for hardware player")
    print("ymparser.py YM_FILE YMR_FILE")
    exit(1)

def peek(filename):
    lha = LhaFile(filename)
    files = lha.namelist()
    print(files)
    assert(len(files) == 1)
    ymdata = lha.read(files[0])
    print(ymdata[0:200])
    print(ymdata[-8:])
    ym = {}
    ym['tag'] = to_string(ymdata[0:4])
    if ym['tag'] in ['YM2!', 'YM3!']:
        ym['frames'] = (len(ymdata) - 4) // 14   

        ym['clock'] = 2000000
        ym['rate'] = 50
        ym['song_name'] = ''
        ym['author_name'] = ''
        ym['song_comment'] = ''
        ym['attributes'] = 1
        ym['samples'] = 0
        ym['loop'] = 0
        ym['additions'] = 0
    
    elif ym['tag'] in ['YM3b']:
        ym['frames'] = (len(ymdata) - 8) // 14 
        ym['loop'] = to_dword_littleendian(ymdata[-4:])

        ym['clock'] = 2000000
        ym['rate'] = 50
        ym['song_name'] = ''
        ym['author_name'] = ''
        ym['song_comment'] = ''
        ym['attributes'] = 1
        ym['samples'] = 0
        ym['additions'] = 0
    
    elif ym['tag'] in ['YM4!']:
        ym['check'] = to_string(ymdata[4:12])
        ym['frames'] = to_dword(ymdata[12:16])
        ym['attributes'] = to_dword(ymdata[16:20])
        ym['samples'] = to_dword(ymdata[20:24])
        ym['loop'] = to_dword(ymdata[24:28])
        song_name_end = ymdata.index(b'\0', 28)
        author_name_end = ymdata.index(b'\0', song_name_end + 1)
        song_comment_end = ymdata.index(b'\0', author_name_end + 1)
        ym['song_name'] = to_string(ymdata[28:song_name_end])
        ym['author_name'] = to_string(ymdata[song_name_end + 1:author_name_end])
        ym['song_comment'] = to_string(ymdata[author_name_end + 1:song_comment_end])
        
        ym['clock'] = 2000000
        ym['rate'] = 50
        ym['additions'] = 0
    
        n = ym['frames']
        ym['tagend'] = to_string(ymdata[song_comment_end + 1 + 16 * n:])

    elif ym['tag'] in ['YM5!', 'YM6!']:
        ym['check'] = to_string(ymdata[4:12])
        ym['frames'] = to_dword(ymdata[12:16])
        ym['attributes'] = to_dword(ymdata[16:20])
        ym['samples'] = to_word(ymdata[20:22])
        ym['clock'] = to_dword(ymdata[22:26])
        ym['rate'] = to_word(ymdata[26:28])
        ym['loop'] = to_dword(ymdata[28:32])
        ym['additions'] = to_word(ymdata[32:34])
        song_name_end = ymdata.index(b'\0', 34)
        author_name_end = ymdata.index(b'\0', song_name_end + 1)
        song_comment_end = ymdata.index(b'\0', author_name_end + 1)
        ym['song_name'] = to_string(ymdata[34:song_name_end])
        ym['author_name'] = to_string(ymdata[song_name_end + 1:author_name_end])
        ym['song_comment'] = to_string(ymdata[author_name_end + 1:song_comment_end])
        n = ym['frames']
        ym['tagend'] = to_string(ymdata[song_comment_end + 1 + 16 * n:])
    print(ym)

if __name__ == '__main__':
    if len(sys.argv) != 2:
        usage()
    if not os.path.isfile(sys.argv[1]):
        usage()

    peek(sys.argv[1])
