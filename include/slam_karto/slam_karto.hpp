/*
 * slam_karto
 * Copyright (c) 2008, Willow Garage, Inc.
 * Copyright Work Modifications (c) 2017, Simbe Robotics, Inc.
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

/* Author: Brian Gerkey */
/* Modified: Steven Macenski */

#include "ros/ros.h"
#include "ros/console.h"
#include "message_filters/subscriber.h"
#include "tf/transform_broadcaster.h"
#include "tf/transform_listener.h"
#include "tf/message_filter.h"
#include <tf/LinearMath/Matrix3x3.h>
#include <tf/transform_datatypes.h>
#include "visualization_msgs/MarkerArray.h"
#include <visualization_msgs/InteractiveMarker.h>
#include <visualization_msgs/InteractiveMarkerControl.h> 
#include <visualization_msgs/InteractiveMarkerFeedback.h>

#include <interactive_markers/interactive_marker_server.h>
#include <interactive_markers/menu_handler.h>
#include <pluginlib/class_loader.h>

#include "nav_msgs/MapMetaData.h"
#include "sensor_msgs/LaserScan.h"
#include "nav_msgs/GetMap.h"

#include "open_karto/Mapper.h"

#include <boost/thread.hpp>

#include "slam_karto/Pause.h"
#include "slam_karto/ClearQueue.h"
#include "slam_karto/ToggleInteractive.h"
#include "slam_karto/Clear.h"
#include "slam_karto/SaveMap.h"
#include "slam_karto/LoopClosure.h"

#include <string>
#include <map>
#include <vector>
#include <queue>
#include <cstdlib>

// compute linear index for given map coords
#define MAP_IDX(sx, i, j) ((sx) * (j) + (i))

struct posed_scan
{
  posed_scan(sensor_msgs::LaserScan::ConstPtr scan_in, karto::Pose2 pose_in) :
             scan(scan_in), pose(pose_in) 
  {
  }
  sensor_msgs::LaserScan::ConstPtr scan;
  karto::Pose2 pose;
};

class SlamKarto
{
public:
  SlamKarto();
  ~SlamKarto();

private:
  // threads
  void Run();
  void publishVisualizations();
  void publishLoop(double transform_publish_period);

  // setup
  void setParams(ros::NodeHandle& nh);
  void setSolver(ros::NodeHandle& private_nh_);
  void setROSInterfaces();

  // callbacks
  void laserCallback(const sensor_msgs::LaserScan::ConstPtr& scan);
  bool mapCallback(nav_msgs::GetMap::Request  &req,
                   nav_msgs::GetMap::Response &res);
  bool pauseCallback(slam_karto::Pause::Request& req,
                     slam_karto::Pause::Response& resp);
  bool clearQueueCallback(slam_karto::ClearQueue::Request& req,
                          slam_karto::ClearQueue::Response& resp);
  bool InteractiveCallback(slam_karto::ToggleInteractive::Request  &req,
                           slam_karto::ToggleInteractive::Response &resp);
  bool clearChangesCallback(slam_karto::Clear::Request  &req,
                            slam_karto::Clear::Response &resp);
  bool saveMapCallback(slam_karto::SaveMap::Request  &req,
                       slam_karto::SaveMap::Response &resp);
  bool manualLoopClosureCallback(slam_karto::LoopClosure::Request  &req,
                                 slam_karto::LoopClosure::Response &resp);

  // functional bits
  bool getOdomPose(karto::Pose2& karto_pose, const ros::Time& t);
  karto::LaserRangeFinder* getLaser(const sensor_msgs::LaserScan::ConstPtr& scan);
  bool addScan(karto::LaserRangeFinder* laser,
               const sensor_msgs::LaserScan::ConstPtr& scan,
               karto::Pose2& karto_pose);
  bool updateMap();
  void publishGraph();
  void MoveNode(const int& id, const Eigen::Vector3d& pose, const bool correct = true);
  void processInteractiveFeedback(const visualization_msgs::InteractiveMarkerFeedbackConstPtr& feedback);
  void ClearMovedNodes();
  void AddMovedNodes(const int& id, Eigen::Vector3d vec);

  // state
  bool isPaused();
  void togglePause();

  // ROS handles
  ros::NodeHandle node_;
  tf::TransformListener tf_;
  tf::TransformBroadcaster* tfB_;
  message_filters::Subscriber<sensor_msgs::LaserScan>* scan_filter_sub_;
  tf::MessageFilter<sensor_msgs::LaserScan>* scan_filter_;
  ros::Publisher sst_, sstm_, marker_publisher_;
  ros::ServiceServer ssMap_, ssPause_, ssClear_, ssInteractive_;
  nav_msgs::GetMap::Response map_;

  // Storage for ROS parameters
  std::string odom_frame_, map_frame_, base_frame_;
  int throttle_scans_;
  ros::Duration map_update_interval_;
  double resolution_, minimum_time_interval_, minimum_travel_distance_;
  bool publish_occupancy_map_;

  // Karto bookkeeping
  karto::Mapper* mapper_;
  karto::Dataset* dataset_;
  std::map<std::string, karto::LaserRangeFinder*> lasers_;
  std::map<std::string, bool> lasers_inverted_;

  // Internal state
  int laser_count_;
  boost::thread *transform_thread_, *run_thread_, *visualization_thread_;
  tf::Transform map_to_odom_;
  bool paused_, inverted_laser_;
  double max_laser_range_;
  karto::Pose2 current_pose_;
  std::queue<posed_scan> q_;
  boost::mutex map_mutex_, pause_mutex_, map_to_odom_mutex_, mapper_mutex_, interactive_mutex_, moved_nodes_mutex;

  // visualization
  interactive_markers::InteractiveMarkerServer* interactive_server_;
  bool interactive_mode_;
  std::map<int, Eigen::Vector3d> moved_nodes_;

  // pluginlib
  pluginlib::ClassLoader<karto::ScanSolver> solver_loader_;
  boost::shared_ptr<karto::ScanSolver> solver_;
};
