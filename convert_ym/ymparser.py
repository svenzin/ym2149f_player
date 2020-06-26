import sys
from lhafile import LhaFile
import struct
import os

# TODO handle loops on HW
# TODO handle custom playback rate on HW

class Error(Exception):
    pass

class ConversionError(Error):
    def __init__(self, message):
        self.message = message

class LhaArchiveError(Error):
    pass

class YmHeaderTypeError(Error):
    pass

def error(message):
    print('Error: {}'.format(message))
    raise ConversionError(message)

def cant_handle_custom_frequencies(ym_content):
    if ym_content['clock'] != 2000000 or ym_content['rate'] != 50:
        error('Cannot handle custom clock or playback rates')

def cant_handle_dd_or_ts_effects(ym_content):
    if (((ym_content['attributes'] & 0xFFFFFFFE) != 0) or
        (ym_content['samples'] != 0) or
        (ym_content['additions'] != 0) or
        (not all([(f[1] & 0b11110000) == 0 for f in ym_content['data']])) or
        (not all([(f[3] & 0b11110000) == 0 for f in ym_content['data']])) or
        (not all([(f[5] & 0b11110000) == 0 for f in ym_content['data']])) or
        (not all([(f[6] & 0b11100000) == 0 for f in ym_content['data']])) or
        (not all([(f[8] & 0b11100000) == 0 for f in ym_content['data']])) or
        (not all([(f[9] & 0b11100000) == 0 for f in ym_content['data']])) or
        (not all([(f[10] & 0b11100000) == 0 for f in ym_content['data']])) or
        (not all([((f[13] & 0b11110000) == 0) or (f[13] == 0xFF) for f in ym_content['data']])) or
        (not all([f[14] == 0 for f in ym_content['data']])) or
        (not all([f[15] == 0 for f in ym_content['data']]))):
        error('Cannot handle special effects')

def cant_handle_loop(ym_content):
    if ym_content['loop'] != 0:
        error('Cannot handle loops')

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

