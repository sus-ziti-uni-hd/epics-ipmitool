#include "ipmi_connection.h"
#include "ipmi_device.h"
#include "ipmi_internal.h"

extern "C" {
#include <ipmitool/ipmi_intf.h>
#include <ipmitool/ipmi_sdr.h>
#include <ipmitool/log.h>
}

#include <dbCommon.h>
#include <errlog.h>

#include <logger.h>
#include <sstream>
#include <subsystem_registrator.h>


namespace {
SuS::logfile::subsystem_registrator log_id("IPMIConn");
} // namespace


namespace {

IPMIIOC::Device* findDevice(const link& _inp) {
  const auto& it = IPMIIOC::s_interfaces.find(_inp.value.abio.link);
  if (it == IPMIIOC::s_interfaces.end()) return NULL;
  else return it->second;
} // findDevice

} // namespace

extern "C" {

  void ipmiConnect(int _id, const char* _hostname, const char* _username,
                   const char* _password, const char* _proto, int _privlevel) {
    ::log_init("ipmitool-ioc", 0, ::verbose);

    try {
      IPMIIOC::Device* d = new IPMIIOC::Device(_id);
      d->connect(_hostname, _username, _password, _proto, _privlevel);
      IPMIIOC::s_interfaces[_id] = d;
    } catch (std::bad_alloc& ba) {
      ::errlogSevPrintf(::errlogMajor, "Memory allocation error: %s\n", ba.what());
      return;
    } // catch
  } // ipmiConnect


  void ipmiInitAiRecord(aiRecord* _rec) {
    IPMIIOC::Device* d = findDevice(_rec->inp);
    if (d) d->initAiRecord(_rec);
  } // ipmiInitAiRecord


  void ipmiReadAiSensor(aiRecord* _rec) {
    IPMIIOC::Device* d = findDevice(_rec->inp);
    if (d) d->readAiSensor(_rec);
  } // ipmiReadAiSensor


  void ipmiInitMbbiDirectRecord(mbbiDirectRecord* _rec) {
    IPMIIOC::Device* d = findDevice(_rec->inp);
    if (d) d->initMbbiDirectRecord(_rec);
  } // ipmiInitMbbiDirectRecord


  void ipmiReadMbbiDirectSensor(mbbiDirectRecord* _rec) {
    IPMIIOC::Device* d = findDevice(_rec->inp);
    if (d) d->readMbbiDirectSensor(_rec);
  } // ipmiReadMbbiDirectSensor


  void ipmiInitMbbiRecord(mbbiRecord* _rec) {
    IPMIIOC::Device* d = findDevice(_rec->inp);
    if (d) d->initMbbiRecord(_rec);
  } // ipmiInitMbbiRecord


  void ipmiReadMbbiSensor(mbbiRecord* _rec) {
    IPMIIOC::Device* d = findDevice(_rec->inp);
    if (d) d->readMbbiSensor(_rec);
  } // ipmiReadMbbiSensor


  void ipmiScanDevice(int _id) {
    const auto& it = IPMIIOC::s_interfaces.find(_id);
    if (it == IPMIIOC::s_interfaces.end()) {
      SuS_LOG_STREAM(warning, log_id(), "Device " << _id << " not found.");
      return;
    }
    it->second->detectSensors();
  } // ipmiScanDevice


  void ipmiDumpDatabase(int _id, const char* _file) {
    if (!_file) {
      SuS_LOG(warning, log_id(), "No filename provided.");
      return;
    }
    const auto& it = IPMIIOC::s_interfaces.find(_id);
    if (it == IPMIIOC::s_interfaces.end()) {
      SuS_LOG(warning, log_id(), "Device not known.");
      return;
    }
    it->second->dumpDatabase(_file);
  }
} // extern "C"
