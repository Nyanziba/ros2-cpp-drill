#include "drill/sensor.hpp"

std::string Sensor::label() const
{
  return "Sensor";
}

double TemperatureSensor::read() const
{
  return 25.0;
}

double HumiditySensor::read() const
{
  return 60.0;
}

std::string HumiditySensor::label() const
{
  return "HumiditySensor";
}
