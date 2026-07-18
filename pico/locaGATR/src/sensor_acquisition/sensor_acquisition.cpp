// sensor_acquisition.cpp

#include "sensor_acquisition/sensor_acquisition.h"

#include <stdio.h>

namespace sensor_acquisition
{

void Sensor::describe(char* out, size_t capacity) const {
    snprintf(out, capacity, "%s %s", name_, brainlink::healthName(health()));
}

const SensorEntry* SensorMap::find(uint8_t sensor_type, uint8_t port) const {
    for (size_t i = 0; i < count; ++i) {
        if (entries[i].sensor_type == sensor_type && entries[i].port == port) {
            return &entries[i];
        }
    }
    return nullptr;
}

void beginAll(Sensor* const* sensors, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        sensors[i]->begin();
    }
}

SensorMap readAll(Sensor* const* sensors, size_t count, uint64_t now_us) {
    SensorMap map;
    map.count = count < kMaxSensors ? count : kMaxSensors;
    for (size_t i = 0; i < map.count; ++i) {
        Sensor&      sensor = *sensors[i];
        SensorEntry& entry  = map.entries[i];
        sensor.update(now_us);
        entry.sensor_type = sensor.sensorType();
        entry.port        = sensor.port();
        entry.name        = sensor.name();
        entry.health      = sensor.health();
        entry.has_reading = sensor.write(entry.reading);
    }
    return map;
}

} // namespace sensor_acquisition
