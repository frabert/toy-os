#include "tasking.h"

#include <stdlib.h>
#include <string.h>
#include <array.h>
#include <kassert.h>
#include "interrupts.h"
#include "paging.h"

#include "debug.h"

using os::Tasking;
static uint32_t maxid = 1;

struct Task {
  uint32_t id;
  Tasking::Thread* thread;
  os::Paging::ThreadData data;
  uint32_t esp;
  os::Tasking::Waitable* waiting;
};

extern "C" void task_switch(Task* current, Task* next);

class TaskQueue {
public:
  TaskQueue()
      : m_head(nullptr)
      , m_tail(nullptr)
      , m_num(0) {}

  void enqueue(Task* task) {
    node* new_elem = new node();
    assert(new_elem != nullptr);
    if(m_head == nullptr) {
      m_head = new_elem;
      m_tail = m_head;
      m_head->next = nullptr;
      m_head->task = task;
      m_num = 1;
    } else {
      new_elem->next = nullptr;
      new_elem->task = task;

      m_tail->next = new_elem;
      m_tail = new_elem;
      m_num++;
    }
  }

  Task* dequeue() {
    if(m_num == 0) return nullptr;
    auto task = m_head->task;

    if(m_tail == m_head) {
      delete m_head;
      m_head = nullptr;
      m_tail = nullptr;
      m_num = 0;
    } else {
      node* next = m_head->next;
      delete m_head;
      m_head = next;
      m_num--;
    }

    return task;
  }

  bool empty() const {
    return m_num == 0;
  }

private:
  struct node {
    Task* task;
    node* next;
  };

  node* m_head;
  node* m_tail;
  size_t m_num;
};

static TaskQueue queue;
static Task* current_task = nullptr;

void Tasking::task_end() {
  if(current_task->thread != nullptr) {
    current_task->thread->m_state = Tasking::Waitable::State::Ready;
  }
  Task* next_task = queue.dequeue();
  assert(next_task != nullptr);
  while(next_task->waiting != nullptr &&
      next_task->waiting->state() != Tasking::Waitable::State::Ready) {
    queue.enqueue(next_task);
    next_task = queue.dequeue();
    assert(next_task != nullptr);
  }
  Task* old_task = current_task;
  current_task = next_task;
  task_switch(old_task, current_task);
}

void Tasking::suspend_task() {
  Task* next_task = queue.dequeue();
  if(next_task == nullptr)
    return;
  while(next_task->waiting != nullptr &&
      next_task->waiting->state() != Tasking::Waitable::State::Ready) {
    queue.enqueue(next_task);
    next_task = queue.dequeue();
  }
  Task* old_task = current_task;
  current_task = next_task;
  queue.enqueue(old_task);
  task_switch(old_task, current_task);
}

Tasking::Waitable::State Tasking::Thread::state() const {
  return m_state;
}

Tasking::Thread* Tasking::Thread::start(Tasking::Thread::function_type* func) {
  return new Thread(func);
}

Tasking::Thread::Thread(os::Tasking::Thread::function_type* func) 
    : m_state(os::Tasking::Waitable::State::Processing)
    , m_func(func) {
  auto context = os::Paging::makeThread();
  uint32_t* stack = (uint32_t*)context.physicalStackStart;
  stack--;
  *stack = (uintptr_t)&Tasking::task_end;
  stack--;
  *stack = (uintptr_t)m_func;
  stack--;
  *stack = 0; // ebx
  stack--;
  *stack = 0; // esi
  stack--;
  *stack = 0; // edi
  stack--;
  *stack = 0; // ebp

  Task* task = new Task();
  task->thread = this;
  task->id = maxid++;
  task->data = context;
  task->esp = context.virtualStackStart - 4 * 6;
  task->waiting = nullptr;

  queue.enqueue(task);
}

void Tasking::init() {
  current_task = new Task();
  current_task->id = 0;
  current_task->data.directory = os::Paging::currentDirectory();
  current_task->thread = nullptr;
}

void Tasking::Waitable::wait() {
  current_task->waiting = this;
  suspend_task();
}

class WaitAll : public Tasking::Waitable {
public:
  WaitAll(Tasking::Waitable** list, size_t n)
    : m_list(list)
    , m_num(n) {}

  State state() const {
    State res = State::Ready;
    for(size_t i = 0; i < m_num; i++) {
      if(m_list[i]->state() == State::Failed) {
        return State::Failed;
      } else if(m_list[i]->state() == State::Processing) {
        res = State::Processing;
      }
    }
    return res;
  }

private:
  Tasking::Waitable** m_list;
  size_t m_num;
};

class WaitOne : public Tasking::Waitable {
public:
  WaitOne(Tasking::Waitable**list, size_t n)
    : m_list(list)
    , m_num(n) {}

  State state() const {
    State res = State::Failed;
    for(size_t i = 0; i < m_num; i++) {
      if(m_list[i]->state() == State::Ready) {
        return State::Ready;
      } else if(m_list[i]->state() == State::Processing) {
        res = State::Processing;
      }
    }
    return res;
  }

private:
  Tasking::Waitable** m_list;
  size_t m_num;
};

Tasking::Waitable* Tasking::Waitable::wait_all(Tasking::Waitable** list, size_t n) {
  return new WaitAll(list, n);
}

Tasking::Waitable* Tasking::Waitable::wait_one(Tasking::Waitable** list, size_t n) {
  return new WaitOne(list, n);
}

void Tasking::switchTasks() {
  // TODO: Preemptive task switch
}