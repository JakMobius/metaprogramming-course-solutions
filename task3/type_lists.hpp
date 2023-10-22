#pragma once

#include <concepts>

#include <type_tuples.hpp>

namespace type_lists {

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

template <class TL1, class... TLRest> struct TypeListListT : std::false_type {};

template <class TL1, class... TLRest>
  requires TypeList<TL1>
struct TypeListListT<TL1, TLRest...> : TypeListListT<TLRest...> {};

template <class TL1>
  requires TypeList<TL1>
struct TypeListListT<TL1> : std::true_type {};

template <class TL1, class... TLRest>
struct TypeSequenceListT : std::false_type {};

template <class TL1, class... TLRest>
  requires TypeSequence<TL1>
struct TypeSequenceListT<TL1, TLRest...> : TypeSequenceListT<TLRest...> {};

template <class TL1>
  requires TypeSequence<TL1>
struct TypeSequenceListT<TL1> : std::true_type {};

template <class... TLL>
concept TypeListList = std::derived_from<TypeListListT<TLL...>, std::true_type>;
template <class... TLL>
concept TypeSequenceList =
    std::derived_from<TypeSequenceListT<TLL...>, std::true_type>;

// MARK: Cons

template <class T, class TL> struct Cons {
  using Head = T;
  using Tail = TL;
};

// MARK: FromTuple

namespace detail {
template <typename... Args> struct FromTupleT {
  using Value = Nil;
};

template <typename TupleHead, typename... TupleRest>
struct FromTupleT<type_tuples::TTuple<TupleHead, TupleRest...>> {
  using Value =
      Cons<TupleHead,
           typename FromTupleT<type_tuples::TTuple<TupleRest...>>::Value>;
};
} // namespace detail

template <typename... Args>
using FromTuple = detail::FromTupleT<Args...>::Value;

// MARK: ToTuple

namespace detail {
template <class TL, class... Converted>
  requires TypeList<TL>
struct ToTupleT {
  using Value = type_tuples::TTuple<Converted...>;
};

template <class TL, class... Converted>
  requires TypeSequence<TL>
struct ToTupleT<TL, Converted...> {
  using Value =
      ToTupleT<typename TL::Tail, Converted..., typename TL::Head>::Value;
};
} // namespace detail

template <typename T> using ToTuple = detail::ToTupleT<T>::Value;

// MARK: Repeat

template <class T> struct Repeat {
  using Head = T;
  using Tail = Repeat<T>;
};

// MARK: Take

template <int N, class TL> struct Take : Nil {};

template <int N, class TL>
  requires TypeSequence<TL> && (N > 0)
struct Take<N, TL> {
  using Head = TL::Head;
  using Tail = Take<N - 1, typename TL::Tail>;
};

// MARK: Drop

template <int N, class TL>
  requires TypeList<TL>
struct Drop : Nil {};

template <int N, class TL>
  requires TypeSequence<TL> &&
           (N > 0) && TypeSequence<Drop<N - 1, typename TL::Tail>>
struct Drop<N, TL> {
  using Head = Drop<N - 1, typename TL::Tail>::Head;
  using Tail = Drop<N - 1, typename TL::Tail>::Tail;
};

template <int N, class TL>
  requires TypeSequence<TL> && (N <= 0)
struct Drop<N, TL> {
  using Head = TL::Head;
  using Tail = Drop<0, typename TL::Tail>;
};

// MARK: Replicate

template <int N, class T> struct Replicate {
  using Head = T;
  using Tail = Replicate<N - 1, T>;
};

template <int N, class T>
  requires(N <= 0)
struct Replicate<N, T> : Nil {};

// MARK: Map

template <template <typename Arg> typename F, class TL>
  requires TypeList<TL>
struct Map {
  using Head = F<typename TL::Head>;
  using Tail = Map<F, typename TL::Tail>;
};

template <template <typename Arg> typename F, class TL>
  requires Empty<TL>
struct Map<F, TL> : Nil {};

// MARK: Filter

template <template <typename Arg> typename F, class TL>
  requires TypeList<TL>
struct Filter : Nil {};

template <template <typename Arg> typename F, class TL>
  requires TypeSequence<TL> && (!F<typename TL::Head>::Value) &&
           TypeSequence<Filter<F, typename TL::Tail>>
struct Filter<F, TL> {
  using Head = Filter<F, typename TL::Tail>::Head;
  using Tail = Filter<F, typename TL::Tail>::Tail;
};

template <template <typename Arg> typename F, class TL>
  requires TypeSequence<TL> && F<typename TL::Head>::Value
struct Filter<F, TL> {
  using Head = TL::Head;
  using Tail = Filter<F, typename TL::Tail>;
};

// MARK: Iterate

template <template <typename Arg> typename F, class T> struct Iterate {
  using Head = T;
  using Tail = Iterate<F, F<T>>;
};

// MARK: Cycle

template <class TL, class ListHead = TL>
  requires TypeList<TL> && TypeList<ListHead>
struct Cycle : Nil {};

template <class TL, class ListHead>
  requires TypeSequence<TL> && TypeSequence<ListHead>
struct Cycle<TL, ListHead> {
  using Head = TL::Head;
  using Tail = Cycle<typename TL::Tail, ListHead>;
};

template <class TL, class ListHead>
  requires Empty<TL> && TypeSequence<ListHead>
struct Cycle<TL, ListHead> {
  using Head = typename ListHead::Head;
  using Tail = Cycle<typename ListHead::Tail, ListHead>;
};

// MARK: Inits

template <class TL, class End = TL, int N = 0>
  requires TypeList<TL>
struct Inits {
  using Head = Take<N, TL>;
  using Tail = Nil;
};

template <class TL, class End, int N>
  requires TypeList<TL> && TypeSequence<End>
struct Inits<TL, End, N> {
  using Head = Take<N, TL>;
  using Tail = Inits<TL, typename End::Tail, N + 1>;
};

// MARK: Tails

template <class TL>
  requires TypeList<TL>
struct Tails {
  using Head = TL;
  using Tail = Tails<typename TL::Tail>;
};

template <class TL>
  requires Empty<TL>
struct Tails<TL> {
  using Head = TL;
  using Tail = Nil;
};

// MARK: Scanl

template <template <typename Arg1, typename Arg2> typename OP, typename T,
          typename TL>
struct Scanl {
  using Head = T;
  using Tail = Scanl<OP, OP<T, typename TL::Head>, typename TL::Tail>;
};

template <template <typename Arg1, typename Arg2> typename OP, typename T,
          typename TL>
  requires Empty<TL>
struct Scanl<OP, T, TL> {
  using Head = T;
  using Tail = Nil;
};

// MARK: Foldl

namespace detail {
template <template <typename Arg1, typename Arg2> typename OP, typename T,
          typename TL>
  requires TypeList<TL>
struct FoldlT {
  using Value = FoldlT<OP, OP<T, typename TL::Head>, typename TL::Tail>::Value;
};

template <template <typename Arg1, typename Arg2> typename OP, typename T,
          typename TL>
  requires Empty<TL>
struct FoldlT<OP, T, TL> {
  using Value = T;
};
} // namespace detail

template <template <typename Arg1, typename Arg2> typename OP, typename T,
          typename TL>
using Foldl = detail::FoldlT<OP, T, TL>::Value;

// MARK: Zip2

template <typename TL1, typename TL2>
  requires TypeList<TL1> && TypeList<TL2>
struct Zip2 : Nil {};

template <typename TL1, typename TL2>
  requires TypeSequence<TL1> && TypeSequence<TL2>
struct Zip2<TL1, TL2> {
  using Head = type_tuples::TTuple<typename TL1::Head, typename TL2::Head>;
  using Tail = Zip2<typename TL1::Tail, typename TL2::Tail>;
};

// MARK: Zip

template <typename... TLL>
  requires TypeListList<TLL...>
struct Zip : Nil {};

namespace detail {
template <typename T>
  requires TypeList<T>
using GetHead = typename T::Head;

template <typename T>
  requires TypeList<T>
using GetTail = typename T::Tail;

template <template <typename Arg> typename F, typename... TLL>
  requires TypeListList<TLL...>
using MapTuple = ToTuple<Map<F, FromTuple<type_tuples::TTuple<TLL...>>>>;

template <typename... Args> struct TupleToZip {};

template <typename... TupleContents>
struct TupleToZip<type_tuples::TTuple<TupleContents...>> {
  using Value = Zip<TupleContents...>;
};
}; // namespace detail

template <typename... TLL>
  requires TypeListList<TLL...> && TypeSequenceList<TLL...>
struct Zip<TLL...> {
  using Head = detail::MapTuple<detail::GetHead, TLL...>;
  using Tail = typename detail::TupleToZip<detail::MapTuple<detail::GetTail, TLL...>>::Value;
};

} // namespace type_lists