//
// Created by prosem on 05.09.19.
//

#ifndef CARTOGRAPHER_JNI_CPP_BRIDGE_H
#define CARTOGRAPHER_JNI_CPP_BRIDGE_H

#include <sys/stat.h>
#include <cartographer/common/configuration_file_resolver.h>
#include <cartographer/mapping/2d/grid_2d.h>
#include <cartographer/io/submap_painter.h>
#include <cartographer/mapping/map_builder.h>

class JniCppBridge {
public:
	JniCppBridge();
	JniCppBridge(std::string, const std::string&);
	void HandleOdometry(const std::string &, long,std::vector<float> );
	void HandleLaserScan(const std::string &, long, std::vector<float>);
	void Finalize();
	std::vector<int> GetGridMap();
    bool AddExpectedSensorId(std::string, std::string);
    std::vector<float> GetCurrentPose();

private:
	void CreateGridMap(const cartographer::mapping::MapById<cartographer::mapping::SubmapId, cartographer::mapping::PoseGraphInterface::SubmapData> &submaps);
	std::set<cartographer::mapping::TrajectoryBuilderInterface::SensorId> GetExpectedSensorIds();

	std::map<cartographer::mapping::SubmapId, cartographer::io::SubmapSlice> submap_slices_;
	cartographer::mapping::TrajectoryBuilderInterface* trajectory_builder_;
	std::unique_ptr<cartographer::mapping::MapBuilderInterface> map_builder;
	int trajectory_id;
	double resolution;
	cartographer::transform::Rigid3d lastPose;
	std::set<cartographer::mapping::TrajectoryBuilderInterface::SensorId> expected_sensor_ids;
};

#endif //CARTOGRAPHER_JNI_CPP_BRIDGE_H
