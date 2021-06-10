/******************************************************************************
 * Copyright 2021 ETC Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************
 * This file is a part of EtcPal. For more information, go to:
 * https://github.com/ETCLabs/EtcPal
 ******************************************************************************/

/// @file etcpal/cpp/error.h
/// @brief C++ wrapper and utilities for etcpal/error.h

#ifndef ETCPAL_CPP_ERROR_H_
#define ETCPAL_CPP_ERROR_H_

#include <cassert>
#include <stdexcept>
#include <string>
#include "etcpal/error.h"
#include "etcpal/cpp/common.h"

namespace etcpal
{
/// @defgroup etcpal_cpp_error error (Error Handling)
/// @ingroup etcpal_cpp
/// @brief C++ utilities for the @ref etcpal_error module.
///
/// This module provides two main utilites: Error, which wraps an error code, and Expected, which
/// represents either an error code or a value of configurable type. See the documentation for each
/// of those classes for more details.

////////////////////////////////////////////////////////////////////////////////////////////////////
// etcpal::Error
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @ingroup etcpal_cpp_error
/// @brief A wrapper class for the EtcPal error type.
///
/// Provides C++ syntactic sugar on top of #etcpal_error_t codes. A Error can be created from an
/// #etcpal_error_t, and contains the code and a string representation. If the Error contains
/// #kEtcPalErrOk, it will evaluate to true in expressions (and IsOk() will return true).
/// Otherwise, the Error evaluates to false.
///
/// The human-readable string associated with the Error can be retrieved with ToString() and
/// ToCString(); this will be the same string available from etcpal_strerror() for the given error
/// code.
///
/// Example with an "OK" error code:
/// @code
/// auto result = etcpal::Error(kEtcPalErrOk); // or etcpal::Error::Ok()
/// if (result)
/// {
///    // This block will be entered...
/// }
/// @endcode
///
/// Example with a non-"OK" error code:
/// @code
/// auto result = etcpal::Error(kEtcPalErrNoMem);
/// if (result)
/// {
///   // This block will not be entered
/// }
/// else
/// {
///   // This block will be entered
///   printf("Error: '%s'", result.ToCString());
///   // Or
///   std::cout << "Error: '" << result.ToString() << "'\n";
///   // Prints "Error: 'Out of memory'"
/// }
/// @endcode
///
/// Error is often used as the return type for functions; it has an implicit constructor from
/// #etcpal_error_t to simplify return statements:
/// @code
/// etcpal::Error DoSomething()
/// {
///   // ... After we did something successfully...
///   return kEtcPalErrOk;
/// }
/// @endcode
class Error
{
public:
  Error() = delete;
  // Note: this constructor is not explicit by design, to allow implicit conversions e.g.
  //   etcpal::Error FunctionReturningError()
  //   {
  //     return kEtcPalErrOk;
  //   }
  constexpr Error(etcpal_error_t code) noexcept;
  Error& operator=(etcpal_error_t code) noexcept;

  constexpr bool           IsOk() const noexcept;
  constexpr etcpal_error_t code() const noexcept;
  std::string              ToString() const;
  const char*              ToCString() const noexcept;

  constexpr explicit operator bool() const noexcept;

