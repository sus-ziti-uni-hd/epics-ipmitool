#include "ipmi_device.h"
#include "ipmi_device_private.h"
#include "ipmi_reader_thread.h"
#include "ipmi_internal.h"

#include <iomanip>
#include <iostream>
#include <string.h>

#include <alarm.h>
#include <aiRecord.h>
#include <dbAccess.h>
#include <errlog.h>
#include <mbbiRecord.h>
#include <recSup.h>

#include <logger.h>
#include <sstream>
#include <subsystem_registrator.h>
#include <fstream>

extern "C" {
// for ipmitool
  int verbose = 0;
  int csv_output = 0;
}


namespace {
SuS::logfile::subsystem_registrator log_id("IPMIDev");
} // namespace


namespace IPMIIOC {

Device::Device(short _id) {
  id_ = _id;
}


Device::~Device() {
  if (intf_->close) intf_->close(intf_);
  for (sensor_list_t::iterator i = sensors_.begin();
       i != sensors_.end(); ++i) {
    free(i->second.common);
  } // for i
} // Device destructor


void Device::aiCallback(::CALLBACK* _cb) {
  void* vpriv;
  callbackGetUser(vpriv, _cb);
  callback_private_t* priv = static_cast<callback_private_t*>(vpriv);

  result_t result = priv->thread->findResult(sensor_id_t(priv->sensor));
  if (result.valid == false) {
    dbScanLock(reinterpret_cast<dbCommon*>(priv->rec));
    // update the record
    priv->rec->udf = 1;
    typedef long(*real_signature)(dbCommon*);
    (*reinterpret_cast<real_signature>(priv->rec->rset->process))(priv->rec);
    dbScanUnlock(reinterpret_cast<dbCommon*>(priv->rec));
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
} // Device::aiCallback


bool Device::aiQuery(const sensor_id_t& _sensor, result_t& _result) {
  mutex_.lock();

  const ::sensor_reading* const sr = ipmiQuery(_sensor);
  if (!sr || !sr->s_reading_valid || !sr->s_has_analog_value) {
    mutex_.unlock();
    _result.valid = false;
    SuS_LOG_STREAM(warning, log_id(), "sensor 0x" << std::hex << +_sensor.ipmb << "/0x" << +_sensor.sensor << ": Read invalid.");
    return false;
  } // if

  _result.value.fval = sr->s_a_val;
  _result.rval = sr->s_reading;
  mutex_.unlock();
  _result.valid = true;
  return true;
} // Device::aiQuery


void Device::mbbiCallback(::CALLBACK* _cb) {
  void* vpriv;
  callbackGetUser(vpriv, _cb);
  callback_private_t* priv = static_cast<callback_private_t*>(vpriv);

  result_t result = priv->thread->findResult(sensor_id_t(priv->sensor));
  if (result.valid == false) {
    dbScanLock(reinterpret_cast<dbCommon*>(priv->rec));
    // update the record
    priv->rec->udf = 1;
    typedef long(*real_signature)(dbCommon*);
    (*reinterpret_cast<real_signature>(priv->rec->rset->process))(priv->rec);
    dbScanUnlock(reinterpret_cast<dbCommon*>(priv->rec));
    return;
  } // if

  dbScanLock((dbCommon*)priv->rec);
  // update the record
  priv->rec->udf = 0;
  ::mbbiRecord* const mbbi = reinterpret_cast< ::mbbiRecord*>(priv->rec);
  mbbi->val = result.value.ival;
  mbbi->rval = result.rval;
  typedef long(*real_signature)(dbCommon*);
  (*reinterpret_cast<real_signature>(priv->rec->rset->process))(reinterpret_cast<dbCommon*>(priv->rec));
  dbScanUnlock((dbCommon*)priv->rec);
} // Device::mbbiCallback


bool Device::mbbiQuery(const sensor_id_t& _sensor, result_t& _result) {
  mutex_.lock();

  const ::sensor_reading* const sr = ipmiQuery(_sensor);
  if (!sr || !sr->s_reading_valid || sr->s_has_analog_value) {
    mutex_.unlock();
    _result.valid = false;
    SuS_LOG_STREAM(warning, log_id(), "sensor 0x" << std::hex << +_sensor.ipmb << "/0x" << +_sensor.sensor << ": Read invalid.");
    return false;
  } // if

  _result.value.ival = sr->s_reading;
  _result.rval = sr->s_reading;
  mutex_.unlock();
  _result.valid = true;
  return true;
} // Device::mbbiQuery


const ::sensor_reading* Device::ipmiQuery(const sensor_id_t& _sensor) {
  sensor_list_t::const_iterator i = sensors_.find(_sensor);
  if (i == sensors_.end()) {
    SuS_LOG_STREAM(warning, log_id(), "sensor 0x" << std::hex << +_sensor.ipmb << "/0x" << +_sensor.sensor << " not found.");
    return nullptr;
  }

  intf_->target_addr = _sensor.ipmb;
  // returns a pointer to an internal variable
  const ::sensor_reading* const sr = ::ipmi_sdr_read_sensor_value(intf_,
                                     i->second.common,
                                     i->second.type, 2 /* precision */);
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
    if ((rsp->data[0] == 0) &&
        ((rsp->data[1] & 0x0F) == PICMG_ATCA_MAJOR_VERSION)) {
      version_accepted = true;
    }
  }
  SuS_LOG_STREAM(fine, log_id(), "PICMG " << (version_accepted ? "" : "not ") << "detected.");
  return version_accepted;
} // Device::check_PICMG


bool Device::connect(const std::string& _hostname, const std::string& _username,
                     const std::string& _password, int _privlevel) {
  SuS_LOG_STREAM(info, log_id(), "Connecting to '" << _hostname << "'.");
  intf_ = ::ipmi_intf_load(const_cast<char*>("lan"));
  if (!intf_) {
    SuS_LOG(severe, log_id(), "Cannot load lan interface.");
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
  SuS_LOG_STREAM(config, log_id(), "Connected to '" << _hostname << "'.");

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
  for (std::set<slave_addr_t>::const_iterator i = slaves_.begin(); i != slaves_.end();
       ++i) iterateSDRs(*i);

  SuS_LOG_STREAM(finer, log_id(), "Total sensor count: " << sensors_.size());

  for (sensor_list_t::iterator i = sensors_.begin();
       i != sensors_.end(); ++i) {
    std::stringstream ss;
    ss << "   @ 0x" << std::hex << std::setw(2) << std::setfill('0') << +i->first.ipmb
       << "/0x" << std::hex << std::setw(2) << std::setfill('0') << +i->first.sensor;
    switch (i->second.type) {
      case SDR_RECORD_TYPE_FULL_SENSOR:
        ss << " : full '" << i->second.full->id_string << "', event type " << +i->second.common->event_type << std::ends;
        break;
      case SDR_RECORD_TYPE_COMPACT_SENSOR:
        ss << " : compact '" << i->second.compact->id_string << "', event type " << +i->second.common->event_type << std::ends;
        break;
      default:
        ss << " : unexpected type 0x" << std::hex << +i->second.type << std::ends;
    }
    SuS_LOG(finer, log_id(), ss.str());
  } // for i
} // Device::detectSensors


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
    std::string name;
    switch (sensor.second.type) {
      case SDR_RECORD_TYPE_FULL_SENSOR:
        name = reinterpret_cast<const char*>(sensor.second.full->id_string);
        break;
      case SDR_RECORD_TYPE_COMPACT_SENSOR:
        name = reinterpret_cast<const char*>(sensor.second.compact->id_string);
        break;
      default:
        std::stringstream ss;
        ss << "unexpected type 0x" << std::hex << +sensor.second.type;
        name = ss.str();
    }

    // take a reading to check the return value type.
    const ::sensor_reading* const sr { ipmiQuery(sensor.first) };
    const std::string rectype { sr->s_has_analog_value ? "ai" : "mbbi" };
    of << std::endl
            << "record(" << rectype << ", \"" << name << "\")" << std::endl
            << "{" << std::endl
            << "   field(DTYP, \"ipmitool\")" << std::endl
            << "   field(INP,  \"#L" << +id_ << " A" << +sensor.first.ipmb
            << " C" << +sensor.first.sensor << " S00 @0\")" << std::endl
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
  sensor_id_t id(_addr, _rec->cmn.keys.sensor_num);
  any_sensor_ptr any;
  any.type = SDR_RECORD_TYPE_FULL_SENSOR;
  any.full = _rec;
  sensors_[ id ] = any;
  std::stringstream ss;
  ss << "  found full 0x" << std::hex << +id.ipmb << "/0x" << +id.sensor << std::dec << " : " << _rec->id_string
     << " (0x" << std::hex << std::setw(2) << std::setfill('0') << +_rec->cmn.sensor.type;
  if (_rec->cmn.sensor.type <= SENSOR_TYPE_MAX)
    ss << " => " << ::sensor_type_desc[_rec->cmn.sensor.type];
  ss << ")." << std::ends;
  SuS_LOG(finest, log_id(), ss.str());
} // Device::handleFullSensor


void Device::handleCompactSensor(slave_addr_t _addr, ::sdr_record_compact_sensor* _rec) {
  sensor_id_t id(_addr, _rec->cmn.keys.sensor_num);
  any_sensor_ptr any;
  any.type = SDR_RECORD_TYPE_COMPACT_SENSOR;
  any.compact = _rec;
  sensors_.emplace(id, any);
  std::stringstream ss;
  ss << "  found compact 0x" << std::hex << +_addr << "/0x" << +_rec->cmn.keys.sensor_num << std::dec << " : " << _rec->id_string
     << " (0x" << std::hex << std::setw(2) << std::setfill('0') << +_rec->cmn.sensor.type;
  if (_rec->cmn.sensor.type <= SENSOR_TYPE_MAX)
    ss << " => " << ::sensor_type_desc[_rec->cmn.sensor.type];
  ss << ")." << std::ends;
  SuS_LOG(finest, log_id(), ss.str());
} // Device::handleCompactSensor


Device::any_sensor_ptr Device::initInputRecord(::dbCommon* _rec, const ::link& _inp) {
  sensor_id_t id(_inp);
  sensor_list_t::const_iterator i = sensors_.find(id);
  if (i == sensors_.end()) {
    SuS_LOG_STREAM(warning, log_id(), "sensor 0x" << std::hex << +id.ipmb << "/0x" << +id.sensor << " not found.");
    return any_sensor_ptr();
  } // if

  dpvt_t* priv = new dpvt_t;
  callbackSetPriority(priorityLow, &priv->cb);
  callback_private_t* cb_priv = new callback_private_t(
    reinterpret_cast< ::dbCommon*>(_rec), id, readerThread_);
  callbackSetUser(cb_priv, &priv->cb);
  priv->cb.timer = NULL;
  _rec->dpvt = priv;

  if (strlen(_rec->desc) == 0) {
    switch (i->second.type) {
      case SDR_RECORD_TYPE_FULL_SENSOR:
        strncpy(_rec->desc, reinterpret_cast<const char*>(i->second.full->id_string), 40);
        break;
      case SDR_RECORD_TYPE_COMPACT_SENSOR:
        strncpy(_rec->desc, reinterpret_cast<const char*>(i->second.compact->id_string), 40);
        break;
      default:
        std::cerr << "Unexpected type 0x" << std::hex << +i->second.common->sensor.type << std::endl;
    }
    _rec->desc[40] = '\0';
  } // if
  return i->second;
}

void Device::initAiRecord(::aiRecord* _pai) {
  auto i = initInputRecord(reinterpret_cast< ::dbCommon*>(_pai), _pai->inp);
  callbackSetCallback(aiCallback, &static_cast<dpvt_t*>(_pai->dpvt)->cb);

  SuS_LOG_STREAM(finest, log_id(), "SENSOR " << +_pai->inp.value.abio.card);
  SuS_LOG_STREAM(finest, log_id(), "  THRESH "
                 << +i.common->mask.type.threshold.read.unr << " "
                 << +i.common->mask.type.threshold.read.ucr << " "
                 << +i.common->mask.type.threshold.read.unc << " "
                 << +i.common->mask.type.threshold.read.lnr << " "
                 << +i.common->mask.type.threshold.read.lcr << " "
                 << +i.common->mask.type.threshold.read.lnc);
  SuS_LOG_STREAM(finest, log_id(), "  HYSTERESIS "
                 << +i.common->sensor.capabilities.hysteresis);
  if (i.type == SDR_RECORD_TYPE_FULL_SENSOR) {
    SuS_LOG_STREAM(finest, log_id(), "  LINEAR " << +i.full->linearization);
  }

  // TODO: for compact sensor
  if (i.common->sensor.type == SDR_RECORD_TYPE_FULL_SENSOR) {
    ::sdr_record_full_sensor* const sdr = i.full;
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
        _pai->hyst = ::sdr_convert_sensor_reading(sdr, hyst);
      } // if
    } // if
  }
} // Device::initAiRecord


