/*                                                                                                                       
 * Copyright (c) 2010, Willow Garage, Inc.                                                                               
 * All rights reserved.                                                                                                  
 *                                                                                                                       
 * Redistribution and use in source and binary forms, with or without                                                    
 * modification, are permitted provided that the following conditions are met:                                           
 *                                                                                                                       
 *     * Redistributions of source code must retain the above copyright                                                  
 *       notice, this list of conditions and the following disclaimer.                                                   
 *     * Redistributions in binary form must reproduce the above copyright                                               
 *       notice, this list of conditions and the following disclaimer in the                                             
 *       documentation and/or other materials provided with the distribution.                                            
 *     * Neither the name of the Willow Garage, Inc. nor the names of its                                                
 *       contributors may be used to endorse or promote products derived from                                            
 *       this software without specific prior written permission.                                                        
 *                                                                                                                       
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"                                           
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE                                             
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE                                            
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE                                              
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR                                                   
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF                                                  
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS                                              
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN                                               
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)                                               
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE                                            
 * POSSIBILITY OF SUCH DAMAGE.                                                                                           
 */
#include "ros/ros.h"
#include "pluginlib/class_list_macros.h"
#include "nodelet/nodelet.h"

#include "sensor_msgs/LaserScan.h"

#include "pcl/point_cloud.h"
#include "pcl_ros/point_cloud.h"
#include "pcl/point_types.h"
#include "pcl/ros/conversions.h"

#include <visualization_msgs/Marker.h>

// Dynamic reconfigure includes.
#include <dynamic_reconfigure/server.h>
// Auto-generated from cfg/ directory.
#include <pcl_to_scan/cloud_to_scan_paramsConfig.h>

namespace pcl_to_scan {
typedef pcl::PointCloud<pcl::PointXYZ> PointCloud;

class CloudToScan: public nodelet::Nodelet {
public:
	//Constructor
	CloudToScan() :
		min_height_(0.10), max_height_(0.15), u_min_(100), u_max_(150),
				output_frame_id_("/openi_depth_frame") {
	}

private:
	virtual void onInit() {
		ros::NodeHandle& nh = getNodeHandle();
		ros::NodeHandle& private_nh = getPrivateNodeHandle();

		dynamic_set = false;

		private_nh.getParam("min_height", min_height_);
		private_nh.getParam("max_height", max_height_);

		private_nh.getParam("row_min", u_min_);
		private_nh.getParam("row_max", u_max_);

		private_nh.getParam("output_frame_id", output_frame_id_);
		pub_ = nh.advertise<sensor_msgs::LaserScan> ("scan", 10);
		sub_ = nh.subscribe<PointCloud> ("cloud", 10, &CloudToScan::callback,
				this);

		marker_pub = nh.advertise<visualization_msgs::Marker> (
				"visualization_marker", 10);

		pcl_to_scan::cloud_to_scan_paramsConfig config;
		config.max_height = max_height_;
		config.min_height = min_height_;

		// Set up a dynamic reconfigure server.
		dr_srv = new dynamic_reconfigure::Server < pcl_to_scan::cloud_to_scan_paramsConfig > ();


		cb = boost::bind(&CloudToScan::configCallback, this, _1, _2);
		dr_srv->updateConfig(config);
		dr_srv->setCallback(cb);
	}


	void configCallback(pcl_to_scan::cloud_to_scan_paramsConfig &config, uint32_t level) {
		// Set class variables to new values. They should match what is input at the dynamic reconfigure GUI.
		min_height_ = config.min_height;
		max_height_ = config.max_height;
	} // end configCallback()

