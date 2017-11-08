/* SPDX-License-Identifier: MIT */
#ifndef IPMI_CONNECTION_H_
#define IPMI_CONNECTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <aiRecord.h>
#include <mbbiDirectRecord.h>
#include <mbbiRecord.h>

void ipmiConnect(int _id, const char* _hostname, const char* _username,
                 const char* _password, const char* _proto, int _privlevel);

void ipmiInitAiRecord(aiRecord* _rec);

void ipmiReadAiSensor(aiRecord* _rec);

void ipmiInitMbbiRecord(mbbiRecord* _rec);

void ipmiReadMbbiSensor(mbbiRecord* _rec);

void ipmiInitMbbiDirectRecord(mbbiDirectRecord* _rec);

void ipmiReadMbbiDirectSensor(mbbiDirectRecord* _rec);

void ipmiScanDevice(int _id);

void ipmiDumpDatabase(int _id, const char* _file);

/** Start first scan over active IPMBs. */
void ipmiInitialScan();

#ifdef __cplusplus
}
#endif

#endif

