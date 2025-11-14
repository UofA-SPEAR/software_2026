#include "include/motor_control/IWheelMotor.hpp"
#include "include/motor_control/WheelMotorControlMode.hpp"

class TestWheelMotor : public IWheelMotor
{
public:
    TestWheelMotor() = default;
    ~TestWheelMotor() override = default;

    void initialize(const EtherCatConfig &etherCatConfig) override
    {
        // Do nothing :)
    }

    void tick(const std::chrono::milliseconds &delta_time) override
    {
        // Simulation of motor behavior:
        // update real values closer to target
        float dt_sec = delta_time.count() / 1000.0f;
        real_position += real_velocity_ * dt_sec;
        switch (control_mode_)
        {
        case WheelMotorControlMode::VELOCITY:
            real_velocity_ += (target_velocity_ - real_velocity_) * dt_sec * 0.1f;
            break;
        case WheelMotorControlMode::POSITION:
            real_velocity_ = 0;
            real_position += (target_position_ - real_position) * dt_sec * 0.1f;
        }
    }

    void set_target_velocity(float velocity_rad_per_s) override
    {
        target_velocity_ = velocity_rad_per_s;
    }

    void set_target_position(float position_rad) override
    {
        target_position_ = position_rad;
    }

    void set_control_mode(WheelMotorControlMode mode) override
    {
        control_mode_ = mode;
    }

    float get_current_velocity() const override
    {
        return real_velocity_;
    }

    float get_current_position() const override
    {
        return real_position;
    }

    WheelMotorControlMode get_control_mode() const override
    {
        return control_mode_;
    }

private:
    float target_velocity_ = 0.0f;
    float target_position_ = 0.0f;

    float real_velocity_ = 0.0f;
    float real_position = 0.0f;
    WheelMotorControlMode control_mode_ = WheelMotorControlMode::VELOCITY;
};
