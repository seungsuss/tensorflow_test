LOCAL_PATH := $(call my-dir)

TFLITE_MAIN_PATH = //home/Tensorflow_android/tftest

include $(CLEAR_VARS)   
    LOCAL_MODULE := tensorflowlite
    LOCAL_C_INCLUDES += $(TFLITE_MAIN_PATH)/include/abseil
    LOCAL_C_INCLUDES += $(TFLITE_MAIN_PATH)/include/flatbuffers/include
    #LOCAL_C_INCLUDES += /home/tensorflow/flatbuffers/include
    LOCAL_C_INCLUDES += $(TFLITE_MAIN_PATH)/include
    #LOCAL_C_INCLUDES += /home/tensorflow 
    #LOCAL_SRC_FILES := $(TFLITE_MAIN_PATH)/lib/arm64-v8a/test_libtensor.so
    LOCAL_SRC_FILES := $(TFLITE_MAIN_PATH)/lib/arm64-v8a/libtensorflowlite.so
include $(PREBUILT_SHARED_LIBRARY)


include $(CLEAR_VARS)   
    LOCAL_CFLAGS += --std=c++14 -fPIC   
    #LOCAL_C_INCLUDES += /home/tensorflow/flatbuffers/include
    LOCAL_C_INCLUDES += $(TFLITE_MAIN_PATH)/include/flatbuffers/include
    LOCAL_C_INCLUDES += $(TFLITE_MAIN_PATH)/include
    #LOCAL_C_INCLUDES += /home/tensorflow 
    LOCAL_MODULE := tflite_code_exe
    LOCAL_SRC_FILES := main.cpp
    #LOCAL_LDLIBS = $(TFLITE_MAIN_PATH)/lib/arm64-v8a/libtensorflowlite.so
    LOCAL_SHARED_LIBRARIES += tensorflowlite    
include $(BUILD_EXECUTABLE)