  static constexpr Error Ok() noexcept;

private:
  etcpal_error_t code_{kEtcPalErrOk};
};

/// @brief Construct a Error from an error code.
constexpr Error::Error(etcpal_error_t code) noexcept : code_(code)
{
}

/// @brief Assign an error code to this Error.
inline Error& Error::operator=(etcpal_error_t code) noexcept
{
  code_ = code;
  return *this;
}

/// @brief Whether this Error contains the code #kEtcPalErrOk.
/// @details #kEtcPalErrOk is the only #etcpal_error_t code that does not indicate an error.
constexpr bool Error::IsOk() const noexcept
{
  return code_ == kEtcPalErrOk;
}

/// @brief Get the underlying code from a Error.
constexpr etcpal_error_t Error::code() const noexcept
{
  return code_;
}

/// @brief Get a descriptive string for this Error as a std::string.
inline std::string Error::ToString() const
{
  return etcpal_strerror(code_);
}

/// @brief Get a descriptive string for this Error as a C string.
inline const char* Error::ToCString() const noexcept
{
  return etcpal_strerror(code_);
}

/// @brief Evaluate the Error inline - evaluates true if the Error is #kEtcPalErrOk, false
///        otherwise.
constexpr Error::operator bool() const noexcept
{
  return IsOk();
}

/// @brief Construct an Error containing #kEtcPalErrOk.
constexpr Error Error::Ok() noexcept
{
  return {kEtcPalErrOk};
}

/// @addtogroup etcpal_cpp_error
/// @{

/// @name Error Relational Operators
/// @{

constexpr bool operator==(etcpal_error_t code, const Error& error)
{
  return code == error.code();
}

constexpr bool operator!=(etcpal_error_t code, const Error& error)
{
  return !(code == error);
}

constexpr bool operator==(const Error& error, etcpal_error_t code)
{
  return error.code() == code;
}

constexpr bool operator!=(const Error& error, etcpal_error_t code)
{
  return !(error == code);
}

constexpr bool operator==(const Error& a, const Error& b)
{
  return a.code() == b.code();
}

constexpr bool operator!=(const Error& a, const Error& b)
{
  return !(a == b);
}

/// @}
/// @}

////////////////////////////////////////////////////////////////////////////////////////////////////
// etcpal::BadExpectedAccess
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Exception representing bad access to an Expected instance.
/// @ingroup etcpal_cpp_error
/// @details
/// Thrown when attempting to access Expected<T>::value() when Expected<T>::has_value() is false.
/// Holds an etcpal::Error; this allows nicer access to error information than just having the
/// #etcpal_error_t code.
class BadExpectedAccess : public std::logic_error
{
public:
  explicit BadExpectedAccess(Error res);
  Error error() const noexcept;

private:
  Error err_;
};

/// @brief Construct from a Error.
/// @throw May throw std::bad_alloc.
inline BadExpectedAccess::BadExpectedAccess(Error err)
    : std::logic_error("Bad access to etcpal::Expected::value(); the Expected instance contained error '" +
                       err.ToString() + "'.")
    , err_(err)
{
}

/// @brief Get the error code which was contained in the associated Expected when the exception occurred.
inline Error BadExpectedAccess::error() const noexcept
{
  return err_;
}

/// @cond detail_expected

////////////////////////////////////////////////////////////////////////////////////////////////////
// etcpal::Expected internals
////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class Expected;

namespace detail
{
// This method of implementing Expected's storage, with copy and move functionality, was borrowed
// with love from a generic header-only expected implementation by martinmoene:
// https://github.com/martinmoene/expected-lite
// There are some NOLINT directives since this class does some tricky unexpected C++ stuff.

template <typename T>
class ExpectedStorageImpl  // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
  using ValueType = T;

  // Do not construct or destruct members on construction or destruction
  ExpectedStorageImpl() {}   // NOLINT(cppcoreguidelines-pro-type-member-init,modernize-use-equals-default)
  ~ExpectedStorageImpl() {}  // NOLINT(modernize-use-equals-default)

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  explicit ExpectedStorageImpl(bool has_value) : has_value_(has_value) {}

  void ConstructValue(const ValueType& v) { new (&value_) ValueType(v); }
  void ConstructValue(ValueType&& v) { new (&value_) ValueType(std::move(v)); }
  void SetError(etcpal_error_t e) { error_ = e; }

  void DestructValue() { value_.~ValueType(); }

  constexpr const ValueType&  value() const& { return value_; }
  ValueType&                  value() & { return value_; }
  constexpr const ValueType&& value() const&& { return std::move(value_); }
  ETCPAL_CONSTEXPR_14 ValueType&& value() && { return std::move(value_); }
  const ValueType*                value_ptr() const { return &value_; }
  ValueType*                      value_ptr() { return &value_; }

  etcpal_error_t error() const { return error_; }

