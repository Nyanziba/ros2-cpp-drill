// I AM NOT DONE
//
// 結城本 第7章 Builder のうち、**本に載っている形**（Director + Builder）です。
// Director が手順を持ち、Builder が部品を作ります。

#include "drill/telemetry_builder.hpp"

void TelemetryDirector::construct()
{
  // TODO: builder_ に対して、決められた順で 5 回呼んでください。
  //
  //   1. make_header("robot telemetry")
  //   2. make_field("battery_voltage", 12.5)
  //   3. make_field("motor_current", 3.25)
  //   4. make_field("cpu_temperature", 41.0)
  //   5. make_footer()
  //
  // ここに書式（カンマや波かっこ）を一切書かないのが Director の仕事です。
  // 書式を知っているのは ConcreteBuilder だけです。
  (void)builder_;
}

void CsvTelemetryBuilder::make_header(const std::string & title)
{
  // TODO: text_ を "# " + title + "\n" にしてください。
  (void)title;
}

void CsvTelemetryBuilder::make_field(const std::string & key, double value)
{
  // TODO: text_ の末尾に key + "," + format_number(value) + "\n" を足してください。
  (void)key;
  (void)value;
}

void CsvTelemetryBuilder::make_footer()
{
  // TODO: text_ の末尾に "# end\n" を足してください。
}

void JsonTelemetryBuilder::make_header(const std::string & title)
{
  // TODO: text_ を次の文字列にしてください（末尾に改行は入れません）。
  //   {
  //     "title": "robot telemetry"
  //
  // C++ の文字列リテラルの中で " を書くには \" と書きます。
  //   text_ = "{\n  \"title\": \"" + title + "\"";
  (void)title;
}

void JsonTelemetryBuilder::make_field(const std::string & key, double value)
{
  // TODO: text_ の末尾に ",\n  \"" + key + "\": " + format_number(value) を足してください。
  //
  // カンマを「項目の前」に置くのがコツです。
  // 「項目の後ろ」に置くと、最後の項目に余分なカンマが残って JSON として壊れます。
  (void)key;
  (void)value;
}

void JsonTelemetryBuilder::make_footer()
{
  // TODO: text_ の末尾に "\n}\n" を足してください。
}
