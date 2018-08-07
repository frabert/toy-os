#pragma once

#include <stddef.h>
#include <stdint.h>

namespace os {
  class Tasking {
  public:
    class Waitable {
    public:
      enum class State {
        Ready,
        Processing,
        Failed
      };

      void wait();
      virtual State state() const = 0;

      static Waitable* wait_all(Waitable** list, size_t n);
      static Waitable* wait_one(Waitable** list, size_t n);
    };

    class Thread : public Waitable {
      friend Tasking;
    public:
      using function_type = void(void);

      static Thread* start(function_type* func);

      State state() const;

    protected:
      State m_state;
      function_type* m_func;
    private:
      Thread(function_type* func);
    };

    static void init();
    static void switchTasks();

  private:
    static void task_end();
    static void suspend_task();
  };
}