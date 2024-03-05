#include "simplebus2_binary_sensor.h"

namespace esphome
{
  namespace simplebus2
  {

    static const char *const TAG = "simplebus2.binary";

    void Simplebus2BinarySensor::turn_on(uint32_t *timer, uint16_t auto_off)
    {
      this->publish_state(true);
      if (auto_off > 0)
      {
        *timer = millis() + (auto_off * 1000);
      }
    }

    void Simplebus2BinarySensor::turn_off(uint32_t *timer)
    {
      this->publish_state(false);
      *timer = 0;
    }

  }
}
