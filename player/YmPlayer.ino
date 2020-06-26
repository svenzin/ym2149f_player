#include "Logger.h"

#include "SdSource.h"
#include "YmrFile.h"

#include "Led.h"
#include "YM2149.h"
#include "Player.h"
#include "Display.h"

#define SERIAL_BAUD 115200
#define SERIAL_TIMEOUT_MS 100

#define PIN_INPUT PA8
#define PIN_SEED  PB0

////////////////////////////////////////////////////////////////////////////////

Led led(PC13);

YM2149 ym(PB9,                // CLOCK
          PB8,                // RESET
          PB0, PB1,           // BDIR BC1
          PA0, PA1, PA2, PA3, // D0-D3
          PA4, PA5, PA6, PA7  // D4-D7
          );

HardwareTimer t(TIM1);

Player player(&ym, &t);

SdSource sdcard(PB15, PB14, PB13, PB12); // MOSI, MISO, CLK, CS

Display dspl(PB10, PB11, 0x3C);

////////////////////////////////////////////////////////////////////////////////
namespace display {

bool enabled = true;

void info(char *song, char *author, char *comment) {
  if (enabled) {
    Serial.println();
    Serial.print("Song:    "); Serial.println(song);
    Serial.print("Author:  "); Serial.println(author);
    Serial.print("Comment: "); Serial.println(comment);
  }
}

void time(uint32_t t, uint32_t rate) {
  const uint32_t m = (t / rate) / 60;
  const uint32_t s = (t / rate) % 60;
  if (m < 10) Serial.print("0");
  Serial.print(m);
  Serial.print(":");
  if (s < 10) Serial.print("0");
  Serial.print(s);
}

void to_time(uint32_t t, uint32_t rate, char *str) {
  const uint32_t m = (t / rate) / 60;
  const uint32_t s = (t / rate) % 60;
  sprintf(str, "%u:%02u", m, s);
}

void progress(uint32_t frame, uint32_t nframe, bool rnd, bool mute, uint32_t cs, uint32_t ps) {
  if (enabled) {
    Serial.print(rnd ? "R" : "r");
    Serial.print(mute ? "M " : "  ");
    
    const uint32_t p = 40 * frame / nframe;
    Serial.print("[");
    for (int i = 0; i < p; ++i) Serial.print("=");
    Serial.print(">");
    for (int i = p; i < 40; ++i) Serial.print(" ");
    Serial.print("] ");

    time(frame, ps);
    Serial.print("/");
    time(nframe, ps);    
  
    Serial.print(" ");
    Serial.print(ps);
    Serial.print("Hz ");
    Serial.print(cs * 0.000001, 2);
    Serial.print("MHz");
  }
}

}
////////////////////////////////////////////////////////////////////////////////
namespace error {

void short_code() {
  led.on();
  delay(100);
  led.off();
  delay(200);
}

void long_code() {
  led.on();
  delay(400);
  led.off();
  delay(200);
}

void end_code() {
  delay(2000);
}

void sd_not_opened() {
  Serial.println("Failure : Couldn't read SD card");
  while (true) {
    short_code();
    short_code();
    short_code(); 
    end_code();
  }
}

void display_unavailable() {
  Serial.println("Failure : Couldn't read SD card");
  while (true) {
    short_code();
    short_code();
    long_code(); 
    end_code();
  }
}

}
////////////////////////////////////////////////////////////////////////////////

namespace Clocks {
  uint32_t rates[5] = { 2000000, 1789800, 1773400, 1750000, 1000000 };
  byte type = 0;
  void next() {
    type = (type + 1) % 5;
    ym.clk.set(rates[type]);
  }
};

namespace PlaybackRates {
  uint32_t rates[2] = { 50, 60 };
  byte type = 0;
  void next() {
    type = (type + 1) % 2;
    player.set_playback_rate(rates[type]);
  }
};

////////////////////////////////////////////////////////////////////////////////

enum Commands {
  PlayPause      = ' ',
  Next           = 'n',
  Randomize      = 'r',
  Mute           = 'm',
  ChangeClock    = 'c',
  ChangePlayback = 'v',
  
  Help    = 'h',
  Display = 'd',

  Debug = 'a',
};

void ISR_button() {
  player.next();
}

void print_help() {
  Serial.println("YM hardware player");
  Serial.println("Playback commands");
  Serial.println("    <space> Play / Pause");
  Serial.println("    <n>     Next song");
  Serial.println("    <r>     Randomize playback");
  Serial.println("    <m>     Mute");
  Serial.println("    <c>     Change YM clock speed");
  Serial.println("    <v>     Change playback speed");
  Serial.println("System commands");
  Serial.println("    <d>     Turn information display on/off");
  Serial.println("    <h>     Display this help message");
}

void change_clock_rate() {
  logger::debug("Command : Change clock");
  Clocks::next();
}

void change_playback_rate() {
  logger::debug("Command : Change playback");
  PlaybackRates::next();
}

void change_display() {
  display::enabled = !display::enabled;
  if (display::enabled) {
    logger::debug("Command : Display on");
  } else {
    logger::debug("Command : Display off");
  }
}

void process_commands() {
  switch (Serial.read()) {
    case Commands::Debug: logger::enabled = !logger::enabled; break;
    
    case Commands::Help:           print_help();           break;
    case Commands::Display:        change_display();       break;
    case Commands::PlayPause:      player.play();          break;
    case Commands::Next:           player.next();          break;
    case Commands::Randomize:      player.randomize();     break;
    case Commands::Mute:           player.mute();          break;
    case Commands::ChangeClock:    change_clock_rate();    break;
    case Commands::ChangePlayback: change_playback_rate(); break;
    default:                                               break;
  }
}

////////////////////////////////////////////////////////////////////////////////

void setup_button() {
  pinMode(PIN_INPUT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_INPUT), ISR_button, RISING);
}

int timer = 0;
void ISR_playback(HardwareTimer *t) {
  player.play_frame(t);
  
  switch(timer % player.current_playback_rate) {
    case 0: led.on();  break;
    case 1: led.off(); break;
    default:           break;
  }
  ++timer;
}

void setup() {
  randomSeed(analogRead(PIN_SEED));

  Serial.begin(SERIAL_BAUD);
  Serial.setTimeout(SERIAL_TIMEOUT_MS);

  if (!dspl.setup()) error::display_unavailable();
  dspl.logo();
  
  if (!sdcard.setup()) error::sd_not_opened();

// TODO
t.attachInterrupt(ISR_playback);
  player.setup(&sdcard);
  setup_button();
  ym.setup();

  dspl.clear();
  
  player.next();
}

char ct[6], tt[6];
void loop() {
  process_commands();
  if (player.loop()) {
    display::info(player.ymrf.header.song_name, player.ymrf.header.author_name, player.ymrf.header.song_comment);
    dspl.set_song(player.ymrf.header.song_name, player.ymrf.header.author_name, player.ymrf.header.frame_count);
  }

  display::to_time(player.frame, player.current_playback_rate, ct);
  display::to_time(player.ymrf.header.frame_count, player.current_playback_rate, tt);
  dspl.set_status(player.is_randomized,
                  player.is_mute,
                  (bool) Serial,
                  ym.clk.get(),
                  player.current_playback_rate);
  dspl.set_progress(player.frame);
  dspl.song();
  
  Serial.print("\r");
  display::progress(player.frame, player.ymrf.header.frame_count,
                    player.is_randomized, player.is_mute,
                    ym.clk.get(), player.current_playback_rate);
  Serial.print("\r");
  delay(100);
}
