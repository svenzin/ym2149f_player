#ifndef _YMRFILE_H_
#define _YMRFILE_H_

#include <Arduino.h>
#include <SdFat.h>

class YmrFile : public SdFile {
public:
  struct Header {
    char btag[5];
    uint32_t frame_count;
    uint32_t clock_rate;
    uint16_t playback_rate;
    char song_name[256];
    char author_name[256];
    char song_comment[256];
    char etag[5];
  
    explicit Header() {
      memset(btag, 5, 0);
      frame_count = 0;
      clock_rate = 0;
      playback_rate = 0;
      memset(song_name, 256, 0);
      memset(author_name, 256, 0);
      memset(song_comment, 256, 0);
      memset(etag, 5, 0);
    }
  };

  Header header;
  bool is_valid;

  void read_ymr() {
    read_tag(header.btag);
    read_dword(header.frame_count);
    read_dword(header.clock_rate);
    read_word(header.playback_rate);
    read_string(header.song_name);
    read_string(header.author_name);
    read_string(header.song_comment);
    unsigned long pos = curPosition();
    seekSet(pos + 14 * header.frame_count);
    read_tag(header.etag);
    seekSet(pos);

    is_valid = false;
    if (memcmp(header.btag, "YMR1", 4) != 0) return;
    if (memcmp(header.etag, "End!", 4) != 0) return;
    is_valid = true;
  }

private:
  void read_tag(char* tag) {
    read(tag, 4);
    tag[4] = 0;
  }
  
  void read_string(char* s) {
    char *p = s;
    do {
      *p = read();
    } while ((*p != 0) && (p++ < s+255));
    s[255] = 0;
  }
  
  void read_dword(uint32_t& dw) {
    read(&dw, 4);
  }
  
  void read_word(uint16_t& w) {
    read(&w, 2);
  }
};

#endif // _YMRFILE_H_
