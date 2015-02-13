#include "ipmi_reader_thread.h"

#include "ipmi_device.h"


namespace IPMIIOC {

ReaderThread::ReaderThread(Device* _d) : thread_id_(0), do_terminate_(false),
  device_(_d) {
  ::pthread_mutex_init(&results_mutex_, NULL);
  ::pthread_mutex_init(&mutex_, NULL);
  ::pthread_cond_init(&cond_, NULL);
} // ReaderThread constructor


ReaderThread::~ReaderThread() {
  ::pthread_mutex_destroy(&results_mutex_);
  ::pthread_mutex_destroy(&mutex_);
  ::pthread_cond_destroy(&cond_);
} // ReaderThread destructor


void ReaderThread::do_query(const Device::sensor_id_t& _sensor) {
  Device::ai_result_t result;
  if (device_->aiQuery(_sensor, result)) {
    ::pthread_mutex_lock(&results_mutex_);
    results_[_sensor] = result;
    ::pthread_mutex_unlock(&results_mutex_);
  } // if
} // ReaderThread::do_query


void ReaderThread::enqueueSensorRead(const Device::sensor_id_t& _sensor) {
  ::pthread_mutex_lock(&mutex_);
  queries_.push_back(_sensor);
  ::pthread_cond_signal(&cond_);
  ::pthread_mutex_unlock(&mutex_);
} // ReaderThread::enqueue


Device::ai_result_t ReaderThread::findResult(const Device::sensor_id_t& _sensor) {
  ::pthread_mutex_lock(&results_mutex_);
  sensor_reading_map_t::iterator i = results_.find(_sensor);
  if (i == results_.end()) {
    ::pthread_mutex_unlock(&results_mutex_);
    Device::ai_result_t r;
    r.valid = false;
    return r;
  } // if
  Device::ai_result_t r = i->second;
  results_.erase(i);
  ::pthread_mutex_unlock(&results_mutex_);
  return r;
} // ReaderThread::findResult


void ReaderThread::start() {
  int r = ::pthread_create(&thread_id_, NULL, run, this);
} // ReaderThread::start


void* ReaderThread::run(void* _instance) {
  ReaderThread* t = static_cast<ReaderThread*>(_instance);
  return t->do_run();
} // ReaderThread::run


bool ReaderThread::join() {
  return ::pthread_join(thread_id_, NULL) == 0;
} // ReaderThread::join


bool ReaderThread::terminate() {
  ::pthread_mutex_lock(&mutex_);
  do_terminate_ = true;
  ::pthread_cond_signal(&cond_);
  ::pthread_mutex_unlock(&mutex_);

  return join();
} // ReaderThread::terminate


void* ReaderThread::do_run() {
  while (true) {
    // take the accumulated events and give the other thread a new queue
    // to fill.
    query_queue_t local;
    ::pthread_mutex_lock(&mutex_);
    queries_.swap(local); // swap is O(1) => mutex is not locked for long
    ::pthread_mutex_unlock(&mutex_);

    // now take our time to process the events
    for (const auto& i : local) {
      do_query(i);
    } // for i
    local.clear();

    // wait until there is work to do or termination is requested
    ::pthread_mutex_lock(&mutex_);
    if (do_terminate_) pthread_exit(NULL);
    if (!queries_.size()) {
      // queries_.size > 0: events have been put in the queue in the
      // meantime. their cond_signal events have been missed!
      ::pthread_cond_wait(&cond_, &mutex_);
    } // if
    ::pthread_mutex_unlock(&mutex_);
  } // while
} // ReaderThread::do_run

} // namespace IPMIIOC
