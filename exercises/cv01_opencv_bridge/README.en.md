# cv01 OpenCV and cv_bridge [Vision]

Find one red or blue marker in a BGR image, and convert it to and from a ROS Image.

## What to do

Implement `src/marker_detect.cpp`. Red is R>=180 and R>G+50 and R>B+50. Blue is B>=180 and B>R+50 and B>G+50. The larger count wins, and a tie is red. Below min_pixels, an empty image, or a type other than CV_8UC3 means klass 0. cx and cy are the truncated means of the winning pixels. `mat_to_ros_image` uses encoding `bgr8`. `ros_image_to_mat` returns empty for any other encoding and for short data.

## Run it

```bash
./drill run cv01
```

## Common pitfalls

Swapping R and B swaps red and blue. `clone` before the temporary from `toCvCopy` dies.

## Tests

```bash
./drill run cv01
```

| Test | What it checks |
| --- | --- |
| `FindsRedBlockCentroid` | centroid of a red blob |
| `FindsBlueBlock` | a blue blob |
| `TiePrefersRed` | a tie is red |
| `RejectsSmallBlobAndWrongType` | a blob that is too small, and a wrong type |
| `RoundTripsBgr8` | a bgr8 round trip |
| `RejectsOtherEncodingsAndShortData` | other encodings and short data |

## References

- [1. OpenCV and cv_bridge](../../docs-en/vision/01_opencv_and_cv_bridge.md)
