LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := main
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../SDL2/include $(LOCAL_PATH)/../../../..
SRCS := $(wildcard $(LOCAL_PATH)/../../../../*.cpp)
LOCAL_SRC_FILES := $(SRCS:$(LOCAL_PATH)/%=%)
LOCAL_SHARED_LIBRARIES := SDL2
LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid
include $(BUILD_SHARED_LIBRARY)
