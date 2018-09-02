#pragma once

#include <vector.h>
#include <function_objects.h>

namespace os {
  namespace std {
    template<typename T,
            typename Container = vector<T>,
            typename Compare = std::less<typename Container::value_type>>
    class priority_queue {
    public:
      using container_type = Container;
      using compare_type = Compare;
      using value_type = typename Container::value_type;
      using size_type = typename Container::size_type;
      using reference = typename Container::reference;
      using const_reference = typename Container::const_reference;

      priority_queue() 
          : m_container() {}

      const_reference top() const {
        return m_container.front();
      }

      void push(const_reference v) {
        m_container.push_back(v);
        bubble_up(m_container.size());
      }

      void pop() {
        m_container[0] = m_container.back();
        m_container.pop_back();
        heapify(1);
      }

      size_t size() const {
        return m_container.size();
      }

      bool empty() const {
        return m_container.empty();
      }

    private:
      Container m_container;

      void heapify(size_t i) {
        size_t l = i * 2;
        size_t r = i * 2 + 1;
        size_t largest;
        if(l <= size() && Compare()(m_container[l - 1], m_container[i - 1])) {
          largest = l;
        } else {
          largest = i;
        }

        if(r <= size() && Compare()(m_container[r - 1], m_container[largest - 1])) {
          largest = r;
        }

        if(largest != i) {
          auto tmp = m_container[i - 1];
          m_container[i - 1] = m_container[largest - 1];
          m_container[largest - 1] = tmp;
          heapify(largest);
        }
      }

      void bubble_up(size_t i) {
        size_t parent = i / 2;
        while(i > 1 && Compare()(m_container[i - 1], m_container[parent - 1])) {
          auto tmp = m_container[i - 1];
          m_container[i - 1] = m_container[parent - 1];
          m_container[parent - 1] = tmp;
          i = parent;
          parent = i / 2;
        }
      }
    };
  }
}