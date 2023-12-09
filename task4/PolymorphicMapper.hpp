#pragma once

#include <optional>

template <class From, auto target> struct Mapping {
  using MappingFrom = From;
  static constexpr auto mappingTarget = target;
};

template <class Base, class Target, class... Mappings>
struct PolymorphicMapper {};

template <class Base, class Target> struct PolymorphicMapper<Base, Target> {
  static std::optional<Target> map(const Base &) { return std::nullopt; }
};

template <class Base, class Target, class Head, class... Tail>
struct PolymorphicMapper<Base, Target, Head, Tail...> {
  static std::optional<Target> map(const Base &object) {
    static_assert(std::is_same_v<const decltype(Head::mappingTarget), const Target>);

    if (dynamic_cast<const typename Head::MappingFrom *>(&object)) {
      return Head::mappingTarget;
    }

    return PolymorphicMapper<Base, Target, Tail...>::map(object);
  }
};