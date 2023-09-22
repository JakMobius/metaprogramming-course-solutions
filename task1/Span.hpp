#include <span>
#include <concepts>
#include <cstdlib>
#include <iterator>
#include <vector>

template<typename T, size_t extent>
class StaticSpan {
public:
    using element_type = T;
    using size_type = size_t;

    StaticSpan(element_type *begin) {
        this->data_ = begin;
    }

    template<typename Container>
    StaticSpan(Container &container) {
        this->data_ = container.data();
    }

    constexpr size_type Size() const {
        return extent;
    }

protected:
    element_type *data_;
};

template<typename T>
class DynamicSpan {
public:
    using element_type = T;
    using size_type = size_t;

    template<typename Container>
    DynamicSpan(Container &container) {
        this->data_ = container.data();
        this->extent_ = container.size();
    }

    template<class Iter>
    DynamicSpan(Iter begin, Iter end) {
        this->data_ = &*begin;
        this->extent_ = end - begin;
    }

    template<class Iter>
    DynamicSpan(Iter begin, size_t size) {
        this->data_ = &*begin;
        this->extent_ = size;
    }

    constexpr size_type Size() const {
        return extent_;
    }

protected:
    size_type extent_;
    element_type *data_;
};

template<typename T, size_t extent = std::dynamic_extent>
class Span;

template<class Base>
class SpanBase : public Base {
public:
    using element_type = typename Base::element_type;
    using iterator = element_type *;
    using reverse_iterator = std::reverse_iterator<element_type *>;

    using value_type = std::remove_cv_t<element_type>;
    using reference = element_type &;
    using const_reference = const element_type &;
    using pointer = element_type *;
    using const_pointer = const element_type *;
    using difference_type = std::ptrdiff_t;

    using Base::Base;

    iterator begin() const {
        return this->data_;
    }

    iterator end() const {
        return this->data_ + this->Size();
    }

    reverse_iterator rbegin() const {
        return std::reverse_iterator(end());
    }

    reverse_iterator rend() const {
        return std::reverse_iterator(begin());
    }

    reference Front() const {
        return *begin();
    }

    reference Back() const {
        return *rbegin();
    }

    pointer Data() const {
        return this->data_;
    }

    reference operator[](int index) const {
        return *(begin() + index);
    };

    Span<element_type> Last(int length) const {
        return Span<element_type>(this->end() - length, this->end());
    }

    template<int length>
    constexpr Span<element_type, length> Last() const {
        return Span<element_type, length>(this->Data() + this->Size() - length);
    }

    Span<element_type> First(int length) const {
        return Span<element_type>(this->Data(), this->Data() + length);
    }

    template<int length>
    constexpr Span<element_type, length> First() const {
        return Span<element_type, length>(this->Data());
    }
};

template<typename T, size_t extent>
class Span : public SpanBase<StaticSpan<T, extent>> {
public:
    using Base = SpanBase<StaticSpan<T, extent>>;
    using Base::Base;
};

template<typename T>
class Span<T, std::dynamic_extent> : public SpanBase<DynamicSpan<T>> {
public:
    using Base = SpanBase<DynamicSpan<T>>;
    using Base::Base;
};

// This overload is only allowed if container size is a constexpr
template<typename Container> requires(Container().size() >= 0)
Span(Container &) -> Span<typename Container::value_type, Container().size()>;

template<typename Container>
Span(Container &) -> Span<typename Container::value_type>;

template<typename Iter>
Span(Iter, Iter) -> Span<typename Iter::value_type>;

template<typename Iter>
Span(Iter, size_t size) -> Span<typename Iter::value_type>;