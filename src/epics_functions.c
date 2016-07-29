#include <epicsExport.h>
#include <iocsh.h>

#include "ipmi_connection.h"

/* ipmiConnect */
static const iocshArg ipmiConnectArg0 = { "connection id", iocshArgInt };
static const iocshArg ipmiConnectArg1 = { "host name", iocshArgString };
static const iocshArg ipmiConnectArg2 = { "username", iocshArgString };
static const iocshArg ipmiConnectArg3 = { "password", iocshArgString };
static const iocshArg ipmiConnectArg4 = { "privlevel", iocshArgInt };
static const iocshArg* ipmiConnectArgs[ 5 ] = { &ipmiConnectArg0,
  &ipmiConnectArg1, &ipmiConnectArg2, &ipmiConnectArg3, &ipmiConnectArg4 };
static const iocshFuncDef ipmiConnectFuncDef = { "ipmiConnect",
    5 /* # arguments */, ipmiConnectArgs };

static void ipmiConnectCallFunc(const iocshArgBuf* _args) {
  // default to empty strings for username and password
  ipmiConnect(_args[0].ival, _args[1].sval,
              _args[2].sval ? _args[2].sval : "",
              _args[3].sval ? _args[3].sval : "",
              _args[4].ival);
} // ipmiConnectCallFunc



/* register the commands */
static void ipmiRegisterCommands(void) {
  static int firstTime = 1;
  if (firstTime) {
    firstTime = 0;
    iocshRegister(&ipmiConnectFuncDef, ipmiConnectCallFunc);
  } // if
} // ipmiRegisterCommands

epicsExportRegistrar(ipmiRegisterCommands);
