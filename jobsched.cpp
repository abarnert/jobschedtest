#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

using namespace std;
using namespace std::chrono;

class JobScheduler {
  thread sched_thread;
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
  void schedule(std::function<void()> job, uint32_t milliseconds) {
    cout << "pushing job\n";
    // pq.push job
    // cv.notify
  }

  /**
   * Will return after all the jobs have been executed
   */
  void stop() {
    cout << "stopping sched\n";
    // pq.push poison
    // cv.notify
    sched_thread.join();
  }

 private:
  void run() {
    // while true
    //   time = pq.empty ? infinity : (pq.peek.time - now)
    //   cv.wait time
    for (int i=0; i!=1000000000; ++i) {
      if (i%100000000 == 0) {
	cout << i << "\n";
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
