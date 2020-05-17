//
// Created by prosem on 05.09.19.
//
#include "cartographer_jni_cpp_bridge.h"

JniCppBridge::JniCppBridge() {}

/**
 * Start the cartographer with the given path and filename to the config
 * @param filePath path where the config file is located
 * @param fileName the filename of the config
 */
JniCppBridge::JniCppBridge(std::string filePath, const std::string& fileName) {
    std::cout << "store logs to glog directory" << std::endl;
    if (mkdir("glogs", 0777) == -1)
        std::cout << "INFO for glog dir creation:  " << strerror(errno) << std::endl;

    FLAGS_log_dir = "./glogs";
	google::InitGoogleLogging("");
	LOG(INFO) << "Hello Google!" << std::endl;

	// config file
	auto file_resolver = cartographer::common::make_unique<
			cartographer::common::ConfigurationFileResolver>(
			std::vector<std::string>{
					std::move(filePath)});
	const std::string code = file_resolver->GetFileContentOrDie(fileName);
	auto lua_parameter_dictionary = cartographer::common::LuaParameterDictionary::NonReferenceCounted(code, std::move(
			file_resolver));

	// get resolution from config
	resolution = lua_parameter_dictionary
			->GetDictionary("trajectory_builder")
			->GetDictionary("trajectory_builder_2d")
			->GetDictionary("submaps")
			->GetDictionary("grid_options_2d")
			->GetDouble("resolution");

	// create map builder
	::cartographer::mapping::proto::MapBuilderOptions map_builder_options = cartographer::mapping::CreateMapBuilderOptions(
			lua_parameter_dictionary->GetDictionary("map_builder").get());
	map_builder = cartographer::common::make_unique<cartographer::mapping::MapBuilder>(map_builder_options);
	// get sensor ids
	const std::set<cartographer::mapping::TrajectoryBuilderInterface::SensorId>
			expected_sensor_ids = GetExpectedSensorIds();

	cartographer::mapping::proto::TrajectoryBuilderOptions trajectory_options =
			::cartographer::mapping::CreateTrajectoryBuilderOptions(
					lua_parameter_dictionary->GetDictionary("trajectory_builder").get());

	// create new trajectory
	trajectory_id = map_builder->AddTrajectoryBuilder(
			expected_sensor_ids, trajectory_options, nullptr);
	trajectory_builder_ = map_builder->GetTrajectoryBuilder(trajectory_id);
}

/**
 * Give odometry data to the cartographer
 * @param sensor_id the sensor that created the odometry data
 * @param timeValue the time when the data was created
 * @param odomDataValue the odometry data
 */
void JniCppBridge::HandleOdometry(const std::string &sensor_id, const long timeValue,
                                  std::vector<float> odomDataValue) {
	// convert time
	cartographer::common::Time time = cartographer::common::FromUniversal(timeValue);
	// convert odometry translation
	std::array<double, 3> translation = {{odomDataValue.at(0), odomDataValue.at(1), odomDataValue.at(2)}};
	// Rotation quaternion as (w, x, y, z).
	std::array<double, 4> rotation = {{odomDataValue.at(3), odomDataValue.at(4), odomDataValue.at(5), odomDataValue.at(6)}};

	// add data to odometry
	lastPose = cartographer::transform::Rigid3d::FromArrays(rotation, translation);
	auto odomData = cartographer::sensor::OdometryData{
			time, lastPose};
	trajectory_builder_->AddSensorData(sensor_id, odomData);
}

/**
 * Give range data to the cartographer
 * @param sensor_id the sensor that created the range data
 * @param timeValue the time when the data was created
 * @param scanData the range data
 */
