// sensor_acquisition_gtest.cpp

#include "sensor_acquisition/sensor_acquisition.h"

#include <gtest/gtest.h>

#include <string>

namespace sensor_acquisition
{
namespace
{

// Only write overridden: every other hook is the default.
class Minimal final : public Sensor {
  public:
    Minimal() : Sensor(0x01, 0, "MIN") {}

    bool write(Reading& out) const override {
        out.data[0]    = 42;
        out.size       = 1;
        out.sampled_us = 7;
        return true;
    }
};

// Every hook overridden, like a sensor with bring-up and retries.
class Full final : public Sensor {
  public:
    Full() : Sensor(0x02, 3, "FULL") {}

    void begin() override { ++begins; }
    void update(uint64_t now_us) override {
        ++updates;
        last_now = now_us;
    }
    bool write(Reading& out) const override {
        if (!has_reading) {
            return false;
        }
        out.size       = 0;
        out.sampled_us = last_now;
        return true;
    }
    Health health() const override { return state; }
    void   describe(char* out, size_t capacity) const override {
        snprintf(out, capacity, "full %d", updates);
    }

    int      begins      = 0;
    int      updates     = 0;
    uint64_t last_now    = 0;
    bool     has_reading = false;
    Health   state       = Health::kRetrying;
};

TEST(SensorAcquisition, DefaultsDoNothingAndReportOk) {
    Minimal       m;
    Sensor* const list[] = {&m};
    beginAll(list, 1);
    const SensorMap map = readAll(list, 1, 1000);

    ASSERT_EQ(map.count, 1u);
    const SensorEntry& e = map.entries[0];
    EXPECT_EQ(e.sensor_type, 0x01);
    EXPECT_EQ(e.port, 0);
    EXPECT_STREQ(e.name, "MIN");
    EXPECT_EQ(e.health, Health::kOk);
    ASSERT_TRUE(e.has_reading);
    EXPECT_EQ(e.reading.data[0], 42);
    EXPECT_EQ(e.reading.sampled_us, 7u);

    char text[32];
    m.describe(text, sizeof(text));
    EXPECT_EQ(std::string(text), "MIN ok");
}

TEST(SensorAcquisition, EveryHookIsCalledInOrder) {
    Full          f;
    Sensor* const list[] = {&f};
    beginAll(list, 1);
    EXPECT_EQ(f.begins, 1);

    SensorMap map = readAll(list, 1, 500);
    EXPECT_EQ(f.updates, 1);
    EXPECT_EQ(map.entries[0].health, Health::kRetrying);
    EXPECT_FALSE(map.entries[0].has_reading);

    f.has_reading = true;
    f.state       = Health::kOk;
    map           = readAll(list, 1, 900);
    EXPECT_EQ(f.updates, 2);
    EXPECT_EQ(map.entries[0].health, Health::kOk);
    ASSERT_TRUE(map.entries[0].has_reading);
    EXPECT_EQ(map.entries[0].reading.sampled_us, 900u); // update ran before write
}

TEST(SensorAcquisition, FindByKindAndPort) {
    Minimal       m;
    Full          f;
    Sensor* const list[] = {&m, &f};
    const SensorMap map = readAll(list, 2, 0);

    ASSERT_NE(map.find(0x02, 3), nullptr);
    EXPECT_STREQ(map.find(0x02, 3)->name, "FULL");
    EXPECT_STREQ(map.find(0x01, 0)->name, "MIN");
    EXPECT_EQ(map.find(0x02, 0), nullptr);
    EXPECT_EQ(map.find(0x09, 3), nullptr);
}

TEST(SensorAcquisition, SensorsPastCapacityAreLeftOut) {
    Minimal       sensors_[kMaxSensors + 1];
    Sensor*       list[kMaxSensors + 1];
    for (size_t i = 0; i < kMaxSensors + 1; ++i) {
        list[i] = &sensors_[i];
    }
    EXPECT_EQ(readAll(list, kMaxSensors + 1, 0).count, kMaxSensors);
}

} // namespace
} // namespace sensor_acquisition
