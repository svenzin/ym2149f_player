#include <SPI.h>
#include <SdFat.h>

#define PIN_LED PC13

#define PIN_MOSI PB15
#define PIN_MISO PB14
#define PIN_SCLK PB13
#define PIN_SSEL PB12

#define PIN_BUTTON PA8
#define PIN_SEED   PB0

#define PIN_CLOCK PB9
#define PIN_RESET PB8
#define PIN_BDIR  PB0
#define PIN_BC1   PB1
uint32_t datapin[8] = { PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7 };

HardwareTimer t(TIM1);

SdFat sd;
SdFile root;
SdFile f;

uint32_t songs_count;
uint16_t song_index[4096];

int timer;
uint16_t playback_rate;

enum eMode {
  Play,
  RandomSong,
} mode;

byte regs[14];

////////////////////////////////////////////////////////////////////////////////

void led_on() { digitalWrite(PIN_LED, LOW); }
void led_off() { digitalWrite(PIN_LED, HIGH); }

////////////////////////////////////////////////////////////////////////////////

void ym_bus_inactive() {
  digitalWrite(PIN_BDIR, LOW);
  digitalWrite(PIN_BC1, LOW);
}

void ym_bus_read() {
  digitalWrite(PIN_BDIR, LOW);
  digitalWrite(PIN_BC1, HIGH);
}

void ym_bus_write() {
  digitalWrite(PIN_BDIR, HIGH);
  digitalWrite(PIN_BC1, LOW);
}

void ym_bus_address() {
  // Switching each pin individually means the order is important
  // We do not want to trigger a write mode trying to setup the address mode
  digitalWrite(PIN_BC1, HIGH);
  digitalWrite(PIN_BDIR, HIGH);
}

void ym_data_write(byte b) {
  for (byte pin = 0; pin < 8; ++pin) {
    digitalWrite(datapin[pin], (b & (1 << pin)) ? HIGH : LOW);
  }
}

void ym_write(byte address, byte value) {
  ym_bus_inactive();

  ym_data_write(address);
  ym_bus_address();
  delayMicroseconds(1);
  ym_bus_inactive();
  
  ym_data_write(value);
  delayMicroseconds(1);
  ym_bus_write();
  delayMicroseconds(1);
  ym_bus_inactive();
}

void ym_reset() {
  ym_bus_inactive();
  delayMicroseconds(1);
  digitalWrite(PIN_RESET, LOW);
  delayMicroseconds(100);
  digitalWrite(PIN_RESET, HIGH);
  delayMicroseconds(100);
}

void ym_mute() {
  ym_write(8, 0);
  ym_write(9, 0);
  ym_write(10, 0);
}

void ym_setup() {
  pinMode(PIN_CLOCK, OUTPUT); // CLOCK
  pinMode(PIN_RESET, OUTPUT); // RESET
  pinMode(PIN_BDIR, OUTPUT);  // BDIR
  pinMode(PIN_BC1, OUTPUT);   // BC1
  for (byte pin = 0; pin < 8; ++pin) {
    pinMode(datapin[pin], OUTPUT);
  }
}

////////////////////////////////////////////////////////////////////////////////

void clock_set(uint32_t clock_rate) {
  analogWriteFrequency(clock_rate);
  analogWriteResolution(8);
  analogWrite(PIN_CLOCK, 127);
}

void playback_set(uint16_t rate) {
  t.setOverflow(rate, HERTZ_FORMAT);
  t.refresh();
  t.resume();
  playback_rate = rate;
}

////////////////////////////////////////////////////////////////////////////////

void ISR_playback(HardwareTimer *t) {
  if (mode == Play) {
    if (f.available()) {
      f.read(regs, 14);
      for (byte r = 0; r < 13; ++r) ym_write(r, regs[r]);
      if (regs[13] != 0xFF) ym_write(13, regs[13]);
    } else {
      mode = RandomSong;
    }
  }
  
  switch(timer % playback_rate) {
    case 0: led_on();  break;
    case 1: led_off(); break;
  }
  ++timer;
}

void ISR_button() {
  mode = RandomSong;
}

////////////////////////////////////////////////////////////////////////////////

void setup() {
  randomSeed(analogRead(PIN_SEED));

  Serial.begin(115200);
  Serial.setTimeout(1000);

  SPI.setMOSI(PIN_MOSI);
  SPI.setMISO(PIN_MISO);
  SPI.setSCLK(PIN_SCLK);
  if (!sd.begin(PIN_SSEL, SPI_FULL_SPEED) || !root.open("/")) {
    while (true) {
      led_on();
      delay(1000);
      led_off();
      delay(1000);
    }
  }
  
  songs_count = 0;
  root.rewind();
  while (f.openNext(&root)) {
    song_index[songs_count] = f.dirIndex();
    ++songs_count;
    f.close();
  }
  
  t.setMode(1, TIMER_OUTPUT_COMPARE);
  t.attachInterrupt(ISR_playback);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), ISR_button, RISING);

  ym_setup();
  ym_reset();

  mode = RandomSong;
}

////////////////////////////////////////////////////////////////////////////////

void loop() {
  if (mode == RandomSong) {
    ym_mute();

    uint32_t n = random(0, songs_count);
    f.close();
    f.open(&root, song_index[n], O_RDONLY);

    byte tag[4];
    f.read(tag, 4);
    if (memcmp(tag, "YMR1", 4) == 0) {
      uint32_t clock_rate;
      f.read(&clock_rate, 4); // frame count
      f.read(&clock_rate, 4);
      clock_set(clock_rate);
      
      uint16_t playback_rate;
      f.read(&playback_rate, 2);
      playback_set(playback_rate);

      while (f.read() != 0) {} // song name
      while (f.read() != 0) {} // author name
      while (f.read() != 0) {} // song comment

      mode = Play;
    }
  }
  delay(100);
}
