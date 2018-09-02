#include "tasking.h"

#include <stdlib.h>
#include <string.h>
#include <array.h>
#include <kassert.h>
#include <priority_queue.h>
#include "interrupts.h"
#include "paging.h"

#include "debug.h"

using namespace os::Tasking;
static uint32_t maxid = 1;

static size_t sched_disable_counter = 0;
static size_t sched_postponed_counter = 0;
static bool sched_postponed = false;

void os::Tasking::lock_scheduler() {
  asm volatile("cli");
  sched_disable_counter++;
}

void os::Tasking::unlock_scheduler() {
  assert(sched_disable_counter > 0);
  sched_disable_counter--;
  if(sched_disable_counter == 0) {
    asm volatile("sti");
  }
}

void os::Tasking::lock_stuff() {
  asm volatile("cli");
  sched_disable_counter++;
  sched_postponed_counter++;
}

void os::Tasking::unlock_stuff() {
  assert(sched_postponed_counter > 0);
  assert(sched_disable_counter > 0);
  sched_postponed_counter--;
  if(sched_postponed_counter == 0) {
    if(sched_postponed) {
      sched_postponed = !sched_postponed;
      schedule();
    }
  }
  sched_disable_counter--;
  if(sched_disable_counter == 0) {
    asm volatile("sti");
  }
}

namespace os {
  namespace std {
    template<>
    struct less<Task*> {
      bool operator()(const Task* lhs, const Task* rhs) const {
        return lhs->dynamic_priority() < rhs->dynamic_priority();
      }
    };
  }
}

extern "C" void task_switch(TaskInfo* current, TaskInfo* next);

void task_switch_wrapper(TaskInfo* current, TaskInfo* next) {
  if(sched_postponed_counter == 0) {
    task_switch(current, next);
  } else {
    sched_postponed = true;
  }
}

using task_ref = os::std::shared_ptr<Task>;

static os::std::priority_queue<task_ref> critical_tasks;
static os::std::priority_queue<task_ref> realtime_tasks;
static os::std::priority_queue<task_ref> normal_tasks;
static os::std::priority_queue<task_ref> background_tasks;

static task_ref current_task = nullptr, next_task = nullptr;

static bool operator<(TaskPriority a, TaskPriority b) {
  switch(a) {
    case TaskPriority::Background: return b != TaskPriority::Background;
    case TaskPriority::Normal: return b != TaskPriority::Background && b != TaskPriority::Normal;
    case TaskPriority::RealTime: return b == TaskPriority::Critical;
    default: return false;
  }
}

static bool operator>(TaskPriority a, TaskPriority b) {
  return b < a;
}

static bool operator<=(TaskPriority a, TaskPriority b) {
  return !(a > b);
}

static bool operator>=(TaskPriority a, TaskPriority b) {
  return !(a < b);
}

static void idle_task() {
  while(true) { asm volatile("hlt"); }
}

static void end_task() {
  current_task->end();
}

static void enqueue_task(task_ref& t) {
  assert(t->state() == TaskState::Ready);
  switch(t->static_priority()) {
    case TaskPriority::Critical:
      critical_tasks.push(t); break;
    case TaskPriority::RealTime:
      realtime_tasks.push(t); break;
    case TaskPriority::Normal:
      normal_tasks.push(t); break;
    case TaskPriority::Background:
      background_tasks.push(t); break;
  }
}

task_ref Task::start(Task::function_type* func, TaskPriority priority) {
  auto ref = std::shared_ptr(new Task());
  ref->m_spriority = priority;
  ref->m_dpriority = 0;
  ref->m_state = Tasking::TaskState::Ready;
  ref->m_id = maxid++;

  auto context = os::Paging::makeThread();
  uint32_t* stack = (uint32_t*)context.physicalStackStart;
  stack--;
  *stack = (uintptr_t)&end_task;
  stack--;
  *stack = (uintptr_t)func;
  stack--;
  *stack = 0; // ebx
  stack--;
  *stack = 0; // esi
  stack--;
  *stack = 0; // edi
  stack--;
  *stack = 0; // ebp

  ref->m_info.data = context;
  ref->m_info.esp = context.virtualStackStart - 4 * 6;

  //lock_scheduler();
  enqueue_task(ref);
  //unlock_scheduler();

  return ref;
}

