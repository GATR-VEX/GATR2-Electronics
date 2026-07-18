// quadrature_encoder_sensor.h
// A tracking wheel encoder on a quadrature connector: interrupt-driven x4
// decode, the count since boot as an encoder record. Arduino only.

#pragma once
#include <stdint.h>

#include "ports.h"
#include "sensor_acquisition/sensor_acquisition.h"

namespace sensor_acquisition
{

class QuadratureEncoderSensor : public Sensor {
  public:
    struct Config {
        ports::Quadrature port;
    };

    explicit QuadratureEncoderSensor(const Config& config);

    void   begin() override;
    void   update(uint64_t now_us) override;
    bool   write(Reading& out) const override;
    Health health() const override;
    void   describe(char* out, size_t capacity) const override;

    // From this encoder's pin interrupts only.
    void step();

  private:
    Config           config_;
    bool             started_       = false;
    uint8_t          state_         = 0;
    volatile int32_t count_         = 0; // written by the interrupts
    int32_t          sampled_count_ = 0;
    uint64_t         sampled_us_    = 0;
};

} // namespace sensor_acquisition
