package cartographerjar.jni;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;

public class Odo_Reader {
    public long time = 0;
    public float[] ododata;
    private BufferedReader reader;

    public Odo_Reader(String path) throws FileNotFoundException {
        reader = new BufferedReader(new FileReader(path));
    }

    public int get_new_data() {
        try {
            String line = reader.readLine();
            if (line == null) {
                reader.close();
                return 1;
            }
            if (!line.contains("time")) {
                reader.close();
                return -2;
            }
            line = reader.readLine();
            time = Long.parseLong(line);
            line = reader.readLine();
            if (!line.contains("pose")) {
                reader.close();
                return -2;
            }
            line = reader.readLine();
            String[] arrayvals = line.split(",");
            ododata = new float[arrayvals.length];
            for (int i = 0; i < arrayvals.length; i++) {
                ododata[i] = Float.parseFloat(arrayvals[i]);
            }
        } catch (IOException e) {
            System.err.println("Reader Odofile failed readline");
            return -1;
        }
        return 0;
    }

}
