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

#include "etcpal/cpp/opaque_id.h"
#include "unity_fixture.h"

#include <climits>
#include <cstdint>
#include <set>
#include <unordered_set>

struct TestIntIdType
{
};
using TestIntId = etcpal::OpaqueId<TestIntIdType, int, -1>;

// Traits should be evaluable at compile time
static_assert(!TestIntId().IsValid(), "Default constructor and IsValid() should work in a constant expression");
static_assert(!TestIntId(), "operator! should work in a constant expression");
static_assert(TestIntId().value() == -1, "Value getter should work in a constant expression");
static_assert(TestIntId(4), "Value constructor and operator bool should work in a constant expression");
static_assert(TestIntId(4).value() == 4, "Value getter should work in a constant expression");

struct TestUint8IdType
{
};
using TestUint8Id = etcpal::OpaqueId<TestUint8IdType, uint8_t, UINT8_MAX>;

extern "C" {
TEST_GROUP(etcpal_cpp_opaque_id);

TEST_SETUP(etcpal_cpp_opaque_id)
{
}

TEST_TEAR_DOWN(etcpal_cpp_opaque_id)
{
}

TEST(etcpal_cpp_opaque_id, default_constructor_works)
{
  TestIntId id;
  TEST_ASSERT_FALSE(id);
  TEST_ASSERT_FALSE(id.IsValid());
  TEST_ASSERT_TRUE(!id);
  TEST_ASSERT_EQUAL(id.value(), -1);
}

TEST(etcpal_cpp_opaque_id, validators_work)
{
  TestIntId id(5);
  TEST_ASSERT_TRUE(id);
  TEST_ASSERT_TRUE(id.IsValid());
  TEST_ASSERT_FALSE(!id);
  TEST_ASSERT_EQUAL(id.value(), 5);
}

TEST(etcpal_cpp_opaque_id, copy_constructor_works)
{
  TestIntId a(4);

  TestIntId b(a);
  TEST_ASSERT_EQUAL_INT(b.value(), 4);
}

TEST(etcpal_cpp_opaque_id, set_value_works)
{
  TestIntId id;
  TEST_ASSERT_FALSE(id.IsValid());
  id.SetValue(10);
  TEST_ASSERT_TRUE(id.IsValid());
  TEST_ASSERT_EQUAL(id.value(), 10);
}

TEST(etcpal_cpp_opaque_id, clear_works)
{
  TestIntId id(10);
  TEST_ASSERT_TRUE(id.IsValid());
  id.Clear();
  TEST_ASSERT_FALSE(id.IsValid());
  TEST_ASSERT_EQUAL(id.value(), -1);
}

TEST(etcpal_cpp_opaque_id, std_set_works)
{
  std::set<TestIntId> ids;
  ids.insert(TestIntId());
  ids.insert(TestIntId(1));

  TEST_ASSERT_EQUAL(ids.size(), 2u);

  auto set_it = ids.begin();
  TEST_ASSERT_EQUAL(set_it->value(), -1);
  ++set_it;
  TEST_ASSERT_EQUAL(set_it->value(), 1);
}

TEST(etcpal_cpp_opaque_id, std_hash_works)
{
  std::unordered_set<TestIntId> ids;
  ids.insert(TestIntId());
  ids.insert(TestIntId(1));

  TEST_ASSERT_EQUAL(ids.size(), 2u);
}

TEST(etcpal_cpp_opaque_id, uint8_id_works)
{
  TestUint8Id id;
  TEST_ASSERT_FALSE(id.IsValid());
  TEST_ASSERT_EQUAL(id.value(), UINT8_MAX);

  TestUint8Id id2(0);
  TEST_ASSERT_TRUE(id2.IsValid());
  TEST_ASSERT_EQUAL(id2.value(), 0u);
}

TEST_GROUP_RUNNER(etcpal_cpp_opaque_id)
{
  RUN_TEST_CASE(etcpal_cpp_opaque_id, default_constructor_works);
  RUN_TEST_CASE(etcpal_cpp_opaque_id, validators_work);
  RUN_TEST_CASE(etcpal_cpp_opaque_id, copy_constructor_works);
  RUN_TEST_CASE(etcpal_cpp_opaque_id, set_value_works);
  RUN_TEST_CASE(etcpal_cpp_opaque_id, clear_works);
  RUN_TEST_CASE(etcpal_cpp_opaque_id, std_set_works);
  RUN_TEST_CASE(etcpal_cpp_opaque_id, std_hash_works);
  RUN_TEST_CASE(etcpal_cpp_opaque_id, uint8_id_works);
}
}
