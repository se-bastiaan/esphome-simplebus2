#pragma once

#include <utility>
#include <vector>
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/automation.h"

namespace esphome
{
  namespace simplebus2
  {

    struct Simplebus2Data
    {
      uint16_t command;
      uint16_t address;
    };

    class Simplebus2Listener
    {
    public:
      virtual void trigger(u_int16_t command, u_int16_t address);
      void loop() {}
    };

    struct Simplebus2ComponentStore
    {
      static void gpio_intr(Simplebus2ComponentStore *arg);

      // If pin triggered
      volatile bool pin_triggered = false;

      ISRInternalGPIOPin rx_pin;
    };

    class Simplebus2Component : public Component
    {
    public:
      void setup() override;
      void dump_config() override;
      void loop() override;

      void message_decode(std::vector<uint16_t> src);
      void dump(std::vector<uint16_t>) const;
      void sending_loop();
      void register_listener(Simplebus2Listener *listener);
      void send_command(Simplebus2Data data);

      void set_rx_pin(InternalGPIOPin *pin) { this->rx_pin = pin; }
      void set_tx_pin(InternalGPIOPin *pin) { this->tx_pin = pin; }
      void set_gain(int gain) { this->gain = gain; }
      void set_voltage_level(int voltage_level) { this->voltage_level = voltage_level; }
      int get_gain() { return this->gain; }
      int get_voltage_level() { return this->voltage_level; }
      void set_event(const char *event) { this->event = event; }

      void set_opv_gain(int gain);
      void set_comparator_voltage_limit(int voltage);

    protected:
      InternalGPIOPin *rx_pin;
      InternalGPIOPin *tx_pin;
      int voltage_level;
      int gain;
      const char *event;
      Simplebus2ComponentStore store_;

      unsigned long last_pause_time = 0;
      int message_bit_array [18];
      volatile int message_position = 0;
      volatile bool message_started = false;
      volatile int message_code = -1;
      volatile int message_addr;

      HighFrequencyLoopRequester high_freq_;
      std::vector<uint16_t> temp_;
      std::vector<Simplebus2Listener *> listeners_{};

      void set_pot_resistance(bool isGain, float resistance);
      void activate_interrupt(bool activate);

      void process_interrupt();
      void int_to_binary(unsigned int input, int start_pos, int no_of_bits, int *bits);
      unsigned int binary_to_int(int start_pos, int no_of_bits, int bin_array[]);
    };

    template <typename... Ts>
    class Simplebus2SendAction : public Action<Ts...>
    {
    public:
      Simplebus2SendAction(Simplebus2Component *parent) : parent_(parent) {}
      TEMPLATABLE_VALUE(uint16_t, command)
      TEMPLATABLE_VALUE(uint16_t, address)

      void play(Ts... x)
      {
        Simplebus2Data data{};
        data.command = this->command_.value(x...);
        data.address = this->address_.value(x...);
        this->parent_->send_command(data);
      }

    protected:
      Simplebus2Component *parent_;
    };

  }
}
