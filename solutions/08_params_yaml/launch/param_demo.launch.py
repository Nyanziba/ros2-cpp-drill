import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    params_file = os.path.join(
        get_package_share_directory('drill_08_params_yaml'),
        'config', 'params.yaml')

    return LaunchDescription([
        Node(
            package='drill_08_params_yaml',
            executable='param_echo',
            name='param_echo',
            output='screen',
            parameters=[params_file],
        ),
    ])
