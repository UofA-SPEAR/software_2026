#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "motor_control/EtherCatConfig.hpp"
#include "motor_control/MotorConfig.hpp"
#include "motor_control/WheelMotorControlMode.hpp"
#include "motor_control/IWheelMotor.hpp"
#include "std_msgs/msg/float32.hpp"
#include "motor_control/action/wheel_fine_control.hpp"

/**
 * This is the frequency at which the control loop runs.
 */
#define WHEEL_MOTOR_CONTROL_FREQUENCY_NAME                     "wheel_motor_control_frequency_hz"
#define WHEEL_MOTOR_CONTROL_FREQUENCY_DEFAULT                  100

/**
 * A check is performed while goals are active to see if the goal has been completed.
 * The check is performed with a frequency defined by WHEEL_MOTOR_GOAL_COMPLETION_CHECK_FREQUENCY_NAME.
 * In order for the goal to be completed, the check must return true for longer than the duration defined by WHEEL_MOTOR_GOAL_COMPLETION_CHECK_DURATION_NAME.
 */
#define WHEEL_MOTOR_GOAL_COMPLETION_CHECK_FREQUENCY_NAME       "wheel_motor_goal_completion_check_frequency_hz"
#define WHEEL_MOTOR_GOAL_COMPLETION_CHECK_FREQUENCY_DEFAULT    10
#define WHEEL_MOTOR_GOAL_COMPLETION_CHECK_DURATION_NAME        "wheel_motor_goal_completion_check_duration_ms"
#define WHEEL_MOTOR_GOAL_COMPLETION_CHECK_DURATION_DEFAULT     100

namespace wheel_motor_control
{
// Boilerplate adapted from:
// https://docs.ros.org/en/humble/Tutorials/Intermediate/Writing-an-Action-Server-Client/Cpp.html

/**
 * @brief Node to control a single wheel motor via EtherCAT.
 *
 * Supports
 * - coarse velocity commands via `/<wheel_name>/cmd_vel` topic
 * - fine position control via `/<wheel_name>/fine_control` action server, which disables velocity control while active
 *
 * See: `docs/wheels/control.md` for details.
 */
class WheelMotorNode : public rclcpp::Node
{
public:
   using WheelFineControl           = motor_control::action::WheelFineControl;
   using GoalHandleWheelFineControl = rclcpp_action::ServerGoalHandle<WheelFineControl>;

   static std::shared_ptr<WheelMotorNode> create(
      const std::string&         wheel_name,
      const rclcpp::NodeOptions& options,
      EtherCatConfig             ethercat_config,
      MotorConfig                motor_config)
   {
      return std::shared_ptr<WheelMotorNode>(
         new WheelMotorNode(
            wheel_name,
            options,
            std::move(ethercat_config),
            std::move(motor_config)));
   }

protected:
   WheelMotorNode(
      const std::string&         wheel_name,
      const rclcpp::NodeOptions& options,
      EtherCatConfig             ethercat_config,
      MotorConfig                motor_config)
      : Node(wheel_name + "_wheel_motor_node", options),
      wheel_name_(wheel_name),
      motor_config_(std::move(motor_config)),
      mode_(WheelMotorControlMode::VELOCITY)
   {
      using namespace std::placeholders;

      // Create the motor interface
      motor_ = create_IWheelMotor();
      motor_->initialize(ethercat_config);

      // Bind the control loop to a timer at `control_frequency_hz`
      declare_parameter(WHEEL_MOTOR_CONTROL_FREQUENCY_NAME, WHEEL_MOTOR_CONTROL_FREQUENCY_DEFAULT);
      int  control_frequency = get_parameter(WHEEL_MOTOR_CONTROL_FREQUENCY_NAME).as_int();
      auto period_ms         = std::chrono::milliseconds(1000 / control_frequency);
      control_timer_ = create_wall_timer(
         period_ms,
         std::bind(&WheelMotorNode::control_loop, this));

      // Bind the action server
      fine_control_action_server_ = rclcpp_action::create_server<WheelFineControl>(
         this,
         wheel_name_ + "/fine_control",
         std::bind(&WheelMotorNode::handle_fine_control_goal, this, _1, _2),
         std::bind(&WheelMotorNode::handle_fine_control_cancel, this, _1),
         std::bind(&WheelMotorNode::handle_fine_control_accepted, this, _1));

      // Bind the topic subscription
      velocity_command_subscription_ = create_subscription<std_msgs::msg::Float32>(
         wheel_name_ + "/cmd_vel",
         10,
         std::bind(&WheelMotorNode::velocity_command_callback, this, _1));


      // set the initialization time
      last_control_time_ = std::chrono::high_resolution_clock::now();

      // set the goal completion check parameters
      declare_parameter(WHEEL_MOTOR_GOAL_COMPLETION_CHECK_FREQUENCY_NAME, WHEEL_MOTOR_GOAL_COMPLETION_CHECK_FREQUENCY_DEFAULT);
      goal_completion_check_frequency_hz_ = get_parameter(WHEEL_MOTOR_GOAL_COMPLETION_CHECK_FREQUENCY_NAME).as_int();
      declare_parameter(WHEEL_MOTOR_GOAL_COMPLETION_CHECK_DURATION_NAME, WHEEL_MOTOR_GOAL_COMPLETION_CHECK_DURATION_DEFAULT);
      goal_completion_check_duration_ms_ = get_parameter(WHEEL_MOTOR_GOAL_COMPLETION_CHECK_DURATION_NAME).as_int();
   }

