#include <boRecord.h>
#include <devSup.h>
#include <epicsExport.h>
#include <mbboDirectRecord.h>
#include <stdlib.h>

#include <stdio.h>

#include "ipmi_connection.h"

static long init(int after) {
   return 0;
}

static long write_bo_record(boRecord* _pbo) {
   ipmiWriteBoSensor(_pbo);
   return 0;
}

static long init_bo_record(boRecord* _pbo) {
  ipmiInitBoRecord(_pbo);
  return 0;
}

static long write_mbboDirect_record(mbboDirectRecord* _pmbboDirect) {
   ipmiWriteMbboDirectSensor(_pmbboDirect);
   return 0;
}

static long init_mbboDirect_record(mbboDirectRecord* _pmbboDirect) {
  ipmiInitMbboDirectRecord(_pmbboDirect);
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
   DEVSUPFUN       write_bo;
   DEVSUPFUN       special_linconv;
} devIpmitoolBo = {
   6, // number
   NULL, // report
   init, // init
   init_bo_record, // init_record
   NULL, // get_ioint_info
   write_bo_record, // read_ai
   NULL  // special_linconv
};
epicsExportAddress(dset,devIpmitoolBo);

struct {
   long            number;
   DEVSUPFUN       report;
   DEVSUPFUN       init;
   DEVSUPFUN       init_record;
   DEVSUPFUN       get_ioint_info;
   DEVSUPFUN       write_mbbo;
} devIpmitoolMbboDirect = {
   5, // number
   NULL, // dev_report
   init, // init
   init_mbboDirect_record, // init_record
   NULL, // get_ioint_info
   write_mbboDirect_record // write_mbbo
};
epicsExportAddress(dset,devIpmitoolMbboDirect);


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
   read_mbbiDirect_record // read
};
epicsExportAddress(dset,devIpmitoolMbbiDirect);

