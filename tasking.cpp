#include "tasking.h"

#include <stdlib.h>
#include <string.h>
#include <array.h>
#include <kassert.h>
#include "interrupts.h"
#include "paging.h"

#include "debug.h"

using os::Tasking::Thread;
static int maxid = 0;

struct Task {
  int id;
  Thread* thread;
  os::Interrupts::Registers state;
  os::Paging::PageDirectory* directory;
};

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

void ret() {
  int a = 4;
  a++;
}

void endTask() {
  delete current_task;
  current_task = nullptr;
}

void os::Tasking::Thread::start() {
  m_state = State::Ready;
  auto context = os::Paging::makeThread();
  Task* task = new Task();
  task->state = {0};
  task->thread = this;
  task->state.useresp = context.second;
  task->state.pusha_registers.esp = context.second;
  task->state.pusha_registers.ebp = 0;
  task->state.eip = ((uintptr_t)m_func);
  task->directory = context.first;
  task->id = maxid++;

  queue.enqueue(task);
}

void os::Tasking::switchTasks(os::Interrupts::Registers* regs) {
  if(current_task != nullptr) {
    current_task->state = *regs;
    queue.enqueue(current_task);
  }

  Task* next_task = queue.dequeue();
  if(next_task == nullptr)
    return;

  regs->useresp = next_task->state.useresp;
  regs->pusha_registers = next_task->state.pusha_registers;
  regs->pusha_registers.esp = next_task->state.useresp;
  regs->eip = next_task->state.eip;
  current_task = next_task;
  os::Paging::switchDirectory(next_task->directory);
  asm volatile("xchgw %bx, %bx");
}