void JniCppBridge::HandleLaserScan(const std::string &sensor_id, const long timeValue, std::vector<float> scanData) {
	// convert time
	cartographer::common::Time time = cartographer::common::FromUniversal(timeValue);
	cartographer::sensor::TimedPointCloudData timedPointCloud = cartographer::sensor::TimedPointCloudData();
	timedPointCloud.time = time;
	// iterate scan data: first 4 values is origin (x,y,z,angle)
	timedPointCloud.origin = Eigen::Vector3f(scanData.at(0), scanData.at(1), scanData.at(2));
	// iterate range data (x,y,z)
	for (int scanIndex = 3; scanIndex + 3 < scanData.size(); scanIndex += 4) {
		timedPointCloud.ranges.push_back(Eigen::Vector4f(
				scanData.at(scanIndex), scanData.at(scanIndex + 1),
				scanData.at(scanIndex + 2), scanData.at(scanIndex + 3)));
	}
    trajectory_builder_->AddSensorData(sensor_id, timedPointCloud);
}

/**
 * Build all submapSlices to build a gridmap later.
 * @param submaps submaps to construct slices for
 */
void JniCppBridge::CreateGridMap(
		const cartographer::mapping::MapById<cartographer::mapping::SubmapId,
		cartographer::mapping::PoseGraphInterface::SubmapData>& submaps) {
	for (auto submap : submaps) {
		// get textures for each submap
		cartographer::mapping::proto::SubmapQuery::Response response_proto;
		submap.data.submap.get()->ToResponseProto(submap.data.pose, &response_proto);
		auto proto_textures = response_proto.textures();
		cartographer::io::SubmapSlice &submap_slice = submap_slices_[submap.id];
		submap_slice.pose = submap.data.pose;

		// convert proto stuff to submap slices
		if (proto_textures.data() == nullptr) {
			continue;
		}
		CHECK(!proto_textures.empty());

		// We use the first texture only. By convention this is the highest
		// resolution texture and that is the one we want to use to construct the
		// map for ROS.
		const auto &proto_texture = proto_textures.Get(0);
		auto slice_pose = cartographer::transform::Rigid3d(
				{proto_texture.slice_pose().translation().x(),
				 proto_texture.slice_pose().translation().y(), proto_texture.slice_pose().translation().z()},
				cartographer::transform::ToEigen(proto_texture.slice_pose().rotation()));
		const std::string compressed_cells(proto_texture.cells().begin(),
		                                   proto_texture.cells().end());
		auto submap_texture = ::cartographer::io::SubmapTexture{
				::cartographer::io::UnpackTextureData(compressed_cells, proto_texture.width(),
				                                      proto_texture.height()),
				proto_texture.width(), proto_texture.height(), proto_texture.resolution(),
				slice_pose};
		submap_slice.width = proto_texture.width();
		submap_slice.height = proto_texture.height();
		submap_slice.slice_pose = slice_pose;
		submap_slice.resolution = proto_texture.resolution();
		submap_slice.cairo_data.clear();
		submap_slice.surface = ::cartographer::io::DrawTexture(
				submap_texture.pixels.intensity, submap_texture.pixels.alpha,
				submap_texture.width, submap_texture.height,
				&submap_slice.cairo_data);
	}
}

/**
 * Get the complete Gridmap with all Submaps.
 * @return the grid map
 */
