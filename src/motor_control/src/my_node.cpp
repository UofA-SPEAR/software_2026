#include <cstdio>
#include "rclcpp/rclcpp.hpp"
#include "WheelMotorNode.cpp"
#include "motor_control/EtherCatConfig.hpp"
#include "motor_control/MotorConfig.hpp"
#include "motor_control/IWheelMotor.hpp"
#include "../test/TestWheelMotor.cpp" // for now

int main(int argc, char **argv)
{
   printf("hello world motor_control package\n");
   rclcpp::init(argc, argv);
   printf("initialized rclcpp\n");
   auto node = wheel_motor_control::WheelMotorNode::create(
      "test",
      rclcpp::NodeOptions(),
      EtherCatConfig {},
      MotorConfig {},
      std::make_unique<TestWheelMotor>()
      );
   rclcpp::spin(node);
   rclcpp::shutdown();
   return 0;
}
