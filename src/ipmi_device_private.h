/* SPDX-License-Identifier: MIT */
#pragma once

#include "ipmi_types.h"

struct dbCommon;

namespace IPMIIOC {
class ReaderThread;

struct callback_private_t {
  ::dbCommon* rec;
  sensor_id_t sensor;
  ReaderThread* thread;
  bool * const good;

  callback_private_t(::dbCommon* _rec, const sensor_id_t& _sensor,
                     ReaderThread* _thread, bool *_good);
}; // struct callback_private_t

}
