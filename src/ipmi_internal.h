/* SPDX-License-Identifier: MIT */
#ifndef IPMI_INTERNAL_H_
#define IPMI_INTERNAL_H_

#include <callback.h>
#include <map>

namespace IPMIIOC {

class Device;

typedef std::map<int, Device*> intf_map_t;
/** Mapping from link no. to the actual interface.
 *
 *  Do not store a local copy of the returned pointer so that all records
 *  on the same link automatically profit when the connection is
 *  re-established after a link error. This might change the pointer!
 */
extern intf_map_t s_interfaces;

/** Private record data stored in dpvt. */
struct dpvt_t {
  CALLBACK cb;
  unsigned pvid;
}; // struct dpvt_t

} // namespace IPMIIOC

#endif
