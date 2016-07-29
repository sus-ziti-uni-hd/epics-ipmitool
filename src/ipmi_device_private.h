#pragma once

#include "ipmi_types.h"

struct dbCommon;

namespace IPMIIOC {
class ReaderThread;

struct callback_private_t {
  ::dbCommon* rec;
  sensorcmd_id_t sensor;
  ReaderThread* thread;
  bool * const good;

  callback_private_t(::dbCommon* _rec, const sensorcmd_id_t& _sensor,
                     ReaderThread* _thread, bool *_good);
}; // struct callback_private_t

}
