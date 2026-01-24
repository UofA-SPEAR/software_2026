from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import ExecuteProcess
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    pkg_share = get_package_share_directory('motor_test')
    urdf_file = os.path.join(pkg_share, 'urdf', 'motor.urdf.xacro')
    controller_config = os.path.join(pkg_share, 'config', 'controllers.yaml')
    
    # Process xacro to get URDF
    robot_description = ExecuteProcess(
        cmd=['xacro', urdf_file],
        output='screen'
    )
    
    return LaunchDescription([
        # Controller manager
        Node(
            package='controller_manager',
            executable='ros2_control_node',
            parameters=[{'robot_description': open(urdf_file).read()}, controller_config],
            output='screen',
        ),
        
        # Joint state broadcaster
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['joint_state_broadcaster'],
            output='screen',
        ),
        
        # Motor controller
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['motor_controller'],
            output='screen',
        ),
        
        # Joy node - publishes joystick data
        Node(
            package='joy',
            executable='joy_node',
            name='joy_node',
            parameters=[{
                'device_id': 0,
                'deadzone': 0.05,
                'autorepeat_rate': 20.0,
            }],
            output='screen',
        ),
        
        # Joy to motor mapper node
        Node(
            package='station_c2',
            executable='joy_to_motor',
            name='joy_to_motor',
            parameters=[{
                'trigger_axis': 5,  # Right trigger (RT) - adjust if needed
                'min_output': 00.0,
                'max_output': 100.0,
                'deadzone': 0.05,
            }],
            output='screen',
        ),
    ])