#ifndef _YM2149_H_
#define _YM2149_H_

class YM2149 {
  
  class Bus {
    uint32_t bdir;
    uint32_t bc1;
    
  public:
    explicit Bus(uint32_t pin_bdir, uint32_t pin_bc1)
    : bdir(pin_bdir), bc1(pin_bc1)
    {}
    
    void inactive() {
      digitalWrite(bdir, LOW);
      digitalWrite(bc1, LOW);
      // GPIOB->BRR = B11;
    }
    
    void read() {
      digitalWrite(bdir, LOW);
      digitalWrite(bc1, HIGH);
    }
    
    void write() {
      digitalWrite(bdir, HIGH);
      digitalWrite(bc1, LOW);
    }
    
    void address() {
      // Switching each pin individually means the order is important
      // We do not want to trigger a write mode trying to setup the address mode
      digitalWrite(bc1, HIGH);
      digitalWrite(bdir, HIGH);
    }

    void setup() {
      pinMode(bdir, OUTPUT);
      pinMode(bc1, OUTPUT);
    }
  };
  
  class Data {
    uint32_t data[8];
    
  public:
    explicit Data(uint32_t d0, uint32_t d1, uint32_t d2, uint32_t d3,
                  uint32_t d4, uint32_t d5, uint32_t d6, uint32_t d7)
    : data { d0, d1, d2, d3, d4, d5, d6, d7 }
    {}

    void write(byte b) {
      for (byte pin = 0; pin < 8; ++pin) {
        digitalWrite(data[pin], (b & (1 << pin)) ? HIGH : LOW);
      }
    }

    void setup() {
      for (byte pin = 0; pin < 8; ++pin) {
        pinMode(data[pin], OUTPUT);
      }
    }
  };
  
  class Clock {
    uint32_t clk;
    uint32_t rate;
  
  public:
    explicit Clock(uint32_t pin_clock)
    : clk(pin_clock)
    {}
  
    uint32_t get() {
      return rate;
    }
  
    void set(uint32_t clock_rate) {
      rate = clock_rate;
      analogWriteFrequency(rate);
      analogWriteResolution(8);
      analogWrite(clk, 127);
    }
    
    void setup() {
      pinMode(clk, OUTPUT);
    }
  };

  uint32_t pin_reset;
  Bus bus;
  Data data;

public:
// TODO
  Clock clk;

  explicit YM2149(uint32_t clock,
                  uint32_t reset,
                  uint32_t bdir, uint32_t bc1,
                  uint32_t data0, uint32_t data1, uint32_t data2, uint32_t data3,
                  uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7)
  : clk(clock),
    pin_reset(reset),
    bus(bdir, bc1),
    data(data0, data1, data2, data3, data4, data5, data6, data7)
  {}
  
  void write(byte address, byte value) {
    bus.inactive();
  
    data.write(address);
    bus.address();
    delayMicroseconds(1);
    bus.inactive();
    
    data.write(value);
    delayMicroseconds(1);
    bus.write();
    delayMicroseconds(1);
    bus.inactive();
  }

  void reset() {
    bus.inactive();
    delayMicroseconds(1);
    digitalWrite(pin_reset, LOW);
    delayMicroseconds(100);
    digitalWrite(pin_reset, HIGH);
    delayMicroseconds(100);
  }

  void setup() {
    clk.setup();
    pinMode(pin_reset, OUTPUT);
    bus.setup();
    data.setup();
    
    reset();
  }
};

#endif // _YM2149_H_
