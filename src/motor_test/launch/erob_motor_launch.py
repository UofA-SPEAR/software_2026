from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    pkg_share = get_package_share_directory('motor_test')
    urdf_file = os.path.join(pkg_share, 'urdf', 'erob_motor.urdf.xacro')
    controller_config = os.path.join(pkg_share, 'config', 'erob_controllers.yaml')

    # Read the xacro file directly (no xacro processing needed if no macros used)
    # If you add xacro macros later, replace with a xacro.process_file() call
    with open(urdf_file, 'r') as f:
        robot_description = f.read()

    return LaunchDescription([

        # Controller manager — loads the hardware interface and spins the control loop
        Node(
            package='controller_manager',
            executable='ros2_control_node',
            parameters=[
                {'robot_description': robot_description},
                controller_config,
            ],
            output='screen',
        ),

        # Joint state broadcaster — publishes /joint_states from state interfaces
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['joint_state_broadcaster'],
            output='screen',
        ),

        # eRob position controller — forwards commands to position command interface
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['erob_controller'],
            output='screen',
        ),

    ])