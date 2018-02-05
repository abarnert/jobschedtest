jobsched: jobsched.cpp
	c++ -o jobsched -O2 -std=c++14 jobsched.cpp

jobsched_d: jobsched.cpp
	c++ -o jobsched_d -g -DDEBUG_TIMER -std=c++14 jobsched.cpp
