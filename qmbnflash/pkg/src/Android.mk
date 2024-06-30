# Copyright 2006 The Android Open Source Project

# XXX using libutils for simulator build only...
#

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE:= qflash
LOCAL_SRC_FILES:= atchannel.c at_tok.c $(LOCAL_MODULE).c
LOCAL_CFLAGS += -Wno-unused-parameter -Wno-sign-compare -pie -fPIE
LOCAL_LDFLAGS += -pie -fPIE
LOCAL_MODULE_TAGS:=optional
include $(BUILD_EXECUTABLE)