   // IWheelMotor creation (for testability)
   virtual std::unique_ptr<IWheelMotor> create_IWheelMotor()
   {
      // TODO
      throw std::runtime_error("create_IWheelMotor() not implemented");
   }

private:
   // Configuration
   std::string wheel_name_;                                   // Wheel name, eg "front_right"
   MotorConfig motor_config_;                                 // Configuration for motor parameters (like whether it is flipped, scaling, anything else)
   WheelMotorControlMode mode_;                               // Control mode (velocity or position)
   std::optional<rclcpp_action::GoalUUID> current_goal_uuid_; // Currently active goal, if any

   // Control state
   float target_velocity_;
   float target_position_;

   // ROS interfaces
   rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr velocity_command_subscription_;
   rclcpp_action::Server<WheelFineControl>::SharedPtr fine_control_action_server_;
   rclcpp::TimerBase::SharedPtr control_timer_;

   std::unique_ptr<IWheelMotor> motor_;      // Interface to the actual motor

   // timing
   std::chrono::high_resolution_clock::time_point last_control_time_;

   int goal_completion_check_frequency_hz_;
   int goal_completion_check_duration_ms_;

   /**
    * @brief Main control loop, executed at fixed rate
    */
   void control_loop()
   {
      auto current_time = std::chrono::high_resolution_clock::now();
      auto delta_time   = std::chrono::duration_cast<std::chrono::milliseconds>(
         current_time - last_control_time_);
      switch (mode_)
      {
      case WheelMotorControlMode::VELOCITY:
         control_velocity();
         break;

      case WheelMotorControlMode::POSITION:
         control_position();
         break;
      }
      motor_->tick(delta_time);
      last_control_time_ = current_time;
      // TODO publish diagnostics
   }

   /**
    * INVARIANT: mode_ must be VELOCITY
    */
   void control_velocity()
   {
      motor_->set_control_mode(WheelMotorControlMode::VELOCITY);
      motor_->set_target_velocity(target_velocity_);
   }

   /**
    * INVARIANT: mode_ must be POSITION
    */
   void control_position()
   {
      motor_->set_control_mode(WheelMotorControlMode::POSITION);
      motor_->set_target_position(target_position_);
   }

   // Action server callbacks
   rclcpp_action::GoalResponse handle_fine_control_goal(
      const rclcpp_action::GoalUUID&                uuid,
      std::shared_ptr<const WheelFineControl::Goal> goal)
   {
      RCLCPP_INFO(get_logger(), "Received wheel fine control goal request");
      (void)goal;

      if (current_goal_uuid_.has_value())
      {
         RCLCPP_WARN(get_logger(), "Rejecting new wheel fine control goal; another is already active");
         return rclcpp_action::GoalResponse::REJECT;
      }
      current_goal_uuid_ = uuid;
      return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
   }

