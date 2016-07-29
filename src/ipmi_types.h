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

struct sensorcmd_id_t {
  sensorcmd_id_t(slave_addr_t _ipmb, uint8_t _ipmb_tunnel, uint8_t _param, const std::string& _command);
  explicit sensorcmd_id_t(const ::link& _loc);

  slave_addr_t ipmb;
  slave_addr_t ipmb_tunnel;
  uint8_t param;
  std::string command;

  bool operator <(const sensorcmd_id_t& _other) const;
  bool operator ==(const sensorcmd_id_t& _other) const;

  std::string prettyPrint() const;
};

struct query_result_t{
  bool valid;
  int len;
  unsigned int retval;
  unsigned char data[32];

  query_result_t(){
    len=0;
    retval=0xFF;
    valid=false;
  }
};

} // namespace IPMIIOC
