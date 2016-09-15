#include "ipmi_types.h"
#include "string.h"

#include <dbCommon.h>
#include <iomanip>
#include <sstream>

namespace IPMIIOC {

result_t::result_t() : valid(false) {
}

sensor_id_t::sensor_id_t(slave_addr_t _ipmb, uint8_t _sensor, uint8_t _entity, uint8_t _inst, const std::string& _name)
  : ipmb(_ipmb), sensor(_sensor), sensor_set(true), entity(_entity), instance(_inst), name(_name) {
} // Device::sensor_id_t constructor


sensor_id_t::sensor_id_t(const ::link& _loc)
  : ipmb(_loc.value.abio.adapter), sensor(0), sensor_set(false), entity(_loc.value.abio.card), instance(_loc.value.abio.signal), name(_loc.value.abio.parm) {
} // Device::sensor_id_t constructor


bool sensor_id_t::operator <(const sensor_id_t& _other) const {
  if (ipmb < _other.ipmb) return true;
  if (ipmb > _other.ipmb) return false;
  // sensor is NOT used 
  if (entity < _other.entity) return true;
  if (entity > _other.entity) return false;
  if (instance < _other.instance) return true;
  if (instance > _other.instance) return false;
  return  name < _other.name;
} // Device::sensor_id_t::operator <


bool sensor_id_t::operator ==(const sensor_id_t& _other) const {
  // sensor is NOT used 

  return (ipmb == _other.ipmb)
    && (entity == _other.entity)
    && (instance == _other.instance)
    && (name == _other.name);
} // Device::sensor_id_t::operator ==


std::string sensor_id_t::prettyPrint() const {
   std::ostringstream ss;
   ss << "ipmb 0x" << std::hex << std::setw(2) << std::setfill('0') << +ipmb
      << " ent 0x" << std::hex << std::setw(2) << std::setfill('0') << +entity
      << " inst 0x" << std::hex << std::setw(2) << std::setfill('0') << +instance
      << " " << name;
   if (sensor_set) {
      ss << " (num 0x" << std::hex << std::setw(2) << std::setfill('0') << +sensor << ")";
   }
   return ss.str();
}

}
