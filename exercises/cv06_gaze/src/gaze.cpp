// I AM NOT DONE
//
// 検出結果からパン・チルトを動かし、ビヘイビアツリーで注視モードを切り替えてください。

#include "drill/gaze.hpp"

namespace drill
{

GazeMemory update_gaze(
  const GazeMemory & previous,
  const std::vector<GazeDetection> & detections,
  const CameraModel & camera)
{
  (void)detections;
  (void)camera;
  return previous;
}

std::string gaze_xml()
{
  return
    "<root BTCPP_format=\"4\"><BehaviorTree ID=\"Gaze\">"
    "<AlwaysSuccess/></BehaviorTree></root>";
}

void register_gaze_nodes(BT::BehaviorTreeFactory & factory)
{
  (void)factory;
}

}  // namespace drill
