#pragma once

#include <array>
#include <cstddef>
#include <iostream>
#include <string_view>
#include <tuple>
#include <utility>

// MARK: Utils

template <class...> class Annotate {};
struct Dummy {};

template <typename T1> struct ArgSubstitute {
  template <template <class...> typename T> using Substitute = Dummy;
};

template <template <class...> typename T1, typename... Args>
struct ArgSubstitute<T1<Args...>> {
  template <template <class...> typename T> using Substitute = T<Args...>;
};

template <template <typename...> class T1, class T2> struct Matches {
  constexpr static bool Value = false;
};

template <template <typename...> class T1, class T2>
  requires(
      std::is_same_v<T2, typename ArgSubstitute<T2>::template Substitute<T1>>)
struct Matches<T1, T2> {
  constexpr static bool Value = true;
};

template <typename T> struct AnnotationsToTuple {};

template <class... Annotation>
struct AnnotationsToTuple<Annotate<Annotation...>> {
  using Value = std::tuple<Annotation...>;
};

// MARK: Getting types of fields using friend injection

template <class T, int N> struct Tag {
  friend auto loophole(Tag<T, N>);
};

template <class T, int N, class F> struct LoopholeSet {
  friend auto loophole(Tag<T, N>) { return F{}; };
};

template <class T, std::size_t I> struct LoopholeUbiq {
  template <class Type> constexpr operator Type() const noexcept {
    LoopholeSet<T, I, Type> unused{};
    return {};
  };
};

template <class T, int N> struct LoopholeGet {
  using Type = decltype(loophole(Tag<T, N>{}));
};

template <class T, size_t... Is>
constexpr auto asTupleImpl(std::index_sequence<Is...>) {
  constexpr T t{LoopholeUbiq<T, Is>{}...};
  return Annotate<typename LoopholeGet<T, Is>::Type...>();
}

// MARK: Getting count of fields in a structure

template <std::size_t I> struct UbiqConstructor {
  template <class Type> constexpr operator Type &() const noexcept;
};

template <class T, class... Args>
concept ConstructibleWithTypes = requires(Args... args) { T{args...}; };

template <typename T, typename IndexSequence> struct ConstructibleWithIndexSeq {
  static constexpr bool Value = false;
};

template <typename T, std::size_t... Indices>
  requires ConstructibleWithTypes<T, UbiqConstructor<Indices>...>
struct ConstructibleWithIndexSeq<T, std::index_sequence<Indices...>> {
  static constexpr bool Value = true;
};

template <typename T, std::size_t N>
using ConstructibleWithNArgs =
    ConstructibleWithIndexSeq<T, decltype(std::make_index_sequence<N>())>;

// MARK: Binary search

template <class T, std::size_t UpperBound, std::size_t LowerBound>
constexpr auto fieldIndicesBinSearch() {
  constexpr std::size_t Middle = (LowerBound + UpperBound) / 2;

  if constexpr (UpperBound == LowerBound) {
    return std::make_index_sequence<LowerBound>();
  } else {
    if constexpr (UpperBound == LowerBound + 1) {
      if constexpr (ConstructibleWithNArgs<T, UpperBound>::Value) {
        return std::make_index_sequence<UpperBound>();
      } else {
        return std::make_index_sequence<LowerBound>();
      }
    } else {
      if constexpr (ConstructibleWithNArgs<T, Middle>::Value) {
        return fieldIndicesBinSearch<T, UpperBound, Middle>();
      } else {
        return fieldIndicesBinSearch<T, Middle, LowerBound>();
      }
    }
  }
}

// MARK: Getting the upper bound for argument count

template <class T, std::size_t UpperBound = 1>
constexpr auto getFieldIndices() {
  // With known upper bound, proceed with binary search
  return fieldIndicesBinSearch<T, UpperBound, UpperBound / 2>();
}

template <class T, std::size_t UpperBound = 1>
  requires ConstructibleWithNArgs<T, UpperBound>::Value
constexpr auto getFieldIndices() {
  // Multiply argument count by two if constructible
  return getFieldIndices<T, UpperBound * 2>();
}

