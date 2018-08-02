#pragma once

#include <stddef.h>
#include <stdint.h>
#include "interrupts.h"

namespace os {
  namespace Tasking {
    class Thread {
    public:
      enum class State {
        Stopped,
        Ready,
        Running,
        Waiting
      };

      using function_type = void(void);

      Thread(function_type* func)
          : m_func(func)
          , m_state(State::Stopped) { }

      void start();

    private:
      function_type* m_func;
      State m_state;
    };

    void switchTasks(os::Interrupts::Registers* regs);
  }
}