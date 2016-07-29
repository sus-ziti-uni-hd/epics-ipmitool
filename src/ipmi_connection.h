#ifndef IPMI_CONNECTION_H_
#define IPMI_CONNECTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <boRecord.h>
#include <mbboDirectRecord.h>
#include <mbbiDirectRecord.h>

void ipmiConnect(int _id, const char* _hostname, const char* _username,
                 const char* _password, int _privlevel);

void ipmiInitBoRecord(boRecord* _rec);
//
void ipmiWriteBoSensor(boRecord* _rec);

void ipmiInitMbboDirectRecord(mbboDirectRecord* _rec);

void ipmiWriteMbboDirectSensor(mbboDirectRecord* _rec);

void ipmiInitMbbiDirectRecord(mbbiDirectRecord* _rec);

void ipmiReadMbbiDirectSensor(mbbiDirectRecord* _rec);

#ifdef __cplusplus
}
#endif

#endif