template <std::size_t N, typename... Types>
using NthType = typename std::tuple_element<N, std::tuple<Types...>>::type;

template <typename T> struct AnnotationChecker {
  static constexpr bool Value = false;
};

template <typename... Annotations>
struct AnnotationChecker<Annotate<Annotations...>> {
  static constexpr bool Value = true;
};

// Helper struct to filter index sequence based on a predicate
template <typename... ArgumentTypes>
constexpr auto getPureArgumentCount(Annotate<ArgumentTypes...>) {
  size_t result = 0;

  std::array<size_t, sizeof...(ArgumentTypes)> array = {
      (AnnotationChecker<ArgumentTypes>::Value ? 0 : 1)...};
  for (auto i : array)
    result += i;

  return result;
}

template <typename Accumulated, typename... Rest> struct AccumulateAnnotations {
  using Annotations = Accumulated;
};

template <typename... Accumulated, typename... Current>
struct AccumulateAnnotations<Annotate<Accumulated...>, Annotate<Current...>> {
  using Annotations = Annotate<Accumulated..., Current...>;
};

template <typename Tuple, std::size_t Index, std::size_t Current,
          std::size_t CurrentArg, typename AnnotationsAcc>
struct FieldExtractor {
  using Type = void;
  using Annotations = AnnotationsAcc;
};

// MARK: AnnotationTemplateFinder

template <typename Tuple, std::size_t Index, template <class...> class Expected>
struct AnnotationTemplateFinder {
  static constexpr bool Value = false;
};

template <typename Tuple, std::size_t Index, template <class...> class Expected>
  requires(Index < std::tuple_size_v<Tuple> &&
           !Matches<Expected,
                    typename std::tuple_element<Index, Tuple>::type>::Value)
struct AnnotationTemplateFinder<Tuple, Index, Expected> {
  static constexpr bool Value =
      AnnotationTemplateFinder<Tuple, Index + 1, Expected>::Value;
};

template <typename Tuple, std::size_t Index, template <class...> class Expected>
  requires(
      Index < std::tuple_size_v<Tuple> &&
      Matches<Expected, typename std::tuple_element<Index, Tuple>::type>::Value)
struct AnnotationTemplateFinder<Tuple, Index, Expected> {
  static constexpr bool Value = true;
};

// MARK: AnnotationClassFinder

template <typename Tuple, std::size_t Index, class Expected>
struct AnnotationClassFinder {
  static constexpr bool Value = false;
};

template <typename Tuple, std::size_t Index, class Expected>
  requires(Index < std::tuple_size_v<Tuple> &&
           !std::is_same_v<Expected,
                           typename std::tuple_element<Index, Tuple>::type>)
struct AnnotationClassFinder<Tuple, Index, Expected> {
  static constexpr bool Value =
      AnnotationClassFinder<Tuple, Index + 1, Expected>::Value;
};

template <typename Tuple, std::size_t Index, class Expected>
  requires(
      Index < std::tuple_size_v<Tuple> &&
      std::is_same_v<Expected, typename std::tuple_element<Index, Tuple>::type>)
struct AnnotationClassFinder<Tuple, Index, Expected> {
  static constexpr bool Value = true;
};

// MARK: AnnotationTemplateCreator

template <typename Tuple, std::size_t Index, template <class...> class Expected>
struct AnnotationTemplateCreator {
  using Type = Dummy;
};

template <typename Tuple, std::size_t Index, template <class...> class Expected>
  requires(Index < std::tuple_size_v<Tuple> &&
           !Matches<Expected,
                    typename std::tuple_element<Index, Tuple>::type>::Value)
struct AnnotationTemplateCreator<Tuple, Index, Expected> {
  using Type = AnnotationTemplateCreator<Tuple, Index + 1, Expected>::Type;
};

template <typename Tuple, std::size_t Index, template <class...> class Expected>
  requires(
      Index < std::tuple_size_v<Tuple> &&
      Matches<Expected, typename std::tuple_element<Index, Tuple>::type>::Value)
struct AnnotationTemplateCreator<Tuple, Index, Expected> {
  using Type =
      typename ArgSubstitute<typename std::tuple_element<Index, Tuple>::type>::
          template Substitute<Expected>;
};

