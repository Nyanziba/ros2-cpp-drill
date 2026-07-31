# I AM NOT DONE
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    # TODO: このパッケージの share ディレクトリにインストールされた
    #       config/params.yaml へのフルパスを組み立てること。
    #       ソースツリーからの相対パスではなく、get_package_share_directory() と
    #       os.path.join() で組むのが公式の作法です
    #       （インストール後、どこから起動しても解決できるようにするため）。

    return LaunchDescription([
        # TODO: package が "drill_08_params_yaml"、executable が "param_echo" の
        #       ノードを 1 つ起動すること。上で組み立てた YAML のパスを
        #       parameters に（リストで）渡して読ませる。
        #       ログを端末で見たいので output も指定しておくとよい。
        #
        # コードの形が知りたければ ./drill hint 08
    ])
