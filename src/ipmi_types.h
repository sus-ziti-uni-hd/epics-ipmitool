#pragma once

#include <cstdint>
#include <epicsTypes.h>
#include <string>

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
  sensor_id_t(slave_addr_t _ipmb, int16_t _sensor, uint8_t _entity, uint8_t _inst, const std::string& _name);
  explicit sensor_id_t(const ::link& _loc);

  slave_addr_t ipmb;
  int16_t sensor;
  uint8_t entity;
  uint8_t instance;
  std::string name;

  bool operator <(const sensor_id_t& _other) const;
  bool operator ==(const sensor_id_t& _other) const;
};

} // namespace IPMIIOC
