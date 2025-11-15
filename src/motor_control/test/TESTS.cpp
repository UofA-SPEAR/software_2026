#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float32.hpp>
#include <chrono>
#include <thread>
#include "../src/WheelMotorNode.cpp"
#include "TestWheelMotor.cpp"
#include "motor_control/EtherCatConfig.hpp"
#include "motor_control/MotorConfig.hpp"

class WheelMotorNodeTest : public ::testing::Test
{
protected:
   void SetUp() override
   {
      rclcpp::init(0, nullptr);
   }

   void TearDown() override
   {
      rclcpp::shutdown();
   }
};

TEST_F(WheelMotorNodeTest, node_creation)
{
   EtherCatConfig ethercat_config; // Assume default constructor is sufficient
   MotorConfig    motor_config;    // Assume default constructor is sufficient
   auto           wheel_motor_node = wheel_motor_control::WheelMotorNode::create(
      "test_wheel",
      rclcpp::NodeOptions(),
      ethercat_config,
      motor_config,
      std::make_unique<TestWheelMotor>());

   ASSERT_NE(wheel_motor_node, nullptr);
   ASSERT_STREQ(wheel_motor_node->get_name(), "test_wheel_wheel_motor_node");
}

TEST(package_name, a_first_test)
{
   ASSERT_EQ(4, 2 + 2);
}

int main(int argc, char **argv)
{
   testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
