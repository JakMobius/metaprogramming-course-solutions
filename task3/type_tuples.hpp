#pragma once

namespace type_tuples {

// MARK: Type Tuple
template <class... Ts> struct TTuple {};

template <class TT>
concept TypeTuple = requires(TT t) { []<class... Ts>(TTuple<Ts...>) {}(t); };

// MARK: Type List

template <class TL>
concept TypeSequence = requires {
  typename TL::Head;
  typename TL::Tail;
};

struct Nil {};

template <class TL>
concept Empty = std::derived_from<TL, Nil>;

template <class TL>
concept TypeList = Empty<TL> || TypeSequence<TL>;

} // namespace type_tuples
