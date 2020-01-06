LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := liblogs

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
    log.c \
    logm.c

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../include

LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)
include $(BUILD_SHARED_LIBRARY)
