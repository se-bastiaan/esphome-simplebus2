#include "simplebus2.h"
#include "esphome/core/log.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome/core/application.h"
#include <Arduino.h>
#include <Wire.h>

#define SDA_1 6
#define SCL_1 7
#define SDA_2 9
#define SCL_2 10

#define MCP4017_MAX_RESISTANCE 100000
#define MCP4017_I2C_ADDRESS 0x2F
#define I2C_FREQ 100000
#define OPV_FIXED_GAIN_RESISTOR 100000

namespace esphome
{
  namespace simplebus2
  {

    static const char *const TAG = "simplebus2";

    void Simplebus2Component::setup()
    {
      ESP_LOGCONFIG(TAG, "Setting up Simplebus2");

      this->rx_pin->setup();
      this->tx_pin->setup();

      ledcSetup(0, 25000, 8);
      ledcAttachPin(this->tx_pin->get_pin(), 0);

      set_opv_gain(gain);
      set_comparator_voltage_limit(this->voltage_level);

      auto &s = this->store_;
      s.filter_us = this->filter_us;

      this->high_freq_.start();

      s.rx_pin = this->rx_pin->to_isr();

      this->activate_interrupt(true);

      for (auto &listener : listeners_)
      {
        listener->turn_off(&listener->timer);
      }

      ESP_LOGCONFIG(TAG, "Setup for Simplebus2 complete");
    }

    void Simplebus2Component::dump_config()
    {
      ESP_LOGCONFIG(TAG, "Simplebus2:");
      LOG_PIN("  Pin RX: ", this->rx_pin);
      LOG_PIN("  Pin TX: ", this->tx_pin);
      ESP_LOGCONFIG(TAG, "  Voltage level: %i", this->voltage_level);
      ESP_LOGCONFIG(TAG, "  Gain: %i", this->gain);
      ESP_LOGCONFIG(TAG, "  Filter: %ius", this->filter_us);
      ESP_LOGCONFIG(TAG, "  Idle:   %ius", this->idle_us);
      if (strcmp(this->event, "esphome.none") != 0)
      {
        ESP_LOGCONFIG(TAG, "  Event: %s", this->event);
      }
      else
      {
        ESP_LOGCONFIG(TAG, "  Event: disabled");
      }
    }

    void Simplebus2Component::loop()
    {
      uint32_t now_millis = millis();
      for (auto &listener : listeners_)
      {
        if (listener->timer && now_millis > listener->timer)
        {
          listener->turn_off(&listener->timer);
        }
      }

      if (this->message_started)
      {
        if (micros() - this->last_bus_bit_time > 1500)
        {
          this->activate_interrupt(true);
        }
      }
      else
      {
        this->activate_interrupt(true);
      }

      if (this->store_.pin_triggered)
      {
        this->process_interrupt();
      }

      if (this->message_code > 0 && this->message_code != 63 && this->message_addr != 255)
      {
        ESP_LOGD(TAG, "Received command %i, address %i", this->message_code, this->message_addr);

        if (strcmp(this->event, "esphome.none") != 0)
        {
          ESP_LOGD(TAG, "Send event to home assistant on %s", this->event);
          auto capi = new esphome::api::CustomAPIDevice();
          capi->fire_homeassistant_event(this->event, {{"command", std::to_string(id(this->message_code))}, {"address", std::to_string(id(this->message_addr))}});
        }
        for (auto &listener : listeners_)
        {
          if (listener->command == this->message_code && listener->address == this->message_addr)
          {
            ESP_LOGD(TAG, "Binary sensor fired! %i %i", listener->command, listener->address);
            listener->turn_on(&listener->timer, listener->auto_off);
          }
        }

        this->message_code = -1;
      }
    }

    void IRAM_ATTR HOT Simplebus2ComponentStore::gpio_intr(Simplebus2ComponentStore *arg)
    {
      unsigned long interrupt_time = micros();

      const uint32_t time_since_change = interrupt_time - arg->last_interrupt_time;
      if (time_since_change <= arg->filter_us)
      {
        return;
      }

      if (!arg->pin_triggered)
      {
        arg->pin_triggered = true;
      }

      arg->last_interrupt_time = interrupt_time;
    }

    void Simplebus2Component::process_interrupt()
    {
      auto &s = this->store_;

      const uint32_t now = micros();
      uint32_t pause_time = now - this->pause_time;

      if (pause_time > 18000 && this->message_started)
      {
        ESP_LOGD(TAG, "Resetting preamble - %i", pause_time);
        this->message_started = false;
      }

      if (pause_time >= 16000 && pause_time <= 18000)
      {
        ESP_LOGD(TAG, "Preamble - %i", pause_time);
        this->message_started = true;
        this->message_position = 0;
      }

      if (this->message_started)
      {
        switch (pause_time)
        {
        case 2400 ... 3700:
        {
          ESP_LOGD(TAG, "0 - %i", pause_time);
          this->message_bit_array[this->message_position] = 0;
          this->last_bus_bit_time = micros();
          this->activate_interrupt(false);
          this->message_position++;
          break;
        }
        case 5000 ... 7200:
        {
          ESP_LOGD(TAG, "1 - %i", pause_time);
          this->message_bit_array[this->message_position] = 1;
          this->last_bus_bit_time = micros();
          this->activate_interrupt(false);
          this->message_position++;
          break;
        }
        default:
        {
          break;
        }
        }
      }

      if (this->message_position == 18)
      {
        this->message_started = false;
        this->message_position = 0;

        unsigned int message_code = binary_to_int(0, 6, this->message_bit_array);
        ESP_LOGD(TAG, "Message Code %u", message_code);
        unsigned int message_addr = binary_to_int(6, 8, this->message_bit_array);
        ESP_LOGD(TAG, "Message Addr %u", message_addr);
        int message_checksum = binary_to_int(14, 4, this->message_bit_array);

        int checksum = 0;
        checksum += __builtin_popcount(message_code);
        checksum += __builtin_popcount(message_addr);

        if (checksum == message_checksum)
        {
          this->message_code = message_code;
          this->message_addr = message_addr;
        }
        else
        {
          ESP_LOGD(TAG, "Incorrect checksum");
          this->message_code = -1;
        }
      }

      this->pause_time = now;
      s.pin_triggered = false;
    }

