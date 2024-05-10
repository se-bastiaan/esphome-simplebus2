#include "simplebus2.h"
#include "esphome/core/log.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome/core/application.h"
#include <Arduino.h>
#include <Wire.h>

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

      set_opv_gain(this->gain);
      set_comparator_voltage_limit(this->voltage_level);

      auto &s = this->store_;

      this->high_freq_.start();

      s.rx_pin = this->rx_pin->to_isr();

      this->rx_pin->attach_interrupt(Simplebus2ComponentStore::gpio_intr, &this->store_, gpio::INTERRUPT_RISING_EDGE);

      ESP_LOGCONFIG(TAG, "Setup for Simplebus2 complete");
    }

    void Simplebus2Component::dump_config()
    {
      ESP_LOGCONFIG(TAG, "Simplebus2:");
      LOG_PIN("  Pin RX: ", this->rx_pin);
      LOG_PIN("  Pin TX: ", this->tx_pin);
      ESP_LOGCONFIG(TAG, "  Voltage level: %i", this->voltage_level);
      ESP_LOGCONFIG(TAG, "  Gain: %i", this->gain);
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
      for (auto &listener : listeners_)
      {
        listener->loop();
      }

      if (this->store_.pin_triggered)
      {
        this->process_interrupt();
      }

      if (this->message_code > 0)
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
          listener->trigger(this->message_code, this->message_addr);
        }

        this->message_code = -1;
      }
    }

    void IRAM_ATTR HOT Simplebus2ComponentStore::gpio_intr(Simplebus2ComponentStore *arg)
    {
      ESP_LOGD(TAG, "gpio_intr");
      if (!arg->pin_triggered)
      {
        arg->pin_triggered = true;
      }
    }

    void Simplebus2Component::process_interrupt()
    {
      auto &s = this->store_;
      
      unsigned long now = micros();
      unsigned long pause_time = now - this->last_pause_time;

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
        case 2000 ... 4900:
        {
          ESP_LOGD(TAG, "0 - %i", pause_time);
          this->message_bit_array[this->message_position] = 0;
          this->message_position++;
          break;
        }
        case 5000 ... 9000:
        {
          ESP_LOGD(TAG, "1 - %i", pause_time);
          this->message_bit_array[this->message_position] = 1;
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

      this->last_pause_time = now;
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

      this->rx_pin->detach_interrupt();

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

      this->rx_pin->attach_interrupt(Simplebus2ComponentStore::gpio_intr, &this->store_, gpio::INTERRUPT_RISING_EDGE);
    }

    void Simplebus2Component::set_pot_resistance(boolean isGain, float resistance)
    {
      if (resistance < 0 || resistance > MCP4017_MAX_RESISTANCE)
      {
        return;
      }

      // pins for gain
      int sdaPin = 6;
      int sclPin = 7;
      if (!isGain)
      {
        // pins for voltage comparator
        sdaPin = 9;
        sclPin = 10;
      }

      int value = round((float((resistance - 325.0) / float(MCP4017_MAX_RESISTANCE)) * 100.0) * 127.0);

      // Reset sequence for the MCP4017
      Wire.begin(sdaPin, sclPin, I2C_FREQ);
      Wire.beginTransmission(0b111111111);
      Wire.endTransmission();

      ESP_LOGD(TAG, "Write value of MCP4017 %s: %i", (isGain ? "gain" : "volt."), value);

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
      set_pot_resistance(true, resistorValue);
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
      set_pot_resistance(false, resistorValue);
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
