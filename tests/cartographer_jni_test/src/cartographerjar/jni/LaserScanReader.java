package cartographerjar.jni;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;

public class LaserScanReader {
    public long time;
    public float[] laserdata;
    private BufferedReader reader;

    public LaserScanReader(String path) throws FileNotFoundException {
        reader = new BufferedReader(new FileReader(path));
    }

    public int get_new_data(){
        try{
            FloatArrayList cloud = new FloatArrayList();
            String line = reader.readLine();
            if (line == null) {
                reader.close();
                return 1;
            }
            if(!line.contains("time")){reader.close(); return -2;}
            line = reader.readLine();
            time = Long.parseLong(line);
            line = reader.readLine();
            if(!line.contains("cloud")){reader.close(); return -2;}
            line = reader.readLine();
            while(!line.contains("pose")){
                String[] cloudvals = line.split(",");
                for(int i = 0; i<cloudvals.length;i++){
                    cloud.add(Float.parseFloat(cloudvals[i]));
                }
                line = reader.readLine();
            }
            line = reader.readLine();
            String[] posevals = line.split(",");
            FloatArrayList pose = new FloatArrayList();
            for(int i = 0; i < 3;i++){
                pose.add(Float.parseFloat(posevals[i]));
            }
            pose.addAll(cloud.toArray());
            laserdata = pose.toArray();
        }
        catch (IOException e){
            System.err.println("Reader Odofile failed readline");
            return -1;
        }
        return 0;
    }
}
