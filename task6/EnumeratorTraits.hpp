#pragma once

#include <cstdint>
#include <numeric>
#include <regex>
#include <string>
#include <type_traits>
#include <array>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"

namespace detail {

constexpr std::string_view extractEnumValue(std::string_view input) {
  constexpr std::string_view enumValueStr = "EnumValue = ";
  constexpr char characters[] = ",;]";

  size_t enumPos = input.find(enumValueStr);
  if (enumPos != std::string_view::npos) {
    enumPos += enumValueStr.size();

    size_t endPos = input.find_first_of(characters, enumPos);

    if (endPos != std::string_view::npos) {
      if (input[enumPos] == '(') // Check for invalid value
        return "";

      input = input.substr(enumPos, endPos - enumPos);

      size_t lastDoubleColon = input.rfind("::");
      if (lastDoubleColon != std::string_view::npos &&
          lastDoubleColon < input.length() - 2)
        return input.substr(lastDoubleColon + 2);

      return input;
    }
  }

  return ""; // Return an empty string if extraction fails
}

template <auto EnumValue>
static constexpr std::string_view getPrettyFunction() {
  return __PRETTY_FUNCTION__;
}

static constexpr bool isEvalValueValid(std::string_view input) {
  return input.size() > 58 && input[58] != '(';
}
} // namespace detail

template <class Enum, std::size_t MAXN = 512>
  requires std::is_enum_v<Enum>
struct EnumeratorTraits {
  using EnumType = std::underlying_type<Enum>::type;
  template <size_t index> static constexpr std::string_view getDescription() {
    if constexpr (std::numeric_limits<EnumType>::min() >
                      (int)index - (int)MAXN ||
                  std::numeric_limits<EnumType>::max() <
                      (int)index - (int)MAXN) {
      return "";
    } else {
      return detail::getPrettyFunction<static_cast<Enum>(index - MAXN)>();
    }
  }

  static consteval std::array<Enum, 2 * MAXN + 1> generateArray() {
    std::array<Enum, 2 * MAXN + 1> result{};
    for (int i = -(int)MAXN; i <= MAXN; ++i) {
      result[i + MAXN] = Enum(i);
    }
    return result;
  }

  static consteval auto getDescriptionsArray() {
    auto descriptions =
        getDescriptions(std::make_index_sequence<MAXN * 2 + 1>{});

    return descriptions;
  }

  static constexpr std::array<std::string_view, 2 * MAXN + 1>
      descriptionsArray = getDescriptionsArray();

  template <size_t... Indices>
  static consteval std::array<std::string_view, MAXN * 2 + 1>
  getDescriptions(std::index_sequence<Indices...>) noexcept {
    return std::array<std::string_view, MAXN * 2 + 1>{
        getDescription<Indices>()...};
  }

  static constexpr std::size_t size() noexcept {
    std::size_t result = 0;

    for (auto description : descriptionsArray) {
      if (detail::isEvalValueValid(description)) {
        result++;
      }
    }
    return result;
  }

  static constexpr Enum at(std::size_t i) noexcept {
    int index = -(int)MAXN;
    for (auto description : descriptionsArray) {
      if (detail::isEvalValueValid(description)) {
        if (i == 0) {
          return Enum(index);
        }
        i--;
      }
      index++;
    }
    return Enum(0);
  }

  static constexpr std::string_view nameAt(std::size_t i) noexcept {
    for (auto description : descriptionsArray) {
      if (detail::isEvalValueValid(description)) {
        if (i == 0) {
          return detail::extractEnumValue(description);
        }
        i--;
      }
    }

    return "";
  }
};

#pragma clang diagnostic pop