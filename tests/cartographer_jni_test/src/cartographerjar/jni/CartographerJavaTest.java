package cartographerjar.jni;

import cartographer.jni.CartographerJniJavaBridge;

import java.io.BufferedWriter;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;

import static java.lang.Math.asin;
import static java.lang.Math.atan2;

public class CartographerJavaTest {

    public static void main(String[] args) {
        System.out.println("Working Directory: " + System.getProperty("user.dir"));
        final String filePath = System.getProperty("user.dir") + "/../test_data/";
        LaserScanReader laserScanReader = null;
        Odo_Reader odoReader = null;
        try {
            laserScanReader = new LaserScanReader(filePath + "lrfdata.txt");
            odoReader = new Odo_Reader(filePath + "odomdata.txt");
        } catch (FileNotFoundException e) {
            System.err.println("Could not open file for reading!");
            e.printStackTrace();
            System.exit(1);
        }
        CartographerJniJavaBridge util = new CartographerJniJavaBridge();
        util.startCartographer("config", "cartographer_jni.lua");

        boolean laserDataAvailable = laserScanReader.get_new_data() == 0;
        boolean odometryDataAvailable = odoReader.get_new_data() == 0;
        while (laserDataAvailable || odometryDataAvailable) {
            if (laserDataAvailable && (laserScanReader.time <= odoReader.time || !odometryDataAvailable)) {
                util.sendLaserScanData("scan", laserScanReader.time * 1000, laserScanReader.laserdata);
                laserDataAvailable = laserScanReader.get_new_data() == 0;
            }
            if (odometryDataAvailable && (odoReader.time < laserScanReader.time || !laserDataAvailable)) {
                if (odoReader.ododata.length == 4) {
                    util.sendOdometryDataWithQuaternion("odom", odoReader.time * 1000, odoReader.ododata);
                } else {
                    util.sendOdometryDataWithEuler("odom", odoReader.time * 1000, odoReader.ododata);
                }
                odometryDataAvailable = odoReader.get_new_data() == 0;
            }
        }

        util.finalizeCartographer();
        int[] gridmap = util.getGridMap();
        int width = gridmap[gridmap.length - 2];
        int height = gridmap[gridmap.length - 1];
        String resultGridPath = System.getProperty("user.dir") + "/build/grid_java.txt";
        try {
            BufferedWriter gridmapWriter = new BufferedWriter(new FileWriter(resultGridPath));
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int value = gridmap[y * width + x];
                    if (value < 0)
                        value = -20;
                    gridmapWriter.write(x + " " + y + " " + value + "\n");
                }
            }
            gridmapWriter.close();
            System.out.println("The result grid is stored at " + resultGridPath);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static float[] quaternionToEuler(float[] quaternion) {
        float roll, pitch, yaw;
        float w = quaternion[3];
        float x = quaternion[4];
        float y = quaternion[5];
        float z = quaternion[6];

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

        return new float[]{quaternion[0], quaternion[1], quaternion[2], yaw, pitch, roll};
    }
}
