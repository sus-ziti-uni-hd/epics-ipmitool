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
      printf("= findDevice %d ", _inp.value.abio.link);
  IPMIIOC::intf_map_t::const_iterator it = IPMIIOC::s_interfaces.find(
        _inp.value.abio.link);
  if (it == IPMIIOC::s_interfaces.end()){
    printf("not found\n");
    return NULL;
  }  
  else{
    printf(" found\n");
    return it->second;
  }
} // findDevice

} // namespace

extern "C" {

  void ipmiConnect(int _id, const char* _hostname, const char* _username,
                   const char* _password, int _privlevel) {
    ::log_init("ipmicmd-ioc", 0, 3/*::verbose*/);

      printf("= ipmi connect id %d\n", _id);
    try {
      IPMIIOC::Device* d = new IPMIIOC::Device(_id);
      d->connect(_hostname, _username, _password, _privlevel);
      IPMIIOC::s_interfaces[_id] = d;
    } catch (std::bad_alloc& ba) {
      ::errlogSevPrintf(::errlogMajor, "Memory allocation error: %s\n", ba.what());
      return;
    } // catch
  } // ipmiConnect


  void ipmiInitBoRecord(boRecord* _rec) {
//     printf("ipmiInitBoRecord\n");
    IPMIIOC::Device* d = findDevice(_rec->out);
//     printf("ipmiInitBoRecord %p\n",d);
    if (d) d->initBoRecord(_rec);
  } // ipmiInitAiRecord


  void ipmiWriteBoSensor(boRecord* _rec) {
//     printf("ipmiWriteBoSensor\n");
    IPMIIOC::Device* d = findDevice(_rec->out);
//     printf("ipmiWriteBoSensor %p\n",d);
    if (d) d->writeBoSensor(_rec);
  } // ipmiReadAiSensor


  void ipmiInitMbboDirectRecord(mbboDirectRecord* _rec) {
    IPMIIOC::Device* d = findDevice(_rec->out);
    if (d) d->initMbboDirectRecord(_rec);
  } // ipmiInitMbbiDirectRecord


  void ipmiWriteMbboDirectSensor(mbboDirectRecord* _rec) {
    IPMIIOC::Device* d = findDevice(_rec->out);
    if (d) d->writeMbboDirectSensor(_rec);
  } // ipmiReadMbbiDirectSensor

  
  void ipmiInitMbbiDirectRecord(mbbiDirectRecord* _rec) {
    IPMIIOC::Device* d = findDevice(_rec->inp);
    if (d) d->initMbbiDirectRecord(_rec);
  } // ipmiInitMbbiDirectRecord


  void ipmiReadMbbiDirectSensor(mbbiDirectRecord* _rec) {
    IPMIIOC::Device* d = findDevice(_rec->inp);
    if (d) d->readMbbiDirectSensor(_rec);
  } // ipmiReadMbbiDirectSensor


} // extern "C"
