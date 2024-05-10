#include "gain_number.h"

namespace esphome
{
    namespace simplebus2
    {

        void GainNumber::control(float value)
        {
            this->parent_->set_opv_gain((int) value);
            this->publish_state(value);
        }

    }
}