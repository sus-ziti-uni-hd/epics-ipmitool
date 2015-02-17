# Makefile
TOP = ..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

LIBRARY_IOC += ipmitool

DBD += ipmitool.dbd
ipmitool_DBD += Logfile.dbd
ipmitool_DBD += BAERER.dbd

ipmitool_SRCS += epics_device.c
ipmitool_SRCS += epics_functions.c
ipmitool_SRCS += ipmi_connection.cc
ipmitool_SRCS += ipmi_device.cc
ipmitool_SRCS += ipmi_device_private.cc
ipmitool_SRCS += ipmi_internal.cc
ipmitool_SRCS += ipmi_reader_thread.cc
ipmitool_SRCS += ipmi_types.cc

ipmitool_LIBS += $(EPICS_BASE_IOC_LIBS)
ipmitool_LIBS += Logfile EPICSLog

LDFLAGS += /home/ritzert/NO_BACKUP/CVS_from_web/ipmitool/lib/.libs/libipmitool.a
LDFLAGS += /home/ritzert/NO_BACKUP/CVS_from_web/ipmitool/src/plugins/.libs/libintf.a
LDFLAGS += -lssl

CXXFLAGS += -I /home/ritzert/NO_BACKUP/CVS_from_web/ipmitool/include
CXXFLAGS += -I /home/ritzert/NO_BACKUP/CVS_from_web/ipmitool/src/plugins/lan
CXXFLAGS += -std=c++11

include $(TOP)/configure/RULES

#----------------------------------------
#  ADD RULES AFTER THIS LINE


