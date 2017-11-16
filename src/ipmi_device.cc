/* SPDX-License-Identifier: MIT */
#include "ipmi_device.h"
#include "ipmi_device_private.h"
#include "ipmi_reader_thread.h"
#include "ipmi_internal.h"

#include <alarm.h>
#include <algorithm>
#include <array>
#include <aiRecord.h>
#include <atomic>
#include <callback.h>
#include <cstdlib>
#include <dbCommon.h>
#include <dbDefs.h>
#include <dbLock.h>
#include <epicsAssert.h>
#include <epicsMutex.h>
#include <epicsTypes.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <link.h>
#include <logger.h>
#include <mbbiDirectRecord.h>
#include <mbbiRecord.h>
#include <memory>
#include <recSup.h>
#include <sstream>
#include <string.h>
#include <subsystem_registrator.h>
#include <utility>

extern "C" {
#include <ipmitool/ipmi.h>
#include <ipmitool/ipmi_intf.h>
#include <ipmitool/ipmi_picmg.h>
#include <ipmitool/ipmi_sdr.h>

// UGLY...
#define class impi_class
#include <ipmitool/ipmi_sel.h>
#undef class

// for ipmitool
  int verbose = 0;
  int csv_output = 0;
}


namespace {
SuS::logfile::subsystem_registrator log_id("IPMIDev");

std::atomic<unsigned> nextid(0U);
} // namespace


