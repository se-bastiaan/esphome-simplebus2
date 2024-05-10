#pragma once
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "../simplebus2.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome
{
  namespace simplebus2
  {
    class Simplebus2TextSensor : public text_sensor::TextSensor, public Simplebus2Listener
    {
    public:
      void trigger(u_int16_t command, u_int16_t addresss) override;
    };
  }
}
