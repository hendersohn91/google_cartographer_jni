#include <iostream>
#include <memory>
#include <cartographer/mapping/map_builder.h>
#include <cartographer/common/configuration_file_resolver.h>
#include <cartographer/io/probability_grid_points_processor.h>
#include "cartographer_jni_cpp_bridge.h"
#ifdef WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

std::ifstream scanDataFile;
std::ifstream odomDataFile;
std::ofstream gridDataFile;


struct Quaternion
{
    double w, x, y, z;
};

Quaternion ToQuaternion(double yaw, double pitch, double roll);

bool readRangeDataFromFile(std::vector<float> *, long *);

bool readOdomDataFromFile(std::vector<float> *, long *);

std::string GetBuildDir( void ) {
    // get absolut path to build directory
	char buff[FILENAME_MAX];
    GetCurrentDir(buff, FILENAME_MAX);
    std::string current_working_dir(buff);
    return current_working_dir;
}

int main(int argc, char **argv) {
    // get laser scan handler
    JniCppBridge jniCppBridge(
    		GetBuildDir() + "/../config",
    		"cartographer_jni.lua");

    // create range data
    std::vector<float> nextOdom;
	long nextOdomTime;
    bool odomAvailable = readOdomDataFromFile(&nextOdom, &nextOdomTime);

    // iterate all data and try to sync them
    int odomCounter = 0;
    int rangeCounter = 0;

	std::vector<float> nextRange;
	long nextRangeTime;
	bool rangeDataAvailable = readRangeDataFromFile(&nextRange, &nextRangeTime);
	using SensorType = cartographer::mapping::TrajectoryBuilderInterface::SensorId::SensorType;
    while (odomAvailable || rangeDataAvailable) {
	    if (rangeDataAvailable && (nextRangeTime <= nextOdomTime || !odomAvailable)) {
		    jniCppBridge.HandleLaserScan("scan", nextRangeTime, nextRange);
            rangeCounter++;
		    // get new data
            rangeDataAvailable = readRangeDataFromFile(&nextRange, &nextRangeTime);
	    }
	    if (odomAvailable && (nextRangeTime >= nextOdomTime || !rangeDataAvailable)) {
		    jniCppBridge.HandleOdometry("odom", nextOdomTime, nextOdom);
		    odomCounter++;
		    // get new data
		    odomAvailable = readOdomDataFromFile(&nextOdom, &nextOdomTime);
	    }
    }

	std::cout << "collected " << rangeCounter << " range data!" << std::endl;
	std::cout << "collected " << odomCounter << " odometry data!" << std::endl;

	jniCppBridge.Finalize();
	std::vector<int> gridmap = jniCppBridge.GetGridMap();
	
	// write gridmap to file
	std::string resultGridPath = GetBuildDir() + "/grid_cpp.txt";
	if (!gridDataFile.is_open()) {
		gridDataFile.open(resultGridPath);
	}
	if (gridDataFile.is_open()) {
		int width = gridmap.at(gridmap.size() - 2);
		int height = gridmap.at(gridmap.size() - 1);
		
		
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int value = gridmap.at(y * width + x);
				if (value < 0)
					value = -20;
				gridDataFile << x << " " << y << " " << value << std::endl;
			}
		}
		
		gridDataFile.close();
		std::cout << "Gridfile is stored at " << resultGridPath << std::endl;
	} else {
		std::cerr << "Could not write grid file!" << std::endl;
	}
    return 0;
}

bool readOdomDataFromFile(std::vector<float> *odomData, long *time) {
	if (!odomDataFile.is_open()) {
		odomDataFile.open(GetBuildDir() + "/../../test_data/odomdata.txt");
	}
	std::string line;
	std::string delimiter = ",";
	int state = 0;
	std::string poseMarker = "pose";
	std::string timeMarker = "time";
	int amountOfSeparators = 27;
	if (odomDataFile.is_open()) {
		while (getline(odomDataFile, line)) {
			if (line.length() > amountOfSeparators + poseMarker.length()
			    && line.substr(amountOfSeparators, poseMarker.length()) == poseMarker) {
				state = 1;
			} else if (line.length() > amountOfSeparators + timeMarker.length()
			           && line.substr(amountOfSeparators, timeMarker.length()) == timeMarker) {
				state = 3;
			} else if (state == 1) {
				// line is pose x, y, z
				float translation_x = std::stof(line.substr(0, line.find(delimiter)));
				line.erase(0, line.find(delimiter) + delimiter.length());
				float translation_y = std::stof(line.substr(0, line.find(delimiter)));
				line.erase(0, line.find(delimiter) + delimiter.length());
				float translation_z = std::stof(line.substr(0, line.find(delimiter)));
				line.erase(0, line.find(delimiter) + delimiter.length());
				float rotation_yaw = std::stof(line.substr(0, line.find(delimiter)));
				line.erase(0, line.find(delimiter) + delimiter.length());
				float rotation_pitch = std::stof(line.substr(0, line.find(delimiter)));
				line.erase(0, line.find(delimiter) + delimiter.length());
				float rotation_roll = std::stof(line.substr(0, line.find(delimiter)));
				line.erase(0, line.find(delimiter) + delimiter.length());

				// Rotation quaternion as (w, x, y, z).
				// add data to odometry
				odomData->clear();
				odomData->emplace_back(translation_x);
				odomData->emplace_back(translation_y);
				odomData->emplace_back(translation_z);
				Quaternion quat = ToQuaternion(rotation_yaw, rotation_pitch, rotation_roll);
				odomData->emplace_back(quat.w);
				odomData->emplace_back(quat.x);
				odomData->emplace_back(quat.y);
				odomData->emplace_back(quat.z);
				return true;
			} else if (state == 3) {
				// line is time
				long x = std::stol(line);
				*time = x * 1000;
			}
		}
		odomDataFile.close();
	} else {
		std::cout << "Unable to open odom file" << std::endl;
	}
	return false;
}

