#include <array>
#include <concepts>
#include <cstdlib>
#include <iterator>
#include <span>

inline constexpr std::ptrdiff_t dynamic_stride = -1;

namespace detail {

template <typename T, T value> class CompileTimeValue {
public:
  constexpr CompileTimeValue() = default;
  constexpr CompileTimeValue(T) noexcept {}

  constexpr const T get() const { return value; }
};

template <typename T> class RuntimeValue {
public:
  constexpr RuntimeValue() = default;
  constexpr RuntimeValue(T value) noexcept : value_(value) {}

  constexpr T &get() { return value_; }

  constexpr const T &get() const { return value_; }

private:
  T value_;
};

template <std::size_t value>
class ExtentValue : public CompileTimeValue<std::size_t, value> {};

template <>
class ExtentValue<std::dynamic_extent> : public RuntimeValue<std::size_t> {};

template <std::ptrdiff_t value>
class StrideValue : public CompileTimeValue<std::ptrdiff_t, value> {};

template <>
class StrideValue<dynamic_stride> : public RuntimeValue<std::ptrdiff_t> {};

template <typename T>
class SkipIterator : public std::random_access_iterator_tag {
public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = std::remove_cv_t<T>;
  using element_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = element_type &;
  using const_reference = const element_type &;

  SkipIterator() = default;
  SkipIterator(pointer ptr, difference_type stride)
      : pointer_(ptr), stride_(stride) {}
  ~SkipIterator() = default;

  SkipIterator &operator++() {
    *this += 1;
    return *this;
  }

  SkipIterator operator++(int) {
    SkipIterator it = *this;
    *this ++;
    return it;
  }

  SkipIterator &operator--() {
    *this -= 1;
    return *this;
  }

  SkipIterator operator--(int) {
    SkipIterator it = *this;
    *this --;
    return it;
  }

  SkipIterator operator+(size_type rhs) const {
    SkipIterator it = *this;
    *this += rhs;
    return it;
  }

  SkipIterator operator-(size_type rhs) const {
    SkipIterator it = *this;
    *this -= rhs;
    return it;
  }

  difference_type operator-(const SkipIterator &that) const {
    return pointer_ - that.pointer_;
  }

  SkipIterator &operator+=(const difference_type n) {
    pointer_ += n * stride_;
    return *this;
  }

  SkipIterator &operator-=(const difference_type n) {
    pointer_ -= n * stride_;
    return *this;
  }

  reference operator*() const { return *pointer_; }

  pointer operator->() { return pointer_; }

  reference operator[](const difference_type i) const {
    return iterator(pointer_ + stride_ * i, stride_);
  }

  bool operator<=>(const SkipIterator &rhs) const {
    return pointer_ <=> rhs.pointer_;
  }
  bool operator!=(const SkipIterator &rhs) const {
    return pointer_ != rhs.pointer_;
  }
  bool operator==(const SkipIterator &rhs) const {
    return pointer_ == rhs.pointer_;
  }

private:
  pointer pointer_;
  difference_type stride_;
};

template <typename T>
SkipIterator<T> operator+(const typename SkipIterator<T>::difference_type n,
                          const SkipIterator<T> iter) {
  SkipIterator it = iter;
  it += n;
  return it;
}

} // namespace detail

template <typename T, std::size_t extent = std::dynamic_extent,
          std::ptrdiff_t stride = 1>
class Slice {
public:
  using element_type = T;
  using iterator = detail::SkipIterator<element_type>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  using value_type = std::remove_cv_t<element_type>;
  using reference = element_type &;
  using const_reference = const element_type &;
  using pointer = element_type *;
  using const_pointer = const element_type *;
  using difference_type = std::ptrdiff_t;
  using size_type = std::size_t;

  Slice() = default;
  Slice(const Slice &) = default;
  explicit Slice(Slice &&) = default;

  template <typename Type, size_t Size>
  constexpr Slice(std::array<Type, Size> &array) {
    data_ = array.data();

    if constexpr (stride == dynamic_stride) {
      stride_.get() = 1;
    }

    if constexpr (extent == std::dynamic_extent) {
      extent_.get() = array.size() / Stride();
    }
  }

  template <typename V>
    requires(std::is_same_v<std::remove_const_t<typename V::value_type>,
                            std::remove_const_t<T>> &&
             extent == std::dynamic_extent)
  constexpr Slice(V &container)
    requires requires {
      container.data();
      container.size();
    }
  {
    data_ = container.data();

    if constexpr (stride == dynamic_stride) {
      stride_.get() = 1;
    }

    extent_.get() = container.size() / Stride();
  }

  template <std::contiguous_iterator It>
  Slice(It first, std::size_t count, std::ptrdiff_t skip) {
    data_ = &*first;

    if constexpr (stride == dynamic_stride) {
      stride_.get() = skip;
    }

    if constexpr (extent == std::dynamic_extent) {
      extent_.get() = count;
    }
  }

