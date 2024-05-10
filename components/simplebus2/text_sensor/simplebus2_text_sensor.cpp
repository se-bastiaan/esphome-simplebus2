#include "simplebus2_text_sensor.h"

namespace esphome
{
  namespace simplebus2
  {

    static const char *const TAG = "simplebus2.text";

    void Simplebus2TextSensor::trigger(u_int16_t command, u_int16_t address)
    {

      std::string message = "{\"address\":" + std::to_string(address) + ", \"command\":" + std::to_string(command) + "}";
      this->publish_state(message);
    }

  }
}
