#include "ipmi_types.h"
#include "string.h"

#include <dbCommon.h>
#include <iomanip>
#include <sstream>

namespace IPMIIOC {

result_t::result_t() : valid(false) {
}

sensorcmd_id_t::sensorcmd_id_t(slave_addr_t _ipmb, uint8_t _ipmb_tunnel, uint8_t _param, const std::string& _command)
  : ipmb(_ipmb), ipmb_tunnel(_ipmb_tunnel), param(_param), command(_command) {
} // Device::sensorcmd_id_t constructor


sensorcmd_id_t::sensorcmd_id_t(const ::link& _loc)
  : ipmb(_loc.value.abio.adapter), ipmb_tunnel(_loc.value.abio.card), param(_loc.value.abio.signal), command(_loc.value.abio.parm) {
} // Device::sensorcmd_id_t constructor


bool sensorcmd_id_t::operator <(const sensorcmd_id_t& _other) const {
  if (ipmb < _other.ipmb) return true;
  if ( ipmb > _other.ipmb) return false;
  // sensor is NOT used 
  if ( ipmb_tunnel < _other.ipmb_tunnel) return true;
  if ( ipmb_tunnel > _other.ipmb_tunnel) return false;
  if ( param < _other.param) return true;
  if ( param > _other.param) return false;
  return  command < _other.command;
} // Device::sensorcmd_id_t::operator <


bool sensorcmd_id_t::operator ==(const sensorcmd_id_t& _other) const {
  // sensor is NOT used 

  return (ipmb == _other.ipmb)
	&& (ipmb_tunnel == _other.ipmb_tunnel)
	&& (param == _other.param)
	&& (command == _other.command);
} // Device::sensorcmd_id_t::operator ==


std::string sensorcmd_id_t::prettyPrint() const {
   std::ostringstream ss;
   ss << "ipmb 0x" << std::hex << std::setw(2) << std::setfill('0') << +ipmb
      << " ipmb_tunnel 0x" << std::hex << std::setw(2) << std::setfill('0') << +ipmb_tunnel
      << " param 0x" << std::hex << std::setw(2) << std::setfill('0') << +param
      << " @" << command ;
   return ss.str();
}

}
