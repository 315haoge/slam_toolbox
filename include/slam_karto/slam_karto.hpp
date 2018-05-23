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

/**

@mainpage karto_gmapping

@htmlinclude manifest.html

*/

#include "ros/ros.h"
#include "ros/console.h"
#include "message_filters/subscriber.h"
#include "tf/transform_broadcaster.h"
#include "tf/transform_listener.h"
#include "tf/message_filter.h"
#include "visualization_msgs/MarkerArray.h"
#include <pluginlib/class_loader.h>

#include "nav_msgs/MapMetaData.h"
#include "sensor_msgs/LaserScan.h"
#include "nav_msgs/GetMap.h"

#include "open_karto/Mapper.h"

#include <boost/thread.hpp>

#include "slam_karto/Pause.h"

#include <string>
#include <map>
#include <vector>
#include <queue>

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

  void Run();

private:
  void laserCallback(const sensor_msgs::LaserScan::ConstPtr& scan);

  void setParams(ros::NodeHandle& nh);
  void setSolver(ros::NodeHandle& private_nh_);
  void setROSInterfaces();

  bool mapCallback(nav_msgs::GetMap::Request  &req,
                   nav_msgs::GetMap::Response &res);
  bool pauseCallback(slam_karto::Pause::Request& req,
                     slam_karto::Pause::Response& resp);

  bool getOdomPose(karto::Pose2& karto_pose, const ros::Time& t);
  karto::LaserRangeFinder* getLaser(const sensor_msgs::LaserScan::ConstPtr& scan);
  bool addScan(karto::LaserRangeFinder* laser,
               const sensor_msgs::LaserScan::ConstPtr& scan,
               karto::Pose2& karto_pose);
  bool updateMap();
  void publishLoop(double transform_publish_period);
  void publishGraphVisualization();

  bool isPaused();
  void togglePause();

  // ROS handles
  ros::NodeHandle node_;
  tf::TransformListener tf_;
  tf::TransformBroadcaster* tfB_;
  message_filters::Subscriber<sensor_msgs::LaserScan>* scan_filter_sub_;
  tf::MessageFilter<sensor_msgs::LaserScan>* scan_filter_;
  ros::Publisher sst_;
  ros::Publisher marker_publisher_;
  ros::Publisher sstm_;
  ros::ServiceServer ss_;
  ros::ServiceServer ssPause_;

  // The map that will be published / send to service callers
  nav_msgs::GetMap::Response map_;

  // Storage for ROS parameters
  std::string odom_frame_;
  std::string map_frame_;
  std::string base_frame_;
  int throttle_scans_;
  ros::Duration map_update_interval_;
  double resolution_;
  boost::mutex map_mutex_;
  boost::mutex map_to_odom_mutex_;
  boost::mutex pause_mutex_;
  bool publish_occupancy_map_;

  // Karto bookkeeping
  karto::Mapper* mapper_;
  karto::Dataset* dataset_;
  std::map<std::string, karto::LaserRangeFinder*> lasers_;
  std::map<std::string, bool> lasers_inverted_;

  // Internal state
  bool got_map_;
  int laser_count_;
  boost::thread *transform_thread_, *run_thread_;
  tf::Transform map_to_odom_;
  bool inverted_laser_;
  double max_laser_range_;
  bool paused_;

  // pluginlib
  pluginlib::ClassLoader<karto::ScanSolver> solver_loader_;
  boost::shared_ptr<karto::ScanSolver> solver_;

  std::queue<posed_scan> q_;
  double minimum_time_interval_;
  double last_scan_time_;

};
