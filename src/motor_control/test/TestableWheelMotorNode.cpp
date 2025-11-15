#include "src/motor_control/src/WheelMotorNode.cpp"
#include "test/TestWheelMotor.cpp"
#include "include/motor_control/IWheelMotor.hpp"

namespace wheel_motor_control
{
class TestableWheelMotorNode : public WheelMotorNode
{
public:
   using WheelMotorNode::WheelMotorNode;

protected:
   std::unique_ptr<IWheelMotor> create_IWheelMotor() override
   {
      return std::make_unique<TestWheelMotor>();
   }
};
} // namespace wheel_motor_control
