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


namespace {

IPMIIOC::Device* findDevice(const link& _inp) {
  IPMIIOC::intf_map_t::const_iterator it = IPMIIOC::s_interfaces.find(
        _inp.value.abio.link);
  if (it == IPMIIOC::s_interfaces.end()) return NULL;
  else return it->second;
} // findDevice

} // namespace

extern "C" {

  void ipmiConnect(int _id, const char* _hostname, const char* _username,
                   const char* _password, int _privlevel) {
    ::log_init("ipmitool-ioc", 0, ::verbose);

    try {
      IPMIIOC::Device* d = new IPMIIOC::Device();
      d->connect(_hostname, _username, _password, _privlevel);
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


  void ipmiInitMbbiRecord(mbbiRecord* _rec) {
    IPMIIOC::Device* d = findDevice(_rec->inp);
    if (d) d->initMbbiRecord(_rec);
  } // ipmiInitMbbiRecord


  void ipmiReadMbbiSensor(mbbiRecord* _rec) {
    IPMIIOC::Device* d = findDevice(_rec->inp);
    if (d) d->readMbbiSensor(_rec);
  } // ipmiReadMbbiSensor


  void ipmiScanDevice(int _id) {
    IPMIIOC::intf_map_t::const_iterator it = IPMIIOC::s_interfaces.find(_id);
    if (it == IPMIIOC::s_interfaces.end()) return;
    it->second->detectSensors();
  } // ipmiScanDevice

} // extern "C"
