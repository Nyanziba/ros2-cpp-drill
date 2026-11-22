# ROS 2 练习册：图像、行为树与自主控制

这一页是简体中文的入口。仓库里原有的 C、C++、ROS 2 和设计模式各章仍是日语和英语。
新加的三个系列有日语、英语和简体中文的读物与题目。代码和测试三种语言共用。

```bash
DRILL_LANG=zh ./drill list
DRILL_LANG=zh ./drill run cv01
```

`DRILL_LANG=zh` 会选用 `title_zh`、`hints_zh` 和 `README.zh.md`。
终端里的「合格」「提示」等界面文字仍是日语。

## 三个系列

| 系列 | 读物 | 练习 |
| --- | --- | --- |
| 图像处理 | [docs-zh/vision/](docs-zh/vision/README.md) | `cv01` 至 `cv06`（第 6 章是视线控制） |
| 行为树 | [docs-zh/behavior-tree/](docs-zh/behavior-tree/README.md) | `bt01` 至 `bt03` |
| 自主控制 | [docs-zh/autonomy/](docs-zh/autonomy/README.md) | `nav01` 至 `nav03` |

总览是 [docs-zh/README.md](docs-zh/README.md)。
网站建好之后，简体中文在 <https://nyanziba.github.io/ros2-cpp-drill/zh/> 。

日语的说明在 [README.md](README.md)，英语在 [README.en.md](README.en.md)。

## CI 会跳过的部分

GPU 上的 YOLO 训练、TensorRT 引擎构建、`robot_localization` 节点、Nav2 和 slam_toolbox 的启动，在没有相应环境时会跳过。
评分的是标签约定、精度与帧预算、融合公式、`ekf.yaml`、状态转移、路径时间和恢复 XML。
