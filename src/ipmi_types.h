#pragma once

#include <cstdint>
#include <epicsTypes.h>

struct link;

namespace IPMIIOC {

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
  sensor_id_t(slave_addr_t _ipmb, uint8_t _sensor);
  explicit sensor_id_t(const ::link& _loc);

  slave_addr_t ipmb;
  uint8_t sensor;

  bool operator <(const sensor_id_t& _other) const;
};

} // namespace IPMIIOC
