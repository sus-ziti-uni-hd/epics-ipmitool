#include "ipmi_types.h"
#include "string.h"

#include <dbCommon.h>

namespace IPMIIOC {

result_t::result_t() : valid(false) {
}

sensor_id_t::sensor_id_t(slave_addr_t _ipmb, int16_t _sensor,uint8_t _entity,uint8_t _inst, uint8_t *_name)
  : ipmb(_ipmb), sensor(_sensor), entity(_entity), insta(_inst) {
  strncpy(name,(char *)_name,sizeof(name));
} // Device::sensor_id_t constructor


sensor_id_t::sensor_id_t(const ::link& _loc)
  : ipmb(_loc.value.abio.adapter), sensor(-1), entity(_loc.value.abio.card), insta(_loc.value.abio.signal) {
  strncpy(name,_loc.value.abio.parm,sizeof(name));
} // Device::sensor_id_t constructor


bool sensor_id_t::operator <(const sensor_id_t& _other) const {
  if (ipmb < _other.ipmb) return true;
  if ( ipmb > _other.ipmb) return false;
  // sensor is NOT used 
  if ( entity < _other.entity) return true;
  if ( entity > _other.entity) return false;
  if ( insta < _other.insta) return true;
  if ( insta > _other.insta) return false;
  if ( strncmp(name,_other.name,sizeof(name))>=0) return false;

  return true;
} // Device::sensor_id_t::operator <


bool sensor_id_t::operator ==(const sensor_id_t& _other) const {
  // sensor is NOT used 

  return ( ipmb == _other.ipmb
	&& entity == _other.entity
	&& insta == _other.insta
	&& strncmp(name,_other.name,sizeof(name))==0);
} // Device::sensor_id_t::operator ==

}
