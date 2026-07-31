"""課題09: talker と listener を起動する launch 記述（Python 版・解答）。"""
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description() -> LaunchDescription:
    return LaunchDescription([
        Node(
            package="drill_01_publisher",
            executable="talker",
            name="talker",
            namespace="demo",
            remappings=[("topic", "chatter")],
        ),
        Node(
            package="drill_02_subscriber",
            executable="listener",
            name="listener",
            namespace="demo",
            remappings=[("topic", "chatter")],
        ),
    ])
