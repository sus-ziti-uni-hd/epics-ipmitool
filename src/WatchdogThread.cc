/* SPDX-License-Identifier: MIT */
#include "WatchdogThread.h"

#include "ipmi_device.h"

namespace IPMIIOC {

WatchdogThread::WatchdogThread(Device* _d) : device_(_d) {
}

WatchdogThread::WatchdogThread(const WatchdogThread& orig) {
}

WatchdogThread::~WatchdogThread() {
}

} // namespace IPMIIOC
