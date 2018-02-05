# jobschedtest
https://docs.google.com/document/d/1qGtoeEY-APW2chpd55pM4XK2GT062vKjiSF-u0g2Z7U/edit

## Requirements

Should work on any modern *nix with a C++14 compiler. No libraries
outside the C++ stdlib are needed.

However, it's only been tested on OS X 10.13 with clang 9.0.0
installed via Xcode 9.1. I did try to avoid language and library
features that g++ added late or had problems with at some point (like
`put_time`), but no promises.

## Use

    $ make
	c++ -o jobsched -O2 -std=c++14 jobsched.cpp
	$ ./jobsched
	job # 4, Scheduled after ms: 2000, Executed after ms: 2004
	job # 3, Scheduled after ms: 3000, Executed after ms: 3004
	job # 2, Scheduled after ms: 4000, Executed after ms: 4003
	job # 1, Scheduled after ms: 5000, Executed after ms: 5004
	job # 0, Scheduled after ms: 6000, Executed after ms: 6000
	$ ./jobsched 1000 100 spam -10 100
	spam: stol: no conversion
	-10: negative times not supported; use 0
	job # 1, Scheduled after ms: 100, Executed after ms: 103
	job # 4, Scheduled after ms: 100, Executed after ms: 103
	job # 0, Scheduled after ms: 1000, Executed after ms: 1000

If you want to see debug logging on the queue internals (and also
disable optimization and enable debugger info):

    $ make jobsched_d
	c++ -o jobsched_d -g -DDEBUG_TIMER -std=c++14 jobsched.cpp
	$ ./jobsched_d 10 100 >/dev/null
	248:17:0.739: starting sched
	248:17:0.740: pushing job @ 248:17:0.750 (10ms)
	248:17:0.740: wait looping
    248:17:0.740: waiting forever
    248:17:0.740: pushing job @ 248:17:0.840 (100ms)
    248:17:0.740: waiting until 248:17:0.750
    248:17:0.740: stopping sched
    248:17:0.740: waiting until 248:17:0.750
    248:17:0.740: waiting until 248:17:0.750
    248:17:0.751: woke
    248:17:0.751: wait looping
    248:17:0.751: waiting until 248:17:0.840
    248:17:0.844: woke

## Implementation

A thread scheduler is just a queue with a condition variable. For jobs
with times, you just need to replace the queue with a pqueue, and wait
until the first job is ready instead of just until there's a job.

## Notes

It's a bit weird that both the jobs and the scheduler have to be told
the wait time.

Also, `uint_t` is not an ideal choice for the API. Using `time_point`
and/or `duration` would make sure everyone's using the same clock,
catch any errors in dividing/multiplying by 1000 at compile time,
allow jobs scheduled in the past to have the same effect as right now
instead of wrapping around to 49 days in the future, etc.

