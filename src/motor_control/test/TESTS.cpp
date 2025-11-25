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

TEST_F(WheelMotorNodeTest, velocity_topic_followed)
{
   const float TARGET_VELOCITY = 1.0f; // rad/s
   rclcpp::init(0, nullptr);

   auto test_node        = std::make_shared<rclcpp::Node>("Test_Node");
   auto wheel_motor_node = wheel_motor_control::WheelMotorNode::create(
      "test_wheel",
      rclcpp::NodeOptions(),
      EtherCatConfig(),
      MotorConfig(),
      std::make_unique<TestWheelMotor>());

   rclcpp::executors::SingleThreadedExecutor exec;
   exec.add_node(test_node);
   exec.add_node(wheel_motor_node);

   float received_velocity = 0.0f;
   bool  got_response      = false;

   auto velocity_subscriber = test_node->create_subscription<std_msgs::msg::Float32>(
      "test_wheel/current_vel", 10,
      [&received_velocity](std_msgs::msg::Float32::SharedPtr float_msg) {
      received_velocity = float_msg->data;
   });

   auto publisher = wheel_motor_node->create_publisher<std_msgs::msg::Float32>("test_wheel/cmd_vel", 10);
   auto message   = std_msgs::msg::Float32();
   message.data = TARGET_VELOCITY;
   publisher->publish(message);

   auto start    = std::chrono::steady_clock::now();
   auto duration = std::chrono::milliseconds(500);

   while (std::chrono::steady_clock::now() - start < duration)
   {
      exec.spin_some(std::chrono::milliseconds(50));
   }

   ASSERT_NEAR(received_velocity, TARGET_VELOCITY, 0.01f);
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
