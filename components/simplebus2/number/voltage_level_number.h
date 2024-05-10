#pragma once
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/components/number/number.h"
#include "../simplebus2.h"

namespace esphome
{
    namespace simplebus2
    {
        class VoltageLevelNumber : public number::Number, public Parented<Simplebus2Component>
        {
        public:
            VoltageLevelNumber() = default;

        protected:
            void control(float value) override;
        };
    }
}