// I AM NOT DONE

#include <algorithm>
#include <cmath>
#include <string>

#include "drill/velocity_filter.hpp"

namespace drill
{

/// 絶対値を上限で切る。
class ClampFilter : public VelocityFilter
{
public:
  void initialize(const rclcpp::Node::SharedPtr & node, const std::string & name) override
  {
    max_linear_ = std::abs(node->declare_parameter(name + ".max_linear", 1.0));
    max_angular_ = std::abs(node->declare_parameter(name + ".max_angular", 2.0));
  }

  geometry_msgs::msg::Twist filter(
    const geometry_msgs::msg::Twist & command, double /*dt_seconds*/) override
  {
    // TODO: command の linear.x を [-max_linear_, +max_linear_] に、
    //       angular.z を [-max_angular_, +max_angular_] に切って返すこと（絶対値を上限で切る）。
    //       負の向きも切ること。linear.x と angular.z 以外は command のままでよい。
    //       std::clamp（<algorithm>）が使える。
    return command;
  }

private:
  double max_linear_{1.0};
  double max_angular_{2.0};
};

/// 前回の出力からの変化を、加速度 × dt までに制限する。
class RateLimitFilter : public VelocityFilter
{
public:
  void initialize(const rclcpp::Node::SharedPtr & node, const std::string & name) override
  {
    max_linear_acceleration_ =
      std::abs(node->declare_parameter(name + ".max_linear_acceleration", 0.5));
    max_angular_acceleration_ =
      std::abs(node->declare_parameter(name + ".max_angular_acceleration", 1.0));
  }

  geometry_msgs::msg::Twist filter(
    const geometry_msgs::msg::Twist & command, double dt_seconds) override
  {
    // 最初の 1 回は、0 からの変化として扱う（previous_ は 0 で始まる）。
    previous_.linear.x = limit_step(
      previous_.linear.x, command.linear.x, max_linear_acceleration_ * dt_seconds);
    previous_.angular.z = limit_step(
      previous_.angular.z, command.angular.z, max_angular_acceleration_ * dt_seconds);
    return previous_;
  }

private:
  static double limit_step(double previous, double target, double max_step)
  {
    return previous + std::clamp(target - previous, -max_step, max_step);
  }

  double max_linear_acceleration_{0.5};
  double max_angular_acceleration_{1.0};
  geometry_msgs::msg::Twist previous_;
};

/// 絶対値がしきい値未満なら 0 にする。
class DeadbandFilter : public VelocityFilter
{
public:
  void initialize(const rclcpp::Node::SharedPtr & node, const std::string & name) override
  {
    linear_threshold_ = node->declare_parameter(name + ".linear_threshold", 0.05);
    angular_threshold_ = node->declare_parameter(name + ".angular_threshold", 0.1);
  }

  geometry_msgs::msg::Twist filter(
    const geometry_msgs::msg::Twist & command, double /*dt_seconds*/) override
  {
    auto result = command;
    if (std::abs(command.linear.x) < linear_threshold_) {
      result.linear.x = 0.0;
    }
    if (std::abs(command.angular.z) < angular_threshold_) {
      result.angular.z = 0.0;
    }
    return result;
  }

private:
  double linear_threshold_{0.05};
  double angular_threshold_{0.1};
};

}  // namespace drill

// TODO: 3 つのクラスを、pluginlib のプラグインとして書き出すこと。
//       <pluginlib/class_list_macros.hpp> を include し、
//       PLUGINLIB_EXPORT_CLASS(<クラス>, <基底クラス>) を 3 行書く。
//       書き出さないと、ライブラリはビルドできても pluginlib がクラスを作れない。
//         ClampFilter / RateLimitFilter / DeadbandFilter を、基底クラス drill::VelocityFilter として。
