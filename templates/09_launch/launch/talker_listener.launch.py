"""課題09: talker と listener を起動する launch 記述（Python 版）。

同じ内容を launch/talker_listener.launch.xml、launch/talker_listener.launch.yaml
にも XML / YAML で書きます。3 つとも構造は同じになるはずです。

起動する内容（README.md の表と同じ）:

| 項目        | talker               | listener               |
| ----------- | -------------------- | ---------------------- |
| package     | drill_01_publisher   | drill_02_subscriber    |
| executable  | talker               | listener               |
| name        | talker               | listener               |
| namespace   | demo                 | demo                   |
| remap       | topic -> chatter     | topic -> chatter       |
"""
from launch import LaunchDescription
from launch_ros.actions import Node

# I AM NOT DONE


def generate_launch_description() -> LaunchDescription:
    return LaunchDescription([
        # TODO: talker 用の Node を 1 つ追加すること。
        #       上の表の package / executable / name / namespace を指定し、
        #       トピックの付け替えは remappings に
        #       （元の名前, 新しい名前）のタプルのリストで渡す。

        # TODO: listener 用の Node も同じ形で追加すること。
        #
        # コードの形が知りたければ ./drill hint 09
    ])
