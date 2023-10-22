#pragma once

#include <value_types.hpp>

namespace detail {
template <typename T> struct IncrementT {};

template <auto V> struct IncrementT<value_types::ValueTag<V>> {
  using Value = value_types::ValueTag<V + 1>;
};

template <typename A, typename B> struct Pair;

template <typename T> using Increment = IncrementT<T>::Value;

template <typename T> struct FibShiftT {};

template <auto A, auto B>
struct FibShiftT<Pair<value_types::ValueTag<A>, value_types::ValueTag<B>>> {
  using Value =
      Pair<value_types::ValueTag<B>, value_types::ValueTag<A + B>>;
};

template <typename T> using FibShift = FibShiftT<T>::Value;

template <typename T> struct FibExtractT {};

template <auto A, auto B>
struct FibExtractT<Pair<value_types::ValueTag<A>, value_types::ValueTag<B>>> {
  using Value = value_types::ValueTag<A>;
};

template <typename T> using FibExtract = FibExtractT<T>::Value;

template<typename T>
struct IsPrime {
  static constexpr bool Value = false;
};

template<auto A>
struct IsPrime<value_types::ValueTag<A>> {
  template<typename T>
  consteval static bool IsPrimeValue(T a) {
    for(T i = 2; i <= a / 2; i++) {
      if((a / i) * i == a) return false;
    }
    return true;
  }

  static constexpr bool Value = IsPrimeValue(A);
};

} // namespace detail

using Nats = type_lists::Iterate<detail::Increment, value_types::ValueTag<0>>;
using Fib = type_lists::Map<
    detail::FibExtract,
    type_lists::Iterate<
        detail::FibShift,
        detail::Pair<value_types::ValueTag<0>, value_types::ValueTag<1>>>>;
using Primes = type_lists::Filter<detail::IsPrime, type_lists::Iterate<detail::Increment, value_types::ValueTag<2>>>;
