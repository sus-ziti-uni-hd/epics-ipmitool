#include "ipmi_internal.h"

extern "C" {
#include "ipmitool/ipmi_intf.h"
}

namespace IPMIIOC {
intf_map_t s_interfaces;

active_ipmb_set_t s_active_ipmb;
}

