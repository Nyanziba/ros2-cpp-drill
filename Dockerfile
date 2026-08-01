# ROS 2 練習帳を Ubuntu 以外でも動かすための環境。
#
#   docker compose build
#   docker compose run --rm drill ./drill list
#
# ROS 2 Jazzy は Ubuntu 24.04 にしか公式パッケージがありません。macOS でも
# Windows でも他のディストリでも、中身は Ubuntu 24.04 にして揃えます。
#
# 教材の実測値は g++ 13.3.0 / Ubuntu 24.04 で取ってあります。このイメージの
# ベースが同じなので、コンパイルエラーの文面も実行結果も本文と一致します。
# **ここを別のベースに変えると教材の出力とズレます。**

FROM ros:jazzy-ros-base

# apt が対話を求めると build が止まる
ARG DEBIAN_FRONTEND=noninteractive

# ドリルが要るもの。
#
# - build-essential / cmake : ament_cmake が使う
# - python3-colcon-*        : ビルドとテストの実行
# - ros-jazzy-*             : 課題の package.xml が依存しているもの
#   （action-tutorials-interfaces は ros-base に入っていないので明示する）
# - python3-pytest          : 08_params_yaml と 09_launch が pytest を使う
# - python3-yaml            : 08_params_yaml のテストが読む
# - python3-venv            : ./drill read --build が読み物サイトを建てるのに使う
# - gdb / less              : つまずいたときに中を見るため
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        gdb \
        git \
        less \
        python3-colcon-common-extensions \
        python3-pytest \
        python3-venv \
        python3-yaml \
        ros-jazzy-ament-cmake-gtest \
        ros-jazzy-ament-cmake-pytest \
        ros-jazzy-action-tutorials-interfaces \
        ros-jazzy-class-loader \
        ros-jazzy-example-interfaces \
        ros-jazzy-rclcpp-action \
        ros-jazzy-rclcpp-components \
        ros-jazzy-std-msgs \
    && rm -rf /var/lib/apt/lists/*

# GUI が要るとき（ROS 2編の turtlesim / rqt / RViz）だけ入れる。
#
#   docker compose build --build-arg WITH_GUI=1
#
# 既定で入れていないのは、これだけでイメージが 1GB 以上太るからです。
# ドリルの課題は全て gtest / pytest なので、GUI 無しで最後まで通ります。
# 必要なのは読み物のほう（turtlesim を動かす章）です。
ARG WITH_GUI=0
RUN if [ "$WITH_GUI" = "1" ]; then \
        apt-get update && apt-get install -y --no-install-recommends \
            ros-jazzy-turtlesim \
            ros-jazzy-rqt-common-plugins \
            ros-jazzy-rqt-graph \
            ros-jazzy-rviz2 \
        && rm -rf /var/lib/apt/lists/* ; \
    fi

# ホストと同じ UID/GID のユーザを作る。
#
# **これをやらないと、コンテナが作ったファイルが root 所有になって**
# **ホスト側のエディタで保存できなくなります。** Linux ホストで実際に困ります。
# macOS / Windows の Docker Desktop は勝手に辻褄を合わせるので影響しません。
#
#   docker compose build --build-arg UID=$(id -u) --build-arg GID=$(id -g)
#
# ros:jazzy には UID 1000 の ubuntu ユーザが既にいます。要求された UID が
# 1000 なら作り直さずそれを使い、違うなら番号を付け替えます。
ARG UID=1000
ARG GID=1000
RUN if [ "$(id -u ubuntu 2>/dev/null)" ]; then \
        groupmod -o -g "$GID" ubuntu && usermod -o -u "$UID" -g "$GID" ubuntu ; \
    else \
        groupadd -o -g "$GID" ubuntu && \
        useradd -o -m -u "$UID" -g "$GID" -s /bin/bash ubuntu ; \
    fi

# ビルド生成物の置き場。
#
# **ホストの build/ install/ log/ とは混ぜません。** CMake は絶対パスと
# コンパイラのパスをキャッシュに焼き込むので、ホストで native にビルドした
# ものとコンテナのものが同じディレクトリにあると壊れます。
# compose.yaml でここに名前付きボリュームを当てています。
#
# ボリュームは初回マウント時にイメージ側の中身と所有者を引き継ぐので、
# ここで先に ubuntu 所有で作っておかないと root 所有になります。
#
# **COLCON_IGNORE を置くのが重要です。** drill は colcon を /ws で走らせるので、
# install/ と log/ も走査対象に入ります。install 空間には package.xml の複製が
# 入るため、目印が無いと同じパッケージが二重に見つかってビルドが壊れます。
# ホスト側の install/ と log/ には元から置いてありますが、ここはボリュームで
# 覆われてホスト側が見えなくなるので、イメージ側にも要ります。
# .venv-docs も分ける。ここで作った venv の shebang は /ws/... を指すので、
# バインドマウント上に置くとホスト側から使えない venv がホストに残る。
RUN mkdir -p /ws/build /ws/install /ws/log /ws/.venv-docs && \
    touch /ws/install/COLCON_IGNORE /ws/log/COLCON_IGNORE /ws/build/COLCON_IGNORE && \
    chown -R "$UID:$GID" /ws

# ROS 2 を毎回 source しなくていいようにしておく。
# drill は自分で source を探すので必須ではありませんが、
# コンテナに入って手で ros2 を叩くときに要ります。
# .bashrc がまだ無い場合に root 所有で作られないよう、最後に chown する。
RUN echo '. /opt/ros/jazzy/setup.bash' >> /home/ubuntu/.bashrc && \
    echo '[ -f /ws/install/setup.bash ] && . /ws/install/setup.bash' >> /home/ubuntu/.bashrc && \
    chown "$UID:$GID" /home/ubuntu/.bashrc

USER ubuntu
WORKDIR /ws

# DDS がホストの他のノードや、同じ LAN の他の受講者と混信しないように閉じる。
# drill も同じ値を setdefault していますが、コンテナに入って手で ros2 を
# 叩くときにも効かせたいので、環境変数としても置いておきます。
ENV ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST \
    ROS_DOMAIN_ID=42 \
    COLCON_LOG_PATH=/ws/log

CMD ["bash"]
