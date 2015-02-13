#include <aiRecord.h>
#include <devSup.h>
#include <epicsExport.h>
#include <stdlib.h>

#include <stdio.h>
static long init(int after) {
   return 0;
}

static long read(aiRecord* _pao) {
   ipmiReadAiSensor(_pao);
   return 2;
}

static long init_ai_record(aiRecord* _pao) {
  ipmiInitAiRecord(_pao);
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
} devIpmitool = {
   6, // number
   NULL, // report
   init, // init
   init_ai_record, // init_record
   NULL, // get_ioint_info
   read, // read_ai
   NULL  // special_linconv
};
epicsExportAddress(dset,devIpmitool);

