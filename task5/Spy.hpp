#pragma once
#include <concepts>
#include <functional>
#include <memory>

namespace detail {
class LoggerInterface {
public:
  virtual ~LoggerInterface() = default;
  virtual void invoke(unsigned int arg) = 0;
  virtual LoggerInterface *clone() = 0;

  void newProxy() { depth_++; }

  void reportAccess() { accesses_++; }

  void proxyDied() {
    depth_--;
    if (depth_ == 0) {
      invoke(accesses_);
      accesses_ = 0;
    }
  }

private:
  int accesses_ = 0;
  int depth_ = 0;
};

class LoggerWrapper {
public:
  LoggerWrapper() : func_(nullptr) {}

  LoggerWrapper(const LoggerWrapper &other) { *this = other; }

  LoggerWrapper(LoggerWrapper &&move) { *this = std::move(move); }

  template <typename F>
  LoggerWrapper(F &&f) : func_(new LoggerImpl<F>(std::forward<F>(f))) {}

  LoggerInterface *get() { return func_.get(); }

  LoggerWrapper &operator=(const LoggerWrapper &other) {
    if (this != &other) {
      if (other.func_) {
        func_.reset(other.func_->clone());
      } else {
        func_ = nullptr;
      }
    }
    return *this;
  }

  LoggerWrapper &operator=(LoggerWrapper &&move) {
    if (this != &move) {
      func_ = std::move(move.func_);
    }
    return *this;
  }

  // Destructor
  ~LoggerWrapper() = default;

private:
  template <typename F> class LoggerImpl : public LoggerInterface {
  public:
    template <typename U> LoggerImpl(U &&f) : func_(std::forward<U &&>(f)) {}

    void invoke(unsigned int arg) override { return func_(arg); }

    LoggerInterface *clone() override {
      if constexpr (std::copyable<F>) {
        return new LoggerImpl(func_);
      } else {
        return nullptr;
      }
    }

  private:
    F func_;
  };

  std::unique_ptr<LoggerInterface> func_;
};

template <typename T> class SpyProxy {
public:
  SpyProxy(T *ptr, LoggerWrapper &wrapper) : ptr_(ptr), logger_(wrapper.get()) {
    if (logger_)
      logger_->newProxy();
  }

  T *operator->() {
    if (logger_)
      logger_->reportAccess();
    return ptr_;
  }

  ~SpyProxy() {
    if (logger_)
      logger_->proxyDied();
  }

private:
  T *ptr_;
  LoggerInterface *logger_;
};
} // namespace detail

template <class T> class Spy {
public:
  Spy() = default;
  explicit Spy(T val) : value_(std::move(val)) {}
  Spy(const Spy &other)
    requires(std::copyable<T>)
      : value_(other.value_), logger_(other.logger_) {}
  Spy(Spy &&other)
    requires(std::movable<T>)
      : value_(std::move(other.value_)), logger_(std::move(other.logger_)) {}

  Spy &operator=(const Spy &other)
    requires(std::copy_constructible<T>)
  {
    value_ = other.value_;
    logger_ = other.logger_;
    return *this;
  }

  Spy &operator=(Spy &&other)
    requires(std::move_constructible<T>)
  {
    value_ = std::move(other.value_);
    logger_ = std::move(other.logger_);
    return *this;
  }

  bool operator==(const Spy &other) const
    requires(std::equality_comparable<T>)
  {
    return value_ == other.value_;
  }
  bool operator!=(const Spy &other) const
    requires(std::equality_comparable<T>)
  {
    return !(*this == other);
  }

  T &operator*() { return value_; }

  const T &operator*() const { return value_; }

  detail::SpyProxy<T> operator->() {
    return detail::SpyProxy<T>(&value_, logger_);
  }

  const T *operator->() const { return &value_; }

  template <std::invocable<unsigned int> U>
  void setLogger(U &&logger)
    requires(std::is_nothrow_destructible_v<U> &&
             !((std::movable<U> && !(std::copyable<U>)) && std::copyable<T>))
  {
    logger_ = detail::LoggerWrapper(std::forward<U>(logger));
  }

  ~Spy() = default;

private:
  T value_;
  unsigned int count_ = 0;
  detail::LoggerWrapper logger_;
};