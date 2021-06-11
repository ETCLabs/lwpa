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

#include "etcpal/cpp/error.h"

#include <memory>
#include <stdexcept>
#include <type_traits>
#include "etcpal/common.h"
#include "unity_fixture.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#pragma clang diagnostic ignored "-Wpessimizing-move"
#endif

extern "C" {

TEST_GROUP(etcpal_result);

TEST_SETUP(etcpal_result)
{
}

TEST_TEAR_DOWN(etcpal_result)
{
}

TEST(etcpal_result, constructors_function_correctly)
{
  // Explicit constructor from "OK" error code
  etcpal::Error exp_ok(kEtcPalErrOk);
  TEST_ASSERT_TRUE(exp_ok);
  TEST_ASSERT_TRUE(exp_ok.IsOk());
  TEST_ASSERT_EQUAL(exp_ok.code(), kEtcPalErrOk);
  TEST_ASSERT_NOT_NULL(exp_ok.ToCString());
  TEST_ASSERT_FALSE(exp_ok.ToString().empty());

  // Explicit constructor from non-"OK" error code
  etcpal::Error exp_not_ok(kEtcPalErrSys);
  TEST_ASSERT_FALSE(exp_not_ok);
  TEST_ASSERT_FALSE(exp_not_ok.IsOk());
  TEST_ASSERT_EQUAL(exp_not_ok.code(), kEtcPalErrSys);
  TEST_ASSERT_NOT_NULL(exp_not_ok.ToCString());
  TEST_ASSERT_FALSE(exp_not_ok.ToString().empty());
}

TEST(etcpal_result, static_ok_works)
{
  auto result = etcpal::Error::Ok();
  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_EQUAL(result.code(), kEtcPalErrOk);

  etcpal::Error result_2(kEtcPalErrOk);
  TEST_ASSERT(result_2 == result);
}

TEST(etcpal_result, assignment_operators)
{
  etcpal::Error result = kEtcPalErrOk;
  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_EQUAL(result.code(), kEtcPalErrOk);

  result = kEtcPalErrSys;
  TEST_ASSERT_FALSE(result);
  TEST_ASSERT_EQUAL(result.code(), kEtcPalErrSys);

  auto result_2 = result;
  TEST_ASSERT_FALSE(result_2);
  TEST_ASSERT_EQUAL(result.code(), result_2.code());
  TEST_ASSERT_EQUAL_STRING(result.ToCString(), result_2.ToCString());
  TEST_ASSERT(result.ToString() == result_2.ToString());
}

TEST(etcpal_result, equality_operators)
{
  etcpal::Error result(kEtcPalErrOk);
  TEST_ASSERT(result == kEtcPalErrOk);
  TEST_ASSERT(kEtcPalErrOk == result);

  etcpal::Error result_2(kEtcPalErrOk);
  TEST_ASSERT(result_2 == result);
  TEST_ASSERT(result == result_2);

  etcpal::Error result_3(kEtcPalErrSys);
  TEST_ASSERT_UNLESS(result_3 == result);
  TEST_ASSERT_UNLESS(result == result_3);
}

TEST(etcpal_result, strings_exist_for_all_codes)
{
  for (int i = 0; i > 0 - ETCPAL_NUM_ERROR_CODES; --i)
  {
    etcpal::Error result(static_cast<etcpal_error_t>(i));
    TEST_ASSERT_UNLESS_MESSAGE(result.ToString().empty(),
                               std::string("Failed on iteration " + std::to_string(i)).c_str());
    TEST_ASSERT_NOT_NULL_MESSAGE(result.ToCString(), std::string("Failed on iteration " + std::to_string(i)).c_str());
  }
}

TEST_GROUP(etcpal_expected);

TEST_SETUP(etcpal_expected)
{
}

TEST_TEAR_DOWN(etcpal_expected)
{
}

// Static assertions for etcpal::Expected

// Storage overhead
struct ExpectedDummy
{
  bool has_value;
  union
  {
    int            value;
    etcpal_error_t err;
  };
};
static_assert(sizeof(etcpal::Expected<int>) == sizeof(ExpectedDummy),
              "Expected should not have more storage overhead than its type plus a bool");

class RuleOfZero
{
private:
  int val_{20};
};

static_assert(std::is_default_constructible<etcpal::Expected<RuleOfZero>>::value,
              "Expected should be default-constructible with a default-constructible class");
static_assert(std::is_copy_constructible<etcpal::Expected<RuleOfZero>>::value,
              "Expected should be copy-constructible with a copy-constructible class");
static_assert(std::is_move_constructible<etcpal::Expected<RuleOfZero>>::value,
              "Expected should be move-constructible with a move-constructible class");

// Constructors
class NoDefaultConstructor
{
public:
  NoDefaultConstructor() = delete;
  NoDefaultConstructor(int val) : val_(val) {}

private:
  int val_;
};

static_assert(!std::is_default_constructible<etcpal::Expected<NoDefaultConstructor>>::value,
              "Expected should not be default-constructible with a non-default-constructible class");

class NoCopyConstructor  // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
  NoCopyConstructor(const NoCopyConstructor& other) = delete;
  NoCopyConstructor(NoCopyConstructor&& other) = default;

private:
  int val_;
};