  bool has_value() const { return has_value_; }
  void SetHasValue(bool v) { has_value_ = v; }

private:
  bool has_value_{false};
  union
  {
    ValueType      value_;
    etcpal_error_t error_;
  };
};

// The ExpectedStorage class provides a way to conditionally delete the copy and/or move
// constructors of etcpal::Expected based on the copyability and moveability of the T that we are
// being specialized with.
//
// We provide a generic template where the copy and move constructors are both deleted, then 3
// specializations, for (copyable and movable), (copyable and not moveable), and (moveable and not
// copyable) respectively.
template <typename T, bool IsCopyConstructible, bool IsMoveConstructible>
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class ExpectedStorage : public ExpectedStorageImpl<T>
{
public:
  ExpectedStorage() = default;
  ~ExpectedStorage() = default;

  explicit ExpectedStorage(bool has_value) : ExpectedStorageImpl<T>(has_value) {}

  ExpectedStorage(const ExpectedStorage& other) = delete;
  ExpectedStorage(ExpectedStorage&& other) = delete;
};

template <typename T>
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class ExpectedStorage<T, true, true> : public ExpectedStorageImpl<T>
{
public:
  ExpectedStorage() = default;
  ~ExpectedStorage() = default;

  explicit ExpectedStorage(bool has_value) : ExpectedStorageImpl<T>(has_value) {}

  ExpectedStorage(const ExpectedStorage& other) : ExpectedStorageImpl<T>(other.has_value())
  {
    if (this->has_value())
      this->ConstructValue(other.value());
    else
      this->SetError(other.error());
  }

  ExpectedStorage(ExpectedStorage&& other) : ExpectedStorageImpl<T>(other.has_value())
  {
    if (this->has_value())
      this->ConstructValue(std::move(other.value()));
    else
      this->SetError(other.error());
  }
};

template <typename T>
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class ExpectedStorage<T, true, false> : public ExpectedStorageImpl<T>
{
public:
  ExpectedStorage() = default;
  ~ExpectedStorage() = default;

  explicit ExpectedStorage(bool has_value) : ExpectedStorageImpl<T>(has_value) {}

  ExpectedStorage(const ExpectedStorage& other) : ExpectedStorageImpl<T>(other.has_value())
  {
    if (this->has_value())
      this->ConstructValue(other.value());
    else
      this->SetError(other.error());
  }

  ExpectedStorage(ExpectedStorage&& other) = delete;
};

template <typename T>
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class ExpectedStorage<T, false, true> : public ExpectedStorageImpl<T>
{
public:
  ExpectedStorage() = default;
  ~ExpectedStorage() = default;

  explicit ExpectedStorage(bool has_value) : ExpectedStorageImpl<T>(has_value) {}

  ExpectedStorage(ExpectedStorage const& other) = delete;

  ExpectedStorage(ExpectedStorage&& other) : ExpectedStorageImpl<T>(other.has_value())
  {
    if (this->has_value())
      this->ConstructValue(std::move(other.value()));
    else
      this->SetError(other.error());
  }
};
};  // namespace detail

/// @endcond

////////////////////////////////////////////////////////////////////////////////////////////////////
// etcpal::Expected
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @ingroup etcpal_cpp_error
/// @brief A type representing either a value or an #etcpal_error_t code.
///
/// This class is modeled after the [std::expected proposal](http://wg21.link/p0323). It is a
/// useful companion to C++17's std::optional, when one desires a concise representation of either
/// a value or an error code representing why that value is not present.
///
/// The proposed std::expected is generic with respect to both the value and the error type, but
/// EtcPal's version always uses #etcpal_error_t as the error. This makes it useful in the context
/// of EtcPal itself and libraries that depend on EtcPal; if you are looking for a
/// generically-useful `expected` implementation, [martinmoene's version](https://github.com/martinmoene/expected-lite)
/// is recommended. This implementation is based heavily on that one.
///
/// **Usage**
///
/// Expected is almost always used as the return type of an API function. Consider a function that
/// either creates a network socket or returns an error.
///
/// @code
/// etcpal::Expected<etcpal_socket_t> CreateSocket(int family, int type);
/// @endcode
///
/// When using this function, check whether the result contains an error:
/// @code
/// auto socket_res = CreateSocket(AF_INET, SOCK_STREAM);
/// if (socket_res) // or socket_res.has_value()
/// {
///   etcpal_socket_t socket = *socket_res; // or socket_res.value()
///   // Now use the socket...
/// }
/// else
/// {
///   printf("CreateSocket failed with result code %d, description '%s'\n", socket_res.error(),
///          socket_res.result().ToCString());
/// }
/// @endcode
///
/// value() throws BadExpectedAccess if the instance does not have a valid value. So you can also
/// use an exception-handling approach:
///
/// @code
/// try
/// {
///   etcpal_socket_t socket = CreateSocket(AF_INET, SOCK_STREAM).value();
/// }
/// catch (const etcpal::BadExpectedAccess& e)
/// {
///   printf("CreateSocket failed with result code %d, description '%s'\n", e.result().code(),
///          e.result().ToCString());
/// }
/// @endcode
///
/// Note that this only works for the value() function, not any of the other accessors (they assert
/// on the correct state instead).
///
/// Inside functions that return Expected, you can use implicit conversions from the value type or
/// #etcpal_error_t to return success or failure:
///
/// @code
/// // Possible implementation of CreateSocket()...
/// etcpal::Expected<etcpal_socket_t> CreateSocket(int family, int type)
/// {
///    etcpal_socket_t socket;
///    etcpal_error_t res = etcpal_socket(family, type, &socket);
///    if (res == kEtcPalErrOk)
///      return socket; // Implicitly converted to Expected, has_value() is true
///    else
///      return res; // Implicitly converted to Expected, has_value() is false
/// }
/// @endcode
///
/// The value_or() function can be useful when you want to use a default value in case of an error
/// condition:
///
/// @code
/// etcpal::Expected<std::string> ConvertIntToString(int val);
/// // ...
/// std::string conversion = ConvertIntToString(arg).value_or("0");
/// // Conversion contains the successful conversion result, or "0" if the conversion failed.
/// @endcode
template <typename T>
class Expected  // NOLINT(cppcoreguidelines-special-member-functions)
{
private:
  template <typename>
  friend class Expected;

public:
  static_assert(!std::is_reference<T>::value, "T must not be a reference");
  static_assert(!std::is_same<T, etcpal_error_t>::value, "T must not be etcpal_error_t");
  static_assert(!std::is_same<T, Error>::value, "T must not be etcpal::Error");
  static_assert(!std::is_void<T>::value, "T must not be void");

