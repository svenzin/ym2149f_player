#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "Logger.h"
#include "SdSource.h"

class Player {
  enum eMode {
    Play,
    Pause,
    NextSong,
    RandomSong,
  };

  eMode mode = Play;

public:
  bool is_randomized = true;
  bool is_mute = false;

  explicit Player(YM2149 *ym2149, HardwareTimer *timer)
  : ym(ym2149), t(timer)
  {}

  void play() {
    if (mode == Play) {
      logger::debug("Command : Pause");
      mode = Pause;
    } else if (mode == Pause) {
      logger::debug("Command : Play");
      mode = Play;
    }
  }

  void next() {
    if (is_randomized) {
      logger::debug("Command : Random song");
      mode = RandomSong;
    } else {
      logger::debug("Command : Next song");
      mode = NextSong;
    }
  }
  
  void randomize() {
    is_randomized = !is_randomized;
    if (is_randomized) {
      logger::debug("Command : Randomize on");
    } else {
      logger::debug("Command : Randomize off");
    }
  }

  void mute() {
    is_mute = !is_mute;
    if (is_mute) {
      logger::debug("Command : Sound off");
    } else {
      logger::debug("Command : Sound on");
    }
  }
  
private:
  SdSource *sd;

public:
uint16_t current_playback_rate = 0;
void set_playback_rate(uint16_t rate) {
  t->setOverflow(rate, HERTZ_FORMAT);
  t->refresh();
  t->resume();
  current_playback_rate = rate;
}

  void setup(SdSource *src) {
    sd = src;
    
    mode = RandomSong;

    t->setMode(1, TIMER_OUTPUT_COMPARE);
    // t->attachInterrupt(play_frame);
  }

private:
  HardwareTimer *t;
  YM2149 *ym;
  byte regs[14];
  
  void mute_ym() {
    ym->write(8, 0);
    ym->write(9, 0);
    ym->write(10, 0);
  }
public:
  YmrFile ymrf;
  uint32_t frame;

  void play_frame(HardwareTimer *t) {
    switch (mode) {
      case Play: {
        if (ymrf.available()) {
          ymrf.read(regs, 14);
          if (is_mute) {
            mute_ym();
          } else {
            for (byte r = 0; r < 13; ++r) ym->write(r, regs[r]);
            if (regs[13] != 0xFF) ym->write(13, regs[13]);
          }
        } else {
          next();
        }
        ++frame;
        break;
      }
      case Pause: {
        mute_ym();
        break;
      }
    }
  }

  bool play_file() {
    mute_ym();
    ymrf.read_ymr();
    if (ymrf.is_valid) {
      ym->clk.set(ymrf.header.clock_rate);
      set_playback_rate(ymrf.header.playback_rate);
      frame = 0;
      mode = Play;
      return true;
    }
    return false;
  }
  
  bool loop() {
    switch (mode) {
      case RandomSong: {
        ymrf.close();
        sd->get_random(ymrf);
        return play_file();
      }
      case NextSong: {
        ymrf.close();
        sd->get_next(ymrf);
        return play_file();
      }
    }
    return false;
  }
};

#endif // _PLAYER_H_
