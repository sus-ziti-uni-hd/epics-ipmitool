#include "ipmi_device.h"
#include "ipmi_device_private.h"
#include "ipmi_reader_thread.h"
#include "ipmi_internal.h"

#include <iomanip>
#include <iostream>
#include <string.h>
#include <algorithm>

#include <alarm.h>
#include <boRecord.h>
#include <dbAccess.h>
#include <errlog.h>
#include <mbbiDirectRecord.h>
#include <mbboDirectRecord.h>
#include <recSup.h>

#include <logger.h>
#include <sstream>
#include <subsystem_registrator.h>
#include <fstream>

// UGLY...
#define class impi_class
#include <ipmitool/ipmi_sel.h>
#undef class

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
} // Device destructor


void Device::mbbiDirectCallback(::CALLBACK* _cb) {
  void* vpriv;
  callbackGetUser(vpriv, _cb);
  callback_private_t* priv = static_cast<callback_private_t*>(vpriv);

  result_t result = priv->thread->findResult(sensorcmd_id_t(priv->sensor));
  if (result.valid == false) {
    dbScanLock(reinterpret_cast<dbCommon*>(priv->rec));
    // update the record
    priv->rec->udf = 1;
    typedef long(*real_signature)(dbCommon*);
    (*reinterpret_cast<real_signature>(priv->rec->rset->process))(priv->rec);
    dbScanUnlock(reinterpret_cast<dbCommon*>(priv->rec));

    if (*priv->good) {
       *priv->good = false;
      SuS_LOG_STREAM(warning, log_id(), "sensor: command failed. " << priv->sensor.prettyPrint());
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
     SuS_LOG_STREAM(info, log_id(), "sensor: Read valid again. " << priv->sensor.prettyPrint());
  } // if
} // Device::mbbiDirectCallback


void Device::mbboDirectCallback(::CALLBACK* _cb) {
  void* vpriv;
  callbackGetUser(vpriv, _cb);
  callback_private_t* priv = static_cast<callback_private_t*>(vpriv);

  result_t result = priv->thread->findResult(sensorcmd_id_t(priv->sensor));
  if (result.valid == false) {
    dbScanLock(reinterpret_cast<dbCommon*>(priv->rec));
    // update the record
    priv->rec->udf = 1;
    typedef long(*real_signature)(dbCommon*);
    (*reinterpret_cast<real_signature>(priv->rec->rset->process))(priv->rec);
    dbScanUnlock(reinterpret_cast<dbCommon*>(priv->rec));

    if (*priv->good) {
       *priv->good = false;
      SuS_LOG_STREAM(warning, log_id(), "sensor: command failed. " << priv->sensor.prettyPrint());
    } // if

    return;
  } // if

  dbScanLock((dbCommon*)priv->rec);
  // update the record
  priv->rec->udf = 0;
  ::mbboDirectRecord* const mbbo = reinterpret_cast< ::mbboDirectRecord*>(priv->rec);
//  mbbo->val = result.value.ival;
  mbbo->rval = result.rval;
  mbbo->val = result.rval;
  typedef long(*real_signature)(dbCommon*);
  (*reinterpret_cast<real_signature>(priv->rec->rset->process))(reinterpret_cast<dbCommon*>(priv->rec));
  dbScanUnlock((dbCommon*)priv->rec);
  if (!*priv->good) {
     *priv->good = true;
     SuS_LOG_STREAM(info, log_id(), "sensor: Read valid again. " << priv->sensor.prettyPrint());
  } // if
} // Device::mbboDirectCallback


void Device::boCallback(::CALLBACK* _cb) {
//  SuS_LOG_STREAM(info, log_id(), "boCallback " <<  (unsigned long long)_cb);
  std::cout << "boCallback " <<  (unsigned long long)_cb << std::endl;
  void* vpriv;
  callbackGetUser(vpriv, _cb);
//   SuS_LOG_STREAM(info, log_id(), " " << (unsigned long long)vpriv);
  callback_private_t* priv = static_cast<callback_private_t*>(vpriv);
//   SuS_LOG_STREAM(info, log_id(), " " << (unsigned long long)priv << " " << (char *)&priv->sensor);

//   SuS_LOG_STREAM(info, log_id(), "find result " << priv->sensor.prettyPrint());
   std::cout << "find result " << priv->sensor.prettyPrint() << std::endl;
  result_t result = priv->thread->findResult(sensorcmd_id_t(priv->sensor));
  SuS_LOG_STREAM(info, log_id(), "result is " << result.valid << "\n");
  if (result.valid == false) {
    dbScanLock(reinterpret_cast<dbCommon*>(priv->rec));
    // update the record
    priv->rec->udf = 1;
    typedef long(*real_signature)(dbCommon*);
    (*reinterpret_cast<real_signature>(priv->rec->rset->process))(priv->rec);
    dbScanUnlock(reinterpret_cast<dbCommon*>(priv->rec));

    if (*priv->good) {
       *priv->good = false;
      SuS_LOG_STREAM(warning, log_id(), "sensor: command failed." << priv->sensor.prettyPrint());
    } // if

    SuS_LOG_STREAM(info, log_id(), "boCallback end 1\n");
    return;
  } // if

  dbScanLock((dbCommon*)priv->rec);
  // update the record
  priv->rec->udf = 0;
  ::boRecord* const bo = reinterpret_cast< ::boRecord*>(priv->rec);
//  mbbo->val = result.value.ival;
  bo->rval = result.rval;
  bo->val = result.rval;
  typedef long(*real_signature)(dbCommon*);
  (*reinterpret_cast<real_signature>(priv->rec->rset->process))(reinterpret_cast<dbCommon*>(priv->rec));
  dbScanUnlock((dbCommon*)priv->rec);
  if (!*priv->good) {
     *priv->good = true;
     SuS_LOG_STREAM(info, log_id(), "sensor: Read valid again." << priv->sensor.prettyPrint());
  } // if
  SuS_LOG_STREAM(info, log_id(), "boCallback end 2\n");
} // Device::boCallback

bool Device::boQuery(const query_job_t& _sensor, result_t& _result) {
  SuS_LOG_STREAM(info, log_id(), "boQuery\n");
  mutex_.lock();

  //const ::sensor_reading* const 
  query_result_t sr = ipmiQuery(_sensor);
//   if (!sr || !sr->s_reading_valid || sr->s_has_analog_value) {
  if(!sr.valid || sr.retval!=0){
    mutex_.unlock();
    _result.valid = false;
    SuS_LOG_STREAM(info, log_id(), "boQuery false\n");
    return false;
  } // if

  _result.value.ival = 0;//sr->s_reading;
  _result.rval = 0;//((sr->s_data3 & 0x7FU) << 8) | sr->s_data2;
  mutex_.unlock();
  _result.valid = true;
  SuS_LOG_STREAM(info, log_id(), "boQuery true\n");
  return true;
} // Device::boQuery


bool Device::mbboDirectQuery(const query_job_t& _sensor, result_t& _result) {
  mutex_.lock();

//   const ::sensor_reading* const 
  query_result_t sr = ipmiQuery(_sensor);
//   if (!sr || !sr->s_reading_valid || sr->s_has_analog_value) {
  if(!sr.valid || sr.retval!=0){
    mutex_.unlock();
    _result.valid = false;
    return false;
  } // if

  _result.value.ival = 0;//sr->s_reading;
  _result.rval = 0;//((sr->s_data3 & 0x7FU) << 8) | sr->s_data2;
  mutex_.unlock();
  _result.valid = true;
  return true;
} // Device::mbboDirectQuery


bool Device::mbbiDirectQuery(const query_job_t& _sensor, result_t& _result) {
  mutex_.lock();

//   const ::sensor_reading* const 
  query_result_t sr = ipmiQuery(_sensor);
//   if (!sr || !sr->s_reading_valid || sr->s_has_analog_value) {
  if(!sr.valid || sr.retval!=0){
    mutex_.unlock();
    _result.valid = false;
    return false;
  } // if

  unsigned long long a=0;
  for(int i=7; i>=0; i--){
    a<<=8;
    if( i<sr.len) a|=sr.data[i];
  }
  printf("==$%llX==\n",a);
  _result.value.ival = a;//sr->s_reading;
  _result.rval = a;//((sr->s_data3 & 0x7FU) << 8) | sr->s_data2;
  mutex_.unlock();
  _result.valid = true;
  return true;
} // Device::mbbiDirectQuery


// const ::sensor_reading*
query_result_t Device::ipmiQuery(const query_job_t& _sensor) {
  // SuS_LOG_STREAM(info, log_id(), "ipmiQuery\n");
  sensor_list_t::const_iterator it = sensors_.find(_sensor.sensor);
  if (it == sensors_.end()) {
     SuS_LOG_PRINTF(warning, log_id(), "sensor %s not found.", _sensor.sensor.prettyPrint().c_str());
     return query_result_t();
  }

  // from ipmitool
  ::ipmi_rq req;
  uint8_t msg_data[16];// unclear what max size could be
  memset(msg_data, 0, sizeof(msg_data));

  intf_->target_addr = _sensor.sensor.ipmb;
  if(_sensor.sensor.ipmb_tunnel!=0) intf_->target_channel=7;
/*  intf_->target_channel = 0;
  intf_->transit_addr = 0;
  intf_->transit_channel = 0;
  intf_->my_addr = 0; //IPMI_BMC_SLAVE_ADDR;
  if(_sensor.ipmb_tunnel!=0){
    intf_->target_channel = 7;
    intf_->transit_addr = _sensor.ipmb_tunnel;
    intf_->transit_channel = 0;
    intf_->my_addr = 0x20;
  }

  printf("= send query -t 0x%X -b %d -T 0x%X -B %d -m 0x%X \n",
    intf_->target_addr,
    intf_->target_channel,
    intf_->transit_addr,
    intf_->transit_channel,
    intf_->my_addr
  );*/
  
  int np=0;
  unsigned int netf=0, cmd=0;
  np=sscanf(_sensor.sensor.command.c_str(),"%d %d %hhd %hhd %hhd %hhd %hhd %hhd %hhd %hhd %hhd %hhd %hhd %hhd %hhd %hhd",
    &netf, &cmd, &msg_data[0], &msg_data[1], &msg_data[2], &msg_data[3], &msg_data[4], &msg_data[5],
    &msg_data[6], &msg_data[7], &msg_data[8], &msg_data[9], &msg_data[10], &msg_data[11], &msg_data[12], &msg_data[13]);
  
  printf("<%d> NETF $%X CMD $%X DATA $%X $%X $%X $%X $%X $%X $%X $%X $%X $%X $%X $%X $%X $%X\n", np,
    netf, cmd, msg_data[0], msg_data[1], msg_data[2], msg_data[3], msg_data[4], msg_data[5],
    msg_data[6], msg_data[7], msg_data[8], msg_data[9], msg_data[10], msg_data[11], msg_data[12], msg_data[13]);


  if(np<2){
    printf("sensor command parameters not valid!\n");
    return query_result_t();
  }
  np-=2;
  memset(&req, 0, sizeof(req));
  req.msg.netfn = netf;
  req.msg.cmd = cmd;
  req.msg.data = msg_data;
  req.msg.data_len = np+_sensor.sensor.param;
  if(_sensor.sensor.param>0){
    printf("(((extra param %d %d: ",(int)_sensor.sensor.param,_sensor.result.len);
      for(int i=0; i< std::min((int)_sensor.sensor.param,_sensor.result.len); i++){
      printf(" $%X ", _sensor.result.data[i]);
      msg_data[np+i]=_sensor.result.data[i];
    }
    printf(")))\n");
  }

  const ::ipmi_rs* const rsp = intf_->sendrecv(intf_, &req);
  intf_->target_addr = local_addr_;
  intf_->target_channel=0;
  if (!rsp){
    printf("return (no rsp)\n");
    return query_result_t();
  }
  printf("return code $%X\n",rsp->ccode);
  if (rsp->ccode){
    return query_result_t();
  }

  printf("--- Values $%02X: ",rsp->ccode);
  for(int i=0; i< rsp->data_len; i++){
    printf(" $%02X",rsp->data[i]);
  }
  printf("\n-----------");

  query_result_t r;
  r.retval=rsp->ccode;
  r.len=std::min((int) sizeof(query_result_t::data), rsp->data_len);
  for(int i=0; i< r.len; i++) r.data[i]=rsp->data[i];
  r.valid=true;

  intf_->target_addr = _sensor.sensor.ipmb; // wozu?
  return r;
}


bool Device::connect(const std::string& _hostname, const std::string& _username,
                     const std::string& _password, int _privlevel) {
  SuS_LOG_STREAM(config, log_id(), "Connecting to '" << _hostname << "'.");
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
  SuS_LOG_STREAM(info, log_id(), "Connected to '" << _hostname << "'.");

  local_addr_ = intf_->target_addr;

  SuS_LOG(finer, log_id(), "Starting ReaderThread.");
  readerThread_ = new ReaderThread(this);
  readerThread_->start();

  return true;
} // Device::connect


// initInputRecord is same...
Device::any_sensor_ptr Device::initOutputRecord(::dbCommon* _rec, const ::link& _out) {
  sensorcmd_id_t id(_out);
  sensor_list_t::iterator i = sensors_.find(id);
  if (i == sensors_.end()) {
    SuS_LOG_PRINTF(warning, log_id(), "sensor %s not found.", id.prettyPrint().c_str());
    //return any_sensor_ptr();
    sensors_[id] = any_sensor_ptr();
    i = sensors_.find(id);
    if (i == sensors_.end()) return any_sensor_ptr(); 
  } // if
  // id doesn't know its sensor number, but the cached version should.

  dpvt_t* priv = new dpvt_t;
  callbackSetPriority(priorityLow, &priv->cb);
  callback_private_t* cb_priv = new callback_private_t(
    reinterpret_cast< ::dbCommon*>(_rec), i->first, readerThread_, &i->second.good);
  callbackSetUser(cb_priv, &priv->cb);
  priv->cb.timer = NULL;
  _rec->dpvt = priv;

  return i->second;
}


void Device::initMbboDirectRecord(::mbboDirectRecord* _pmbbo) {
  auto i = initOutputRecord(reinterpret_cast< ::dbCommon*>(_pmbbo), _pmbbo->out);
  callbackSetCallback(mbboDirectCallback, &static_cast<dpvt_t*>(_pmbbo->dpvt)->cb);
} // Device::initMbboDirectRecord


void Device::initMbbiDirectRecord(::mbbiDirectRecord* _pmbbi) {
  auto i = initOutputRecord(reinterpret_cast< ::dbCommon*>(_pmbbi), _pmbbi->inp);
  callbackSetCallback(mbbiDirectCallback, &static_cast<dpvt_t*>(_pmbbi->dpvt)->cb);
} // Device::initMbbiDirectRecord


void Device::initBoRecord(::boRecord* _pmbbo) {
//   printf("initBoRecord\n");
  auto i = initOutputRecord(reinterpret_cast< ::dbCommon*>(_pmbbo), _pmbbo->out);
//   printf("initBoRecord set callb\n");
  callbackSetCallback(boCallback, &static_cast<dpvt_t*>(_pmbbo->dpvt)->cb);
//   printf("initBoRecord done\n");
} // Device::initBoRecord


bool Device::ping() {
  mutex_.lock();
  bool ret = intf_->keepalive(intf_) == 0;
  mutex_.unlock();
  if (!ret) {
    SuS_LOG(warning, log_id(), "keepalive failed!");
  }
  return ret;
} // Device::ping


bool Device::writeBoSensor(::boRecord* _pmbbo) {
//   SuS_LOG_STREAM(info, log_id(), "writeBoSensor\n");
  if (!_pmbbo->dpvt) {
//     SuS_LOG_STREAM(info, log_id(), "!_pmbbo->dpvt\n");
    // when dpvt is not set, init failed
    _pmbbo->udf = 1;
    return false;
  } // if
  if (!_pmbbo->pact) {
//     SuS_LOG_STREAM(info, log_id(), "!_pmbbo->pact\n");
    _pmbbo->pact = TRUE;
    readerThread_->enqueueSensorRead(query_job_t(_pmbbo->out, &Device::boQuery));
    dpvt_t* priv = static_cast<dpvt_t*>(_pmbbo->dpvt);
    ::callbackRequestDelayed(&priv->cb, 2.0);
    return true;
    // start async. operation
  } else {
//     SuS_LOG_STREAM(info, log_id(), "_pmbbo->pact\n");
    _pmbbo->pact = FALSE;
    return true;
  } // else
} // Device::writeBoSensor


bool Device::writeMbboDirectSensor(::mbboDirectRecord* _pmbbo) {
  if (!_pmbbo->dpvt) {
    // when dpvt is not set, init failed
    _pmbbo->udf = 1;
    return false;
  } // if
  if (!_pmbbo->pact) {
    _pmbbo->pact = TRUE;
    query_job_t q(_pmbbo->out, &Device::mbboDirectQuery);
    {
      unsigned int v=_pmbbo->val;
      printf("<<write : $%X $%X $%X $%X $%X>>\n",(unsigned int)_pmbbo->val,_pmbbo->rval,_pmbbo->oraw,_pmbbo->rbv,_pmbbo->orbv);

      for(int i=0; i<q.sensor.param; i++){
        q.result.data[i]=v & 0xFF;
        v>>=8;
      }
      q.result.len=q.sensor.param;
    }
    readerThread_->enqueueSensorRead(q);
    dpvt_t* priv = static_cast<dpvt_t*>(_pmbbo->dpvt);
    ::callbackRequestDelayed(&priv->cb, 2.0);
    return true;
    // start async. operation
  } else {
    _pmbbo->pact = FALSE;
    return true;
  } // else
} // Device::writeMbboDirectSensor


bool Device::readMbbiDirectSensor(::mbbiDirectRecord* _pmbbi) {
  if (!_pmbbi->dpvt) {
    // when dpvt is not set, init failed
    _pmbbi->udf = 1;
    return false;
  } // if
  if (!_pmbbi->pact) {
    _pmbbi->pact = TRUE;
    readerThread_->enqueueSensorRead(query_job_t(_pmbbi->inp, &Device::mbbiDirectQuery));
    dpvt_t* priv = static_cast<dpvt_t*>(_pmbbi->dpvt);
    ::callbackRequestDelayed(&priv->cb, 2.0);
    return true;
    // start async. operation
  } else {
    _pmbbi->pact = FALSE;
    return true;
  } // else
} // Device::readMbbiDirectSensor


Device::query_job_t::query_job_t(const ::link& _loc, query_func_t _f,query_result_t r)
  : sensor(_loc), query_func(_f), result( r) {
} // Device::query_job_t constructor

Device::query_job_t::query_job_t(const ::link& _loc, query_func_t _f)
  : sensor(_loc), query_func(_f), result() {
} // Device::query_job_t constructor

} // namespace IPMIIOC