    void Simplebus2Component::register_listener(Simplebus2Listener *listener)
    {
      this->listeners_.push_back(listener);
    }

    void send_pwm()
    {
      ledcWrite(0, 50);
      delay(3);
      ledcWrite(0, 0);
    }

    void send_message(bool bitToSend)
    {
      if (bitToSend)
      {
        send_pwm();
        delay(6);
      }
      else
      {
        send_pwm();
        delay(3);
      }
    }

    void send_message_start()
    {
      send_pwm();
      delay(17);
    }

    void Simplebus2Component::send_command(Simplebus2Data data)
    {
      ESP_LOGD(TAG, "Sending command %i, address %i", data.command, data.address);

      this->activate_interrupt(false);

      int msgArray[18];
      int checksum = 0;

      checksum += __builtin_popcount(data.command);
      checksum += __builtin_popcount(data.address);
      Serial.print(checksum);
      Serial.println(" ");
      int_to_binary(data.command, 0, 6, msgArray);
      int_to_binary(data.address, 6, 8, msgArray);
      int_to_binary(checksum, 14, 4, msgArray);
      send_message_start();
      for (int i = 0; i < 18; i++)
      {
        send_message(msgArray[i]);
      }
      send_pwm();
      this->activate_interrupt(true);
    }

    void Simplebus2Component::activate_interrupt(bool activate)
    {
      if (activate && !this->interrupt_attached)
      {
        this->interrupt_attached = true;
        this->rx_pin->attach_interrupt(Simplebus2ComponentStore::gpio_intr, &this->store_, gpio::INTERRUPT_ANY_EDGE);
      }
      else if (this->interrupt_attached)
      {
        this->interrupt_attached = false;
        this->rx_pin->detach_interrupt();
      }
    }

    void Simplebus2Component::set_pot_resistance(int i2cNumber, float resistance)
    {
      int sclPin = -1;
      int sdaPin = -1;
      switch (i2cNumber)
      {
      case 1: // Gain
      {
        sdaPin = SDA_1;
        sclPin = SCL_1;
        break;
      }
      case 2: // Voltage comparator
      {
        sdaPin = SDA_2;
        sclPin = SCL_2;
        break;
      }
      default:
      {
        break;
      }
      }
      Wire.begin(sdaPin, sclPin, I2C_FREQ);
      Wire.write(0b111111111);
      Wire.endTransmission(false);
      Wire.write(0);
      Wire.endTransmission();

      if (resistance < 0 || resistance > MCP4017_MAX_RESISTANCE)
      {
        return;
      }
      byte value = round((float(resistance - 325) / float(MCP4017_MAX_RESISTANCE)) * 127.0);

      Wire.beginTransmission(MCP4017_I2C_ADDRESS);
      Wire.write(value);
      Wire.endTransmission();
      Wire.end();
    }

    // Set gain factor of the OPV
    void Simplebus2Component::set_opv_gain(int gain)
    {
      ESP_LOGD(TAG, "Gain to set: %i", gain);
      // Gain = 1+(R2/R1)
      if (gain < 2)
      {
        return;
      }
      float resistorValue = float(OPV_FIXED_GAIN_RESISTOR) / (gain - 1);
      ESP_LOGD(TAG, "Gain resistor value: %f", resistorValue);
      set_pot_resistance(1, resistorValue);
    }

    // Set ref voltage of the comparator in [mV]
    void Simplebus2Component::set_comparator_voltage_limit(int voltage)
    {
      ESP_LOGD(TAG, "Voltage to set: %i", voltage);
      if (voltage < 100 || voltage > 1500)
      {
        return;
      }
      float resistorValue = (float(voltage) * float(OPV_FIXED_GAIN_RESISTOR)) / (3300.0 - float(voltage));
      ESP_LOGD(TAG, "Voltage limit resistor value: %f", resistorValue);
      set_pot_resistance(2, resistorValue);
    }

    void Simplebus2Component::int_to_binary(unsigned int input, int start_pos, int no_of_bits, int *bits)
    {
      unsigned int mask = 1;
      int zeroedstart_pos = start_pos - 1;
      for (int i = start_pos; i < no_of_bits + start_pos; i++)
      {
        bits[i] = (input & (1 << (i - start_pos))) != 0;
      }
    }

    unsigned int Simplebus2Component::binary_to_int(int start_pos, int no_of_bits, int bin_array[])
    {
      unsigned int integer = 0;
      unsigned int mask = 1;
      for (int i = start_pos; i < no_of_bits + start_pos; i++)
      {
        if (bin_array[i])
        {
          integer |= mask;
        }
        mask = mask << 1;
      }
      return integer;
    }
  }
}