void Device::initMbbiRecord(::mbbiRecord* _pmbbi) {
  auto i = initInputRecord(reinterpret_cast< ::dbCommon*>(_pmbbi), _pmbbi->inp);
  callbackSetCallback(mbbiCallback, &static_cast<dpvt_t*>(_pmbbi->dpvt)->cb);
} // Device::initMbbiRecord


void Device::iterateSDRs(slave_addr_t _addr, bool _force_internal) {
  SuS_LOG_STREAM(finest, log_id(), "iterating @0x" << std::hex << +_addr << std::dec << (_force_internal ? ", internal." : "."));
  intf_->target_addr = _addr;

  ::ipmi_sdr_iterator* itr = ::ipmi_sdr_start(intf_, _force_internal ? 1 : 0);
  if (!itr) return;

  unsigned found = 0;
  while (::sdr_get_rs* header = ::ipmi_sdr_get_next_header(intf_, itr)) {
    uint8_t* const rec = ::ipmi_sdr_get_record(intf_, header, itr);
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
        break;
      default:
        SuS_LOG_STREAM(finest, log_id(), "ignoring sensor type 0x" << std::hex << +header->type << ".");
        break;
    } // switch
  } // while

  SuS_LOG_STREAM(finer, log_id(), "found " << found << " sensors @0x" << std::hex << +_addr << ".");
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
    readerThread_->enqueueSensorRead(query_job_t(_pai->inp, &Device::aiQuery));
    dpvt_t* priv = static_cast<dpvt_t*>(_pai->dpvt);
    ::callbackRequestDelayed(&priv->cb, 1.0);
    return true;
    // start async. operation
  } else {
    _pai->pact = FALSE;
    return true;
  } // else
} // Device::readAiSensor


bool Device::readMbbiSensor(::mbbiRecord* _pmbbi) {
  if (!_pmbbi->dpvt) {
    // when dpvt is not set, init failed
    _pmbbi->udf = 1;
    return false;
  } // if
  if (!_pmbbi->pact) {
    _pmbbi->pact = TRUE;
    readerThread_->enqueueSensorRead(query_job_t(_pmbbi->inp, &Device::mbbiQuery));
    dpvt_t* priv = static_cast<dpvt_t*>(_pmbbi->dpvt);
    ::callbackRequestDelayed(&priv->cb, 1.0);
    return true;
    // start async. operation
  } else {
    _pmbbi->pact = FALSE;
    return true;
  } // else
} // Device::readMbbiSensor


Device::query_job_t::query_job_t(const ::link& _loc, query_func_t _f)
  : sensor(_loc), query_func(_f) {
} // Device::query_job_t constructor

} // namespace IPMIIOC

