#include "ipmi_device_private.h"

#include <dbCommon.h>

namespace IPMIIOC {
callback_private_t::callback_private_t( ::dbCommon* _rec,
        const sensor_id_t& _sensor, ReaderThread* _thread)
        : rec(_rec), sensor(_sensor), thread(_thread)
{
} // Device::callback_private_t constructor

}