  /// The value type.
  using ValueType = T;

  // Typical ETC library style is to place all function definitions outside the class definition;
  // however, this gets nasty with these templates so the constructors are an exception here.

  // clang-format off

  /// @brief Default constructor - enabled if T is default-constructible.
  ///
  /// Default-constructs an instance of T. This Expected instance has a value after this
  /// constructor is called.
  template <bool B = std::is_default_constructible<T>::value, typename std::enable_if<B, int>::type = 0>
  ETCPAL_CONSTEXPR_14 Expected() : contained_(true)
  {
    contained_.ConstructValue(ValueType());
  }

  /// @brief Copy constructor - enabled if T is copy-constructible.
  ///
  /// If other has a value, copies its value into this Expected.
  /// If other does not have a value, copies its error code into this Expected.
  ETCPAL_CONSTEXPR_14 Expected(const Expected& other) = default;

  /// @brief Move constructor - enabled if T is move-constructible.
  ///
  /// If other has a value, moves its value into this Expected.
  /// If other does not have a value, copies its error code into this Expected.
  ETCPAL_CONSTEXPR_14 Expected(Expected&& other) = default;

  /// @brief Explicit conversion copy constructor - enabled for U that can be explicitly converted to T.
  ///
  /// If other has a value, copies its value into this Expected.
  /// If other does not have a value, copies its error code into this Expected.
  template<typename U>
  ETCPAL_CONSTEXPR_14 explicit Expected(const Expected<U>& other,
                                        ETCPAL_ENABLE_IF_ARG(
                                            std::is_constructible<T, const U&>::value &&
                                           !std::is_constructible<T, Expected<U>&>::value &&
                                           !std::is_constructible<T, Expected<U>       &&   >::value &&
                                           !std::is_constructible<T, Expected<U> const &    >::value &&
                                           !std::is_constructible<T, Expected<U> const &&   >::value &&
                                           !std::is_convertible<     Expected<U>       & , T>::value &&
                                           !std::is_convertible<     Expected<U>       &&, T>::value &&
                                           !std::is_convertible<     Expected<U> const & , T>::value &&
                                           !std::is_convertible<     Expected<U> const &&, T>::value &&
                                           !std::is_convertible<const U&, T>::value
                                       ))
      : contained_(other.has_value())
  {
    if (has_value())
      contained_.ConstructValue(T{other.contained_.value()});
    else
      contained_.SetError(other.contained_.error());
  }

