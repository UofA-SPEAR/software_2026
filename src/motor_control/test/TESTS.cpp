#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <std_msgs/msg/float32.hpp>
#include <std_msgs/msg/float64.hpp>
#include <chrono>
#include <thread>
#include "../src/WheelMotorNode.cpp"
#include "TestWheelMotor.cpp"
#include "motor_control/EtherCatConfig.hpp"
#include "motor_control/MotorConfig.hpp"
#include "motor_control/action/wheel_fine_control.hpp"

class WheelMotorNodeTest : public ::testing::Test
{
protected:
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

   auto test_node        = std::make_shared<rclcpp::Node>("Test_Node_Velocity_Topic_Followed");
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
      [&received_velocity, &got_response](std_msgs::msg::Float32::SharedPtr float_msg) {
      received_velocity = float_msg->data;
      got_response      = true;
   });

   auto publisher = wheel_motor_node->create_publisher<std_msgs::msg::Float32>("test_wheel/cmd_vel", 10);
   auto message   = std_msgs::msg::Float32();
   message.data = TARGET_VELOCITY;
   publisher->publish(message);

   auto start = std::chrono::steady_clock::now();
   auto end   = start + std::chrono::milliseconds(500);

   while (std::chrono::steady_clock::now() < end)
   {
      exec.spin_some(std::chrono::milliseconds(20));
   }

   ASSERT_TRUE(got_response) << "Never received a response from the node";

   ASSERT_NEAR(received_velocity, TARGET_VELOCITY, 0.01f);
}

// Modified from ChatGPT generation on 2025-Nov-24
TEST_F(WheelMotorNodeTest, position_supercedes_velocity)
{
   const float TARGET_POSITION    = 2.5f;  // rad
   const float INITIAL_VELOCITY   = 1.0f;  // rad/s
   const float ZERO_VELOCITY      = 0.0f;
   const float POSITION_TOLERANCE = 0.01f; // rad


   auto test_node        = std::make_shared<rclcpp::Node>("Test_Node_Pos_Supercedes");
   auto wheel_motor_node = wheel_motor_control::WheelMotorNode::create(
      "test_wheel",
      rclcpp::NodeOptions(),
      EtherCatConfig(),
      MotorConfig(),
      std::make_unique<TestWheelMotor>());

   rclcpp::executors::SingleThreadedExecutor exec;
   exec.add_node(test_node);
   exec.add_node(wheel_motor_node);


   float received_position = 0.0f;
   bool  got_position      = false;

   auto position_sub = test_node->create_subscription<std_msgs::msg::Float64>(
      "test_wheel/current_position",
      10,
      [&](const std_msgs::msg::Float32::SharedPtr msg) {
      received_position = msg->data;
      got_position      = true;
   });

   auto vel_pub = wheel_motor_node->create_publisher<std_msgs::msg::Float32>(
      "test_wheel/cmd_vel", 10);

   std_msgs::msg::Float32 vel_msg;
   vel_msg.data = INITIAL_VELOCITY;
   vel_pub->publish(vel_msg);

   // Let the wheel spin up for a bit...
   auto start = std::chrono::steady_clock::now();
   auto end   = start + std::chrono::milliseconds(300);
   while (std::chrono::steady_clock::now() < end)
   {
      exec.spin_some(std::chrono::milliseconds(20));
   }

   // Send a goal to move to TARGET_POSITION +- POSITION_TOLERANCE
   auto action_client = rclcpp_action::create_client<motor_control::action::WheelFineControl>(test_node, "test_wheel/fine_control");
   if (!action_client->wait_for_action_server(std::chrono::seconds(2)))
   {
      FAIL() << "Action server not available after waiting";
   }
   motor_control::action::WheelFineControl::Goal goal;
   goal.target_position = TARGET_POSITION;
   goal.max_velocity    = INITIAL_VELOCITY * 10; // need some kind of limit
   goal.tolerance       = POSITION_TOLERANCE;
   goal.timeout         = 5.0f;                  // 5 seconds timeout


   rclcpp_action::Client<motor_control::action::WheelFineControl>::SendGoalOptions options;
   auto goal_future_handle = action_client->async_send_goal(goal, options);
   auto goal_handle        = goal_future_handle.get();
   ASSERT_NE(goal_handle, nullptr) << "Goal was rejected by server";
   // Publish 0 velocity so it doesn't spin back up after the goal is finished
   vel_msg.data = ZERO_VELOCITY;
   vel_pub->publish(vel_msg);
   // Wait for the result
   auto result_future = action_client->async_get_result(goal_handle);
   auto result        = result_future.get();
   ASSERT_EQ(result.code, rclcpp_action::ResultCode::SUCCEEDED) << "Action did not succeed";



   // validate we reached the target
   ASSERT_TRUE(got_position)
      << "Never received test_wheel/current_position message";

   EXPECT_NEAR(received_position, TARGET_POSITION, POSITION_TOLERANCE)
      << "Wheel did not reach target position within tolerance";
}


int main(int argc, char **argv)
{
   testing::InitGoogleTest(&argc, argv);
   rclcpp::init(argc, argv);
   int ret = RUN_ALL_TESTS();
   rclcpp::shutdown();
   return ret;
}
