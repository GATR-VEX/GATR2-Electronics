// sensor_acquisition.h
// Step 1 of the loop. Every board sensor has one shape, Sensor: it overrides
// write and any other hook it needs; the rest do nothing. readAll updates
// every sensor and collects its health and newest reading into one
// SensorMap; later steps take the map and grab what they need. Portable.

#pragma once
#include <array>
#include <stddef.h>
#include <stdint.h>

#include "brainlink/wire.h"

namespace sensor_acquisition
{

using Health = brainlink::Health;

// A newest reading, already in its record format (records/<type>.h).
struct Reading {
    uint64_t                                       sampled_us = 0; // time_us_64()
    std::array<uint8_t, brainlink::kMaxRecordData> data{};
    size_t                                         size = 0;
};

class Sensor {
  public:
    // sensor_type: records::sensor_type of its data. port, name: its
    // connector, ports.h.
    Sensor(uint8_t sensor_type, uint8_t port, const char* name)
        : sensor_type_(sensor_type), port_(port), name_(name) {}

    Sensor(const Sensor&)            = delete;
    Sensor& operator=(const Sensor&) = delete;

    // The newest reading. False when there is none.
    virtual bool write(Reading& out) const = 0;

    // Once at boot. Short; a slow bring-up goes in update.
    virtual void begin() {}

    // Every loop: poll, bring-up, retries. Never waits on the hardware.
    virtual void update(uint64_t /*now_us*/) {}

    virtual Health health() const { return Health::kOk; }

    // Status line text. Default: name and health.
    virtual void describe(char* out, size_t capacity) const;

    uint8_t     sensorType() const { return sensor_type_; }
    uint8_t     port() const { return port_; }
    const char* name() const { return name_; }

  protected:
    ~Sensor() = default;

  private:
    uint8_t     sensor_type_;
    uint8_t     port_;
    const char* name_;
};

constexpr size_t kMaxSensors = 8;

struct SensorEntry {
    uint8_t     sensor_type = 0;
    uint8_t     port        = 0;
    const char* name        = "";
    Health      health      = Health::kStarting;
    bool        has_reading = false;
    Reading     reading;
};

// One entry per sensor, in list order.
struct SensorMap {
    std::array<SensorEntry, kMaxSensors> entries{};
    size_t                               count = 0;

    // The entry for this sensor type on this connector, or nullptr.
    const SensorEntry* find(uint8_t sensor_type, uint8_t port) const;
};

// Each sensor's begin, once at boot.
void beginAll(Sensor* const* sensors, size_t count);

// Each sensor's update, then its health and newest reading. Sensors past
// kMaxSensors are left out.
SensorMap readAll(Sensor* const* sensors, size_t count, uint64_t now_us);

} // namespace sensor_acquisition