  /// @brief Implicit conversion copy constructor - enabled for U that can be implicitly converted to T.
  ///
  /// If other has a value, copies its value into this Expected.
  /// If other does not have a value, copies its error code into this Expected.
  template<typename U>
  ETCPAL_CONSTEXPR_14 Expected(const Expected<U>& other,
                               ETCPAL_ENABLE_IF_ARG(
                                   std::is_constructible<T, const U&>::value &&
                                  !std::is_constructible<T, Expected<U>       &    >::value &&
                                  !std::is_constructible<T, Expected<U>       &&   >::value &&
                                  !std::is_constructible<T, Expected<U> const &    >::value &&
                                  !std::is_constructible<T, Expected<U> const &&   >::value &&
                                  !std::is_convertible<     Expected<U>       & , T>::value &&
                                  !std::is_convertible<     Expected<U>       &&, T>::value &&
                                  !std::is_convertible<     Expected<U> const  &, T>::value &&
                                  !std::is_convertible<     Expected<U> const &&, T>::value &&
                                  std::is_convertible<const U&, T>::value
                              ))
      : contained_(other.has_value())
  {
    if (has_value())
      contained_.ConstructValue(other.contained_.value());
    else
      contained_.SetError(other.contained_.error());
  }

  /// @brief Explicit conversion move constructor - enabled for U that can be explicitly converted to T.
  ///
  /// If other has a value, moves its value into this Expected.
  /// If other does not have a value, copies its error code into this Expected.
  template<typename U>
  ETCPAL_CONSTEXPR_14 explicit Expected(Expected<U>&& other,
                                        ETCPAL_ENABLE_IF_ARG(
                                            std::is_constructible<T, U>::value &&
                                           !std::is_constructible<T, Expected<U>       &    >::value &&
                                           !std::is_constructible<T, Expected<U>       &&   >::value &&
                                           !std::is_constructible<T, Expected<U> const &    >::value &&
                                           !std::is_constructible<T, Expected<U> const &&   >::value &&
                                           !std::is_convertible<     Expected<U>       & , T>::value &&
                                           !std::is_convertible<     Expected<U>       &&, T>::value &&
                                           !std::is_convertible<     Expected<U> const & , T>::value &&
                                           !std::is_convertible<     Expected<U> const &&, T>::value &&
                                           !std::is_convertible<U, T>::value
                                       ))
      : contained_(other.has_value())
  {
    if (has_value())
      contained_.ConstructValue(T{std::move(other.contained_.value())});
    else
      contained_.SetError(other.contained_.error());
  }

  /// @brief Implicit conversion move constructor - enabled for U that can be implicitly converted to T.
  ///
  /// If other has a value, moves its value into this Expected.
  /// If other does not have a value, copies its error code into this Expected.
  template<typename U>
  ETCPAL_CONSTEXPR_14 Expected(Expected<U>&& other,
                               ETCPAL_ENABLE_IF_ARG(
                                   std::is_constructible<T, U>::value &&
                                  !std::is_constructible<T, Expected<U>      &     >::value &&
                                  !std::is_constructible<T, Expected<U>      &&    >::value &&
                                  !std::is_constructible<T, Expected<U> const &    >::value &&
                                  !std::is_constructible<T, Expected<U> const &&   >::value &&
                                  !std::is_convertible<     Expected<U>       & , T>::value &&
                                  !std::is_convertible<     Expected<U>       &&, T>::value &&
                                  !std::is_convertible<     Expected<U> const & , T>::value &&
                                  !std::is_convertible<     Expected<U> const &&, T>::value &&
                                   std::is_convertible<U, T>::value
                              ))
      : contained_(other.has_value())
  {
    if (has_value())
      contained_.ConstructValue(std::move(other.contained_.value()));
    else
      contained_.SetError(other.contained_.error());
  }

  /// @brief Construct from value, explicit.
  ///
  /// Constructs an Expected from some value U which is explicitly (but not implicitly) convertible
  /// to T. The constructor is made explicit as a result. This Expected instance has a value after
  /// this constructor is called.
  ///
  /// @param value Value to construct from.
  template <typename U = T>
  ETCPAL_CONSTEXPR_14 explicit Expected(U&& value,
                                        ETCPAL_ENABLE_IF_ARG( std::is_constructible<T, U&&>::value &&
                                                             !std::is_convertible<U&&, T>::value
                                                            )) noexcept(std::is_nothrow_move_constructible<U>::value)
      : contained_(true)
  {
    contained_.ConstructValue(T{std::forward<U>(value)});
  }

