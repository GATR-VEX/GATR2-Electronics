// firmware.cpp

#include "firmware.h"

#include <Arduino.h>
#include <iterator>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "brainlink/rs485.h"
#include "brainlink/streamer.h"

namespace firmware
{
namespace
{

constexpr size_t kSensorCount = std::size(cfg::kSensors);
static_assert(kSensorCount <= sensor_acquisition::kMaxSensors,
              "more sensors than a SensorMap holds");

uint16_t            g_boot_id = 0; // random per boot, so the Brain sees a restart
brainlink::Streamer g_streamer(cfg::kStreamPeriodUs);
uint32_t            g_loops  = 0; // since the last status line
uint32_t            g_frames = 0; // frames at the last status line

// One status line, cut short when full.
struct Line {
    char text[256] = "";

    void append(const char* more) {
        const size_t used = strlen(text);
        snprintf(text + used, sizeof(text) - used, "%s", more);
    }
};

uint16_t randomBootId() {
    const uint32_t x  = rp2040.hwrand32() ^ time_us_32();
    const uint16_t id = static_cast<uint16_t>(x ^ (x >> 16));
    return id != 0 ? id : 1;
}

} // namespace

void begin() {
    if (cfg::kUsbDebug) {
        Serial.begin(cfg::kUsbBaud);
    }
    pinMode(ports::kLed, OUTPUT);
    brainlink::begin(cfg::kBaud);
    sensor_acquisition::beginAll(cfg::kSensors, kSensorCount);
    g_boot_id = randomBootId();
}

sensor_acquisition::SensorMap readSensors() {
    ++g_loops;
    return sensor_acquisition::readAll(cfg::kSensors, kSensorCount, time_us_64());
}

const brainlink::Stats& streamToBrain(const sensor_acquisition::SensorMap& sensors) {
    if (const std::optional<brainlink::Frame> frame = g_streamer.next(sensors, g_boot_id, time_us_64())) {
        brainlink::send(*frame);
    }
    return g_streamer.stats();
}

void logStatus(const sensor_acquisition::SensorMap& sensors, const brainlink::Stats& link) {
    static uint32_t last_ms = 0;
    const uint32_t  now_ms  = millis();
    if (!cfg::kUsbDebug || now_ms - last_ms < cfg::kUsbDebugPeriodMs) {
        return;
    }
    const uint32_t elapsed_ms   = now_ms - last_ms;
    const uint32_t loops_per_s  = g_loops * 1000u / elapsed_ms;
    const uint32_t frames_per_s = (link.frames - g_frames) * 1000u / elapsed_ms;
    last_ms                     = now_ms;
    g_loops                     = 0;
    g_frames                    = link.frames;
    if (!Serial) {
        return;
    }

    Line line;
    snprintf(line.text, sizeof(line.text), "locagatr boot=%04X loops=%lu/s frames=%lu/s left_out=%lu",
             g_boot_id, static_cast<unsigned long>(loops_per_s),
             static_cast<unsigned long>(frames_per_s), static_cast<unsigned long>(link.left_out));
    for (size_t i = 0; i < sensors.count; ++i) {
        char text[96];
        cfg::kSensors[i]->describe(text, sizeof(text));
        line.append(" | ");
        line.append(text);
    }
    line.append("\r\n");

    const size_t n = strlen(line.text);
    if (Serial.availableForWrite() >= static_cast<int>(n)) {
        Serial.write(reinterpret_cast<const uint8_t*>(line.text), n);
    }
}

void blinkLed() { digitalWrite(ports::kLed, (millis() / cfg::kLedHalfMs) % 2 ? HIGH : LOW); }

} // namespace firmware
