/* SPDX-License-Identifier: MIT */
#ifndef WATCHDOGTHREAD_H
#define	WATCHDOGTHREAD_H

namespace IPMIIOC {

class Device;

class WatchdogThread {
  public:
    WatchdogThread(Device* _d);
    virtual ~WatchdogThread();

  private:
    Device* device_;
}; // class WatchdogThread

} // namespace IPMIIOC

#endif	/* WATCHDOGTHREAD_H */