template <typename Tuple, template <class...> class Expected>
struct AnnotationTemplateCreatorWrapper {
  using Type = AnnotationTemplateCreator<Tuple, 0, Expected>::Type;
  constexpr static bool Valid = !std::is_same_v<Type, Dummy>;
};

// MARK: FieldDescriptor

template <typename Tuple, size_t Index> struct FieldDescriptor {
  using Type = typename FieldExtractor<Tuple, Index, 0, 0, Annotate<>>::Type;
  using Annotations =
      typename FieldExtractor<Tuple, Index, 0, 0, Annotate<>>::Annotations;

  template <template <class...> class AnnotationTemplate>
  static constexpr bool has_annotation_template =
      AnnotationTemplateFinder<typename AnnotationsToTuple<Annotations>::Value,
                               0, AnnotationTemplate>::Value;

  template <class Annotation>
  static constexpr bool has_annotation_class =
      AnnotationClassFinder<typename AnnotationsToTuple<Annotations>::Value, 0,
                            Annotation>::Value;

  template <template <class...> class AnnotationTemplate>
    requires(AnnotationTemplateCreatorWrapper<
                typename AnnotationsToTuple<Annotations>::Value,
                AnnotationTemplate>::Valid)
  using FindAnnotation = AnnotationTemplateCreatorWrapper<
      typename AnnotationsToTuple<Annotations>::Value,
      AnnotationTemplate>::Type;
};

// Case: Body[Index] is an annotation
template <typename... Body, std::size_t Index, std::size_t Current,
          std::size_t CurrentArg, typename... AnnotationsAcc>
  requires(AnnotationChecker<NthType<Current, Body...>>::Value &&
           Current < sizeof...(Body))
struct FieldExtractor<std::tuple<Body...>, Index, Current, CurrentArg,
                      Annotate<AnnotationsAcc...>> {
  using CurrentAnnotations =
      typename AccumulateAnnotations<Annotate<AnnotationsAcc...>,
                                     NthType<Current, Body...>>::Annotations;

  using Type = typename FieldExtractor<std::tuple<Body...>, Index, Current + 1,
                                       CurrentArg, Annotate<>>::Type;
  using Annotations =
      typename FieldExtractor<std::tuple<Body...>, Index, Current + 1,
                              CurrentArg, CurrentAnnotations>::Annotations;
};

// Case: Body[Index] is a required field
template <typename... Body, std::size_t Index, std::size_t Current,
          std::size_t CurrentArg, typename... AnnotationsAcc>
  requires(!AnnotationChecker<NthType<Current, Body...>>::Value &&
           CurrentArg == Index && Current < sizeof...(Body))
struct FieldExtractor<std::tuple<Body...>, Index, Current, CurrentArg,
                      Annotate<AnnotationsAcc...>> {
  using Type = NthType<Current, Body...>;
  using Annotations = Annotate<AnnotationsAcc...>;
};

// Case: Body[Index] is a field, but not the asked one
template <typename... Body, std::size_t Index, std::size_t Current,
          std::size_t CurrentArg, typename... AnnotationsAcc>
  requires(!AnnotationChecker<NthType<Current, Body...>>::Value &&
           CurrentArg < Index && Current < sizeof...(Body))
struct FieldExtractor<std::tuple<Body...>, Index, Current, CurrentArg,
                      Annotate<AnnotationsAcc...>> {
  using Type = typename FieldExtractor<std::tuple<Body...>, Index, Current + 1,
                                       CurrentArg + 1, Annotate<>>::Type;
  using Annotations =
      typename FieldExtractor<std::tuple<Body...>, Index, Current + 1,
                              CurrentArg + 1, Annotate<>>::Annotations;
};

template <class T> struct Describe {
  using ArgumentsTuple = decltype(asTupleImpl<T>(getFieldIndices<T>()));
  static constexpr size_t num_fields =
      getPureArgumentCount(ArgumentsTuple());

  template <std::size_t Index>
  using Field = FieldDescriptor<typename AnnotationsToTuple<ArgumentsTuple>::Value, Index>;
};
