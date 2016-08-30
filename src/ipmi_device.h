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
struct aiRecord;
struct link;
struct mbbiDirectRecord;
struct mbbiRecord;

namespace IPMIIOC {

class ReaderThread;
struct result_t;

class Device {
  public:
    Device(short _id);
    ~Device();

    bool connect(const std::string& _hostname, const std::string& _username,
                 const std::string& _password, int _privlevel);

    void initAiRecord(::aiRecord* _rec);
    bool readAiSensor(::aiRecord* _rec);

    void initMbbiDirectRecord(::mbbiDirectRecord* _rec);
    bool readMbbiDirectSensor(::mbbiDirectRecord* _rec);

    void initMbbiRecord(::mbbiRecord* _rec);
    bool readMbbiSensor(::mbbiRecord* _rec);

    void detectSensors();
    void dumpDatabase(const std::string& _file);

    bool ping();

  private:
    friend class ReaderThread;

    typedef bool (Device::*query_func_t)(const sensor_id_t&, result_t&);

    // description of a job queued for reading.
    // sensor id and function to call for it.
    // also the id of the PV that owns the job.
    struct query_job_t {
      sensor_id_t sensor;
      query_func_t query_func;
      unsigned pvid;

      query_job_t(const ::link& _loc, query_func_t _f, unsigned _pvid);
    };

    void handleFullSensor(slave_addr_t _addr, ::sdr_record_full_sensor* _rec);
    void handleCompactSensor(slave_addr_t _addr, ::sdr_record_compact_sensor* _rec);

    static void aiCallback(::CALLBACK* _cb);
    bool aiQuery(const sensor_id_t& _sensor, result_t& _result);

    static void mbbiDirectCallback(::CALLBACK* _cb);
    bool mbbiDirectQuery(const sensor_id_t& _sensor, result_t& _result);

    static void mbbiCallback(::CALLBACK* _cb);
    bool mbbiQuery(const sensor_id_t& _sensor, result_t& _result);

    const ::sensor_reading* ipmiQuery(const sensor_id_t& _sensor);

    bool check_PICMG();
    void find_ipmb();
    void iterateSDRs(slave_addr_t _addr, bool _force_internal = false);

    ::ipmi_intf* intf_ = nullptr;

    typedef struct {
      uint8_t type;
      union {
        ::sdr_record_common_sensor* common;
        ::sdr_record_full_sensor* full;
        ::sdr_record_compact_sensor* compact;
      };
      /// last reading was usable.
      bool good{true};
    } any_sensor_ptr;
    typedef std::map<sensor_id_t, any_sensor_ptr> sensor_list_t;
    sensor_list_t sensors_;

    any_sensor_ptr initInputRecord(::dbCommon* _rec, const ::link& _inp);

    short id_;
    ::epicsMutex mutex_;
    ReaderThread* readerThread_;
    uint8_t local_addr_;

    std::set<slave_addr_t> slaves_;
}; // class Device

} // namespace IPMIIOC

#endif