  /// @brief Construct from value, implicit.
  ///
  /// Constructs an Expected from some value U implicitly convertible to T. The constructor is not
  /// made explicit as a result. This Expected instance has a value after this constructor is
  /// called.
  ///
  /// @param value Value to construct from.
  template <typename U = T>
  ETCPAL_CONSTEXPR_14 Expected(U&& value, ETCPAL_ENABLE_IF_ARG(std::is_constructible<T, U&&>::value &&
                                                               std::is_convertible<U&&, T>::value
                                                              )) noexcept(std::is_nothrow_move_constructible<U>::value)
      : contained_(true)
  {
    contained_.ConstructValue(std::forward<U>(value));
  }

  // clang-format on

  // Always implicit by design
  ETCPAL_CONSTEXPR_14 Expected(etcpal_error_t error);
  ~Expected();

  constexpr const T*  operator->() const;
  T*                  operator->();
  constexpr const T&  operator*() const&;
  T&                  operator*() &;
  constexpr const T&& operator*() const&&;
  ETCPAL_CONSTEXPR_14 T&&   operator*() &&;
  constexpr explicit        operator bool() const noexcept;
  constexpr bool            has_value() const noexcept;
  ETCPAL_CONSTEXPR_14 const T& value() const&;
  T&                           value() &;
  ETCPAL_CONSTEXPR_14 const T&& value() const&&;
  ETCPAL_CONSTEXPR_14 T&&  value() &&;
  constexpr etcpal_error_t error_code() const noexcept;
  constexpr Error          error() const noexcept;

  // clang-format off

  /// @brief Get the value, or a default value if this Expected contains an error.
  /// @param def_val The default value to return on error - must be convertible to T.
  template <typename U,
            ETCPAL_ENABLE_IF_TEMPLATE(std::is_copy_constructible<T>::value && std::is_convertible<U&&, T>::value)>
  constexpr T value_or(U&& def_val) const&
  {
    return has_value() ? contained_.value() : static_cast<T>(std::forward<U>(def_val));
  }

  /// @brief Get the value, or a default value if this Expected contains an error.
  /// @param def_val The default value to return on error - must be convertible to T.
  template <typename U,
            ETCPAL_ENABLE_IF_TEMPLATE(std::is_move_constructible<T>::value && std::is_convertible<U&&, T>::value)>
  T value_or(U&& def_val) &&
  {
    return has_value() ? contained_.value() : static_cast<T>(std::forward<U>(def_val));
  }

  // clang-format on

private:
  detail::ExpectedStorage<T, std::is_copy_constructible<T>::value, std::is_move_constructible<T>::value> contained_;
};  // namespace etcpal

/// @brief Construct an Expected instance containing an error code.
template <typename T>
ETCPAL_CONSTEXPR_14 Expected<T>::Expected(etcpal_error_t error) : contained_(false)
{
  contained_.SetError(error);
}

/// @brief Calls the contained value's destructor if and only if has_value() is true.
template <typename T>
Expected<T>::~Expected()
{
  if (has_value())
    contained_.DestructValue();
}

/// @brief Access members of a composite contained value.
///
/// If has_value() is false, the behavior is undefined.
template <typename T>
constexpr const T* Expected<T>::operator->() const
{
  // Comma syntax is to satisfy C++11 requirements for constexpr
  return assert(has_value()), contained_.value_ptr();
}

/// @brief Access members of a composite contained value.
///
/// If has_value() is false, the behavior is undefined.
template <typename T>
T* Expected<T>::operator->()
{
  // Comma syntax is to satisfy C++11 requirements for constexpr
  return assert(has_value()), contained_.value_ptr();
}

/// @brief Get the underlying value.
///
/// If has_value() is false, the behavior is undefined.
template <typename T>
constexpr const T& Expected<T>::operator*() const&
{
  // Comma syntax is to satisfy C++11 requirements for constexpr
  return assert(has_value()), contained_.value();
}

/// @brief Get the underlying value.
///
/// If has_value() is false, the behavior is undefined.
template <typename T>
T& Expected<T>::operator*() &
{
  // Comma syntax is to satisfy C++11 requirements for constexpr
  return assert(has_value()), contained_.value();
}

/// @brief Get the underlying value.
///
/// If has_value() is false, the behavior is undefined.
template <typename T>
constexpr const T&& Expected<T>::operator*() const&&
{
  // Comma syntax is to satisfy C++11 requirements for constexpr
  return assert(has_value()), std::move(contained_.value());
}

