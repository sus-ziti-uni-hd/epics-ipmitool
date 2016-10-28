#pragma once

#include <algorithm>
#include <cstdint>
#include <epicsTypes.h>
#include <map>
#include <memory>
#include <string>

struct aiRecord;
struct dbCommon;
struct link;
struct mbbiDirectRecord;
struct mbbiRecord;
struct sdr_record_common_sensor;
struct sdr_record_compact_sensor;
struct sdr_record_full_sensor;


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

/** The data we use to identify a sensor in the system. */
struct sensor_id_t {
  sensor_id_t(slave_addr_t _ipmb, uint8_t _entity, uint8_t _inst, const std::string& _name);
  /** Construct from an EPICS link. */
  explicit sensor_id_t(const ::link& _loc);
  /** Construct from SDR + IPMP address. */
  sensor_id_t(slave_addr_t _ipmb, const ::sdr_record_full_sensor* _sdr);
  sensor_id_t(slave_addr_t _ipmb, const ::sdr_record_compact_sensor* _sdr);

  slave_addr_t ipmb;
  uint8_t entity;
  uint8_t instance;
  std::string name;

  bool operator <(const sensor_id_t& _other) const;
  bool operator ==(const sensor_id_t& _other) const;

  std::string prettyPrint() const;
};


struct any_sensor_ptr {
   uint8_t type;

   explicit any_sensor_ptr();
   explicit any_sensor_ptr(::sdr_record_full_sensor* _p);
   explicit any_sensor_ptr(::sdr_record_compact_sensor* _p);

   /// Is a record available?
   operator bool() const;

   operator ::sdr_record_full_sensor*() const;
   operator ::sdr_record_common_sensor*() const;
   operator ::sdr_record_compact_sensor*() const;

   ::sdr_record_common_sensor* operator()() const;

private:
   std::shared_ptr<::sdr_record_common_sensor> data_ptr;
};


/** Current state of the sensor.
  * Can be online, not found, read error.
  */
struct sensor_status_t {
   /** Id of the sensor on the bus.
    *
    * Only valid if any_sensor_ptr().
    */
   uint8_t sensor_id { 0U };
   any_sensor_ptr sdr_record {};
   /** Last reading has been valid. */
   std::shared_ptr<bool> good { std::make_shared<bool>(false) };
};

/** Current status for each sensor. */
using status_map_t = std::map<sensor_id_t, sensor_status_t>;

/** Enum of all supported EPICS record types. */
enum class RecordType {
   ai,
   mbbi,
   mbbiDirect,
};

struct any_record_ptr {
   const RecordType type;

   any_record_ptr(::aiRecord *_rec);
   any_record_ptr(::mbbiDirectRecord *_rec);
   any_record_ptr(::mbbiRecord *_rec);

   operator ::aiRecord*() const;
   operator ::dbCommon*() const;
   operator ::mbbiDirectRecord*() const;
   operator ::mbbiRecord*() const;

   ::dbCommon* operator()() const;

   std::string pvName() const;

private:
   union {
      ::dbCommon *common;
      ::aiRecord *ai;
      ::mbbiDirectRecord *mbbiDirect;
      ::mbbiRecord *mbbi;
   } data_ptr;
};


} // namespace IPMIIOC
