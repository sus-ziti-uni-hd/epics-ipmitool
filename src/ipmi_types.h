#pragma once

#include <cstdint>
#include <epicsTypes.h>
#include <memory>
#include <string>

extern "C" {
#include <ipmitool/ipmi_sdr.h>
}
struct link;

namespace IPMIIOC {

/** Result of a reading by the reader thread. */
struct result_t {
  epicsInt32 rval;
  union {
    epicsInt32 ival;
    epicsFloat64 fval;
  } value;
  bool valid;

  result_t();
}; // struct result_t

typedef uint8_t slave_addr_t;

struct sensor_id_t {
  sensor_id_t(slave_addr_t _ipmb, uint8_t _sensor, uint8_t _entity, uint8_t _inst, const std::string& _name);
  explicit sensor_id_t(const ::link& _loc);

  slave_addr_t ipmb;
  uint8_t sensor;
  bool sensor_set;
  uint8_t entity;
  uint8_t instance;
  std::string name;

  bool operator <(const sensor_id_t& _other) const;
  bool operator ==(const sensor_id_t& _other) const;

  std::string prettyPrint() const;
};

struct any_sensor_ptr {
   uint8_t type;
   /// last reading was usable.
   bool good{true};

   explicit any_sensor_ptr();
   explicit any_sensor_ptr(::sdr_record_full_sensor* _p);
   explicit any_sensor_ptr(::sdr_record_compact_sensor* _p);

   operator ::sdr_record_full_sensor*() const;
   operator ::sdr_record_common_sensor*() const;
   operator ::sdr_record_compact_sensor*() const;

   ::sdr_record_common_sensor* operator()() const;

private:
   std::shared_ptr<::sdr_record_common_sensor> data_ptr;
};

} // namespace IPMIIOC