std::vector<int> JniCppBridge::GetGridMap() {
	CreateGridMap(map_builder->pose_graph()->GetAllSubmapData());
	// paint submap slices
	auto painted_submap_slices = PaintSubmapSlices(submap_slices_, resolution);
	const uint32_t *pixel_data = reinterpret_cast<uint32_t *>(
			cairo_image_surface_get_data(painted_submap_slices.surface.get()));
	const int width = cairo_image_surface_get_width(painted_submap_slices.surface.get());
	const int height =
			cairo_image_surface_get_height(painted_submap_slices.surface.get());

	std::vector<int> occupancy_grid;
	for (int y = height - 1; y >= 0; --y) {
		for (int x = 0; x < width; ++x) {
			const uint32_t packed = pixel_data[y * width + x];
			const unsigned char color = packed >> 16;
			const unsigned char observed = packed >> 8;
			const int value =
					observed == 0
					? -1
					: ::cartographer::common::RoundToInt((1. - color / 255.) * 100.);
			CHECK_LE(-1, value);
			CHECK_GE(100, value);
			occupancy_grid.push_back(value);
		}
	}

	occupancy_grid.push_back(painted_submap_slices.origin.x());
	occupancy_grid.push_back(painted_submap_slices.origin.y());
	occupancy_grid.push_back(width);
	occupancy_grid.push_back(height);

	// paint grid with gnuplot
	// data structure:
	// x-value y-value grid-value
	// unobserved values should have a low negative value for gnuplot
	// commands:
	// set palette defined ( -1 "#000000", 0 "#00FF00", 50 "#0000FF", 100 "#FF0000" )
	// plot "grid.txt" u 1:2:3 ls 5 lc palette z

	return occupancy_grid;
}

/**
 * Get all possible sensors with id and type.
 * @return the expected sensors
 */
std::set<cartographer::mapping::TrajectoryBuilderInterface::SensorId> JniCppBridge::GetExpectedSensorIds() {
	using SensorId = cartographer::mapping::TrajectoryBuilderInterface::SensorId;
	using SensorType = SensorId::SensorType;

	if (expected_sensor_ids.empty()) {
	    LOG(INFO) << "No sensors added. Use default (scan and odometry)";
		expected_sensor_ids.insert(SensorId{SensorType::RANGE, "scan"});
		expected_sensor_ids.insert(SensorId{SensorType::ODOMETRY, "odom"});
	}
	return expected_sensor_ids;
}

/**
 * Add a sensor.
 * @param sensorType RANGE, IMU, ODOMETRY, FIXED_FRAME_POSE, LANDMARK, LOCAL_SLAM_RESULT
 * @param sensorIdValue the id for the sensor
 * @return true if sensor was added, false otherwise
 */
bool JniCppBridge::AddExpectedSensorId(std::string sensorType, std::string sensorIdValue) {
    using SensorId = cartographer::mapping::TrajectoryBuilderInterface::SensorId;
    using SensorType = SensorId::SensorType;
    if (sensorType == "RANGE") {
        auto newRangeSensorId = SensorId{SensorType::RANGE, sensorIdValue};
        if (expected_sensor_ids.count(newRangeSensorId) == 0) {
            expected_sensor_ids.insert(newRangeSensorId);
            return true;
        }
    } else if (sensorType == "ODOMETRY") {
        auto newOdomSensorId = SensorId{SensorType::ODOMETRY, sensorIdValue};
        if (expected_sensor_ids.count(newOdomSensorId) == 0) {
            expected_sensor_ids.insert(newOdomSensorId);
            return true;
        }
    }
    return false;
}

std::vector<float> JniCppBridge::GetCurrentPose() {
    std::vector<float> result;
    cartographer::transform::Rigid3d current_pose =
            map_builder->pose_graph()->GetLocalToGlobalTransform(trajectory_id) *
            lastPose;
    result.emplace_back(current_pose.translation().x());
    result.emplace_back(current_pose.translation().y());
    result.emplace_back(current_pose.translation().z());
    result.emplace_back(current_pose.rotation().w());
    result.emplace_back(current_pose.rotation().x());
    result.emplace_back(current_pose.rotation().y());
    result.emplace_back(current_pose.rotation().z());
    return result;
}

/**
 * Finalize the trajectory.
 */
void JniCppBridge::Finalize() {
	map_builder->FinishTrajectory(trajectory_id);
	map_builder->pose_graph()->RunFinalOptimization();

	std::cout << "Trajectory Nodes: " << map_builder->pose_graph()->GetTrajectoryNodes().size()
	          << " Submaps: " << map_builder->pose_graph()->GetAllSubmapData().SizeOfTrajectoryOrZero(trajectory_id) << std::endl;
}
