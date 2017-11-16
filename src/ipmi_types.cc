/* SPDX-License-Identifier: MIT */
#include "ipmi_types.h"

#include <dbCommon.h>
#include <epicsAssert.h>
#include <iomanip>
#include <sstream>
#include <stdlib.h>
#include <link.h>

extern "C" {
#include <ipmitool/ipmi.h>
#include <ipmitool/ipmi_entity.h>
#include <ipmitool/ipmi_fru.h>
#include <ipmitool/ipmi_sdr.h>
}

namespace IPMIIOC {

result_t::result_t() : valid(false) {
}


sensor_id_t::sensor_id_t(slave_addr_t _ipmb, uint8_t _entity, uint8_t _inst, const std::string& _name)
  : ipmb(_ipmb), entity(_entity), instance(_inst), name(_name) {
} // Device::sensor_id_t constructor


sensor_id_t::sensor_id_t(const ::link& _loc)
  : ipmb(_loc.value.abio.adapter), entity(_loc.value.abio.card), instance(_loc.value.abio.signal), name(_loc.value.abio.parm) {
} // Device::sensor_id_t constructor


sensor_id_t::sensor_id_t(slave_addr_t _ipmb, const ::sdr_record_full_sensor* _sdr)
   : ipmb{_ipmb}, entity{_sdr->cmn.entity.id}, instance(_sdr->cmn.entity.instance)
{
   name = decodeIdString(&_sdr->cmn, SDR_RECORD_TYPE_FULL_SENSOR);
}

sensor_id_t::sensor_id_t(slave_addr_t _ipmb, const ::sdr_record_compact_sensor* _sdr)
   : ipmb{_ipmb}, entity{_sdr->cmn.entity.id}, instance(_sdr->cmn.entity.instance)
{
   name = decodeIdString(&_sdr->cmn, SDR_RECORD_TYPE_COMPACT_SENSOR);
}


bool sensor_id_t::operator <(const sensor_id_t& _other) const {
  if (ipmb < _other.ipmb) return true;
  if (ipmb > _other.ipmb) return false;
  if (entity < _other.entity) return true;
  if (entity > _other.entity) return false;
  if (instance < _other.instance) return true;
  if (instance > _other.instance) return false;
  return  name < _other.name;
} // Device::sensor_id_t::operator <


bool sensor_id_t::operator ==(const sensor_id_t& _other) const {
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
   /*
   TODO: get this information from the status map.
   if (sensor_set) {
      ss << " (num 0x" << std::hex << std::setw(2) << std::setfill('0') << +sensor << ")";
   }
   */
   return ss.str();
}


any_sensor_ptr::any_sensor_ptr()
   : type{0U}, data_ptr{nullptr}
{}

any_sensor_ptr::any_sensor_ptr(::sdr_record_full_sensor* _p)
   : type{SDR_RECORD_TYPE_FULL_SENSOR}, data_ptr{std::shared_ptr<::sdr_record_common_sensor>(&_p->cmn, [](::sdr_record_common_sensor *p){::free(p);})}
{}

any_sensor_ptr::any_sensor_ptr(::sdr_record_compact_sensor* _p)
   : type{SDR_RECORD_TYPE_COMPACT_SENSOR}, data_ptr{std::shared_ptr<::sdr_record_common_sensor>(&_p->cmn, [](::sdr_record_common_sensor *p){::free(p);})}
{}

any_sensor_ptr::operator bool() const
{
   return data_ptr.get() != nullptr;
}

any_sensor_ptr::operator ::sdr_record_full_sensor*() const
{
   assert(type == SDR_RECORD_TYPE_FULL_SENSOR);
   return reinterpret_cast<::sdr_record_full_sensor*>(data_ptr.get());
}

any_sensor_ptr::operator ::sdr_record_common_sensor*() const
{
   return data_ptr.get();
}

any_sensor_ptr::operator ::sdr_record_compact_sensor*() const
{
   assert(type == SDR_RECORD_TYPE_COMPACT_SENSOR);
   return reinterpret_cast<::sdr_record_compact_sensor*>(data_ptr.get());
}

::sdr_record_common_sensor *any_sensor_ptr::operator()() const {
   return data_ptr.get();
}


any_record_ptr::any_record_ptr(::aiRecord *_rec) : type(RecordType::ai) {
   data_ptr.ai = _rec;
}

any_record_ptr::any_record_ptr(::mbbiDirectRecord *_rec) : type(RecordType::mbbiDirect) {
   data_ptr.mbbiDirect = _rec;
}

any_record_ptr::any_record_ptr(::mbbiRecord *_rec) : type(RecordType::mbbi) {
   data_ptr.mbbi = _rec;
}

any_record_ptr::operator ::aiRecord*() const {
   assert(type == RecordType::ai);
   return data_ptr.ai;
}

any_record_ptr::operator ::dbCommon*() const {
   return data_ptr.common;
}

any_record_ptr::operator ::mbbiDirectRecord*() const {
   assert(type == RecordType::mbbiDirect);
   return data_ptr.mbbiDirect;
}

any_record_ptr::operator ::mbbiRecord*() const {
   assert(type == RecordType::mbbi);
   return data_ptr.mbbi;
}

::dbCommon *any_record_ptr::operator()() const {
   return data_ptr.common;
}

std::string any_record_ptr::pvName() const {
   if (!data_ptr.common) {
      return "/NULL/";
   } else {
      return data_ptr.common->name;
   }
}


std::string decodeIdString(const ::sdr_record_common_sensor* _sdr, uint8_t _type) {
   unsigned i = 0;
   const uint8_t *data;
   switch (_type) {
      case SDR_RECORD_TYPE_FULL_SENSOR:
         data = &reinterpret_cast<const ::sdr_record_full_sensor*>(_sdr)->id_code;
         break;
      case SDR_RECORD_TYPE_COMPACT_SENSOR:
         data = &reinterpret_cast<const ::sdr_record_compact_sensor*>(_sdr)->id_code;
         break;
      default:
         return "";
   }
   std::unique_ptr<char, decltype(::free)*> name{::get_fru_area_str(const_cast<uint8_t*>(data), &i), ::free};
   return std::string{name.get()};
}

}
