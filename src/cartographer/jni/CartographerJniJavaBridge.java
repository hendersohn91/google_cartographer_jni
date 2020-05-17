package cartographer.jni;

import java.nio.file.FileSystems;
import java.util.Arrays;

import static java.lang.Math.*;

/**
 * This class provides a JNI for google Cartographer 2D Mapping.
 */
public class CartographerJniJavaBridge {
    // Ensure library is only loaded once
    static {
        if (System.getProperty("os.name").startsWith("Windows")) {
            // Windows based
            System.err.println("Cartographer is currently only supported for Ubuntu.");
            System.exit(1);
        } else {
            // Unix based
            try {
                System.load(
                        FileSystems.getDefault()
                                .getPath("./build/natives/linux/libcartographer_native_interface.so")  // Dynamic link
                                .normalize().toAbsolutePath().toString());
            } catch (UnsatisfiedLinkError e) {
                System.err.println("Could not load " + FileSystems.getDefault()
                        .getPath("./build/natives/linux/libcartographer_native_interface.so")
                        .normalize().toAbsolutePath().toString());
                throw e;
            }
        }
    }

    /**
     * Start the cartographer.
     * @param configFilePath the path to the location of the config file
     * @param configFileName the name of the config file
     */
    public native void startCartographer(String configFilePath, String configFileName);

    /**
     * Send odometry data to the cartographer
     * @param sensorId the sensor id calculated at cartographer_jni_cpp_bridge.cpp
     * @param time the time of the data in nanoseconds
     * @param data the data with translation and rotation as quaternion
     *             (translation_x,translation_y,translation_z,
     *             rotation_x, rotation_y, rotation_z, rotation_w)
     */
    private native void sendOdometryData(String sensorId, long time,float[] data);

    /**
     * Send odometry data to the cartographer with odometry rotation as quaterion
     * @param sensorId the sensor id calculated at cartographer_jni_cpp_bridge.cpp
     * @param time the time of the data in nanoseconds
     * @param data the data with translation and rotation as quaternion
     *             (translation_x,translation_y,translation_z,
     *             rotation_x, rotation_y, rotation_z, rotation_w)
     */
    public void sendOdometryDataWithQuaternion(String sensorId, long time,float[] data) {
        if (data.length == 7) {
            this.sendOdometryData(sensorId, time, data);
        } else {
            throw new IllegalArgumentException("Pose array is too short for using quaternions" +
                    "Length is " + data.length + " but should be 7 (translation_x,translation_y,translation_z, " +
                    "rotation_x, rotation_y, rotation_z, rotation_w)");
        }
    }

    /**
     * Send odometry data to the cartographer with odometry rotation as euler angles
     * @param sensorId the sensor id calculated at cartographer_jni_cpp_bridge.cpp
     * @param time the time of the data in nanoseconds
     * @param data the data with translation and rotation as quaternion
     *             (translation_x,translation_y,translation_z,
     *             yaw (Z), pitch (Y), roll (X))
     */
    public void sendOdometryDataWithEuler(String sensorId, long time,float[] data) {
        if (data.length == 6) {
            // convert euler to quaternion
            float[] eulerArray = Arrays.copyOfRange(data, 3, 6);
            float[] quaternionArray = convertEulerAngleToQuaternion(eulerArray);
            if (quaternionArray == null) {
                throw new IllegalArgumentException("Cannot convert euler angles to quaternions!" + Arrays.toString(data));
            } else {
                float[] quaternionData = {data[0], data[1], data[2],
                        quaternionArray[0], quaternionArray[1], quaternionArray[2], quaternionArray[3]};
                this.sendOdometryData(sensorId, time, quaternionData);
            }
        } else {
            throw new IllegalArgumentException("Pose array is too short for using euler angles" +
                    "Length is " + data.length + " but should be 6 (translation_x,translation_y,translation_z, yaw (Z), " +
                    "pitch (Y), roll (X)))");
        }
    }

    /**
     * Send range data to the cartographer.
     * @param sensorId the sensor id calculated at cartographer_jni_cpp_bridge.cpp
     * @param time the time of the scan in nanoseconds
     * @param data the range data (x, y, z, time)
     */
    public native void sendLaserScanData(String sensorId, long time, float[] data);

    /**
     * Finalize trajectory and run final optimizations;
     */
    public native void finalizeCartographer();