namespace IPMIIOC {

std::map<RecordType, Device::FunctionTable> Device::record_functions_ = {
   { RecordType::ai, { .fill = &Device::fillAiRecord } },
   { RecordType::mbbiDirect, { .fill = &Device::initRecordDesc } },
   { RecordType::mbbi, { .fill = &Device::fillMbbiRecord } },
};

Device::Device(short _id) {
  id_ = _id;
  readerThread_=NULL;
}


Device::~Device() {
  if (intf_->close) intf_->close(intf_);
} // Device destructor


void Device::aiCallback(::CALLBACK* _cb) {
  void* vpriv;
  callbackGetUser(vpriv, _cb);
  callback_private_t* priv = static_cast<callback_private_t*>(vpriv);

  result_t result = priv->thread->findResult(static_cast<dpvt_t*>(priv->rec->dpvt)->pvid);
  if (result.valid == false) {
    dbScanLock(reinterpret_cast<dbCommon*>(priv->rec));
    // update the record
    priv->rec->udf = 1;
    typedef long(*real_signature)(dbCommon*);
    (*reinterpret_cast<real_signature>(priv->rec->rset->process))(priv->rec);
    dbScanUnlock(reinterpret_cast<dbCommon*>(priv->rec));

    if (*priv->good) {
       *priv->good = false;
      SuS_LOG_PRINTF(warning, log_id(), "sensor 0x%02x/%s: Read invalid.", priv->sensor.ipmb, priv->sensor.name.c_str());
    } // if
    return;
  } // if

  dbScanLock((dbCommon*)priv->rec);
  // update the record
  priv->rec->udf = 0;
  ::aiRecord* const ai = reinterpret_cast< ::aiRecord*>(priv->rec);
  ai->val = result.value.fval;
  ai->rval = result.rval;
  typedef long(*real_signature)(dbCommon*);
  (*reinterpret_cast<real_signature>(priv->rec->rset->process))(reinterpret_cast<dbCommon*>(priv->rec));
  dbScanUnlock((dbCommon*)priv->rec);
  if (!*priv->good) {
     *priv->good = true;
     SuS_LOG_PRINTF(info, log_id(), "sensor 0x%02x/%s: Read valid again.", priv->sensor.ipmb, priv->sensor.name.c_str());
  } // if
} // Device::aiCallback


bool Device::aiQuery(const sensor_id_t& _sensor, result_t& _result) {
  mutex_.lock();

  const ::sensor_reading* const sr = ipmiQuery(_sensor);
  if (!sr || !sr->s_reading_valid || !sr->s_has_analog_value) {
    mutex_.unlock();
    _result.valid = false;
    return false;
  } // if

  _result.value.fval = sr->s_a_val;
  _result.rval = sr->s_reading;
  mutex_.unlock();
  _result.valid = true;
  return true;
} // Device::aiQuery


void Device::mbbiDirectCallback(::CALLBACK* _cb) {
  void* vpriv;
  callbackGetUser(vpriv, _cb);
  callback_private_t* priv = static_cast<callback_private_t*>(vpriv);

  result_t result = priv->thread->findResult(static_cast<dpvt_t*>(priv->rec->dpvt)->pvid);
  if (result.valid == false) {
    dbScanLock(reinterpret_cast<dbCommon*>(priv->rec));
    // update the record
    priv->rec->udf = 1;
    typedef long(*real_signature)(dbCommon*);
    (*reinterpret_cast<real_signature>(priv->rec->rset->process))(priv->rec);
    dbScanUnlock(reinterpret_cast<dbCommon*>(priv->rec));

    if (*priv->good) {
       *priv->good = false;
      SuS_LOG_PRINTF(warning, log_id(), "sensor 0x%02x/%s: Read invalid.", priv->sensor.ipmb, priv->sensor.name.c_str());
    } // if
    return;
  } // if

  dbScanLock((dbCommon*)priv->rec);
  // update the record
  priv->rec->udf = 0;
  ::mbbiDirectRecord* const mbbi = reinterpret_cast< ::mbbiDirectRecord*>(priv->rec);
//  mbbi->val = result.value.ival;
  mbbi->rval = result.rval;
  mbbi->val = result.rval;
  typedef long(*real_signature)(dbCommon*);
  (*reinterpret_cast<real_signature>(priv->rec->rset->process))(reinterpret_cast<dbCommon*>(priv->rec));
  dbScanUnlock((dbCommon*)priv->rec);
  if (!*priv->good) {
     *priv->good = true;
     SuS_LOG_PRINTF(info, log_id(), "sensor 0x%02x/%s: Read valid again.", priv->sensor.ipmb, priv->sensor.name.c_str());
  } // if
} // Device::mbbiDirectCallback


bool Device::mbbiDirectQuery(const sensor_id_t& _sensor, result_t& _result) {
  mutex_.lock();

  const ::sensor_reading* const sr = ipmiQuery(_sensor);
  if (!sr || !sr->s_reading_valid || sr->s_has_analog_value) {
    mutex_.unlock();
    _result.valid = false;
    return false;
  } // if

  _result.value.ival = sr->s_reading;
  _result.rval = ((sr->s_data3 & 0x7FU) << 8) | sr->s_data2;
  mutex_.unlock();
  _result.valid = true;
  return true;
} // Device::mbbiDirectQuery


void Device::mbbiCallback(::CALLBACK* _cb) {
  void* vpriv;
  callbackGetUser(vpriv, _cb);
  callback_private_t* priv = static_cast<callback_private_t*>(vpriv);

  result_t result = priv->thread->findResult(static_cast<dpvt_t*>(priv->rec->dpvt)->pvid);
  if (result.valid == false) {
    dbScanLock(reinterpret_cast<dbCommon*>(priv->rec));
    // update the record
    priv->rec->udf = 1;
    typedef long(*real_signature)(dbCommon*);
    (*reinterpret_cast<real_signature>(priv->rec->rset->process))(priv->rec);
    dbScanUnlock(reinterpret_cast<dbCommon*>(priv->rec));

    if (*priv->good) {
       *priv->good = false;
      SuS_LOG_PRINTF(warning, log_id(), "sensor 0x%02x/%s: Read invalid.", priv->sensor.ipmb, priv->sensor.name.c_str());
    } // if
    return;
  } // if

  dbScanLock((dbCommon*)priv->rec);
  // update the record
  priv->rec->udf = 0;
  ::mbbiRecord* const mbbi = reinterpret_cast< ::mbbiRecord*>(priv->rec);
//  mbbi->val = result.value.ival;
  mbbi->rval = 0;
  SuS_LOG_PRINTF(finest, log_id(), "%s data %04x reading %02x", priv->rec->name, result.rval, result.value.ival);
  while(result.rval >>= 1) ++mbbi->rval;
  typedef long(*real_signature)(dbCommon*);
  (*reinterpret_cast<real_signature>(priv->rec->rset->process))(reinterpret_cast<dbCommon*>(priv->rec));
  dbScanUnlock((dbCommon*)priv->rec);
  if (!*priv->good) {
     *priv->good = true;
     SuS_LOG_PRINTF(info, log_id(), "sensor 0x%02x/%s: Read valid again.", priv->sensor.ipmb, priv->sensor.name.c_str());
  } // if
} // Device::mbbiCallback


bool Device::mbbiQuery(const sensor_id_t& _sensor, result_t& _result) {
  mutex_.lock();

  const ::sensor_reading* const sr = ipmiQuery(_sensor);
  if (!sr || !sr->s_reading_valid || sr->s_has_analog_value) {
    mutex_.unlock();
    _result.valid = false;
    return false;
  } // if

  _result.value.ival = sr->s_reading;
  _result.rval = ((sr->s_data3 & 0x7FU) << 8) | sr->s_data2;
  mutex_.unlock();
  _result.valid = true;
  return true;
} // Device::mbbiQuery


const ::sensor_reading* Device::ipmiQuery(const sensor_id_t& _sensor) {
  const auto& i = sensors_.find(_sensor);
  if (i == sensors_.end()) {
    SuS_LOG_PRINTF(severe, log_id(), "Sensor %s not found.", _sensor.prettyPrint().c_str());
    assert(false);
  }

  intf_->target_addr = _sensor.ipmb;
  // returns a pointer to an internal variable
  // TODO: global lock!
  const ::sensor_reading* const sr = ::ipmi_sdr_read_sensor_value(intf_,
                                     i->second.sdr_record,
                                     i->second.sdr_record.type, 2 /* precision */);
  return sr;
}


bool Device::check_PICMG() {
  // from ipmitool, ipmi_main.c
  ::ipmi_rq req;
  bool version_accepted = false;

  memset(&req, 0, sizeof(req));
  req.msg.netfn = IPMI_NETFN_PICMG;
  req.msg.cmd = PICMG_GET_PICMG_PROPERTIES_CMD;
  uint8_t msg_data = 0x00;
  req.msg.data = &msg_data;
  req.msg.data_len = 1;
  msg_data = 0;

  intf_->target_addr = local_addr_;
  const ::ipmi_rs* const rsp = intf_->sendrecv(intf_, &req);
  if (rsp && !rsp->ccode) {
    if (rsp->data[0] == 0){
      if(((rsp->data[1] & 0x0F) == PICMG_ATCA_MAJOR_VERSION ||
        (rsp->data[1] & 0x0F) == PICMG_AMC_MAJOR_VERSION ||
        (rsp->data[1] & 0x0F) == PICMG_UTCA_MAJOR_VERSION)) {
      version_accepted = true;
    }
      SuS_LOG_STREAM(finer, log_id(), "PICMG " <<(int)(rsp->data[1] & 0x0F) <<"."<< (int)((rsp->data[1] & 0xF0)>>4)<<" detected.");
  }
  }
  SuS_LOG_STREAM(finer, log_id(), "PICMG " << (version_accepted ? "" : "not ") << "accepted.");
  return version_accepted;
} // Device::check_PICMG


bool Device::connect(const std::string& _hostname, const std::string& _username,
                     const std::string& _password, const std::string& _proto, int _privlevel) {
  SuS_LOG_STREAM(config, log_id(), "Connecting to '" << _hostname << "'.");
  intf_ = ::ipmi_intf_load(const_cast<char*>(_proto.c_str()));
  if (!intf_) {
    SuS_LOG_PRINTF(severe, log_id(), "Cannot load interface '%s'.", _proto.c_str());
    return false;
  }

  ::ipmi_intf_session_set_hostname(intf_, const_cast<char*>(_hostname.c_str()));
  ::ipmi_intf_session_set_username(intf_, const_cast<char*>(_username.c_str()));
  ::ipmi_intf_session_set_password(intf_, const_cast<char*>(_password.c_str()));
  if (_privlevel > 0) ::ipmi_intf_session_set_privlvl(intf_, (uint8_t)_privlevel);

  if ((intf_->open == NULL) || (intf_->open(intf_) == -1)) {
    SuS_LOG_STREAM(severe, log_id(), "Connection to '" << _hostname << "' failed.");
    ::ipmi_cleanup(intf_);
    intf_ = NULL;
    return false;
  } // if
  // else
  SuS_LOG_STREAM(info, log_id(), "Connected to '" << _hostname << "'.");

  local_addr_ = intf_->target_addr;

  SuS_LOG(finer, log_id(), "Starting ReaderThread.");
  readerThread_ = new ReaderThread(this);
  readerThread_->start();

  return true;
} // Device::connect


void Device::detectSensors() {
  if (!intf_) {
    SuS_LOG(warning, log_id(), "Not scanning: not connected.");
    return;
  }
  sensors_.clear();
  iterateSDRs(local_addr_);
  if (sensors_.size() == 0) iterateSDRs(local_addr_, true);
  find_ipmb();
  for (const auto& i : slaves_) iterateSDRs(i);

  SuS_LOG_STREAM(fine, log_id(), "Total sensor count: " << sensors_.size());

  for (const auto& i : sensors_) {
    if (!i.second.sdr_record()) {
       // only defined by a PV
       continue;
    }
    std::stringstream ss;
    ss << "sensor 0x" << std::hex << std::setw(2) << std::setfill('0') << +i.first.ipmb
       << "/0x" << std::hex << std::setw(2) << std::setfill('0') << +i.second.sensor_id;
    std::string name {decodeIdString(i.second.sdr_record, i.second.sdr_record.type)};

    switch (i.second.sdr_record.type) {
      case SDR_RECORD_TYPE_FULL_SENSOR:
        ss << " : full '" << name
                << "', event type 0x" << std::hex << std::setw(2) << std::setfill('0') << +i.second.sdr_record()->event_type
                << ", type 0x" << std::hex << std::setw(2) << std::setfill('0') << +i.second.sdr_record()->sensor.type;
        break;
      case SDR_RECORD_TYPE_COMPACT_SENSOR:
        ss << " : compact '" << name
                << "', event type 0x" << std::hex << std::setw(2) << std::setfill('0') << +i.second.sdr_record()->event_type
                << ", type 0x" << std::hex << std::setw(2) << std::setfill('0') << +i.second.sdr_record()->sensor.type;
        break;
      default:
        ss << " : unexpected type 0x" << std::hex << +i.second.sdr_record.type;
    }
    if (i.second.sdr_record()->sensor.type <= SENSOR_TYPE_MAX)
      ss << " (" << ::sensor_type_desc[i.second.sdr_record()->sensor.type] << ")";
    ss << "." << std::ends;

    SuS_LOG(finer, log_id(), ss.str());
  } // for i
} // Device::detectSensors


void Device::scanActiveIPMBs() {
  if (!intf_) {
    SuS_LOG(warning, log_id(), "Not scanning: not connected.");
    return;
  }
  for (const auto i : active_ipmbs_) iterateSDRs(i);
}


void Device::dumpDatabase(const std::string& _file) {
  if (!intf_) {
    SuS_LOG(warning, log_id(), "Not connected.");
    return;
  }

  std::ofstream of(_file);
  if(!of.is_open()) {
    SuS_LOG_STREAM(warning, log_id(), "Cannot open '" << _file << "' for writing.");
    return;
  }

  of << "# This file has been automatically created." << std::endl
          << "# It is not expected to work as-is." << std::endl
          << "# Please update the PV names and add SCAN fields as required." << std::endl;

  for (const auto& sensor : sensors_) {
    if (!sensor.second.sdr_record()) {
       // only defined by a PV
       continue;
    }
    std::string name {decodeIdString(sensor.second.sdr_record, sensor.second.sdr_record.type)};
    if (name.empty()) {
        std::stringstream ss;
        ss << "unexpected type 0x" << std::hex << +sensor.second.sdr_record.type;
        name = ss.str();
    }

    std::string pvname(name);
    std::replace( pvname.begin(), pvname.end(), '.', '_');
    std::replace( pvname.begin(), pvname.end(), ' ', '_');

    // take a reading to check the return value type.
    const ::sensor_reading* const sr { ipmiQuery(sensor.first) };
    if (!sr) {
       // TODO
       continue;
    }
    const std::string rectype { sr->s_has_analog_value ? "ai" : "mbbi" };
    of << std::endl
            << "# " << name
            << ", event type 0x" << std::hex << std::setw(2) << std::setfill('0') << +sensor.second.sdr_record()->event_type
            << ", type 0x" << std::hex << std::setw(2) << std::setfill('0') << +sensor.second.sdr_record()->sensor.type;
    if (sensor.second.sdr_record()->sensor.type <= SENSOR_TYPE_MAX)
      of << " (" << ::sensor_type_desc[sensor.second.sdr_record()->sensor.type] << ")";

    of << std::endl
            << "record(" << rectype << ", \"" << pvname << "\")" << std::endl
            << "{" << std::endl
            << "   field(DTYP, \"ipmitool\")" << std::endl << std::dec
            << "   field(INP,  \"#L" << +id_ << " A" << +sensor.first.ipmb
            << " C" << +sensor.first.entity << " S" << +sensor.first.instance <<" @"<< name <<"\")" << std::endl
            << "}" << std::endl;
  } // for sensor
 
  if (!of.good()) {
    SuS_LOG(warning, log_id(), "Write failed.");
  }
  of.close();
} // Device::dumpDatabase


void Device::find_ipmb() {
  if (!check_PICMG()) return;

  // from ipmitool
  ::ipmi_rq req;
  uint8_t msg_data[5];

  memset(&req, 0, sizeof(req));
  req.msg.netfn = IPMI_NETFN_PICMG;
  req.msg.cmd = PICMG_GET_ADDRESS_INFO_CMD;
  req.msg.data = msg_data;
  req.msg.data_len = 2;
  msg_data[0] = 0;   /* picmg identifier */
  msg_data[1] = 0;   /* default fru id */

  const ::ipmi_rs* const rsp = intf_->sendrecv(intf_, &req);
  if (!rsp) return;
  else if (rsp->ccode) return;

  slaves_.insert(rsp->data[2]);
} // Device::find_ipmb

void Device::handleFullSensor(slave_addr_t _addr, ::sdr_record_full_sensor* _rec) {
  sensor_id_t id{_addr, _rec};
  // creates entry, if not found
  auto &i = sensors_[id];
  if (i.sdr_record()) {
     // already known
     return;
  }
  i.sdr_record = any_sensor_ptr{_rec};
  i.sensor_id = _rec->cmn.keys.sensor_num;
  *i.good = true;
  std::stringstream ss;
  ss << "  found full " << id.prettyPrint()
     << " (0x" << std::hex << std::setw(2) << std::setfill('0') << +_rec->cmn.sensor.type;
  if (_rec->cmn.sensor.type <= SENSOR_TYPE_MAX)
    ss << " => " << ::sensor_type_desc[_rec->cmn.sensor.type];
  ss << ")." << std::ends;
  SuS_LOG(finest, log_id(), ss.str());

  fillPVsFromSDR(id, i.sdr_record);
} // Device::handleFullSensor


void Device::handleCompactSensor(slave_addr_t _addr, ::sdr_record_compact_sensor* _rec) {
  sensor_id_t id{_addr, _rec};
  // creates entry, if not found
  auto &i = sensors_[id];
  if (i.sdr_record()) {
     // already known
     return;
  }
  i.sdr_record = any_sensor_ptr{_rec};
  i.sensor_id = _rec->cmn.keys.sensor_num;
  *i.good = true;
  std::stringstream ss;
  ss << "  found compact " << id.prettyPrint()
     << " (0x" << std::hex << std::setw(2) << std::setfill('0') << +_rec->cmn.sensor.type;
  if (_rec->cmn.sensor.type <= SENSOR_TYPE_MAX)
    ss << " => " << ::sensor_type_desc[_rec->cmn.sensor.type];
  ss << ")." << std::ends;
  SuS_LOG(finest, log_id(), ss.str());

  fillPVsFromSDR(id, i.sdr_record);
} // Device::handleCompactSensor


void Device::initInputRecord(::dbCommon* _rec, const ::link& _inp) {
  sensor_id_t id{_inp};
  active_ipmbs_.insert(id.ipmb);

  dpvt_t* priv = new dpvt_t;
  priv->pvid = nextid++;
  callbackSetPriority(priorityLow, &priv->cb);
  auto &i = sensors_[id];
  callback_private_t* cb_priv = new callback_private_t{
    reinterpret_cast< ::dbCommon*>(_rec), id, readerThread_, i.good.get()};
  callbackSetUser(cb_priv, &priv->cb);
  priv->cb.timer = NULL;
  _rec->dpvt = priv;
}


void Device::initRecordDesc(const any_record_ptr& _rec, const any_sensor_ptr& _sdr) {
  if (::strlen(_rec()->desc) == 0) {
    std::string name {decodeIdString(_sdr(), _sdr.type)};
    if (name.empty()) {
        std::cerr << "Unexpected type 0x" << std::hex << +_sdr()->sensor.type << std::endl;
    }
    strncpy(_rec()->desc, name.c_str(), 40);
    _rec()->desc[40] = '\0';
  } // if
}


void Device::initAiRecord(::aiRecord* _pai) {
  initInputRecord(reinterpret_cast< ::dbCommon*>(_pai), _pai->inp);
  pv_map_.emplace(sensor_id_t{_pai->inp}, _pai);
  if(_pai->dpvt==NULL) return;
  callbackSetCallback(aiCallback, &static_cast<dpvt_t*>(_pai->dpvt)->cb);
}

void Device::fillAiRecord(const any_record_ptr& _rec, const any_sensor_ptr& i) {
  initRecordDesc(_rec, i);
  auto _pai = static_cast<::aiRecord *>(_rec);
  SuS_LOG_STREAM(finest, log_id(), "SENSOR " << +_pai->inp.value.abio.card);
  SuS_LOG_STREAM(finest, log_id(), "  THRESH "
                 << +i()->mask.type.threshold.read.unr << " "
                 << +i()->mask.type.threshold.read.ucr << " "
                 << +i()->mask.type.threshold.read.unc << " "
                 << +i()->mask.type.threshold.read.lnr << " "
                 << +i()->mask.type.threshold.read.lcr << " "
                 << +i()->mask.type.threshold.read.lnc);
  SuS_LOG_STREAM(finest, log_id(), "  HYSTERESIS "
                 << +i()->sensor.capabilities.hysteresis);
  if (i.type == SDR_RECORD_TYPE_FULL_SENSOR) {
    SuS_LOG_STREAM(finest, log_id(), "  LINEAR " << +static_cast<::sdr_record_full_sensor*>(i)->linearization);
  }

  // TODO: for compact sensor
  if (i.type == SDR_RECORD_TYPE_FULL_SENSOR) {
    ::sdr_record_full_sensor* const sdr = i;
    // swap for 1/x conversions, cf. section 36.5 of IPMI specification
    bool swap_hi_lo = (sdr->linearization == SDR_SENSOR_L_1_X);

    if (swap_hi_lo) {
      _pai->lopr = ::sdr_convert_sensor_reading(sdr, sdr->sensor_max);
      _pai->hopr = ::sdr_convert_sensor_reading(sdr, sdr->sensor_min);
    } else {
      _pai->hopr = ::sdr_convert_sensor_reading(sdr, sdr->sensor_max);
      _pai->lopr = ::sdr_convert_sensor_reading(sdr, sdr->sensor_min);
    } // else

    if (sdr->cmn.mask.type.threshold.read.ucr) {
      if (swap_hi_lo) {
        _pai->lolo = ::sdr_convert_sensor_reading(sdr, sdr->threshold.upper.critical);
        _pai->llsv = ::epicsSevMajor;
      } else {
        _pai->hihi = ::sdr_convert_sensor_reading(sdr, sdr->threshold.upper.critical);
        _pai->hhsv = ::epicsSevMajor;
      } // else
    } // if
    if (sdr->cmn.mask.type.threshold.read.lcr) {
      if (swap_hi_lo) {
        _pai->hihi = ::sdr_convert_sensor_reading(sdr, sdr->threshold.lower.critical);
        _pai->hhsv = ::epicsSevMajor;
      } else {
        _pai->lolo = ::sdr_convert_sensor_reading(sdr, sdr->threshold.lower.critical);
        _pai->llsv = ::epicsSevMajor;
      } // else
    } // if

    if (sdr->cmn.mask.type.threshold.read.unc) {
      if (swap_hi_lo) {
        _pai->low = ::sdr_convert_sensor_reading(sdr, sdr->threshold.upper.non_critical);
        _pai->lsv = ::epicsSevMinor;
      } else {
        _pai->high = ::sdr_convert_sensor_reading(sdr, sdr->threshold.upper.non_critical);
        _pai->hsv = ::epicsSevMinor;
      } // else
    } // if
    if (sdr->cmn.mask.type.threshold.read.lnc) {
      if (swap_hi_lo) {
        _pai->high = ::sdr_convert_sensor_reading(sdr, sdr->threshold.lower.non_critical);
        _pai->hsv = ::epicsSevMinor;
      } else {
        _pai->low = ::sdr_convert_sensor_reading(sdr, sdr->threshold.lower.non_critical);
        _pai->lsv = ::epicsSevMinor;
      } // else
    } // if

    // hysteresis is given in raw values. converting to a fixed value only makes
    // sense for linear conversions.
    if (+sdr->linearization == SDR_SENSOR_L_LINEAR) {
      if ((sdr->cmn.sensor.capabilities.hysteresis == 1)
          || (sdr->cmn.sensor.capabilities.hysteresis == 2)) {
        uint8_t hyst = std::min(sdr->threshold.hysteresis.positive, sdr->threshold.hysteresis.negative);
        _pai->hyst = ::sdr_convert_sensor_hysterisis(sdr, hyst);
      } // if
    } // if

    {// Units
      const char *c=ipmi_sdr_get_unit_string(sdr->cmn.unit.pct,
        sdr->cmn.unit.modifier, sdr->cmn.unit.type.base, sdr->cmn.unit.type.modifier);
      if(c) strncpy(_pai->egu,c,sizeof(_pai->egu));
    }
  }
} // Device::fillAiRecord


void Device::initMbbiDirectRecord(::mbbiDirectRecord* _pmbbi) {
  initInputRecord(reinterpret_cast< ::dbCommon*>(_pmbbi), _pmbbi->inp);
  pv_map_.emplace(sensor_id_t{_pmbbi->inp}, _pmbbi);
  if(_pmbbi->dpvt==NULL) return;
  callbackSetCallback(mbbiDirectCallback, &static_cast<dpvt_t*>(_pmbbi->dpvt)->cb);
} // Device::initMbbiDirectRecord


void Device::initMbbiRecord(::mbbiRecord* _pmbbi) {
  initInputRecord(reinterpret_cast< ::dbCommon*>(_pmbbi), _pmbbi->inp);
  pv_map_.emplace(sensor_id_t{_pmbbi->inp}, _pmbbi);
  if(_pmbbi->dpvt==NULL) return;
  callbackSetCallback(mbbiCallback, &static_cast<dpvt_t*>(_pmbbi->dpvt)->cb);
}

void Device::fillMbbiRecord(const any_record_ptr& _rec, const any_sensor_ptr& i) {
  initRecordDesc(_rec, i);
  auto _pmbbi = static_cast<::mbbiRecord*>(_rec);
  static const std::array<epicsUInt32 mbbiRecord::*, 16> mbbiValues = {
    &mbbiRecord::zrvl, &mbbiRecord::onvl, &mbbiRecord::twvl, &mbbiRecord::thvl,
    &mbbiRecord::frvl, &mbbiRecord::fvvl, &mbbiRecord::sxvl, &mbbiRecord::svvl,
    &mbbiRecord::eivl, &mbbiRecord::nivl, &mbbiRecord::tevl, &mbbiRecord::elvl,
    &mbbiRecord::tvvl, &mbbiRecord::ttvl, &mbbiRecord::ftvl, &mbbiRecord::ffvl
  };

  static const std::array<char (mbbiRecord::*)[26], 16> mbbiStrings = {
    &mbbiRecord::zrst, &mbbiRecord::onst, &mbbiRecord::twst, &mbbiRecord::thst,
    &mbbiRecord::frst, &mbbiRecord::fvst, &mbbiRecord::sxst, &mbbiRecord::svst,
    &mbbiRecord::eist, &mbbiRecord::nist, &mbbiRecord::test, &mbbiRecord::elst,
    &mbbiRecord::tvst, &mbbiRecord::ttst, &mbbiRecord::ftst, &mbbiRecord::ffst
  };

  const struct ipmi_event_sensor_types *evt;
  uint8_t typ;
  if (i()->event_type == 0x6f) {
    evt = sensor_specific_event_types;
    typ = i()->sensor.type;
  } else {
    evt = generic_event_types;
    typ = i()->event_type;
  }

  for (; evt->desc != NULL; evt++) {
    if ((evt->code != typ) || (evt->data != 0xFF))
    {
      continue;
    }

    _pmbbi->*mbbiValues[evt->offset] = evt->offset;
    strncpy(_pmbbi->*mbbiStrings[evt->offset], evt->desc, 26);
    (_pmbbi->*mbbiStrings[evt->offset])[25] = '\0';
  }
} // Device::fillMbbiRecord


void Device::iterateSDRs(slave_addr_t _addr, bool _force_internal) {
  SuS_LOG_STREAM(finest, log_id(), "iterating @0x" << std::hex << +_addr << (_force_internal ? ", internal." : "."));
  intf_->target_addr = _addr;

  std::unique_ptr<::ipmi_sdr_iterator, decltype(::free)*> itr{::ipmi_sdr_start(intf_, _force_internal ? 1 : 0), ::free};
  if (!itr) return;

  unsigned found = 0;
  while (::sdr_get_rs* header = ::ipmi_sdr_get_next_header(intf_, itr.get())) {
    uint8_t* const rec = ::ipmi_sdr_get_record(intf_, header, itr.get());
    if (rec == NULL) {
      // TODO
      continue;
    } // if

    ++found;
    switch (header->type) {
      case SDR_RECORD_TYPE_FULL_SENSOR:
        handleFullSensor(_addr, reinterpret_cast< ::sdr_record_full_sensor*>(rec));
        break;
      case SDR_RECORD_TYPE_COMPACT_SENSOR:
        handleCompactSensor(_addr, reinterpret_cast< ::sdr_record_compact_sensor*>(rec));
        break;
      case SDR_RECORD_TYPE_MC_DEVICE_LOCATOR:
        slaves_.insert(reinterpret_cast< ::sdr_record_mc_locator*>(rec)->dev_slave_addr);
        ::free(rec);
        break;
      default:
        SuS_LOG_STREAM(finest, log_id(), "ignoring sensor type 0x" << std::hex << +header->type << ".");
        ::free(rec);
        break;
    } // switch
  } // while

  SuS_LOG_STREAM(finer, log_id(), "found " << found << " sensors @0x" << std::hex << +_addr << (_force_internal ? ", internal." : "."));
} // Device::iterateSDRs


bool Device::ping() {
  mutex_.lock();
  bool ret = intf_->keepalive(intf_) == 0;
  mutex_.unlock();
  if (!ret) {
    SuS_LOG(warning, log_id(), "keepalive failed!");
  }
  return ret;
} // Device::ping


bool Device::readAiSensor(::aiRecord* _pai) {
  if (!_pai->dpvt) {
    // when dpvt is not set, init failed
    _pai->udf = 1;
    return false;
  } // if
  if (!_pai->pact) {
    _pai->pact = TRUE;
    dpvt_t* priv = static_cast<dpvt_t*>(_pai->dpvt);
    readerThread_->enqueueSensorRead(query_job_t{_pai->inp, &Device::aiQuery, priv->pvid});
    ::callbackRequestDelayed(&priv->cb, 10.0);
    return true;
    // start async. operation
  } else {
    _pai->pact = FALSE;
    return true;
  } // else
} // Device::readAiSensor


bool Device::readMbbiDirectSensor(::mbbiDirectRecord* _pmbbi) {
  if (!_pmbbi->dpvt) {
    // when dpvt is not set, init failed
    _pmbbi->udf = 1;
    return false;
  } // if
  if (!_pmbbi->pact) {
    _pmbbi->pact = TRUE;
    dpvt_t* priv = static_cast<dpvt_t*>(_pmbbi->dpvt);
    readerThread_->enqueueSensorRead(query_job_t(_pmbbi->inp, &Device::mbbiDirectQuery, priv->pvid));
    ::callbackRequestDelayed(&priv->cb, 10.0);
    return true;
    // start async. operation
  } else {
    _pmbbi->pact = FALSE;
    return true;
  } // else
} // Device::readMbbiDirectSensor


bool Device::readMbbiSensor(::mbbiRecord* _pmbbi) {
  if (!_pmbbi->dpvt) {
    // when dpvt is not set, init failed
    _pmbbi->udf = 1;
    return false;
  } // if
  if (!_pmbbi->pact) {
    _pmbbi->pact = TRUE;
    dpvt_t* priv = static_cast<dpvt_t*>(_pmbbi->dpvt);
    readerThread_->enqueueSensorRead(query_job_t(_pmbbi->inp, &Device::mbbiQuery, priv->pvid));
    ::callbackRequestDelayed(&priv->cb, 10.0);
    return true;
    // start async. operation
  } else {
    _pmbbi->pact = FALSE;
    return true;
  } // else
} // Device::readMbbiSensor


void Device::fillPVsFromSDR(const sensor_id_t &_id, const any_sensor_ptr &_sdr) {
  const auto range = pv_map_.equal_range(_id);
  for (auto pv = range.first; pv != range.second; ++pv) {
     const auto &fill = record_functions_[pv->second.type].fill;
     if (fill) (this->*fill)(pv->second, _sdr);
  }
}

Device::query_job_t::query_job_t(const ::link& _loc, query_func_t _f, unsigned _pvid)
  : sensor(_loc), query_func(_f), pvid(_pvid) {
} // Device::query_job_t constructor


} // namespace IPMIIOC