static_assert(!std::is_copy_constructible<etcpal::Expected<NoCopyConstructor>>::value,
              "Expected should not be copy-constructible with a non-copy-constructible class");
static_assert(std::is_move_constructible<etcpal::Expected<NoCopyConstructor>>::value,
              "Expected should be move-constructible with a move-constructible class");

class NoMoveConstructor  // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
  NoMoveConstructor() = default;
  NoMoveConstructor(const NoMoveConstructor& other) = default;
  NoMoveConstructor(NoMoveConstructor&& other) = delete;

private:
  int val_;
};

// TODO this does not currently work
// Reason seems to be related to https://github.com/martinmoene/expected-lite/issues/32
// Keep an eye on this issue for possible resolution

// static_assert(std::is_copy_constructible<etcpal::Expected<NoMoveConstructor>>::value,
//               "Expected should be copy-constructible with a copy-constructible class");
// static_assert(!std::is_move_constructible<etcpal::Expected<NoMoveConstructor>>::value,
//               "Expected should not be move-constructible with a non-move-constructible class");

TEST(etcpal_expected, default_constructor_default_init)
{
  etcpal::Expected<int> v;
  TEST_ASSERT_EQUAL_INT(v.value(), 0);

  etcpal::Expected<void*>      v2;
  etcpal::Expected<RuleOfZero> v3;

  // etcpal::Expected<NoMoveConstructor> e;
}

TEST(etcpal_expected, default_constructor_is_called)
{
  static bool constructor_ran;

  class ConstructorWithSideEffect
  {
  public:
    ConstructorWithSideEffect() { constructor_ran = true; }

  private:
    int i_{42};
  };

  constructor_ran = false;
  etcpal::Expected<ConstructorWithSideEffect> v;
  TEST_ASSERT_TRUE(constructor_ran);
}

TEST(etcpal_expected, copy_constructor_works)
{
  etcpal::Expected<int> v1{20};
  etcpal::Expected<int> v2{v1};
  TEST_ASSERT_TRUE(v2.has_value());
  TEST_ASSERT_EQUAL_INT(v2.value(), 20);
}

TEST(etcpal_expected, move_constructor_works)
{
  struct Foo
  {
    int bar{30};
  };

  etcpal::Expected<std::unique_ptr<Foo>> v1 = std::unique_ptr<Foo>(new Foo);
  etcpal::Expected<std::unique_ptr<Foo>> v2{std::move(v1)};
  TEST_ASSERT_TRUE(v2.has_value());
  TEST_ASSERT_EQUAL_INT(v2.value()->bar, 30);
}

