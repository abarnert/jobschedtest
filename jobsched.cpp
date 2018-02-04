#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using namespace std;
using namespace std::chrono;

class JobScheduler {
  struct SJob {
    time_point<steady_clock> target;
    function<void()> job;
    SJob(time_point<steady_clock> t, function<void()> j) : target(t), job(j) {}
    bool operator<(SJob rhs) const { return target < rhs.target; }
  };
  
  thread sched_thread;
  priority_queue<SJob> pq;
  bool done = false;
  mutex mtx;
  condition_variable cv;
 public:
  /**
   * Starts the executor. Returns immediately
   */
  void start() {
    cout << "starting sched\n";
    sched_thread = thread([=]{run();});
  }

  /**
   * Executes/Schedules the job "job" after "milliseconds"
   * It will return immediately 
   */
  void schedule(std::function<void()> job, uint32_t ms) {
    cout << "pushing job\n";
    auto target_time = steady_clock::now() + milliseconds(ms);
    auto sjob = SJob(target_time, job);
    unique_lock<mutex> lock(mtx);
    pq.push(sjob);
    cv.notify_one();
  }

  /**
   * Will return after all the jobs have been executed
   */
  void stop() {
    cout << "stopping sched\n";
    {
      unique_lock<mutex> lock(mtx);
      done = true;
      cv.notify_one();
    }
    sched_thread.join();
  }

 private:
  void run() {
    while (true) {
      unique_lock<mutex> lock(mtx);
      cout << "Waiting\n";
      while (pq.empty() && !done) cv.wait(lock);
      cout << "Woke\n";
      if (!pq.empty()) {
	pq.top().job();
	pq.pop();
      }
      if (pq.empty() && done) {
	break;
      }
    }
  }
};

function<void()> make_job(int index, time_point<steady_clock> start_time) {
  time_point<steady_clock> sched_time = steady_clock::now();
  return [=] {
    time_point<steady_clock> exec_time = steady_clock::now();
    // TODO: min width for times?
    cout << "job # " << index
	 << ", Scheduled after ms: "
	 << duration_cast<milliseconds>(sched_time - start_time).count()
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
    scheduler.schedule(make_job(i, start_time), 6000 - i*1000);
  }

  scheduler.stop();
}

