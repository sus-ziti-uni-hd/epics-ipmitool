#include <aiRecord.h>
#include <devSup.h>
#include <epicsExport.h>
#include <mbbiRecord.h>
#include <mbbiDirectRecord.h>
#include <stdlib.h>

#include <stdio.h>

static long init(int after) {
   return 0;
}

static long read_ai_record(aiRecord* _pai) {
   ipmiReadAiSensor(_pai);
   return 2;
}

static long init_ai_record(aiRecord* _pai) {
  ipmiInitAiRecord(_pai);
  return 0;
}

static long read_mbbi_record(mbbiRecord* _pmbbi) {
   ipmiReadMbbiSensor(_pmbbi);
   return 0;
}

static long init_mbbi_record(mbbiRecord* _pmbbi) {
  ipmiInitMbbiRecord(_pmbbi);
  return 0;
}

static long read_mbbiDirect_record(mbbiDirectRecord* _pmbbiDirect) {
   ipmiReadMbbiDirectSensor(_pmbbiDirect);
   return 0;
}

static long init_mbbiDirect_record(mbbiDirectRecord* _pmbbiDirect) {
  ipmiInitMbbiDirectRecord(_pmbbiDirect);
  return 0;
}


struct {
   long            number;
   DEVSUPFUN       report;
   DEVSUPFUN       init;
   DEVSUPFUN       init_record;
   DEVSUPFUN       get_ioint_info;
   DEVSUPFUN       read_ai;
   DEVSUPFUN       special_linconv;
} devIpmitoolAi = {
   6, // number
   NULL, // report
   init, // init
   init_ai_record, // init_record
   NULL, // get_ioint_info
   read_ai_record, // read_ai
   NULL  // special_linconv
};
epicsExportAddress(dset,devIpmitoolAi);

struct {
   long            number;
   DEVSUPFUN       dev_report;
   DEVSUPFUN       init;
   DEVSUPFUN       init_record;
   DEVSUPFUN       get_ioint_info;
   DEVSUPFUN       read_mbbi;
} devIpmitoolMbbi = {
   5, // number
   NULL, // dev_report
   init, // init
   init_mbbi_record, // init_record
   NULL, // get_ioint_info
   read_mbbi_record // read_mbbi
};
epicsExportAddress(dset,devIpmitoolMbbi);

struct {
   long            number;
   DEVSUPFUN       report;
   DEVSUPFUN       init;
   DEVSUPFUN       init_record;
   DEVSUPFUN       get_ioint_info;
   DEVSUPFUN       read_mbbi;
} devIpmitoolMbbiDirect = {
   5, // number
   NULL, // dev_report
   init, // init
   init_mbbiDirect_record, // init_record
   NULL, // get_ioint_info
   read_mbbiDirect_record // read_mbbi
};
epicsExportAddress(dset,devIpmitoolMbbiDirect);

