#pragma once

#include <stddef.h>
#include <stdint.h>

#include <vector.h>
#include <shared_ptr.h>
#include "paging.h"
#include "time.h"

namespace os {
  namespace Tasking {
    enum class TaskState {
      Running,
      Ready,
      Waiting,
      Stopped
    };

    enum class TaskPriority {
      Critical,
      RealTime,
      Normal,
      Background
    };

    class Task;

    class Waitable {
    public:
      Waitable() : m_wait_list(), m_ready(false) {}
      const std::vector<std::shared_ptr<Task>>& wait_list() const { return m_wait_list; }

      void wait();

    protected:
      void finish();

    private:
      std::vector<std::shared_ptr<Task>> m_wait_list;
      bool m_ready;
    };

    struct TaskInfo {
        os::Paging::ThreadData data;
        uint32_t esp;
    };

    class Task : public Waitable {
      friend Waitable;
    public:
      ~Task() {
        os::Paging::freeThread(m_info.data);
      }
      using function_type = void(void);

      inline TaskPriority static_priority() const { return m_spriority; }
      inline uint8_t dynamic_priority() const { return m_dpriority; }
      inline void decrease_dynamic_priority() { if(m_dpriority < 255) m_dpriority++; }
      inline void increase_dynamic_priority() { if(m_dpriority > 0) m_dpriority--; }
      inline TaskState state() const { return m_state; }
      inline void set_state(TaskState s) { m_state = s; }
      inline void set_timeslice_start(Time::TimeSpan start) { m_timeslice_start = start; }
      
      inline Time::TimeSpan timeslice_start() const { return m_timeslice_start; }

      static std::shared_ptr<Task> start(function_type* func, TaskPriority priority = TaskPriority::Normal);
      static std::shared_ptr<Task> create();

      void suspend();
      void resume();
      void end();

    private:
      Task() : m_timeslice_start(0), m_just_started(true) {}
      TaskPriority m_spriority;
      uint8_t m_dpriority;
      TaskState m_state;
      uint32_t m_id;
      TaskInfo m_info;
      Time::TimeSpan m_timeslice_start;
      bool m_just_started;
    };

    void init();

    void lock_scheduler();
    void unlock_scheduler();
    void lock_stuff();
    void unlock_stuff();

    // Scheduler needs to be locked before calling this procedure
    void schedule();

    void timer_tick(os::Time::TimeSpan time);
  }
}