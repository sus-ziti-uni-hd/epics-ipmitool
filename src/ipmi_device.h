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

struct aiRecord;

namespace IPMIIOC {

class ReaderThread;

class Device {
  public:
    ~Device();

    bool connect(const std::string& _hostname, const std::string& _username,
                 const std::string& _password, int _privlevel);

    void initAiRecord(::aiRecord* _rec);
    bool readAiSensor(::aiRecord* _rec);

    void detectSensors();

    bool ping();

  private:
    friend class ReaderThread;

    typedef uint8_t slave_addr_t;

    struct result_t {
      epicsInt32 rval;
      union {
        epicsInt32 ival;
        epicsFloat64 fval;
      } value;
      bool valid;
    }; // struct result_t

    struct sensor_id_t {
      slave_addr_t ipmb;
      uint8_t sensor;

      bool operator <(const sensor_id_t& _other) const;
    };

    void handleFullSensor(slave_addr_t _addr, ::sdr_record_full_sensor* _rec);
    void handleCompactSensor(slave_addr_t _addr, ::sdr_record_compact_sensor* _rec);

    static void aiCallback(::CALLBACK* _cb);
    bool aiQuery(const sensor_id_t& _sensor, result_t& _result);

    static sensor_id_t make_sensor_id(const ::aiRecord* _rec);

    bool check_PICMG();
    void find_ipmb();
    void iterateSDRs(slave_addr_t _addr, bool _force_internal=false);

    ::ipmi_intf* intf_;

    typedef struct {
      uint8_t type;
      union {
         ::sdr_record_common_sensor* common;
         ::sdr_record_full_sensor* full;
         ::sdr_record_compact_sensor* compact;
      };
    } any_sensor_ptr;
    typedef std::map<sensor_id_t, any_sensor_ptr> sensor_list_t;
    sensor_list_t sensors_;

    ::epicsMutex mutex_;
    ReaderThread* readerThread_;
    uint8_t local_addr_;

    std::set<slave_addr_t> slaves_;
}; // class Device

} // namespace IPMIIOC

#endif

