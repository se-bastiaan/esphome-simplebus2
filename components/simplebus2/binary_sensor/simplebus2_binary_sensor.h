#pragma once
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "../simplebus2.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome
{
  namespace simplebus2
  {

    class Simplebus2BinarySensor : public binary_sensor::BinarySensor, public Simplebus2Listener
    {
    public:
      void turn_on(uint32_t *timer, uint16_t auto_off) override;
      void turn_off(uint32_t *timer) override;

    protected:
      uint16_t address;
      uint16_t command;
    };

  }
}
