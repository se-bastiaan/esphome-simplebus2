#include "voltage_level_number.h"

namespace esphome
{
    namespace simplebus2
    {

        void VoltageLevelNumber::control(float value)
        {
            this->parent_->set_comparator_voltage_limit((int) value);
            this->publish_state(value);
        }

    }
}