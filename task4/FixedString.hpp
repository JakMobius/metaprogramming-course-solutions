#pragma once

#include <cstring>
#include <string_view>
#include <stdexcept>

template<size_t max_length>
struct FixedString {
  char data[max_length + 1] = {};

  constexpr FixedString() {}

  template<size_t other_size> requires(other_size <= max_length)
  constexpr FixedString(const FixedString<other_size>& other_string): FixedString(other_string.data, other_size) {}

  constexpr FixedString(const char* string, size_t length) {
    if (length > max_length) {
      throw std::out_of_range("String length exceeds the maximum length.");
    }
    for(int i = 0; i < length; i++) {
      data[i] = string[i];
    }
    data[length] = '\0';
  }

  constexpr operator std::string_view() const {
    return std::string_view(data);
  }
};

// A better way of doing things:
//
// template <size_t N>
// constexpr FixedString<N> make_fixed_string(const char* str, size_t length) {
//   return FixedString<N>(str, length);
// }
//
// template <typename CharT, CharT... Chars>
// constexpr auto operator"" _cstr() {
//   char string[] = {Chars...};
//   return make_fixed_string<sizeof...(Chars)>(string, sizeof...(Chars));
// }

 constexpr auto operator"" _cstr(const char* str, size_t length) {
   return FixedString<256>(str, length);
 }
