// 解答例
//
// 結城本 第13章 Visitor を C++ で書いたもの。
//   その1: GoF 版の二重ディスパッチ（accept → visit）
//   その2: std::variant + std::visit + overloaded イディオム
// 同じ木に対して同じ結果を返します。

#include "drill/diagnostics.hpp"

#include <string>
#include <utility>

using diag_format::group_line;
using diag_format::indent_of;
using diag_format::motor_line;
using diag_format::sensor_line;

// ---------------------------------------------------------------------------
// GoF 版: accept が 1 回目の仮想呼び出し
// ---------------------------------------------------------------------------
//
// 3 つとも本文は「visitor.visit(*this);」で字面は同じです。
// しかし *this の静的型がそれぞれ SensorCheck / MotorCheck / CheckGroup なので、
// 呼ばれる visit のオーバーロードは 3 つとも別物になります。
// これを基底クラスに 1 個書いてまとめることはできません（そこでは *this が
// DiagNode になってしまい、対応する visit が存在しない）。

void SensorCheck::accept(DiagVisitor & visitor) const
{
  visitor.visit(*this);
}

void MotorCheck::accept(DiagVisitor & visitor) const
{
  visitor.visit(*this);
}

void CheckGroup::accept(DiagVisitor & visitor) const
{
  visitor.visit(*this);
}

void CheckGroup::add(std::unique_ptr<DiagNode> child)
{
  children_.push_back(std::move(child));
}

// ---------------------------------------------------------------------------
// 訪問者その1: 集計
// ---------------------------------------------------------------------------

void FailureCountVisitor::visit(const SensorCheck & node)
{
  ++checked_count_;
  if (node.failed()) {
    ++failure_count_;
  }
}

void FailureCountVisitor::visit(const MotorCheck & node)
{
  ++checked_count_;
  if (node.failed()) {
    ++failure_count_;
  }
}

void FailureCountVisitor::visit(const CheckGroup & node)
{
  // 木をたどるのは訪問者側の仕事です（結城本の ListVisitor と同じ）。
  for (const std::unique_ptr<DiagNode> & child : node.children()) {
    child->accept(*this);
  }
}

// ---------------------------------------------------------------------------
// 訪問者その2: 整形
// ---------------------------------------------------------------------------

void TextReportVisitor::visit(const SensorCheck & node)
{
  text_ += indent_of(depth_) + sensor_line(node.name(), node.value_mv(), node.failed());
}

void TextReportVisitor::visit(const MotorCheck & node)
{
  text_ += indent_of(depth_) + motor_line(node.name(), node.fault_bits(), node.failed());
}

void TextReportVisitor::visit(const CheckGroup & node)
{
  text_ += indent_of(depth_) + group_line(node.name());
  ++depth_;
  for (const std::unique_ptr<DiagNode> & child : node.children()) {
    child->accept(*this);
  }
  --depth_;
}

// ---------------------------------------------------------------------------
// variant 版
// ---------------------------------------------------------------------------

std::size_t DiagArena::add(DiagValue value)
{
  nodes_.push_back(std::move(value));
  return nodes_.size() - 1U;
}

namespace
{

/// overloaded イディオム。
/// 複数のラムダを継承でまとめ、using で全部の operator() を可視にします。
/// C++17 では推論ガイドを自分で書く必要があります（C++20 では不要）。
template <class ... Ts>
struct overloaded : Ts ...
{
  using Ts::operator() ...;
};

template <class ... Ts>
overloaded(Ts ...) -> overloaded<Ts ...>;

int count_failures_at(const DiagArena & arena, std::size_t index)
{
  return std::visit(
    overloaded{
      [&](const SensorSample & sensor) {
        return sensor.value_mv > sensor.limit_mv ? 1 : 0;
      },
      [&](const MotorSample & motor) {
        return motor.fault_bits != 0U ? 1 : 0;
      },
      [&](const GroupSample & group) {
        int sum = 0;
        for (std::size_t child : group.children) {
          sum += count_failures_at(arena, child);
        }
        return sum;
      }},
    arena.at(index));
}

void append_report_at(
  const DiagArena & arena, std::size_t index, int depth, std::string & out)
{
  std::visit(
    overloaded{
      [&](const SensorSample & sensor) {
        out += indent_of(depth) +
          sensor_line(sensor.name, sensor.value_mv, sensor.value_mv > sensor.limit_mv);
      },
      [&](const MotorSample & motor) {
        out += indent_of(depth) + motor_line(motor.name, motor.fault_bits, motor.fault_bits != 0U);
      },
      [&](const GroupSample & group) {
        out += indent_of(depth) + group_line(group.name);
        for (std::size_t child : group.children) {
          append_report_at(arena, child, depth + 1, out);
        }
      }},
    arena.at(index));
}

}  // namespace

int count_failures(const DiagArena & arena, std::size_t root)
{
  return count_failures_at(arena, root);
}

std::string make_report(const DiagArena & arena, std::size_t root)
{
  std::string out;
  append_report_at(arena, root, 0, out);
  return out;
}
