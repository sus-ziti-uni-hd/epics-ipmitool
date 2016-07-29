TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = ipmi1
# ipmi1.dbd will be created and installed
DBD += ipmi1.dbd

# ipmi1.dbd will be made up from these files:
ipmi1_DBD += base.dbd
ipmi1_DBD += ipmicmd.dbd
ipmi1_DBD += Logfile.dbd

# Include dbd files from all support applications:
#ipmi1_DBD += xxx.dbd

# Add all the support libraries needed by this IOC
#ipmi1_LIBS += xxx

# ipmi1_registerRecordDeviceDriver.cpp derives from ipmi1.dbd
ipmi1_SRCS += ipmi1_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
ipmi1_SRCS_DEFAULT += ipmi1Main.cpp
ipmi1_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#ipmi1_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
ipmi1_LIBS += $(EPICS_BASE_IOC_LIBS)
ipmi1_LIBS += ipmitool
ipmi1_LIBS += Logfile EPICSLog

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