TEST(etcpal_expected, conversion_copy_constructor_works)
{
  // Implicit conversion
  etcpal::Expected<int> v = etcpal::Expected<unsigned int>(40u);
  TEST_ASSERT_TRUE(v.has_value());
  TEST_ASSERT_EQUAL_INT(v.value(), 40);

  etcpal::Expected<int> v2 = etcpal::Expected<unsigned int>(kEtcPalErrAlready);
  TEST_ASSERT_FALSE(v2.has_value());
  TEST_ASSERT_EQUAL(v2.error_code(), kEtcPalErrAlready);

  // Explicit conversion
  class ExplicitFromInt
  {
  public:
    explicit ExplicitFromInt(int val) : val_(val) {}
    int number() const noexcept { return val_; }

  private:
    int val_{};
  };

  etcpal::Expected<ExplicitFromInt> v3{etcpal::Expected<int>(50)};
  TEST_ASSERT_TRUE(v3.has_value());
  TEST_ASSERT_EQUAL_INT(v3.value().number(), 50);
}

TEST(etcpal_expected, conversion_move_constructor_works)
{
  etcpal::Expected<int> v = std::move(etcpal::Expected<unsigned int>(40u));
  TEST_ASSERT_TRUE(v.has_value());
  TEST_ASSERT_EQUAL_INT(v.value(), 40);

  etcpal::Expected<int> v2 = std::move(etcpal::Expected<unsigned int>(kEtcPalErrAlready));
  TEST_ASSERT_FALSE(v2.has_value());
  TEST_ASSERT_EQUAL(v2.error_code(), kEtcPalErrAlready);

  // Explicit conversion
  class ExplicitFromInt
  {
  public:
    explicit ExplicitFromInt(int val) : val_(val) {}
    int number() const noexcept { return val_; }

  private:
    int val_{};
  };

  etcpal::Expected<ExplicitFromInt> v3{std::move(etcpal::Expected<int>(50))};
  TEST_ASSERT_TRUE(v3.has_value());
  TEST_ASSERT_EQUAL_INT(v3.value().number(), 50);
}

TEST(etcpal_expected, has_value_is_correct)
{
  etcpal::Expected<int> v = 20;
  TEST_ASSERT_TRUE(v.has_value());
  TEST_ASSERT_TRUE(v);

  etcpal::Expected<int> e = kEtcPalErrAddrInUse;
  TEST_ASSERT_FALSE(e.has_value());
  TEST_ASSERT_FALSE(e);
}

TEST(etcpal_expected, observers_are_correct)
{
  etcpal::Expected<int> v = 20;
  TEST_ASSERT_EQUAL_INT(v.value(), 20);
  TEST_ASSERT_EQUAL_INT(*v, 20);

  struct Foo
  {
    int val{20};
  };
  etcpal::Expected<Foo> v2;
  TEST_ASSERT_EQUAL_INT(v2.value().val, 20);
  TEST_ASSERT_EQUAL_INT((*v2).val, 20);
  TEST_ASSERT_EQUAL_INT(v2->val, 20);
}

TEST(etcpal_expected, error_is_correct)
{
  etcpal::Expected<int> e = kEtcPalErrNoMem;
  TEST_ASSERT_EQUAL(e.error_code(), kEtcPalErrNoMem);
  TEST_ASSERT_EQUAL(e.error().code(), kEtcPalErrNoMem);
}

#if ETCPAL_BUILDING_WITH_EXCEPTIONS
TEST(etcpal_expected, value_throws_on_error)
{
  try
  {
    int val = etcpal::Expected<int>(kEtcPalErrSys).value();
    (void)val;
    TEST_FAIL_MESSAGE("This block should not be entered");
  }
  catch (const etcpal::BadExpectedAccess& e)
  {
    TEST_ASSERT_EQUAL_INT(e.error().code(), kEtcPalErrSys);
  }
}
#endif

TEST(etcpal_expected, with_error_constructor_destructor_not_called)
{
  class DoNotConstructOrDestruct  // NOLINT(cppcoreguidelines-special-member-functions)
  {
  public:
    DoNotConstructOrDestruct() { TEST_FAIL(); }
    ~DoNotConstructOrDestruct() { TEST_FAIL(); }
  };

  etcpal::Expected<DoNotConstructOrDestruct> e{kEtcPalErrSys};
}

