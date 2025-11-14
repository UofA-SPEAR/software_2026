#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "include/motor_control/EtherCatConfig.hpp"
#include "include/motor_control/MotorConfig.hpp"
#include "std_msgs/msg/float32.hpp"
#include "motor_control/action/wheel_fine_control.hpp"

namespace motor_control
{
    enum class WheelMotorNodeMode
    {
        VELOCITY,
        POSITION
    };

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
        using WheelFineControl = motor_control::action::WheelFineControl;
        using GoalHandleWheelFineControl = rclcpp_action::ServerGoalHandle<WheelFineControl>;

        WheelMotorNode(
            const std::string &wheel_name,
            const rclcpp::NodeOptions &options,
            EtherCatConfig ethercat_config,
            MotorConfig motor_config) : Node(wheel_name + "_wheel_motor_node", options),
                                        wheel_name_(wheel_name),
                                        ethercat_config_(std::move(ethercat_config)),
                                        motor_config_(std::move(motor_config)),
                                        mode_(WheelMotorNodeMode::VELOCITY)
        {
            using namespace std::placeholders;

            // Bind the control loop to a timer at `control_frequency_hz`
            declare_parameter("control_frequency_hz", 100);
            int control_frequency = get_parameter("control_frequency_hz").as_int();
            auto period_ms = std::chrono::milliseconds(1000 / control_frequency);
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
        }

    private:
        // Configuration
        std::string wheel_name_;         // Wheel name, eg "front_right"
        EtherCatConfig ethercat_config_; // Configuration for EtherCAT communication
        MotorConfig motor_config_;       // Configuration for motor parameters (like whether it is flipped, scaling, anything else)
        WheelMotorNodeMode mode_;        // Control mode (velocity or position)

        // Control state
        double target_velocity_;
        double target_position;

        // ROS interfaces
        rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr velocity_command_subscription_;
        rclcpp_action::Server<WheelFineControl>::SharedPtr fine_control_action_server_;
        rclcpp::TimerBase::SharedPtr control_timer_;

        /**
         * @brief Main control loop, executed at fixed rate
         */
        void control_loop()
        {
            switch (mode_)
            {
            case WheelMotorNodeMode::VELOCITY:
                control_velocity();
                break;
            case WheelMotorNodeMode::POSITION:
                control_position();
                break;
            }
            // TODO publish diagnostics
        }

        /**
         * INVARIANT: mode_ must be VELOCITY
         */
        void control_velocity()
        {
        }

        /**
         * INVARIANT: mode_ must be POSITION
         */
        void control_position()
        {
        }

        // Action server callbacks
        rclcpp_action::GoalResponse handle_fine_control_goal(
            const rclcpp_action::GoalUUID &uuid,
            std::shared_ptr<const WheelFineControl::Goal> goal)
        {
            RCLCPP_INFO(get_logger(), "Received wheel fine control goal request");
            (void)uuid;
            return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
        }

        rclcpp_action::CancelResponse handle_fine_control_cancel(
            const std::shared_ptr<GoalHandleWheelFineControl> goal_handle)
        {
            RCLCPP_INFO(get_logger(), "Received request to cancel wheel fine control goal");
            (void)goal_handle;
            return rclcpp_action::CancelResponse::ACCEPT;
        }

        void handle_fine_control_accepted(
            const std::shared_ptr<GoalHandleWheelFineControl> goal_handle)
        {
            RCLCPP_INFO(get_logger(), "Starting wheel fine control goal execution");
            return;
        }

        // Topic callback
        void velocity_command_callback(const std_msgs::msg::Float32::SharedPtr msg)
        {
            RCLCPP_INFO(get_logger(), "Received velocity command: %f", msg->data);
        }
    };
} // namespace motor_control