task_ref Task::create() {
  auto ref = os::std::shared_ptr(new Task());
  ref->m_spriority = TaskPriority::Normal;
  ref->m_dpriority = 0;
  ref->m_state = TaskState::Running;
  ref->m_id = 0;
  ref->m_info.data.directory = os::Paging::currentDirectory();

  return ref;
}

void os::Tasking::init() {
  // Create metadata for currently running thread
  current_task = Task::create();

  // Create an idle task with lowest priority that never waits
  Task::start(&idle_task, TaskPriority::Background);
}

void Waitable::wait() {
  // Tasks are only preempted when waiting would block, otherwise
  // they are kept running
  lock_scheduler();
  if(!m_ready) {
    current_task->m_state = TaskState::Waiting;
    current_task->increase_dynamic_priority();
    m_wait_list.push_back(current_task);
    os::Tasking::schedule();
  }
  unlock_scheduler();
}

static bool should_preempt(TaskPriority cur_s, uint8_t cur_d, TaskPriority next_s, uint8_t next_d) {
  switch(cur_s) {
    case TaskPriority::Critical:
    case TaskPriority::RealTime:
    case TaskPriority::Normal: return next_s > cur_s || (next_s == cur_s && next_d > cur_d);
    default: return next_s > cur_s || next_d > cur_d;
  }
}

void Waitable::finish() {
  lock_scheduler();
  m_ready = true;
  for(auto& task : m_wait_list) {
    assert(task->state() == TaskState::Waiting);
    task->set_state(TaskState::Ready);
    enqueue_task(task);
  }

  os::Tasking::schedule();
  unlock_scheduler();
}

void Task::end() {
  finish();
  m_state = TaskState::Stopped;
}

void Task::suspend() {
  lock_scheduler();
  m_state = TaskState::Waiting;
  schedule();
  unlock_scheduler();
}

void Task::resume() {
  lock_scheduler();
  if(next_task == nullptr) {
    auto cur_task = current_task;
    current_task = this;
    task_switch_wrapper(cur_task, this);
  } else {
    next_task = this;
  }
  unlock_scheduler();
}

static task_ref find_next_task() {
  os::std::priority_queue<task_ref>* queue = nullptr;
  if(!critical_tasks.empty()) {
    queue = &critical_tasks;
  } else if(!realtime_tasks.empty()) {
    queue = &realtime_tasks;
  } else if(!normal_tasks.empty()) {
    queue = &normal_tasks;
  } else if(!background_tasks.empty()) {
    queue = &background_tasks;
  }
  assert(queue != nullptr);
  auto ref = queue->top();
  assert(ref->state() == TaskState::Ready);
  queue->pop();
  return ref;
}

// Critical tasks preempt all tasks which have a lower static priority or dynamic priority, and are never suspended by time limits
// Realtime tasks preempt all tasks which have a lower static priority or dynamic priority
// Normal tasks preempt all tasks which have a lower static priority or dynamic priority
// Background tasks never preempt other tasks
void os::Tasking::schedule() {
  if(sched_postponed_counter != 0) {
    sched_postponed = true;
    return;
  }

  next_task = find_next_task();

  if(next_task != nullptr) {
    next_task->resume();
  }

  /*if(current_task->state() != TaskState::Running) {
    auto next_task = find_next_task();
    auto old_task = current_task;
    current_task = next_task;
    current_task->set_state(TaskState::Running);
    old_task->suspend(*current_task);
  } else {
    auto next = find_next_task();
    auto cs = current_task->static_priority();
    auto cd = current_task->dynamic_priority();
    auto ns = next->static_priority();
    auto nd = next->dynamic_priority();

    if(should_preempt(cs, cd, ns, nd)) {
      current_task->set_state(TaskState::Ready);
      auto old_task = current_task;
      current_task = next;
      current_task->set_state(TaskState::Running);
      current_task->set_timeslice_start(os::Time::since_boot());
      old_task->suspend(*current_task);
    } else {
      enqueue_task(next);
    }
  }*/
}

using namespace os::Time;

constexpr TimeSpan max_timeslice = 100_ms;

void os::Tasking::timer_tick(os::Time::TimeSpan time) {
  if(!current_task) return;
  lock_stuff();
  auto slice = time - current_task->timeslice_start();
  if(slice > max_timeslice && current_task->static_priority() != TaskPriority::Critical) {
    current_task->decrease_dynamic_priority();
    enqueue_task(current_task);
  }
  unlock_stuff();
}