    /**
     * Paint the grid map and get probability grid.
     * @return probability grid
     * format: yx
     * [00,01,02,03,04,...,
     *  10,11,12,13,14,...,
     *  20,21,22,23,24,...,
     *  trajectory_start_pos_x, trajectory_start_pos_y,
     *  width, height]
     */
    public native int[] getGridMap();

    /**
     * Get the optimized pose.
     * @return optimized pose
     * [translation.x,translation.y,translation.z,
     * quaternion.w,quaternion.x,quaternion.y,quaternion.z]
     */
    private native float[] getCurrentPose();

    /**
     * see @getCurrentPose
     * @return the current pose with rotation in quaternions
     * [translation.x,translation.y,translation.z,
     * quaternion.w,quaternion.x,quaternion.y,quaternion.z]
     */
    public float[] getCurrentPoseWithQuaternion() {
        return getCurrentPose();
    }

    /**
     * see @getCurrentPose with euler angles
     * @return the current pose with rotation in euler angles
     * [translation.x,translation.y,translation.z,
     *  yaw, pitch, roll]
     */
    public float[] getCurrentPoseWithEuler() {
        float[] data = getCurrentPose();
        if (data.length == 7) {
            // convert quaternion to euler
            float[] quaternionArray = Arrays.copyOfRange(data, 3, 7);
            float[] eulerArray = convertQuaternionToEulerAngles(quaternionArray);
            if (eulerArray != null) {
                return new float[]{data[0], data[1], data[2],
                        eulerArray[0], eulerArray[1], eulerArray[2]};
            }
        } else {
            throw new IllegalArgumentException("Cannot convert current pose to euler angles." + Arrays.toString(data));
        }
        return null;
    }

    /**
     * Register a sensor.
     * @param sensorType RANGE, IMU, ODOMETRY, FIXED_FRAME_POSE, LANDMARK, LOCAL_SLAM_RESULT
     *                   currently this bridge only supports range and odometry
     * @param sensorId a sensor id to add data later
     * @return true if sensor was added, false otherwise
     */
    public native boolean addSensor(String sensorType, String sensorId);

    /**
     * Convert euler angles to quaternion
     * @param eulerAngles yaw (Z), pitch (Y), roll (X)
     */
    private float[] convertEulerAngleToQuaternion(float[] eulerAngles) {
        if (eulerAngles.length != 3) {
            return null;
        }
        float yaw = eulerAngles[0];
        float pitch = eulerAngles[1];
        float roll = eulerAngles[2];

        double cy = cos(yaw * 0.5);
        double sy = sin(yaw * 0.5);
        double cp = cos(pitch * 0.5);
        double sp = sin(pitch * 0.5);
        double cr = cos(roll * 0.5);
        double sr = sin(roll * 0.5);

        float[] quaternion = new float[4];
        // w
        quaternion[0] = (float) (cy * cp * cr + sy * sp * sr);
        // x
        quaternion[1] = (float) (cy * cp * sr - sy * sp * cr);
        // y
        quaternion[2] = (float) (sy * cp * sr + cy * sp * cr);
        // z
        quaternion[3] = (float) (sy * cp * cr - cy * sp * sr);

        return quaternion;
    }

    /**
     * Convert euler angles to quaternion.
     * @param quaternion w,x,y,z
     * @return euler angle with yaw, pitch, roll
     */
    private float[] convertQuaternionToEulerAngles(float[] quaternion) {
        if (quaternion.length != 4) {
            return null;
        }
        float roll, pitch, yaw;
        float w = quaternion[0];
        float x = quaternion[1];
        float y = quaternion[2];
        float z = quaternion[3];

        // roll (x-axis rotation)
        double sinr_cosp = +2.0 * (w * x + y * z);
        double cosr_cosp = +1.0 - 2.0 * (x * x + y * y);
        roll = (float) atan2(sinr_cosp, cosr_cosp);

        // pitch (y-axis rotation)
        double sinp = +2.0 * (w * y - z * x);
        if (Math.abs(sinp) >= 1)
            pitch = (float) Math.copySign(Math.PI / 2, sinp); // use 90 degrees if out of range
        else
            pitch = (float) asin(sinp);

        // yaw (z-axis rotation)
        double siny_cosp = +2.0 * (w * z + x * y);
        double cosy_cosp = +1.0 - 2.0 * (y * y + z * z);
        yaw = (float) atan2(siny_cosp, cosy_cosp);

        return new float[]{yaw, pitch, roll};
    }
}
