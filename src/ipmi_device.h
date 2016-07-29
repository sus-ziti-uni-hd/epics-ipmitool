#ifndef IPMI_DEVICE_H_
#define IPMI_DEVICE_H_

extern "C" {
#include <ipmitool/ipmi_intf.h>
#include <ipmitool/ipmi_picmg.h>
#include <ipmitool/ipmi_sdr.h>
} // extern "C"

#include <callback.h>
#include <epicsMutex.h>
#include <epicsTypes.h>

#include <map>
#include <string>
#include <set>

#include "ipmi_types.h"

struct dbCommon;
struct boRecord;
struct link;
struct mbboDirectRecord;
struct mbbiDirectRecord;

namespace IPMIIOC {

class ReaderThread;
struct result_t;

class Device {
  public:
    Device(short _id);
    ~Device();

    bool connect(const std::string& _hostname, const std::string& _username,
                 const std::string& _password, int _privlevel);

    void initBoRecord(::boRecord* _rec);
    bool writeBoSensor(::boRecord* _rec);

    void initMbboDirectRecord(::mbboDirectRecord* _rec);
    bool writeMbboDirectSensor(::mbboDirectRecord* _rec);

    void initMbbiDirectRecord(::mbbiDirectRecord* _rec);
    bool readMbbiDirectSensor(::mbbiDirectRecord* _rec);

    bool ping();

  private:
    friend class ReaderThread;

    struct query_job_t;
    typedef bool (Device::*query_func_t)(const query_job_t&, result_t&);

    // description of a job queued for reading.
    // sensor id and function to call for it.
    struct query_job_t {
      sensorcmd_id_t sensor;
      query_func_t query_func;
      query_result_t result;

      query_job_t(const ::link& _loc, query_func_t _f);
      query_job_t(const ::link& _loc, query_func_t _f, query_result_t result);
    };

    static void boCallback(::CALLBACK* _cb);
    bool boQuery(const query_job_t&, result_t& _result);

    static void mbboDirectCallback(::CALLBACK* _cb);
    bool mbboDirectQuery(const query_job_t&, result_t& _result);

    static void mbbiDirectCallback(::CALLBACK* _cb);
    bool mbbiDirectQuery(const query_job_t&, result_t& _result);

//     const ::sensor_reading* 
    query_result_t ipmiQuery(const query_job_t&);

    ::ipmi_intf* intf_ = nullptr;

    typedef struct {
      bool good{true};
    } any_sensor_ptr;
    typedef std::map<sensorcmd_id_t, any_sensor_ptr> sensor_list_t;
    sensor_list_t sensors_;

    any_sensor_ptr initOutputRecord(::dbCommon* _rec, const ::link& _link);

    short id_;
    ::epicsMutex mutex_;
    ReaderThread* readerThread_;
    uint8_t local_addr_;

    std::set<slave_addr_t> slaves_;
}; // class Device

} // namespace IPMIIOC

#endif

