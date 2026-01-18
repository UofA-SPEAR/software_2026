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
        Node(
            package='controller_manager',
            executable='ros2_control_node',
            parameters=[{'robot_description': open(urdf_file).read()}, controller_config],
            output='screen',
        ),
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['joint_state_broadcaster'],
            output='screen',
        ),
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['motor_controller'],
            output='screen',
        ),
    ])