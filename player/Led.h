#ifndef _LED_H_
#define _LED_H_

class Led {
  uint32_t pin;

public:
  explicit Led(uint32_t led_pin)
  : pin(led_pin)
  {}
  
  bool state = false;

  void on() {
    state = true;
    digitalWrite(pin, LOW);
  }
  
  void off() {
    state = false;
    digitalWrite(pin, HIGH);
  }
  
  void flip() {
    state ? off() : on();
  }

};

#endif // _LED_H_
