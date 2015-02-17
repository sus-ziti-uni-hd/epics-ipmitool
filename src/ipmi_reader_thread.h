#ifndef IPMI_READER_THREAD_
#define IPMI_READER_THREAD_

#include "ipmi_device.h"
#include "ipmi_types.h"

#include <map>
#include <pthread.h>
#include <list>

namespace IPMIIOC {

class ReaderThread {
  public:
    ReaderThread(Device* _d);
    ~ReaderThread();

    void enqueueSensorRead(const Device::query_job_t& _sensor);
    result_t findResult(const sensor_id_t& _sensor);

    void start();

    bool join();

    bool terminate();

  private:
    typedef std::list<Device::query_job_t> query_queue_t;
    query_queue_t queries_;

    typedef std::map<sensor_id_t, result_t> sensor_reading_map_t;
    sensor_reading_map_t results_;
    ::pthread_mutex_t results_mutex_;

    static void* run(void* _instance);

    void* do_run();
    void do_query(const Device::query_job_t& _sensor);

    ::pthread_t thread_id_;

    ::pthread_mutex_t mutex_;
    ::pthread_cond_t cond_;

    bool do_terminate_;

    Device* device_;
}; // class ReaderThread

} // namespace IPMIIOC

#endif
