#pragma once

#include <stddef.h>

namespace os {
  namespace std {
    template<typename InputIt, typename UnaryFunction>
    UnaryFunction for_each(InputIt first, InputIt last, UnaryFunction f) {
      for (; first != last; ++first) {
        f(*first);
      }
      return f;
    }

    template<class InputIt, class UnaryPredicate>
    size_t count_if(InputIt first, InputIt last, UnaryPredicate p) {
      size_t ret = 0;
      for (; first != last; ++first) {
        if (p(*first)) {
          ret++;
        }
      }
      return ret;
    }
  }
}