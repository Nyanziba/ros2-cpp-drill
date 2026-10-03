// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include "drill/sensor.hpp"

TEST(InheritanceTest, TemperatureSensorReturnsCorrectValue)
{
  TemperatureSensor ts;
  EXPECT_DOUBLE_EQ(ts.read(), 25.0);
}

TEST(InheritanceTest, HumiditySensorReturnsCorrectValue)
{
  HumiditySensor hs;
  EXPECT_DOUBLE_EQ(hs.read(), 60.0);
}

TEST(InheritanceTest, TemperatureSensorUsesDefaultLabel)
{
  TemperatureSensor ts;
  EXPECT_EQ(ts.label(), "Sensor");
}

TEST(InheritanceTest, HumiditySensorOverridesLabel)
{
  HumiditySensor hs;
  EXPECT_EQ(hs.label(), "HumiditySensor");
}

TEST(InheritanceTest, DispatchesPolymorphically)
{
  std::vector<std::unique_ptr<Sensor>> sensors;
  sensors.push_back(std::make_unique<TemperatureSensor>());
  sensors.push_back(std::make_unique<HumiditySensor>());

  EXPECT_DOUBLE_EQ(sensors[0]->read(), 25.0);
  EXPECT_EQ(sensors[0]->label(), "Sensor");

  EXPECT_DOUBLE_EQ(sensors[1]->read(), 60.0);
  EXPECT_EQ(sensors[1]->label(), "HumiditySensor");
}
