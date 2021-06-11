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

/// @file etcpal/cpp/opaque_id.h
/// @brief Provides the OpaqueId template type and function definitions.

#ifndef ETCPAL_CPP_OPAQUE_ID_H_
#define ETCPAL_CPP_OPAQUE_ID_H_

#include <functional>
#include "etcpal/cpp/common.h"

namespace etcpal
{
/// @defgroup etcpal_cpp_opaque_id opaque_id (Strongly typed handles)
/// @ingroup etcpal_cpp
/// @brief Provides the OpaqueId type, a type-safe identifier or handle value.

/// @ingroup etcpal_cpp_opaque_id
/// @brief A strongly-typed ID with arbitrary internal representation.
///
/// A typical coding pattern uses handles to represent resources stored elsewhere by a module. The
/// handles are usually some integer type, given another name by a type definition, e.g.:
///
/// @code
/// using FooHandle = int;
///
/// Foo GetFoo(FooHandle handle);
/// @endcode
///
/// Unfortunately, this is not as type-safe as it could be. One would expect that handles cannot be
/// mixed with other integers or magic numbers:
///
/// @code
/// int regular_ol_number = 12;
/// GetFoo(regular_ol_number);
/// GetFoo(42);
/// @endcode
///
/// Although handles are typically meant to be treated as opaque (and thus should not be mixable
/// with regular numbers), the block above will compile without warnings.
///
/// This can be avoided using the OpaqueId class. Usage is as follows:
/// @code
/// struct FooHandleType {};
/// using FooHandle = etcpal::OpaqueId<FooHandleType, int, -1>;
///
/// Foo GetFoo(FooHandle handle);
///
/// int regular_ol_number = 12;
/// GetFoo(regular_ol_number); // Error
/// GetFoo(42); // Error
/// GetFoo(FooHandle(42)); // No error
///
/// FooHandle foo_handle(42);
/// if (foo_handle == 42) // Error
/// {
///   // ...
/// }
///
/// if (foo_handle.value() == 42) // No error
/// {
///   // ...
/// }
///
/// struct BarHandleType {};
/// using BarHandle = etcpal::OpaqueId<BarHandleType, int, -1>;
///
/// BarHandle bar_handle(42);
/// if (bar_handle == foo_handle) // Error
/// {
///   // ...
/// }
/// @endcode
///
/// When using the OpaqueId type, errors from mixing numbers with opaque handles are avoided. Each
/// distinct specialization of an OpaqueId requires a new struct or class to be defined. This
/// struct/class is typically empty and its only use is to serve as a distinct type such that
/// OpaqueIds used for different purposes cannot be mixed with each other.
///
/// The underlying type for an OpaqueId follows the template rules for a
/// [non-type template parameter](https://en.cppreference.com/w/cpp/language/template_parameters),
/// since an "invalid" value must be passed as a template parameter when specializing OpaqueId.
/// This means that in pre-C++20 environments, the underlying type can pretty much only be an
/// integral or enumeration type. In practice, it is almost always an integer type.
///
/// OpaqueIds also have the concept of an invalid value, which is what a default-constructed
/// OpaqueId contains.
///
/// @code
/// FooHandle handle;
/// if (!handle) // Evaluation in boolean predicate OK
/// {
///   // This block will be executed
/// }
///
/// EXPECT_FALSE(handle.IsValid());
/// EXPECT_EQ(handle, FooHandle::Invalid());
/// @endcode
template <class IdType, class ValueType, ValueType InvalidValue>
class OpaqueId
{
public:
  OpaqueId() = default;
  constexpr explicit OpaqueId(ValueType value);

  static constexpr OpaqueId Invalid();

  constexpr ValueType value() const;
  constexpr bool      IsValid() const;
  constexpr explicit  operator bool() const;
  constexpr bool      operator!() const;

