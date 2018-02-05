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

#define DEBUG_TIMER 1

using namespace std;
using namespace std::chrono;

#ifdef DEBUG_TIMER
mutex _log_mtx;
#define LOG_TIMER(expr) do { unique_lock<mutex> _log_lock(_log_mtx); cerr << expr; } while (false)
#else
#define LOG_TIMER(expr)
#endif

string debug_time(time_point<steady_clock> tp) {
  stringstream ss;
  auto tpms = time_point_cast<milliseconds>(tp);
  auto ms = tpms.time_since_epoch();
  ss << fixed << setprecision(3) << (ms.count()/1000.0);
  return ss.str();
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
    LOG_TIMER("starting sched\n");
    sched_thread = thread([=]{run();});
  }

  /**
   * Executes/Schedules the job "job" after "milliseconds"
   * It will return immediately 
   */
  void schedule(std::function<void()> job, uint32_t ms) {
    auto target_time = steady_clock::now() + milliseconds(ms);
    LOG_TIMER("pushing " << ms
	      << " job at " << debug_time(steady_clock::now())
	      << " for " << debug_time(target_time) << endl);
    auto sjob = SJob(target_time, job);
    unique_lock<mutex> lock(mtx);
    pq.push(sjob);
    cv.notify_one();
  }

  /**
   * Will return after all the jobs have been executed
   */
  void stop() {
    LOG_TIMER("stopping sched\n");
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
      LOG_TIMER("Wait looping at " << debug_time(steady_clock::now()) << endl);
      while (!ready()) {
	if (pq.empty()) {
	  LOG_TIMER("Waiting forever at "
		    << debug_time(steady_clock::now()) << endl);
	  cv.wait(lock);
	} else {
	  LOG_TIMER("Waiting until "
		    << debug_time(pq.top().target)
		    << " at " << debug_time(steady_clock::now()) << endl);
	  cv.wait_until(lock, pq.top().target);
	}
      }
      LOG_TIMER("Woke at " << debug_time(steady_clock::now()) << endl);
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
    // TODO: min width for times?
    cout << "job # " << index
	 << ", Scheduled after ms: "
	 << duration_cast<milliseconds>(sched_time + milliseconds(wait) - start_time).count()
	 << ", Executed after ms: "
	 << duration_cast<milliseconds>(exec_time - start_time).count()
	 << "\n";
  };
}

int main() {
  JobScheduler scheduler;
  scheduler.start();
  time_point<steady_clock> start_time = steady_clock::now();

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

  for (int i=0; i!=5; ++i) {
    auto wait = 6000 - i*1000;
    scheduler.schedule(make_job(i, start_time, wait), wait);
  }

  scheduler.stop();
}

