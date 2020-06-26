#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static const byte PROGMEM shuffle_on[] = {
  B00000000, B00010000, B00000000, B00011000, B11000000, B01111100, B11100000, B11111110,
  B11110001, B11111100, B00111001, B10011000, B00011100, B00010000, B00001110, B00000000,
  B00000111, B00010000, B00110011, B10011000, B11110001, B11111100, B11100000, B11111110,
  B11000000, B01111100, B00000000, B00011000, B00000000, B00010000, B00000000, B00000000,
};
static const byte PROGMEM mute_on[] = {
  B00000000, B10000000, B00000001, B10000000, B00000011, B10000000, B00000110, B00000000,
  B00001110, B10000010, B11011110, B11000110, B11011110, B01101100, B11011111, B00111000,
  B11011110, B01101100, B11011110, B11000110, B00001110, B10000010, B00000110, B00000000,
  B00000011, B10000000, B00000001, B10000000, B00000000, B10000000, B00000000, B00000000,
};  
static const byte PROGMEM remote_on[] = {
  B00000111, B11000000, B00011111, B11110000, B00111000, B00111000, B01100000, B00001100,
  B11000011, B10000110, B10001111, B11100010, B00011100, B01110000, B00110000, B00011000,
  B00100011, B10001000, B00000111, B11000000, B00000100, B01000000, B00000000, B00000000,
  B00000011, B10000000, B00000011, B10000000, B00000011, B10000000, B00000000, B00000000,
};

class Display {
  Adafruit_SSD1306 display;

  uint32_t scl;
  uint32_t sda;
  uint32_t address;
public:  
  explicit Display(uint32_t pin_scl, uint32_t pin_sda, uint32_t addr)
  : scl(pin_scl), sda(pin_sda), address(addr), display(128, 64)
  {}
  
  bool setup() {
    Wire.setSCL(scl);
    Wire.setSDA(sda);
    if (!display.begin(SSD1306_SWITCHCAPVCC, address)) return false;

    display.cp437(true);
    display.setTextWrap(false);
    display.setTextColor(SSD1306_WHITE);

    clear();

    return true;
  }

  void clear() {
    display.clearDisplay();
    display.display();
  }
  
  void logo() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextWrap(false);
    display.setCursor(0, 0);
    display.print("YM2149F");
    display.display();
  }

  char t[6] = "";
  void print_time(uint32_t frame, uint32_t rate) {
    int m = (frame / rate) / 60;
    int s = (frame / rate) % 60;
    sprintf(t, "%u:%02u", m, s);
    display.print(t);
  }
  int dx = 0;
  int lx = 0;
  int vx = -2;
  void song() {
    display.clearDisplay();

    display.setTextSize(2);
    display.setCursor(dx > 0 ? 0 : dx, 0);
    display.print(song_name);

    display.setTextSize(1);
    display.setCursor(0, 2*8+2);
    display.print(song_auth);

    int x = 0;
    if (is_randomized) {
      display.drawBitmap(x, 48, shuffle_on, 16, 16, SSD1306_WHITE);
      x += 18;
    }
    if (is_mute) {
      display.drawBitmap(x, 48, mute_on, 16, 16, SSD1306_WHITE);
      x += 18;
    }
    if (has_remote) {
      display.drawBitmap(x, 48, remote_on, 16, 16, SSD1306_WHITE);
      x += 18;
    }

    uint32_t p = 128 * song_frame / song_nframes;
    x = p;
    if (x < 15) x = 15;
    if (x > (128 - 15)) x = 128 - 15;
    display.setCursor(x-15, 4*8-3);
    print_time(song_frame, fplayback);
    if (x+15 < (128 - 30)) {
      display.setCursor(128 - 30, 4*8-3);
      print_time(song_nframes, fplayback);
    }
    display.fillRect(0, 5*8-1, p, 5, SSD1306_WHITE);

    display.setCursor(56, 7*8);
    display.print(fplayback);
    display.print("Hz ");
    display.print(fclock * 0.000001, 2);
    display.print("MHz");

    display.display();

    if (vx < 0) {
      if (lx + dx > 128) dx += vx;
      else vx = -vx;
    } else {
      if (dx < 24) dx += vx;
      else vx = -vx;
    }
  }

  const char * song_name = "";
  const char * song_auth = "";
  uint32_t song_nframes = 0;
  void set_song(const char *name,
                const char *auth,
                uint32_t nframes) {
    song_name = name;
    song_auth = auth;
    song_nframes = nframes;

    lx = strlen(song_name) * 2 * 6;
    dx = 24;
    vx = -3;
  }

  bool is_randomized;
  bool is_mute;
  bool has_remote;
  uint32_t fclock;
  uint32_t fplayback;
  void set_status(bool randomized,
                  bool muted,
                  bool remote,
                  uint32_t clock_freq,
                  uint32_t playback_freq) {
    is_randomized = randomized;
    is_mute = muted;
    has_remote = remote;
    fclock = clock_freq;
    fplayback = playback_freq;
  }

  uint32_t song_frame;
  void set_progress(uint32_t frame) {
    song_frame = frame;
  }

};

#endif // _DISPLAY_H_
