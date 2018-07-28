#pragma once

namespace os {
  namespace std {
    template<typename T1, typename T2>
    struct pair {
      using first_type = T1;
      using second_type = T2;

      T1 first;
      T2 second;
    };

    template<typename T1, typename T2>
    constexpr bool operator==(const pair<T1,T2>& lhs, const pair<T1,T2>& rhs) {
      return lhs.first == rhs.first && lhs.second == rhs.second;
    }

    template<typename T1, typename T2>
    constexpr bool operator!=(const pair<T1,T2>& lhs, const pair<T1,T2>& rhs) {
      return !(lhs == rhs);
    }

    template<typename T1, typename T2>
    constexpr bool operator<(const pair<T1,T2>& lhs, const pair<T1,T2>& rhs) {
      if(lhs.first < rhs.first) return true;
      if(lhs.first > rhs.first) return false;
      return lhs.second < rhs.second;
    }

    template<typename T1, typename T2>
    constexpr bool operator<=(const pair<T1,T2>& lhs, const pair<T1,T2>& rhs) {
      return !(rhs < lhs);
    }

    template<typename T1, typename T2>
    constexpr bool operator>(const pair<T1,T2>& lhs, const pair<T1,T2>& rhs) {
      return rhs < lhs;
    }

    template<typename T1, typename T2>
    constexpr bool operator>=(const pair<T1,T2>& lhs, const pair<T1,T2>& rhs) {
      return rhs <= lhs;
    }

    template< class T1, class T2 >
    constexpr pair<T1,T2> make_pair(T1 t, T2 u) {
      return { t, u };
    }
  }
}