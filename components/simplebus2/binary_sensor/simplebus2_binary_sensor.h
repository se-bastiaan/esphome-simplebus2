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
      void trigger(u_int16_t command, u_int16_t address) override;
      void loop();

      void set_command(uint16_t command) { this->command = command; }
      void set_address(uint16_t address) { this->address = address; }
      void set_auto_off(uint16_t auto_off) { this->auto_off = auto_off; }    
    protected:
      uint32_t timer;
      uint16_t address;
      uint16_t command;
      uint16_t auto_off;
    };

  }
}
