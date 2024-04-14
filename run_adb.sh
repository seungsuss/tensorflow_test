adb push /Users/odin/Tensorflow_android/tftest/obj/local/arm64-v8a/tflite_code_exe /data/local/tmp/tflite_test
adb push /Users/odin/Tensorflow_android/tftest/obj/local/arm64-v8a/libtensorflowlite.so /data/local/tmp/tflite_test
adb push /Users/odin/Tensorflow_android/tftest/obj/local/arm64-v8a/libc++_shared.so /data/local/tmp/tflite_test
adb push /Users/odin/Tensorflow_android/tftest/swin_model.tflite /data/local/tmp/tflite_test

adb shell  "export LD_LIBRARY_PATH= /data/local/tmp/tflite_test
                  && tflite_code_exe"
