#include "ipmi_types.h"

#include <dbCommon.h>

namespace IPMIIOC {

result_t::result_t() : valid(false) {
}

sensor_id_t::sensor_id_t(slave_addr_t _ipmb, uint8_t _sensor)
  : ipmb(_ipmb), sensor(_sensor)
{
} // Device::sensor_id_t constructor


sensor_id_t::sensor_id_t(const ::link& _loc)
  : ipmb(_loc.value.abio.adapter), sensor(_loc.value.abio.card)
{
} // Device::sensor_id_t constructor


bool sensor_id_t::operator <(const sensor_id_t& _other) const {
  if (ipmb < _other.ipmb) return true;
  else if (ipmb == _other.ipmb) return sensor < _other.sensor;
  else return false;
} // Device::sensor_id_t::operator <

}
