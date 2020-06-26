#ifndef _LOGGER_H_
#define _LOGGER_H_

namespace logger {
  
  bool enabled = false;
  
  void debug(const char *message) {
    if (enabled) {
      Serial.print("[debug] ");
      Serial.println(message);
    }
  }

}

#endif // _LOG_H_
