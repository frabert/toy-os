#pragma once

namespace os {
  namespace std {
    template<class InputIt, class T>
    InputIt find(InputIt first, InputIt last, const T& value) {
      for (; first != last; ++first) {
        if (*first == value) {
          return first;
        }
      }
      return last;
    }

    template<class InputIt, class UnaryPredicate>
    InputIt find_if(InputIt first, InputIt last, UnaryPredicate p) {
      for (; first != last; ++first) {
        if (p(*first)) {
          return first;
        }
      }
      return last;
    }

    template<class InputIt, class UnaryPredicate>
    InputIt find_if_not(InputIt first, InputIt last, UnaryPredicate q) {
      for (; first != last; ++first) {
        if (!q(*first)) {
          return first;
        }
      }
      return last;
    }

    template< class InputIt, class UnaryPredicate >
    bool all_of(InputIt first, InputIt last, UnaryPredicate p) {
      return std::find_if_not(first, last, p) == last;
    }

    template< class InputIt, class UnaryPredicate >
    bool any_of(InputIt first, InputIt last, UnaryPredicate p) {
      return std::find_if(first, last, p) != last;
    }

    template< class InputIt, class UnaryPredicate >
    bool none_of(InputIt first, InputIt last, UnaryPredicate p) {
      return std::find_if(first, last, p) == last;
    }

    template<class InputIt, class UnaryFunction>
    UnaryFunction for_each(InputIt first, InputIt last, UnaryFunction f) {
      for (; first != last; ++first) {
        f(*first);
      }
      return f;
    }

    template<class InputIt, class OutputIt>
    OutputIt copy(InputIt first, InputIt last, 
                  OutputIt d_first) {
      while (first != last) {
        *d_first++ = *first++;
      }
      return d_first;
    }

    template<class InputIt, class OutputIt, class UnaryPredicate>
    OutputIt copy_if(InputIt first, InputIt last, 
                     OutputIt d_first, UnaryPredicate pred) {
      while (first != last) {
        if (pred(*first))
          *d_first++ = *first;
        first++;
      }
      return d_first;
    }

    template<class InputIt, class OutputIt, class UnaryOperation>
    OutputIt transform(InputIt first1, InputIt last1, OutputIt d_first, 
                       UnaryOperation unary_op) {
      while (first1 != last1) {
        *d_first++ = unary_op(*first1++);
      }
      return d_first;
    }

    template<class InputIt1, class InputIt2, 
             class OutputIt, class BinaryOperation>
    OutputIt transform(InputIt1 first1, InputIt1 last1, InputIt2 first2, 
                       OutputIt d_first, BinaryOperation binary_op) {
      while (first1 != last1) {
        *d_first++ = binary_op(*first1++, *first2++);
      }
      return d_first;
    }
  }
}