  ETCPAL_CONSTEXPR_14 void SetValue(const ValueType& new_value);
  ETCPAL_CONSTEXPR_14 void Clear();

private:
  ValueType value_{InvalidValue};
};

/// @brief Explicitly create an invalid ID.
///
/// Example:
/// @code
/// using FooId = etcpal::OpaqueId<FooIdType, int, -1>;
/// auto invalid_id = FooId::Invalid();
/// EXPECT_FALSE(invalid_id.IsValid());
/// @endcode
template <class IdType, class ValueType, ValueType InvalidValue>
constexpr OpaqueId<IdType, ValueType, InvalidValue> OpaqueId<IdType, ValueType, InvalidValue>::Invalid()
{
  return OpaqueId();
}

/// @brief Explicitly create a valid ID based on a value.
template <class IdType, class ValueType, ValueType InvalidValue>
constexpr OpaqueId<IdType, ValueType, InvalidValue>::OpaqueId(ValueType value) : value_(value)
{
}

/// @brief Get the underlying value from an ID.
template <class IdType, class ValueType, ValueType InvalidValue>
constexpr ValueType OpaqueId<IdType, ValueType, InvalidValue>::value() const
{
  return value_;
}

/// @brief Explicitly determine whether an ID is valid.
template <class IdType, class ValueType, ValueType InvalidValue>
constexpr bool OpaqueId<IdType, ValueType, InvalidValue>::IsValid() const
{
  return !(value_ == InvalidValue);
}

/// @brief Determine whether an ID is valid by testing as a boolean predicate.
///
/// Example:
/// @code
/// if (id)
/// {
///   // Do things if id is valid
/// }
/// else
/// {
///   // Do things if id is invalid
/// }
/// @endcode
template <class IdType, class ValueType, ValueType InvalidValue>
constexpr OpaqueId<IdType, ValueType, InvalidValue>::operator bool() const
{
  return IsValid();
}

/// @brief Determine whether an ID is not valid.
template <class IdType, class ValueType, ValueType InvalidValue>
constexpr bool OpaqueId<IdType, ValueType, InvalidValue>::operator!() const
{
  return !IsValid();
}

/// @brief Set a new underlying value for an ID.
/// @param new_value The new value to set.
template <class IdType, class ValueType, ValueType InvalidValue>
ETCPAL_CONSTEXPR_14_OR_INLINE void OpaqueId<IdType, ValueType, InvalidValue>::SetValue(const ValueType& new_value)
{
  value_ = new_value;
}

/// @brief Clear any valid value from an ID and set it back to the invalid value.
/// @post value.IsValid() will return false.
template <class IdType, class ValueType, ValueType InvalidValue>
ETCPAL_CONSTEXPR_14_OR_INLINE void OpaqueId<IdType, ValueType, InvalidValue>::Clear()
{
  value_ = InvalidValue;
}

/// @addtogroup etcpal_cpp_opaque_id
/// @{

/// @name OpaqueId relational operators
/// @{

template <class IdType, class ValueType, ValueType InvalidValue>
constexpr bool operator==(const OpaqueId<IdType, ValueType, InvalidValue>& a,
                          const OpaqueId<IdType, ValueType, InvalidValue>& b)
{
  return a.value() == b.value();
}

template <class IdType, class ValueType, ValueType InvalidValue>
constexpr bool operator!=(const OpaqueId<IdType, ValueType, InvalidValue>& a,
                          const OpaqueId<IdType, ValueType, InvalidValue>& b)
{
  return !(a == b);
}

/// @}
/// @}

};  // namespace etcpal

/// @cond std namespace specializations

namespace std
{
// Inject a std::less specialization for any template specialization for OpaqueId. This takes the
// place of operator< for the purposes of OpaqueId usage in ordered containers. We don't provide
// relational operators other than == and != to avoid the temptation to treat OpaqueIds like
// numbers.
template <class IdType, class ValueType, ValueType InvalidValue>
struct less<etcpal::OpaqueId<IdType, ValueType, InvalidValue>>
{
  ETCPAL_CONSTEXPR_14 bool operator()(const etcpal::OpaqueId<IdType, ValueType, InvalidValue>& a,
                                      const etcpal::OpaqueId<IdType, ValueType, InvalidValue>& b) const
  {
    return a.value() < b.value();
  }
};

// Inject a new std::hash specialization for any template specialization of OpaqueId, so that
// OpaqueIds can be used in hash-based containers (e.g. unordered_map and unordered_set) without
// a user needing to create a hash specialization.
//
// The std::hash specialization simply passes through to a hash of the underlying value. If the
// underlying value is not hashable, this specialization should simply be eliminated without a
// compile error through SFINAE.
template <class IdType, class ValueType, ValueType InvalidValue>
struct hash<etcpal::OpaqueId<IdType, ValueType, InvalidValue>>
{
  std::size_t operator()(const etcpal::OpaqueId<IdType, ValueType, InvalidValue>& id) const noexcept
  {
    return std::hash<ValueType>()(id.value());
  }
};
};  // namespace std

/// @endcond

#endif  // ETCPAL_CPP_OPAQUE_ID_H_
