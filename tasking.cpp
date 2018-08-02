#include "tasking.h"

#include <stdlib.h>
#include <array.h>
#include <kassert.h>
#include "interrupts.h"
#include "paging.h"

using os::Tasking::Thread;

struct Task {
  Thread* thread;
  os::Interrupts::Registers state;
  os::Paging::PageDirectory* directory;
};

class TaskQueue {
public:
  TaskQueue()
      : m_first(nullptr)
      , m_last(nullptr) {}

  void enqueue(Task* task) {
    node* n = new node();
    n->task = task;
    assert(n != nullptr);
    n->prev = m_last;
    n->next = nullptr;

    if(m_first == nullptr) {
      m_first = n;
      m_last = n;
    } else {
      m_last->next = n;
    }
  }

  Task* dequeue() {
    if(m_first == nullptr) {
      return nullptr;
    }

    node* tmp = m_first;
    Task* task = tmp->task;
    m_first = m_first->next;
    delete tmp;

    return task;
  }

  bool empty() const {
    return m_first == nullptr;
  }

private:
  struct node {
    Task* task;
    node* prev;
    node* next;
  };

  node* m_first;
  node* m_last;
};

static TaskQueue queue;
static Task* current_task = nullptr;

void ret() {
  int a = 4;
  a++;
}

void os::Tasking::Thread::start() {
  m_state = State::Ready;
  auto context = os::Paging::makeThread();
  Task* task = new Task();
  task->state = {0};
  task->thread = this;
  task->state.useresp = context.second;
  task->state.ebp = context.second;
  task->state.eip = ((uintptr_t)&m_func);
  task->directory = context.first;

  queue.enqueue(task);
}

extern "C" uint32_t switch_context(os::Interrupts::Registers* regs, os::Paging::PageDirectory* dir);

void os::Tasking::switchTasks(os::Interrupts::Registers* regs) {
  if(current_task != nullptr) {
    current_task->state = *regs;
    queue.enqueue(current_task);
  }

  Task* next_task = queue.dequeue();
  if(next_task == nullptr) return;

  regs->useresp = next_task->state.useresp;
  regs->ebp = next_task->state.ebp;
  regs->eip = next_task->state.eip;
  current_task = next_task;
  os::Paging::switchDirectory(next_task->directory);
}