TEST(etcpal_expected, value_or_works)
{
  etcpal::Expected<int> v{20};
  TEST_ASSERT_EQUAL_INT(v.value_or(30), 20);

  etcpal::Expected<int> e{kEtcPalErrBusy};
  TEST_ASSERT_EQUAL_INT(v.value_or(20), 20);

  etcpal::Expected<std::string> s{kEtcPalErrSys};
  TEST_ASSERT_EQUAL_STRING(s.value_or("Default string").c_str(), "Default string");
}

TEST(etcpal_expected, relational_operators_work)
{
  etcpal::Expected<int> v{10};
  etcpal::Expected<int> v_equal{10};
  etcpal::Expected<int> v_not_equal{20};
  etcpal::Expected<int> e{kEtcPalErrInvalid};
  etcpal::Expected<int> e_equal{kEtcPalErrInvalid};
  etcpal::Expected<int> e_not_equal{kEtcPalErrSys};

  // Comparison with values
  TEST_ASSERT_TRUE(v == v_equal);
  TEST_ASSERT_FALSE(v != v_equal);
  TEST_ASSERT_TRUE(v != v_not_equal);
  TEST_ASSERT_FALSE(v == v_not_equal);
  // Comparison with T
  TEST_ASSERT_TRUE(v == 10);
  TEST_ASSERT_FALSE(v != 10);
  TEST_ASSERT_FALSE(v == 20);
  TEST_ASSERT_TRUE(v != 20);
  // Comparison between value and error
  TEST_ASSERT_TRUE(v != e);
  TEST_ASSERT_FALSE(v == e);
  // Comparison between errors
  TEST_ASSERT_TRUE(e == e_equal);
  TEST_ASSERT_FALSE(e != e_equal);
  TEST_ASSERT_TRUE(e != e_not_equal);
  TEST_ASSERT_FALSE(e == e_not_equal);
  // Comparison with etcpal_error_t
  TEST_ASSERT_TRUE(e == kEtcPalErrInvalid);
  TEST_ASSERT_FALSE(e != kEtcPalErrInvalid);
  TEST_ASSERT_TRUE(e != kEtcPalErrSys);
  TEST_ASSERT_FALSE(e == kEtcPalErrSys);
}

TEST_GROUP_RUNNER(etcpal_cpp_error)
{
  RUN_TEST_CASE(etcpal_result, constructors_function_correctly);
  RUN_TEST_CASE(etcpal_result, static_ok_works);
  RUN_TEST_CASE(etcpal_result, assignment_operators);
  RUN_TEST_CASE(etcpal_result, equality_operators);
  RUN_TEST_CASE(etcpal_result, strings_exist_for_all_codes);
  RUN_TEST_CASE(etcpal_expected, default_constructor_default_init);
  RUN_TEST_CASE(etcpal_expected, default_constructor_is_called);
  RUN_TEST_CASE(etcpal_expected, copy_constructor_works);
  RUN_TEST_CASE(etcpal_expected, move_constructor_works);
  RUN_TEST_CASE(etcpal_expected, conversion_copy_constructor_works);
  RUN_TEST_CASE(etcpal_expected, conversion_move_constructor_works);
  RUN_TEST_CASE(etcpal_expected, has_value_is_correct);
  RUN_TEST_CASE(etcpal_expected, observers_are_correct);
  RUN_TEST_CASE(etcpal_expected, error_is_correct);
#if ETCPAL_BUILDING_WITH_EXCEPTIONS
  RUN_TEST_CASE(etcpal_expected, value_throws_on_error);
#endif
  RUN_TEST_CASE(etcpal_expected, with_error_constructor_destructor_not_called);
  RUN_TEST_CASE(etcpal_expected, value_or_works);
  RUN_TEST_CASE(etcpal_expected, relational_operators_work)
}
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