	void callback(const PointCloud::ConstPtr& cloud) {
		sensor_msgs::LaserScanPtr output(new sensor_msgs::LaserScan());
		NODELET_DEBUG("Got cloud");
		//Copy Header
		output->header = cloud->header;
		output->header.frame_id = output_frame_id_;

                /*output->angle_min = -M_PI / 1.2 ; //Default: -M_PI/6
		output->angle_max = M_PI / 1.2 ; //Default: M_PI/6
		output->angle_increment = M_PI / 180.0 / (1.0 / 5.0) ; //Default: M_PI / 180.0 / 2.0
		output->time_increment = 0.0;
		output->scan_time = 1.0 / 30.0; //Default: 1.0 / 30.0
		output->range_min = 0.1;
		output->range_max = 10.0;*/
		output->angle_min = -M_PI / 6;
		output->angle_max = M_PI / 6;
		output->angle_increment = M_PI / 180.0 / 2.0;
		output->time_increment = 0.0;
		output->scan_time = 1.0 / 30.0;
		output->range_min = 0.1;
		output->range_max = 10.0;

		uint32_t ranges_size = std::ceil(
				(output->angle_max - output->angle_min)
						/ output->angle_increment);
		output->ranges.assign(ranges_size, output->range_max + 1.0);

		//pcl::PointCloud<pcl::PointXYZ> cloud;
		//pcl::fromROSMsg(*input, cloud);

		visualization_msgs::Marker line_list;

		float min_x = 100;
		float max_x = -100;
		float min_y = 100;
		float max_y = -100;

		//    NODELET_INFO("New scan...");
		for (PointCloud::const_iterator it = cloud->begin(); it != cloud->end(); ++it) {
			const float &x = it->x;
			const float &y = it->y;
			const float &z = it->z;

			if (x < min_x)
				min_x = x;
			if (x > max_x)
				max_x = x;

			if (y < min_y)
				min_y = y;
			if (y > max_y)
				max_y = y;

			/*    for (uint32_t u = std::max((uint32_t)u_min_, 0U); u < std::min((uint32_t)u_max_, cloud.height - 1); u++)
			 for (uint32_t v = 0; v < cloud.width -1; v++)
			 {
			 //NODELET_ERROR("%d %d,  %d %d", u, v, cloud.height, cloud.width);
			 pcl::PointXYZ point = cloud.at(v,u); ///WTF Column major?
			 const float &x = point.x;
			 const float &y = point.y;
			 const float &z = point.z;
			 */

			if (std::isnan(x) || std::isnan(y) || std::isnan(z)) {
				NODELET_DEBUG("rejected for nan in point(%f, %f, %f)\n", x, y,
						z);
				continue;
			}

			if (y > max_height_ || y < min_height_) {
				NODELET_DEBUG("rejected for height %f not in range (%f, %f)\n",
						y, min_height_, max_height_);
				continue;
			}
			double angle = atan2(x, z);
			if (angle < output->angle_min || angle > output->angle_max) {
				NODELET_DEBUG("rejected for angle %f not in range (%f, %f)\n",
						angle, output->angle_min, output->angle_max);
				continue;
			}

			int index = (angle - output->angle_min) / output->angle_increment;
			//    NODELET_INFO("index xyz( %f %f %f) angle %f index %d", x, y, z, angle, index);
			double range_sq = x * x + z * z;
			if (output->ranges[index] * output->ranges[index] > range_sq)
				output->ranges[index] = sqrt(range_sq);

		}

		NODELET_INFO("X: %f %f, Y: %f %f", min_x, max_x, min_y, max_y);

		line_list.header = cloud->header;
		line_list.header.frame_id = output_frame_id_;
		line_list.ns = "points_and_lines";
		line_list.action = visualization_msgs::Marker::ADD;
		line_list.pose.orientation.w = 1.0;

		line_list.id = 0;

		line_list.type = visualization_msgs::Marker::LINE_LIST;

		line_list.scale.z = 0.1;
		//    line_list.color.r = 1.0;
		line_list.color.a = 1.0;

		// Create the vertices for the points and lines
		for (uint32_t i = 0; i < ranges_size; ++i) {
			float rng = output->ranges[i];
			float a = output->angle_min + i * output->angle_increment;
			float z = rng * cos(a);
			float x = rng * sin(a);

			geometry_msgs::Point p;
			std_msgs::ColorRGBA col;
			p.z = z;
			p.x = x;
			p.y = min_height_;

			col.g = rng / (output->range_max);
			col.r = 1.0 - col.g;
			line_list.colors.push_back(col);
			line_list.colors.push_back(col);

			// The line list needs two points for each line
			line_list.points.push_back(p);
			p.z = max_height_;
			line_list.points.push_back(p);
		}

		marker_pub.publish(line_list);
		pub_.publish(output);
	}

	double min_height_, max_height_;
	int32_t u_min_, u_max_;
	std::string output_frame_id_;

	bool dynamic_set;

	ros::Publisher pub_;
	ros::Subscriber sub_;
	ros::Publisher marker_pub;

	dynamic_reconfigure::Server < pcl_to_scan::cloud_to_scan_paramsConfig > * dr_srv;
	dynamic_reconfigure::Server<pcl_to_scan::cloud_to_scan_paramsConfig>::CallbackType cb;
};

PLUGINLIB_DECLARE_CLASS(pcl_to_scan, CloudToScan, pcl_to_scan::CloudToScan, nodelet::Nodelet);
}