  Slice &operator=(Slice &&) = default;
  Slice &operator=(const Slice &) = default;

  constexpr iterator begin() const noexcept {
    return iterator(data_, Stride());
  }

  constexpr iterator end() const noexcept {
    return iterator(data_ + Stride() * Size(), Stride());
  }

  constexpr reverse_iterator rbegin() const noexcept {
    return std::make_reverse_iterator(end());
  }

  constexpr reverse_iterator rend() const noexcept {
    return std::make_reverse_iterator(begin());
  }

  Slice<T, std::dynamic_extent, stride> First(std::size_t count) const {
    return Slice<T, std::dynamic_extent, stride>(data_, count, Stride());
  }

  template <std::size_t count> Slice<T, count, stride> First() const {
    return Slice<T, count, stride>(data_, count, Stride());
  }

  Slice<T, std::dynamic_extent, stride> Last(std::size_t count) const {
    return Slice<T, std::dynamic_extent, stride>(
        data_ + Stride() * (Size() - count), count, Stride());
  }

  template <std::size_t count> Slice<T, count, stride> Last() const {
    return Slice<T, count, stride>(data_ + Stride() * (Size() - count), count,
                                   Stride());
  }

  template <std::size_t count>
  Slice<T, std::dynamic_extent, stride> DropFirst() const {
    return Slice<T, std::dynamic_extent, stride>(data_ + Stride() * count,
                                                 Size() - count, Stride());
  }

  Slice<T, std::dynamic_extent, stride> DropFirst(std::size_t count) const {
    return Slice<T, std::dynamic_extent, stride>(data_ + Stride() * count,
                                                 Size() - count, Stride());
  }

  Slice<T, std::dynamic_extent, stride> DropLast(std::size_t count) const {
    return Slice<T, std::dynamic_extent, stride>(data_, Size() - count,
                                                 Stride());
  }

  template <std::size_t count>
  Slice<T, std::dynamic_extent, stride> DropLast() const {
    return Slice<T, std::dynamic_extent, stride>(data_, Size() - count,
                                                 Stride());
  }

  Slice<T, std::dynamic_extent, dynamic_stride>
  Skip(std::ptrdiff_t skip) const {
    return Slice<T, std::dynamic_extent, dynamic_stride>(
        data_, extentWithSkipDynamic_(skip), strideWithSkipDynamic_(skip));
  }

  template <std::ptrdiff_t skip> auto Skip() const {
    return Slice<T, extentWithSkipStatic_(skip), strideWithSkipStatic_(skip)>(
        data_, extentWithSkipDynamic_(skip), strideWithSkipDynamic_(skip));
  }

  constexpr reference operator[](size_type i) const noexcept {
    return *(data_ + i * Stride());
  }

  template <typename otherT, auto... params>
  bool operator==(const Slice<otherT, params...> &rhs) const {
    auto a = begin();
    auto b = rhs.begin();

    while (*a == *b && a != end() && b != rhs.end()) {
      ++a;
      ++b;
    }

    return a == end() && b == rhs.end();
  }

  template <typename otherT, auto... params>
    requires(std::is_same_v<otherT, T> || std::is_same_v<otherT, const T>)
  operator Slice<otherT, params...>() const {
    return {data_, Size(), Stride()};
  }

  constexpr pointer Data() const noexcept { return data_; }
  constexpr size_type Size() const noexcept { return extent_.get(); }
  constexpr difference_type Stride() const noexcept { return stride_.get(); }

private:
  size_type extentWithSkipDynamic_(const difference_type skip) const {
    return (skip - 1 + Size()) / skip;
  }

  difference_type strideWithSkipDynamic_(const difference_type skip) const {
    return Stride() * skip;
  }

  static consteval size_type extentWithSkipStatic_(const difference_type skip) {
    return extent == std::dynamic_extent ? std::dynamic_extent
                                         : (skip - 1 + extent) / skip;
  }

  static consteval difference_type
  strideWithSkipStatic_(const difference_type skip) {
    return stride == dynamic_stride ? dynamic_stride : stride * skip;
  }

  T *data_;
  [[no_unique_address]] detail::ExtentValue<extent> extent_;
  [[no_unique_address]] detail::StrideValue<stride> stride_;
};

template <typename T> Slice(T &) -> Slice<typename T::value_type>;

template <typename Type, size_t Size>
Slice(std::array<Type, Size> &) -> Slice<Type, Size>;

template <std::contiguous_iterator It>
Slice(It first, std::size_t count, std::ptrdiff_t skip)
    -> Slice<typename It::value_type, std::dynamic_extent, dynamic_stride>;