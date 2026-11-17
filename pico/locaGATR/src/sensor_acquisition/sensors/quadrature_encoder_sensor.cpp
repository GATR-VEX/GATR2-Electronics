// quadrature_encoder_sensor.cpp

#include "sensor_acquisition/sensors/quadrature_encoder_sensor.h"

#include <Arduino.h>
#include <iterator>
#include <stdio.h>

#include "records/encoder.h"

namespace sensor_acquisition
{
namespace
{

constexpr uint8_t kChannels = std::size(ports::kQuadrature);

constexpr bool idsMatchIndex() {
    for (uint8_t i = 0; i < kChannels; ++i) {
        if (ports::kQuadrature[i].id != i) {
            return false;
        }
    }
    return true;
}
static_assert(idsMatchIndex(), "ports::kQuadrature[i].id must be i");

// Count change per (previous, new) state, indexed (prev << 2) | now. State is
// (A << 1) | B. Both lines changing at once counts 0.
constexpr int8_t kDelta[16] = {
    0, +1, -1, 0, -1, 0, 0, +1, +1, 0, 0, -1, 0, -1, +1, 0,
};

// The encoder on each quadrature connector, for the interrupt handlers.
QuadratureEncoderSensor* g_on[kChannels] = {};

void isr0() { g_on[0]->step(); }
void isr1() { g_on[1]->step(); }
void isr2() { g_on[2]->step(); }

void (*const kIsr[])() = {isr0, isr1, isr2};
static_assert(std::size(kIsr) == kChannels, "one interrupt handler per quadrature connector");

uint8_t readState(const ports::Quadrature& port) {
    return static_cast<uint8_t>((digitalRead(port.pin_a) << 1) | digitalRead(port.pin_b));
}

} // namespace

QuadratureEncoderSensor::QuadratureEncoderSensor(const Config& config)
    : Sensor(records::encoder::kSensorType, config.port.id, config.port.name), config_(config) {}

void QuadratureEncoderSensor::begin() {
    const ports::Quadrature& p = config_.port;
    if (p.id >= kChannels) {
        return;
    }
    pinMode(p.pin_a, INPUT_PULLUP);
    pinMode(p.pin_b, INPUT_PULLUP);
    state_ = readState(p);
    count_ = 0;

    g_on[p.id] = this;
    attachInterrupt(digitalPinToInterrupt(p.pin_a), kIsr[p.id], CHANGE);
    attachInterrupt(digitalPinToInterrupt(p.pin_b), kIsr[p.id], CHANGE);
    started_ = true;
}

void QuadratureEncoderSensor::step() {
    const uint8_t now = readState(config_.port);
    count_ += kDelta[(state_ << 2) | now];
    state_ = now;
}

void QuadratureEncoderSensor::update(uint64_t now_us) {
    if (!started_) {
        return;
    }
    noInterrupts();
    sampled_count_ = count_;
    sampled_us_    = now_us;
    interrupts();
}

bool QuadratureEncoderSensor::write(Reading& out) const {
    if (!started_) {
        return false;
    }
    records::encoder::write(sampled_count_, out.data.data());
    out.size       = records::encoder::kSize;
    out.sampled_us = sampled_us_;
    return true;
}

Health QuadratureEncoderSensor::health() const {
    return started_ ? Health::kOk : Health::kFailed;
}

void QuadratureEncoderSensor::describe(char* out, size_t capacity) const {
    if (started_) {
        snprintf(out, capacity, "%s=%ld", name(), static_cast<long>(sampled_count_));
    } else {
        snprintf(out, capacity, "%s not started", name());
    }
}

} // namespace sensor_acquisition
