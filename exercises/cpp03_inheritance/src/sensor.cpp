// I AM NOT DONE
//
// Sensor を継承した TemperatureSensor と HumiditySensor を実装してください。

#include "drill/sensor.hpp"

std::string Sensor::label() const
{
  return "Sensor";
}

// TODO: TemperatureSensor を実装してください。
// read() は override で 25.0 を返します。
// label() は override しません（基底クラスのデフォルト実装を使用）。
double TemperatureSensor::read() const
{
  return 0.0;
}

// TODO: HumiditySensor を実装してください。
// read() は override で 60.0 を返します。
// label() は override して "HumiditySensor" を返します。
double HumiditySensor::read() const
{
  return 0.0;
}

std::string HumiditySensor::label() const
{
  return "";
}
