# cv01 OpenCV 与 cv_bridge 〔图像处理〕

在 BGR 图像里找出一块红或蓝标记，并与 ROS 的 Image 来回转换。

## 要做的事

实现 `src/marker_detect.cpp`。红是 R>=180 且 R>G+50 且 R>B+50，蓝是 B>=180 且 B>R+50 且 B>G+50。数量多的颜色获胜，相同算红。少于 min_pixels、空图像或不是 CV_8UC3 时 klass 为 0。cx、cy 是获胜像素的整数平均。`mat_to_ros_image` 使用编码 `bgr8`。`ros_image_to_mat` 对其他编码和过短的 data 返回空。

## 运行

```bash
./drill run cv01
```

## 容易卡住的地方

把 R 和 B 弄反会把红蓝对调。在 `toCvCopy` 的临时对象销毁前 `clone`。

## 测试

```bash
./drill run cv01
```

| 测试 | 检查的内容 |
| --- | --- |
| `FindsRedBlockCentroid` | 红色块的中心 |
| `FindsBlueBlock` | 蓝色块 |
| `TiePrefersRed` | 数量相同算红 |
| `RejectsSmallBlobAndWrongType` | 太小的块和错误类型 |
| `RoundTripsBgr8` | bgr8 往返 |
| `RejectsOtherEncodingsAndShortData` | 其他编码和过短的 data |

## 参考

- [1. OpenCV 与 cv_bridge](../../docs-zh/vision/01_opencv_and_cv_bridge.md)
