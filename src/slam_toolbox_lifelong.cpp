/*
 * slam_toolbox
 * Copyright Work Modifications (c) 2019, Samsung Research America
 *
 * THE WORK (AS DEFINED BELOW) IS PROVIDED UNDER THE TERMS OF THIS CREATIVE
 * COMMONS PUBLIC LICENSE ("CCPL" OR "LICENSE"). THE WORK IS PROTECTED BY
 * COPYRIGHT AND/OR OTHER APPLICABLE LAW. ANY USE OF THE WORK OTHER THAN AS
 * AUTHORIZED UNDER THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * BY EXERCISING ANY RIGHTS TO THE WORK PROVIDED HERE, YOU ACCEPT AND AGREE TO
 * BE BOUND BY THE TERMS OF THIS LICENSE. THE LICENSOR GRANTS YOU THE RIGHTS
 * CONTAINED HERE IN CONSIDERATION OF YOUR ACCEPTANCE OF SUCH TERMS AND
 * CONDITIONS.
 *
 */

/* Author: Steven Macenski */

#include "slam_toolbox/slam_toolbox_lifelong.hpp"

namespace slam_toolbox
{

// the current_scans_ is a copy on the data stored by the dataset, can we hijack to not duplicate?
// or add a parameter to disable interactive mode & not add to the scan holder/disable interactive mode cb

// we keep increasing the vector of nodes/scans/constraints even though freeing the memory

using namespace ::karto;

/*****************************************************************************/
LifelongSlamToolbox::LifelongSlamToolbox(ros::NodeHandle& nh)
: SlamToolbox(nh)
/*****************************************************************************/
{
  loadPoseGraphByParams(nh);
  nh.param("lifelong_search_use_tree", use_tree, false);
}

/*****************************************************************************/
void LifelongSlamToolbox::laserCallback(
  const sensor_msgs::LaserScan::ConstPtr& scan)
/*****************************************************************************/
{
  // no odom info
  Pose2 pose;
  if(!pose_helper_->getOdomPose(pose, scan->header.stamp))
  {
    return;
  }

  // ensure the laser can be used
  karto::LaserRangeFinder* laser = getLaser(scan);

  if(!laser)
  {
    ROS_WARN_THROTTLE(5., "Failed to create laser device for"
      " %s; discarding scan", scan->header.frame_id.c_str());
    return;
  }

    // additional bounded node increase parameter (rate, or total for run or at all?)
    // pseudo-localization mode. If want to add a scan, but not deleting a scan, add to local buffer?
    // if (eval() && dont_add_more_scans) {addScan()} else {localization_add_scan()}
    // if (eval() && ctr / total < add_rate_scans) {addScan()} else {localization_add_scan()}

  if (addScan(laser, scan, pose))
  {
    evaluateNodeDepreciation(getLocalizedRangeScan(laser, scan, pose));  
  }

  return;
}

/*****************************************************************************/
void LifelongSlamToolbox::evaluateNodeDepreciation(
  LocalizedRangeScan* range_scan)
/*****************************************************************************/
{
  if (range_scan)
  {
    boost::mutex::scoped_lock lock(smapper_mutex_);

    const BoundingBox2& bb = range_scan->GetBoundingBox();
    const Size2<double> bb_size = bb.GetSize();
    double radius = sqrt(bb_size.GetWidth()*bb_size.GetWidth() +
      bb_size.GetHeight()*bb_size.GetHeight());
    std::map<double, Vertex<T>*> near_scan_vertices = 
      FindScansWithinRadius(range_scan, radius);

    computeScores(near_scan_vertices, range_scan);

    std::map<double, Vertex<T>*>::iterator it;
    for (it = near_scan_vertices.begin(); it != near_scan_vertices.end(); ++it)
    {
      if (it->first < 0.1)
      {
        removeFromSlamGraph(it->second);
      }
      else
      {
        updateScoresSlamGraph(it->first, it->second);
      }
    }

    delete range_scan;
  }

  return;
}

/*****************************************************************************/
std::map<double, Vertex<T>*> LifelongSlamToolbox::FindScansWithinRadius(
  LocalizedRangeScan* scan, const double& radius)
/*****************************************************************************/
{
  std::vector<Vertex<T>*> vertices;
  std::map<double, Vertex<T>*> vertices_labeled;

  // return with vertex pointer. new  Traverse function to return struct of object & vertex ptr
  if (use_tree)
  {
    // nanoflann adaptor: VertexVectorScanCenterNanoFlannAdaptor version of FindNearByScan -> FindNearByVertices
  }
  else
  {
    std::vector<Vertex<LocalizedRangeScan>*> vertices =
      FindNearLinkedVertices(scan, radius);
  }

  std::vector<Vertex<T>*>::iterator it;
  for (it = vertices.begin(); it != vertices.end(); ++it)
  {
    vertices_labeled.insert(std::pair<double, Vertex<T>*>(1.0, *it));
  }

  return vertices_labeled;
}

/*****************************************************************************/
void LifelongSlamToolbox::computeScores(
  std::map<double, Vertex<T>*>& near_scans,
  LocalizedRangeScan* range_scan)
/*****************************************************************************/
{
  // for each

  // must have some minimum overlap % before any score applied

      //    compute metric (probablistic, intersect over union, compute information loss)
      //      intersect over union on the 2 BB size/positions
      //      what about over multiple scans rather than a 1:1 mapping? -- decay so a series will remove
      //      score 0->1 given current score to scale/subtract
      //        number of area/total area overlapping decay score
      //        any scale store for intersect or only union?
}

/*****************************************************************************/
void LifelongSlamToolbox::removeFromSlamGraph(Vertex<T>*& vertex)
/*****************************************************************************/
{
  // must have LocalizedRangeScan* & vertex object, vertex->GetObject() gives you scan ptr
    // then I can refactor out the localization deletion code to use here

  // what do we do about the contraints that node had about it?Nothing?Transfer?
}

/*****************************************************************************/
void LifelongSlamToolbox::updateScoresSlamGraph(const double& score, 
  Vertex<T>*& vertex)
/*****************************************************************************/
{
  // update the vertex with its score
  // inputted graph itself, serialize -- persists between sessions and runs
}

/*****************************************************************************/
bool LifelongSlamToolbox::deserializePoseGraphCallback(
  slam_toolbox::DeserializePoseGraph::Request& req,
  slam_toolbox::DeserializePoseGraph::Response& resp)
/*****************************************************************************/
{
  if (req.match_type == procType::LOCALIZE_AT_POSE)
  {
    ROS_ERROR("Requested a localization deserialization "
      "in non-localization mode.");
    return false;
  }

  return SlamToolbox::deserializePoseGraphCallback(req, resp);
}

} // end namespace

int main(int argc, char** argv)
{
  ros::init(argc, argv, "slam_toolbox");
  ros::NodeHandle nh("~");
  ros::spinOnce();

  int stack_size;
  if (nh.getParam("stack_size_to_use", stack_size))
  {
    ROS_INFO("Node using stack size %i", (int)stack_size);
    const rlim_t max_stack_size = stack_size;
    struct rlimit stack_limit;
    getrlimit(RLIMIT_STACK, &stack_limit);
    if (stack_limit.rlim_cur < stack_size)
    {
      stack_limit.rlim_cur = stack_size;
    }
    setrlimit(RLIMIT_STACK, &stack_limit);
  }

  slam_toolbox::LifelongSlamToolbox llst(nh);

  ros::spin();
  return 0;
}
