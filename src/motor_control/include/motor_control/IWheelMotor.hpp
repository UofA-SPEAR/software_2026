#include <chrono>
#include "include/motor_control/WheelMotorControlMode.hpp"
#include "include/motor_control/EtherCatConfig.hpp"

/**
 * @brief Interface for a wheel motor controlled via EtherCAT.
 * Exists as an interface to allow mocking for testing and simulation.
 */
class IWheelMotor
{
public:
  virtual ~IWheelMotor() = default;

  virtual void initialize(const EtherCatConfig & etherCatConfig) = 0;

  virtual void tick(const std::chrono::milliseconds & delta_time) = 0;

  virtual void set_target_velocity(float velocity_rad_per_s) = 0;
  virtual void set_target_position(float position_rad) = 0;
  virtual void set_control_mode(WheelMotorControlMode mode) = 0;

  virtual float get_current_velocity() const = 0;
  virtual float get_current_position() const = 0;
  virtual WheelMotorControlMode get_control_mode() const = 0;
};