/// @brief Get the underlying value.
///
/// If has_value() is false, the behavior is undefined.
template <typename T>
ETCPAL_CONSTEXPR_14 T&& Expected<T>::operator*() &&
{
  // Comma syntax is to satisfy C++11 requirements for constexpr
  return assert(has_value()), std::move(contained_.value());
}

/// @brief Evaluate the Expected instance inline - evaluates to has_value().
template <typename T>
constexpr Expected<T>::operator bool() const noexcept
{
  return has_value();
}

/// @brief Whether this Expected instance contains a valid value. If not, contains an error code.
template <typename T>
constexpr bool Expected<T>::has_value() const noexcept
{
  return contained_.has_value();
}

/// @brief Get the underlying value.
/// @throw BadExpectedAccess if has_value() is false.
template <typename T>
ETCPAL_CONSTEXPR_14 const T& Expected<T>::value() const&
{
  if (!has_value())
    ETCPAL_THROW(BadExpectedAccess(contained_.error()));
  return contained_.value();
}

/// @brief Get the underlying value.
/// @throw BadExpectedAccess if has_value() is false.
template <typename T>
T& Expected<T>::value() &
{
  if (!has_value())
    ETCPAL_THROW(BadExpectedAccess(contained_.error()));
  return contained_.value();
}

/// @brief Get the underlying value.
/// @throw BadExpectedAccess if has_value() is false.
template <typename T>
ETCPAL_CONSTEXPR_14 const T&& Expected<T>::value() const&&
{
  if (!has_value())
    ETCPAL_THROW(BadExpectedAccess(contained_.error()));
  return std::move(contained_.value());
}

/// @brief Get the underlying value.
/// @throw BadExpectedAccess if has_value() is false.
template <typename T>
ETCPAL_CONSTEXPR_14 T&& Expected<T>::value() &&
{
  if (!has_value())
    ETCPAL_THROW(BadExpectedAccess(contained_.error()));
  return std::move(contained_.value());
}

/// @brief Get the error code.
///
/// If has_value() is true, the behavior is undefined.
template <typename T>
constexpr etcpal_error_t Expected<T>::error_code() const noexcept
{
  // Comma syntax is to satisfy C++11 requirements for constexpr
  return assert(!has_value()), contained_.error();
}

/// @brief Get the error code as a Error object.
///
/// If has_value() is true, the behavior is undefined.
template <typename T>
constexpr Error Expected<T>::error() const noexcept
{
  // Comma syntax is to satisfy C++11 requirements for constexpr
  return assert(!has_value()), contained_.error();
}

/// @addtogroup etcpal_cpp_error
/// @{

/// @name Expected Relational Operators
/// @{

template <typename T1, typename T2>
constexpr bool operator==(const Expected<T1>& x, const Expected<T2>& y)
{
  return bool(x) != bool(y) ? false : bool(x) == false ? x.error_code() == y.error_code() : *x == *y;
}

template <typename T1, typename T2>
constexpr bool operator!=(const Expected<T1>& x, const Expected<T2>& y)
{
  return !(x == y);
}

template <typename T1, typename T2>
constexpr bool operator==(const Expected<T1>& x, const T2& v)
{
  return bool(x) ? *x == v : false;
}

template <typename T1, typename T2>
constexpr bool operator==(const T2& v, const Expected<T1>& x)
{
  return bool(x) ? v == *x : false;
}

template <typename T1, typename T2>
constexpr bool operator!=(const Expected<T1>& x, const T2& v)
{
  return bool(x) ? *x != v : true;
}

template <typename T1, typename T2>
constexpr bool operator!=(const T2& v, const Expected<T1>& x)
{
  return bool(x) ? v != *x : true;
}

template <typename T>
constexpr bool operator==(const Expected<T>& x, etcpal_error_t e)
{
  return (!x) ? x.error_code() == e : false;
}

template <typename T>
constexpr bool operator==(etcpal_error_t e, const Expected<T>& x)
{
  return (x == e);
}

template <typename T>
constexpr bool operator!=(const Expected<T>& x, etcpal_error_t e)
{
  return !(x == e);
}

template <typename T>
constexpr bool operator!=(etcpal_error_t e, const Expected<T>& x)
{
  return !(x == e);
}

/// @}
/// @}

};  // namespace etcpal

#endif  // ETCPAL_CPP_ERROR_H_
