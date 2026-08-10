// 解答例
//
// 結城本 第7章 Builder のうち、**本に載っている形**（Director + Builder）。
// Director が手順を持ち、Builder が部品を作ります。

#include "drill/telemetry_builder.hpp"

void TelemetryDirector::construct()
{
  // 手順だけがここにあります。カンマも波かっこも一切書きません。
  // だからこの関数は CSV でも JSON でも、まだ存在しない形式でもそのまま動きます。
  builder_.make_header("robot telemetry");
  builder_.make_field("battery_voltage", 12.5);
  builder_.make_field("motor_current", 3.25);
  builder_.make_field("cpu_temperature", 41.0);
  builder_.make_footer();
}

void CsvTelemetryBuilder::make_header(const std::string & title)
{
  text_ = "# " + title + "\n";
}

void CsvTelemetryBuilder::make_field(const std::string & key, double value)
{
  text_ += key + "," + format_number(value) + "\n";
}

void CsvTelemetryBuilder::make_footer()
{
  text_ += "# end\n";
}

void JsonTelemetryBuilder::make_header(const std::string & title)
{
  text_ = "{\n  \"title\": \"" + title + "\"";
}

void JsonTelemetryBuilder::make_field(const std::string & key, double value)
{
  // カンマを「項目の前」に置きます。後ろに置くと最後の項目に余分なカンマが残ります。
  text_ += ",\n  \"" + key + "\": " + format_number(value);
}

void JsonTelemetryBuilder::make_footer()
{
  text_ += "\n}\n";
}
