#ifndef IPMI_CONNECTION_H_
#define IPMI_CONNECTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <aiRecord.h>

  void ipmiConnect(int _id, const char* _hostname, const char* _username,
                   const char* _password, int _privlevel);

  void ipmiInitAiRecord(aiRecord* _rec);

  void ipmiReadAiSensor(aiRecord* _rec);

  void ipmiScanDevice(int _id);

#ifdef __cplusplus
}
#endif

#endif