   rclcpp_action::CancelResponse handle_fine_control_cancel(
      const std::shared_ptr<GoalHandleWheelFineControl> goal_handle)
   {
      RCLCPP_INFO(get_logger(), "Received request to cancel wheel fine control goal");
      (void)goal_handle;
      current_goal_uuid_.reset();              // Clear the active goal
      mode_ = WheelMotorControlMode::VELOCITY; // Return to velocity control mode
      return rclcpp_action::CancelResponse::ACCEPT;
   }

   void handle_fine_control_accepted(
      const std::shared_ptr<GoalHandleWheelFineControl> goal_handle)
   {
      mode_ = WheelMotorControlMode::POSITION;
      auto goal = goal_handle->get_goal();
      target_position_ = goal->target_position;
      RCLCPP_INFO(get_logger(), "Starting wheel fine control goal execution");
      using namespace std::placeholders;
      std::thread{ std::bind(&WheelMotorNode::wait_for_stabilized_completion, this, _1), goal_handle }.detach();
   }

   // Topic callback
   void velocity_command_callback(const std_msgs::msg::Float32::SharedPtr msg)
   {
      target_velocity_ = msg->data;
   }

   // Goal processing function
   void wait_for_stabilized_completion(
      const std::shared_ptr<GoalHandleWheelFineControl> goal_handle)
   {
      rclcpp::Rate loop_rate(goal_completion_check_frequency_hz_);
      const auto   goal = goal_handle->get_goal();
      const int    stable_checks_required = goal_completion_check_duration_ms_ / (1000 / goal_completion_check_frequency_hz_);
      int          stable_checks_count    = 0;
      const float  timeout_seconds        = goal->timeout;
      const auto   start_time             = std::chrono::high_resolution_clock::now();
      const auto   motor_start_position   = motor_->get_current_position();

      while (rclcpp::ok())
      {
         // Check Timeout and cancellation requests
         bool timeout_reached = false;
         if ((timeout_seconds > 0.0f) &&
             (std::chrono::duration_cast<std::chrono::duration<float> >(
                 std::chrono::high_resolution_clock::now() - start_time).count() >= timeout_seconds))
         {
            timeout_reached = true;
            RCLCPP_INFO(get_logger(), "Wheel fine control goal timeout reached");
         }
         if (goal_handle->is_canceling() || timeout_reached)
         {
            RCLCPP_INFO(get_logger(), "Wheel fine control goal canceled");
            auto result = std::make_shared<WheelFineControl::Result>();
            result->final_position = motor_->get_current_position();
            result->position_error = std::abs(result->final_position - goal->target_position);
            goal_handle->canceled(result);
            return;
         }
         // Check if the goal is achieved
         float current_position = motor_->get_current_position();
         float position_error   = std::abs(current_position - goal->target_position);
         if (position_error <= goal->tolerance)
         {
            stable_checks_count++;
         }
         else
         {
            stable_checks_count = 0;
         }
         if (stable_checks_count >= stable_checks_required)
         {
            // Done :)
            RCLCPP_INFO(get_logger(), "Wheel fine control goal succeeded");
            auto result = std::make_shared<WheelFineControl::Result>();
            result->final_position = current_position;
            result->position_error = position_error;
            goal_handle->succeed(result);
         }
         else
         {
            // Not done yet, send feedback fields
            auto feedback = std::make_shared<WheelFineControl::Feedback>();
            feedback->current_position = current_position;
            feedback->position_error   = position_error;
            feedback->time_elapsed     = std::chrono::duration_cast<std::chrono::duration<float> >(
               std::chrono::high_resolution_clock::now() - start_time).count();
            feedback->percent_complete = 100.0f * std::abs(current_position - motor_start_position) /
                                         (std::abs(goal->target_position - motor_start_position) + 1e-6f);
            goal_handle->publish_feedback(feedback);
         }
         loop_rate.sleep();
      }
   }
};
} // namespace whell_motor_control
