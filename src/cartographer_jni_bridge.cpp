#include "cartographer_jni_bridge.h"

JniCppBridge jniCppBridge = JniCppBridge();

/*
 * Class:     cartographer_jni_CartographerJniJavaBridge
 * Method:    startCartographer
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_cartographer_jni_CartographerJniJavaBridge_startCartographer
  (JNIEnv *env, jobject jobj, jstring jFilePath, jstring jFileName) {
    std::string filePath = env->GetStringUTFChars(jFilePath, NULL);
    std::string fileName = env->GetStringUTFChars(jFileName, NULL);

	jniCppBridge = JniCppBridge(filePath, fileName);
}

/*
 * Class:     cartographer_jni_CartographerJniJavaBridge
 * Method:    sendOdometryData
 * Signature: (Ljava/lang/String;J[F)V
 */
JNIEXPORT void JNICALL Java_cartographer_jni_CartographerJniJavaBridge_sendOdometryData
  (JNIEnv *env, jobject, jstring j_sensor_id, jlong j_time, jfloatArray j_data) {
	// convert sensor id
	std::string sensor_id = env->GetStringUTFChars(j_sensor_id, NULL);
	// convert time
	long time = (jlong) j_time;

	// Convert the incoming JNI jintarray to C's jint[]
	jfloat *data_array = (*env).GetFloatArrayElements(j_data, NULL);
	if (NULL == data_array) return;
	jsize length = env->GetArrayLength(j_data);

	std::vector<float> data_vector;

	for (int i = 0; i < length; i++) {
		data_vector.push_back(data_array[i]);
	}

	jniCppBridge.HandleOdometry(sensor_id, time, data_vector);
}

/*
 * Class:     cartographer_jni_CartographerJniJavaBridge
 * Method:    sendLaserScanData
 * Signature: (Ljava/lang/String;J[F)V
 */
JNIEXPORT void JNICALL Java_cartographer_jni_CartographerJniJavaBridge_sendLaserScanData
		(JNIEnv *env, jobject, jstring j_sensor_id, jlong j_time, jfloatArray j_data) {
	// convert sensor id
	std::string sensor_id = env->GetStringUTFChars(j_sensor_id, NULL);
	// convert time
	long time = (jlong) j_time;

	// Convert the incoming JNI jintarray to C's jint[]
	jfloat *data_array = (*env).GetFloatArrayElements(j_data, NULL);
	if (NULL == data_array) return;
	jsize length = env->GetArrayLength(j_data);

	std::vector<float> data_vector;

	for (int i = 0; i < length; i++) {
		data_vector.push_back(data_array[i]);
	}

	jniCppBridge.HandleLaserScan(sensor_id, time, data_vector);
}

/*
 * Class:     cartographer_jni_CartographerJniJavaBridge
 * Method:    finalizeCartographer
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_cartographer_jni_CartographerJniJavaBridge_finalizeCartographer
  (JNIEnv *, jobject) {
	jniCppBridge.Finalize();
}

/*
 * Class:     cartographer_jni_CartographerJniJavaBridge
 * Method:    getGridMap
 * Signature: ()[I
 */
JNIEXPORT jintArray JNICALL Java_cartographer_jni_CartographerJniJavaBridge_getGridMap
  (JNIEnv *env, jobject) {
	std::vector<int> gridMap = jniCppBridge.GetGridMap();
	const jsize length = gridMap.size();
	jintArray int_array = env->NewIntArray(length);
	env->SetIntArrayRegion( int_array, 0, gridMap.size(), ( jint * ) &gridMap[0] );
	return int_array;
}

/*
 * Class:     cartographer_jni_CartographerJniJavaBridge
 * Method:    getCurrentPose
 * Signature: ()[F
 */
JNIEXPORT jfloatArray JNICALL Java_cartographer_jni_CartographerJniJavaBridge_getCurrentPose
  (JNIEnv *env, jobject) {
	std::vector<float> currentPose = jniCppBridge.GetCurrentPose();
	const jsize length = currentPose.size();
	jfloatArray float_array = env->NewFloatArray(length);
	env->SetFloatArrayRegion( float_array, 0, currentPose.size(), ( jfloat * ) &currentPose[0] );
	return float_array;
}

/*
 * Class:     cartographer_jni_CartographerJniJavaBridge
 * Method:    addSensor
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_cartographer_jni_CartographerJniJavaBridge_addSensor
  (JNIEnv *env, jobject, jstring j_sensor_type, jstring j_sensor_id) {
    std::string sensor_id = env->GetStringUTFChars(j_sensor_id, NULL);
    std::string sensor_type = env->GetStringUTFChars(j_sensor_type, NULL);
    jboolean result = jniCppBridge.AddExpectedSensorId(sensor_type, sensor_id);
    return result;
}