bool readRangeDataFromFile(std::vector<float> *scanData, long *time) {
    if (!scanDataFile.is_open()) {
	    scanDataFile.open(GetBuildDir() + "/../../test_data/lrfdata.txt");
    }
    std::string line;
    std::string delimiter = ",";
    int state = 0;
    std::string poseMarker = "pose";
    std::string cloudMarker = "cloud";
    std::string timeMarker = "time";
    std::string outputMarker = "output";
    int amountOfSeparators = 27;
	::cartographer::sensor::TimedPointCloud ranges;
    if (scanDataFile.is_open()) {
        while (getline(scanDataFile, line)) {
            if (line.length() > amountOfSeparators + poseMarker.length()
                && line.substr(amountOfSeparators, poseMarker.length()) == poseMarker) {
                state = 1;
            } else if (line.length() > amountOfSeparators + cloudMarker.length()
                       && line.substr(amountOfSeparators, cloudMarker.length()) == cloudMarker) {
	            state = 2;
            } else if (line.length() > amountOfSeparators + timeMarker.length()
                       && line.substr(amountOfSeparators, timeMarker.length()) == timeMarker) {
	            state = 3;
            } else if (line.length() > amountOfSeparators + outputMarker.length()
                       && line.substr(amountOfSeparators, outputMarker.length()) == outputMarker) {
	            state = 4;
            } else if (line.length() > 4 && line.substr(0,4) == "Time") {
            } else if (state == 1) {
                // line is pose x, y, z
                float x = std::stof(line.substr(0, line.find(delimiter)));
                line.erase(0, line.find(delimiter) + delimiter.length());
                float y = std::stof(line.substr(0, line.find(delimiter)));
                line.erase(0, line.find(delimiter) + delimiter.length());
                float z = std::stof(line.substr(0, line.find(delimiter)));
                line.erase(0, line.find(delimiter) + delimiter.length());
                // clear scan data and write all data to vector
                scanData->clear();
                scanData->emplace_back(x);
                scanData->emplace_back(y);
                scanData->emplace_back(z);
                for (auto range : ranges) {
                	scanData->emplace_back(range.x());
                	scanData->emplace_back(range.y());
                	scanData->emplace_back(range.z());
	                scanData->emplace_back(range.w());
                }
	            return true;
            } else if (state == 2) {
                // line is point of cloud x, y, z
                float x = std::stof(line.substr(0, line.find(delimiter)));
                line.erase(0, line.find(delimiter) + delimiter.length());
                float y = std::stof(line.substr(0, line.find(delimiter)));
                line.erase(0, line.find(delimiter) + delimiter.length());
                float z = std::stof(line.substr(0, line.find(delimiter)));
                line.erase(0, line.find(delimiter) + delimiter.length());
                // timing not available -> 0
                ranges.push_back(Eigen::Vector4f(x, y, z, 0));
            } else if (state == 3) {
            	// line is time
            	long timeValue = std::stol(line);
	            *time = timeValue * 1000;
            }
        }
        scanDataFile.close();
    } else {
        std::cout << "Unable to open range file" << std::endl;
    }
	return false;
}

Quaternion ToQuaternion(double yaw, double pitch, double roll) // yaw (Z), pitch (Y), roll (X)
{
    // Abbreviations for the various angular functions
    double cy = cos(yaw * 0.5);
    double sy = sin(yaw * 0.5);
    double cp = cos(pitch * 0.5);
    double sp = sin(pitch * 0.5);
    double cr = cos(roll * 0.5);
    double sr = sin(roll * 0.5);

    Quaternion q;
    q.w = cy * cp * cr + sy * sp * sr;
    q.x = cy * cp * sr - sy * sp * cr;
    q.y = sy * cp * sr + cy * sp * cr;
    q.z = sy * cp * cr - cy * sp * sr;

    return q;
}
