EPICS IPMItool IOC
==================

Introduction
------------
This EPICS IOC can be used to communicate with IPMI-capable devices. The
IPMItool library is used to implement the IPMI protocol.

Building
--------
The IPMItool build system does not install the libraries required by this
program. Therefore, a built source tree of IPMItool has to be available.
Point the IPMITOOL entry in configure/RELEASE to the root of this tree.

Another dependency is the EPICS Logfile library available at
https://github.com/sus-ziti-uni-hd/epics-Logfile.

Usage
-----
First, a connection to the IPMI device has to be established. The
`ipmiConnect` command is used to establish a connection and assign a link id to
it.

    ipmiConnect(id, host, user, password, protocol, privLevel)

- The _id_ is used to reference this connection from the following commands and
  EPICS records.
- The _protocol_ can be any protocol supported by the IPMItool library. Most
  often, you will use `"lan"` here, which is also the default value.
- The optional _privLevel_ parameter can be used to request a specific
  priviledge level from callback (1) to OEM (5).

After the connection has been establised, the device can be scanned for all
available sensors.

    ipmiScanDevice(id)

- The only parameter is the connection id for the device to be scanned.

The scan result can be dumped to an EPICS database with

    ipmiDumpDatabase(id, file)

- _id_ refers to the connection to a previously scanned device.
- _file_ is the name of the output file. The output format is an EPICS database.
  Note that manual preprocessing of the database is required. As a minimum, SCAN
  fields have to be added as required.

In subsequent runs, the scan and dump steps can be omitted.

License
-------
This code is released under the MIT License, see LICENSE.md.
