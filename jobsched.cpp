#include <chrono>
#include <condition_variable>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

//#define DEBUG_TIMER 1

using namespace std;
using namespace std::chrono;

#ifdef DEBUG_TIMER
mutex _log_mtx;
#define LOG_TIMER(expr) \
  do { \
    unique_lock<mutex> _log_lock(_log_mtx); \
    cerr << steady_clock::now() << ": " << expr << endl; \
  } while (false)
#else
#define LOG_TIMER(expr)
#endif

template <typename OS, typename Clock>
OS &operator<<(OS &os, time_point<Clock> tp) {
  stringstream ss;
  auto ms = time_point_cast<milliseconds>(tp).time_since_epoch().count();
  auto hr = ms / 3600000; ms %= 3600000;
  auto mn = ms / 60000; ms %= 60000;
  auto sc = ms / 1000; ms %= 1000;
  ss << setfill('0') << setw(2)
     << hr << ':' << mn << ':' << sc << '.' << setw(3) << ms;
  return os << ss.str();
}

class JobScheduler {
  struct SJob {
    time_point<steady_clock> target;
    function<void()> job;
    SJob(time_point<steady_clock> t, function<void()> j) : target(t), job(j) {}
    bool operator<(SJob rhs) const { return target < rhs.target; }
    bool operator>(SJob rhs) const { return target > rhs.target; }
  };
  
  thread sched_thread;
  priority_queue<SJob, vector<SJob>, greater<SJob>> pq;
  bool done = false;
  mutex mtx;
  condition_variable cv;
 public:
  /**
   * Starts the executor. Returns immediately
   */
  void start() {
    LOG_TIMER("starting sched");
    sched_thread = thread([=]{run();});
  }

  /**
   * Executes/Schedules the job "job" after "milliseconds"
   * It will return immediately 
   */
  void schedule(std::function<void()> job, uint32_t ms) {
    auto target_time = steady_clock::now() + milliseconds(ms);
    LOG_TIMER("pushing job @ " << target_time << " (" << ms << "ms)");
    auto sjob = SJob(target_time, job);
    unique_lock<mutex> lock(mtx);
    pq.push(sjob);
    cv.notify_one();
  }

  /**
   * Will return after all the jobs have been executed
   */
  void stop() {
    LOG_TIMER("stopping sched");
    {
      unique_lock<mutex> lock(mtx);
      done = true;
      cv.notify_one();
    }
    sched_thread.join();
  }

 private:
  // Must run inside lock
  bool ready() const {
    if (pq.empty()) return done;
    if (pq.top().target <= steady_clock::now()) return true;
    return false;
  }
  
  void run() {
    while (true) {
      unique_lock<mutex> lock(mtx);
      LOG_TIMER("wait looping");
      while (!ready()) {
	if (pq.empty()) {
	  LOG_TIMER("waiting forever");
	  cv.wait(lock);
	} else {
	  LOG_TIMER("waiting until " << pq.top().target);
	  cv.wait_until(lock, pq.top().target);
	}
      }
      LOG_TIMER("woke");
      if (!pq.empty()) {
	auto sjob = pq.top();
	auto now = steady_clock::now();
	if (sjob.target <= now) {
	  sjob.job();
	  pq.pop();
	}
      }
      if (pq.empty() && done) {
	break;
      }
    }
  }
};

function<void()> make_job(int index,
			  time_point<steady_clock> start_time,
			  uint32_t wait) {
  auto sched_time = steady_clock::now();
  return [=] {
    auto exec_time = steady_clock::now();
    cout << "job # " << index
	 << ", Scheduled after ms: "
	 << duration_cast<milliseconds>(sched_time + milliseconds(wait) - start_time).count()
	 << ", Executed after ms: "
	 << duration_cast<milliseconds>(exec_time - start_time).count()
	 << "\n";
  };
}

int main(int argc, char *argv[]) {
  JobScheduler scheduler;
  scheduler.start();
  auto start_time = steady_clock::now();

  /* Create and schedule for execution at least 5 jobs at different times */
  /* Each job should print for proof of execution the following line: */
  /* “job # {#}, Scheduled after ms: {ms}, Executed after ms: {ms}” */  
  /* Example of output:
job # 4, Scheduled after ms: 2000, Executed after ms: 2000
job # 3, Scheduled after ms: 3000, Executed after ms: 3001
job # 2, Scheduled after ms: 4000, Executed after ms: 4001
job # 1, Scheduled after ms: 5000, Executed after ms: 5000
job # 0, Scheduled after ms: 6000, Executed after ms: 6001
  */

  if (argc < 2) {
    for (int i=0; i!=5; ++i) {
      auto wait = 6000 - i*1000;
      scheduler.schedule(make_job(i, start_time, wait), wait);
    }
  } else {
    for (int i=1; i!=argc; ++i) {
      try {
	auto wait = stol(argv[i]);
	if (wait < 0) {
	  cerr << argv[i] << ": negative times not supported; use 0\n";
	} else {
	  scheduler.schedule(make_job(i-1, start_time, wait), wait);
	}
      } catch (invalid_argument &x) {
	cerr << argv[i] << ": " << x.what() << "\n";
      }
    }
  }

  scheduler.stop();
}
