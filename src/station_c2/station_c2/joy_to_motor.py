#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Joy
from std_msgs.msg import Float64MultiArray


class JoyToMotorNode(Node):
    def __init__(self):
        super().__init__('joy_to_motor')
        
        # Declare parameters
        self.declare_parameter('trigger_axis', 5)  # Right trigger on most controllers (RT)
        self.declare_parameter('min_output', 0.0)
        self.declare_parameter('max_output', 100.0)
        self.declare_parameter('deadzone', 0.05)
        
        # Get parameters
        self.trigger_axis = self.get_parameter('trigger_axis').get_parameter_value().integer_value
        self.min_output = self.get_parameter('min_output').get_parameter_value().double_value
        self.max_output = self.get_parameter('max_output').get_parameter_value().double_value
        self.deadzone = self.get_parameter('deadzone').get_parameter_value().double_value
        
        # Create subscriber and publisher
        self.joy_sub = self.create_subscription(
            Joy,
            'joy',
            self.joy_callback,
            10
        )
        
        self.motor_pub = self.create_publisher(
            Float64MultiArray,
            '/motor_controller/commands',
            10
        )
        
        self.get_logger().info(f'Joy to Motor node started')
        self.get_logger().info(f'Trigger axis: {self.trigger_axis}')
        self.get_logger().info(f'Output range: [{self.min_output}, {self.max_output}]')
    
    def joy_callback(self, msg):
        """
        Callback for joystick messages.
        Maps trigger input (typically -1.0 to 1.0 range) to motor command (50 to 100).
        """
        # Check if the trigger axis exists in the message
        if len(msg.axes) <= self.trigger_axis:
            self.get_logger().warn(
                f'Trigger axis {self.trigger_axis} not available. '
                f'Only {len(msg.axes)} axes in message.',
                throttle_duration_sec=5.0
            )
            return
        
        # Get trigger value (typically ranges from 1.0 (not pressed) to -1.0 (fully pressed))
        # or from -1.0 to 1.0 depending on the controller
        trigger_raw = msg.axes[self.trigger_axis]
        
        # Normalize trigger value to 0.0 to 1.0 range
        # Most triggers report 1.0 when not pressed and -1.0 when fully pressed
        # We'll handle both conventions
        if trigger_raw >= 0:
            # Convention: 0 to 1 range (already normalized, or idle at 1)
            # Check if idle position is at 1.0 (not pressed)
            trigger_normalized = 1.0 - trigger_raw  # Convert so 0 = not pressed, 1 = fully pressed
        else:
            # Already in -1 to 0 range
            trigger_normalized = abs(trigger_raw)
        
        # Apply deadzone
        if trigger_normalized < self.deadzone:
            trigger_normalized = 0.0
        
        # Clamp to [0, 1]
        trigger_normalized = max(0.0, min(1.0, trigger_normalized))
        
        # Map to output range [min_output, max_output]
        motor_command = self.min_output + (trigger_normalized * (self.max_output - self.min_output))
        
        # Create and publish message
        motor_msg = Float64MultiArray()
        motor_msg.data = [motor_command]
        
        self.motor_pub.publish(motor_msg)
        
        # Log occasionally
        if trigger_normalized > self.deadzone:
            self.get_logger().debug(
                f'Trigger: {trigger_raw:.2f} -> Normalized: {trigger_normalized:.2f} -> Motor: {motor_command:.2f}'
            )


def main(args=None):
    rclpy.init(args=args)
    node = JoyToMotorNode()
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()