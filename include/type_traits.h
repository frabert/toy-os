#pragma once
namespace os {
  namespace std {
    template<typename T, T v>
    struct integral_constant {
      using value_type = T;
      using type = integral_constant<T, v>;

      static constexpr T value = v;

      constexpr operator value_type() const noexcept {
        return v;
      }

      constexpr operator()() const noexcept {
        return v;
      }
    };

    template<bool B>
    using bool_constant = integral_constant<bool, B>;

    using true_type = bool_constant<true>;
    using false_type = bool_constant<false>;

    template<bool B, typename T, typename F>
    struct conditional {
      using type = T;
    };
 
    template<typename T, typename F>
    struct conditional<false, T, F> {
      using type = F;
    };
  }
}