def convert(filename, ofilename):
    try:
        lha = LhaFile(filename)
        files = lha.namelist()
        # print(files)
        assert(len(files) == 1)
        ymdata = lha.read(files[0])
    except:
        raise LhaArchiveError

    # print(ymdata[0:200])
    ym = {}
    ym['tag'] = to_string(ymdata[0:4])
    if not ym['tag'] in ['YM2!', 'YM3!', 'YM3b', 'YM4!', 'YM5!', 'YM6!']:
        raise YmHeaderTypeError

    if ym['tag'] in ['YM2!', 'YM3!']:
        assert((len(ymdata) - 4) % 14 == 0)
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
        
        n = ym['frames']
        ym['data'] = [[0] * 16 for i in range(n)]
        for f in range(n):
            for r in range(14):
                ym['data'][f][r] = to_byte(ymdata[4 + r * n + f])
        
        # cant_handle_custom_frequencies(ym)
        cant_handle_dd_or_ts_effects(ym)
        # cant_handle_loop(ym)

    if ym['tag'] in ['YM3b']:
        assert((len(ymdata) - 8) % 14 == 0)
        ym['frames'] = (len(ymdata) - 8) // 14 
        ym['loop'] = to_dword(ymdata[-4:])
        if ym['loop'] >= ym['frames']:
            ym['loop'] = to_dword_littleendian(ymdata[-4:])
        assert(ym['loop'] < ym['frames'])
        
        ym['clock'] = 2000000
        ym['rate'] = 50
        ym['song_name'] = ''
        ym['author_name'] = ''
        ym['song_comment'] = ''
        ym['attributes'] = 1
        ym['samples'] = 0
        ym['additions'] = 0
        
        n = ym['frames']
        ym['data'] = [[0] * 16 for i in range(n)]
        for f in range(n):
            for r in range(14):
                ym['data'][f][r] = to_byte(ymdata[4 + r * n + f])

        # cant_handle_custom_frequencies(ym)
        cant_handle_dd_or_ts_effects(ym)
        # cant_handle_loop(ym)

    elif ym['tag'] in ['YM4!']:
        ym['check'] = to_string(ymdata[4:12])
        assert(ym['check'] == 'LeOnArD!')
        ym['frames'] = to_dword(ymdata[12:16])
        ym['attributes'] = to_dword(ymdata[16:20])
        ym['samples'] = to_dword(ymdata[20:24])
        ym['loop'] = to_dword(ymdata[24:28])
        assert(ym['loop'] < ym['frames'])
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
        assert(ym['tagend'] == 'End!')
        # print(ym)
        #
        ym['data'] = [[0] * 16 for i in range(n)]

        f0 = song_comment_end + 1
        if ym['attributes'] == 1:
            for f in range(n):
                for r in range(16):
                    ym['data'][f][r] = to_byte(ymdata[f0 + r * n + f])
        elif ym['attributes'] == 0:
            for f in range(n):
                for r in range(16):
                    ym['data'][f][r] = to_byte(ymdata[f0 + 16 * f + r])

        # cant_handle_custom_frequencies(ym)
        cant_handle_dd_or_ts_effects(ym)
        # cant_handle_loop(ym)

    elif ym['tag'] in ['YM5!', 'YM6!']:
        ym['check'] = to_string(ymdata[4:12])
        assert(ym['check'] == 'LeOnArD!')
        ym['frames'] = to_dword(ymdata[12:16])
        ym['attributes'] = to_dword(ymdata[16:20])
        ym['samples'] = to_word(ymdata[20:22])
        ym['clock'] = to_dword(ymdata[22:26])
        ym['rate'] = to_word(ymdata[26:28])
        ym['loop'] = to_dword(ymdata[28:32])
        assert(ym['loop'] < ym['frames'])
        ym['additions'] = to_word(ymdata[32:34])
        song_name_end = ymdata.index(b'\0', 34)
        author_name_end = ymdata.index(b'\0', song_name_end + 1)
        song_comment_end = ymdata.index(b'\0', author_name_end + 1)
        ym['song_name'] = to_string(ymdata[34:song_name_end])
        ym['author_name'] = to_string(ymdata[song_name_end + 1:author_name_end])
        ym['song_comment'] = to_string(ymdata[author_name_end + 1:song_comment_end])
        
        n = ym['frames']
        ym['tagend'] = to_string(ymdata[song_comment_end + 1 + 16 * n:])

        assert(ym['tagend'] == 'End!')
        # print(ym)
        #
        ym['data'] = [[0] * 16 for i in range(n)]

        f0 = song_comment_end + 1
        if ym['attributes'] == 1:
            for f in range(n):
                for r in range(16):
                    ym['data'][f][r] = to_byte(ymdata[f0 + r * n + f])
        elif ym['attributes'] == 0:
            for f in range(n):
                for r in range(16):
                    ym['data'][f][r] = to_byte(ymdata[f0 + 16 * f + r])

        # cant_handle_custom_frequencies(ym)
        cant_handle_dd_or_ts_effects(ym)
        # cant_handle_loop(ym)

    with open(ofilename, 'wb') as of:
        of.write('YMR1'.encode())
        of.write(struct.pack('<L', ym['frames']))
        of.write(struct.pack('<L', ym['clock']))
        of.write(struct.pack('<H', ym['rate']))
        of.write(ym['song_name'].encode())
        of.write(b'\0')
        of.write(ym['author_name'].encode())
        of.write(b'\0')
        of.write(ym['song_comment'].encode())
        of.write(b'\0')
        for f in range(n):
            for r in range(14):
                of.write(from_byte(ym['data'][f][r]))
        of.write('End!'.encode())

if __name__ == '__main__':
    # if len(sys.argv) != 3:
    #     usage()
    # if not os.path.isfile(sys.argv[1]):
    #     usage()
    # if os.path.exists(os.path.exists(sys.argv[2])):
    #     usage()
    convert(sys.argv[1], sys.argv[2])
