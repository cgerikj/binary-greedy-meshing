#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <string>
#include <iostream>

class Timer {
private:
  bool started = false;
  bool ended = false;
  std::chrono::time_point<std::chrono::high_resolution_clock> t1;
  std::string description;
  
public:
  Timer(std::string description = "", bool start_ = false): description(description) {
    if (start_) {
      start();
    }
  };

  void start() {
    started = true;
    ended = false;
    t1 = std::chrono::high_resolution_clock::now();
  }

  int end() {
    if (!started || ended) return 0;
    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    auto duration_ms = duration_us / 1000.0f;
    if (description != "") {
      std::cout << description << " took " << duration_us << "us (" << duration_ms << "ms)" << std::endl;
    }
    ended = true;
    return duration_us;
  }

  ~Timer() {
    end();
  };
};

#endif