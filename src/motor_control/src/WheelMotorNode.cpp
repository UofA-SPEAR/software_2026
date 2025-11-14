#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "include/motor_control/EtherCatConfig.hpp"
#include "include/motor_control/MotorConfig.hpp"
#include "std_msgs/msg/float32.hpp"
#include "motor_control/action/wheel_fine_control.hpp"

enum WheelMotorNodeMode
{
    VELOCITY,
    POSITION
};

// Boilerplate adapted from:
// https://docs.ros.org/en/humble/Tutorials/Intermediate/Writing-an-Action-Server-Client/Cpp.html

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
                                    motor_config_(std::move(motor_config))
    {
        using namespace std::placeholders;
        this->fine_control_action_server_ = rclcpp_action::create_server<WheelFineControl>(
            this,
            this->wheel_name_ + "/fine_control",
            std::bind(&WheelMotorNode::handle_fine_control_goal, this, _1, _2),
            std::bind(&WheelMotorNode::handle_fine_control_cancel, this, _1),
            std::bind(&WheelMotorNode::handle_fine_control_accepted, this, _1));

        this->velocity_command_subscription_ = this->create_subscription<std_msgs::msg::Float32>(
            this->wheel_name_ + "/cmd_vel",
            10,
            std::bind(&WheelMotorNode::velocity_command_callback, this, _1));
    }

private:
    std::string wheel_name_;
    EtherCatConfig ethercat_config_;
    MotorConfig motor_config_;
    WheelMotorNodeMode mode_;

    rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr velocity_command_subscription_;
    rclcpp_action::Server<WheelFineControl>::SharedPtr fine_control_action_server_;

    rclcpp_action::GoalResponse handle_fine_control_goal(
        const rclcpp_action::GoalUUID &uuid,
        std::shared_ptr<const WheelFineControl::Goal> goal)
    {
        RCLCPP_INFO(this->get_logger(), "Received wheel fine control goal request");
        (void)uuid;
        return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
    }

    rclcpp_action::CancelResponse handle_fine_control_cancel(
        const std::shared_ptr<GoalHandleWheelFineControl> goal_handle)
    {
        RCLCPP_INFO(this->get_logger(), "Received request to cancel wheel fine control goal");
        (void)goal_handle;
        return rclcpp_action::CancelResponse::ACCEPT;
    }

    void handle_fine_control_accepted(
        const std::shared_ptr<GoalHandleWheelFineControl> goal_handle)
    {
        RCLCPP_INFO(this->get_logger(), "Starting wheel fine control goal execution");
        return;
    }

    void velocity_command_callback(const std_msgs::msg::Float32::SharedPtr msg)
    {
        RCLCPP_INFO(this->get_logger(), "Received velocity command: %f", msg->data);
    }
};