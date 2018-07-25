#pragma once

namespace os {
  namespace std {

    template<typename T>
    struct plus {
      constexpr T operator()(const T &lhs, const T &rhs) const {
        return lhs + rhs;
      }
    };
    
    template<typename T>
    struct minus {
      constexpr T operator()(const T &lhs, const T &rhs) const {
        return lhs - rhs;
      }
    };
    
    template<typename T>
    struct multiplies {
      constexpr T operator()(const T &lhs, const T &rhs) const {
        return lhs * rhs;
      }
    };
    
    template<typename T>
    struct divides {
      constexpr T operator()(const T &lhs, const T &rhs) const {
        return lhs / rhs;
      }
    };
    
    template<typename T>
    struct modulus {
      constexpr T operator()(const T &lhs, const T &rhs) const {
        return lhs % rhs;
      }
    };
    
    template<typename T>
    struct negate {
      constexpr T operator()(const T &x) const {
        return -x;
      }
    };
    
    template<typename T>
    struct equal_to {
      constexpr bool operator()(const T &lhs, const T &rhs) const {
        return lhs == rhs;
      }
    };
    
    template<typename T>
    struct not_equal_to {
      constexpr bool operator()(const T &lhs, const T &rhs) const {
        return lhs != rhs;
      }
    };
    
    template<typename T>
    struct greater {
      constexpr bool operator()(const T &lhs, const T &rhs) const {
        return lhs > rhs;
      }
    };
    
    template<typename T>
    struct less {
      constexpr bool operator()(const T &lhs, const T &rhs) const {
        return lhs < rhs;
      }
    };
    
    template<typename T>
    struct greater_equal {
      constexpr bool operator()(const T &lhs, const T &rhs) const {
        return lhs >= rhs;
      }
    };
    
    template<typename T>
    struct less_equal {
      constexpr bool operator()(const T &lhs, const T &rhs) const {
        return lhs <= rhs;
      }
    };
    
    template<typename T>
    struct logical_and {
      constexpr bool operator()(const T &lhs, const T &rhs) const {
        return lhs && rhs;
      }
    };
    
    template<typename T>
    struct logical_or {
      constexpr bool operator()(const T &lhs, const T &rhs) const {
        return lhs || rhs;
      }
    };
    
    template<typename T>
    struct logical_not {
      constexpr bool operator()(const T &x) const {
        return !x;
      }
    };
    
    template<typename T>
    struct bit_and {
      constexpr T operator()(const T &lhs, const T &rhs) const {
        return lhs & rhs;
      }
    };
    
    template<typename T>
    struct bit_or {
      constexpr T operator()(const T &lhs, const T &rhs) const {
        return lhs | rhs;
      }
    };
    
    template<typename T>
    struct bit_not {
      constexpr T operator()(const T &x) const {
        return ~x;
      }
    };
    
    template<typename T>
    struct hash {
      size_t operator()(const T &x);
    };

  }
}