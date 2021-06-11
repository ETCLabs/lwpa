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

#include "etcpal/cpp/uuid.h"
#include "unity_fixture.h"

extern "C" {
TEST_GROUP(etcpal_cpp_uuid);

TEST_SETUP(etcpal_cpp_uuid)
{
}

TEST_TEAR_DOWN(etcpal_cpp_uuid)
{
}

TEST(etcpal_cpp_uuid, default_constructor_works)
{
  const etcpal::Uuid uuid;
  TEST_ASSERT_TRUE(uuid.IsNull());
}

TEST(etcpal_cpp_uuid, data_constructor_works)
{
  const std::array<uint8_t, 16> uuid_data{0x00, 0x00, 0x11, 0x11, 0x22, 0x22, 0x33, 0x33,
                                          0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb};
  const etcpal::Uuid            uuid(uuid_data.data());

  TEST_ASSERT_EQUAL_UINT8_ARRAY(uuid.data(), uuid_data.data(), 16);
  TEST_ASSERT_EQUAL_UINT32(uuid.time_low(), 0x00001111);
  TEST_ASSERT_EQUAL_UINT32(uuid.time_mid(), 0x2222);
  TEST_ASSERT_EQUAL_UINT32(uuid.time_hi_and_version(), 0x3333);
  TEST_ASSERT_TRUE(
      (uuid.clock_seq_and_node() == std::array<uint8_t, 8>{0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb}));
}

#define UUID_INITIALIZER                                 \
  {                                                      \
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 \
  }

TEST(etcpal_cpp_uuid, copy_constructors_work)
{
  // Copy from an EtcPalUuid
  const EtcPalUuid   c_uuid{UUID_INITIALIZER};
  const etcpal::Uuid uuid1(c_uuid);
  TEST_ASSERT_TRUE(uuid1.get() == c_uuid);

  // Construct through an EtcPalUuid implicitly
  const etcpal::Uuid uuid2(UUID_INITIALIZER);
  TEST_ASSERT_TRUE(uuid1.get() == uuid2.get());
  TEST_ASSERT_TRUE(uuid1 == uuid2);

  // Use the default copy constructor
  const etcpal::Uuid uuid3(uuid2);
  TEST_ASSERT_TRUE(uuid3.get() == uuid2.get());
  TEST_ASSERT_TRUE(uuid3 == uuid2);
}

TEST(etcpal_cpp_uuid, assignment_operators_work)
{
  etcpal::Uuid uuid;
  EtcPalUuid   c_uuid{UUID_INITIALIZER};

  // Assign to an EtcPalUuid
  uuid = c_uuid;
  TEST_ASSERT_TRUE(uuid.get() == c_uuid);
  TEST_ASSERT_FALSE(uuid.IsNull());

  // Test the default assignment operator
  const etcpal::Uuid null_uuid;
  uuid = null_uuid;
  TEST_ASSERT_TRUE(uuid == null_uuid);
  TEST_ASSERT_TRUE(uuid.get() == null_uuid.get());
  TEST_ASSERT_TRUE(uuid.IsNull());
}

TEST(etcpal_cpp_uuid, to_string_works)
{
  const etcpal::Uuid uuid(UUID_INITIALIZER);
  TEST_ASSERT_TRUE(uuid.ToString() == "00010203-0405-0607-0809-0a0b0c0d0e0f");

  const etcpal::Uuid null_uuid;
  TEST_ASSERT_TRUE(null_uuid.ToString() == "00000000-0000-0000-0000-000000000000");
}

// We do more rigorous testing of the string conversion functions in the core C unit tests, so we
// will only test one bad string in this one.
TEST(etcpal_cpp_uuid, from_string_works)
{
  auto               uuid = etcpal::Uuid::FromString("00010203-0405-0607-0809-0a0b0c0d0e0f");
  const etcpal::Uuid uuid_cmp(UUID_INITIALIZER);
  TEST_ASSERT_TRUE(uuid == uuid_cmp);
  TEST_ASSERT_FALSE(uuid.IsNull());

  uuid = etcpal::Uuid::FromString(std::string{"00010203-0405-0607-0809-0a0b0c0d0e0f"});
  TEST_ASSERT_TRUE(uuid == uuid_cmp);
  TEST_ASSERT_FALSE(uuid.IsNull());

  uuid = etcpal::Uuid::FromString("Bad string");
  TEST_ASSERT_TRUE(uuid.IsNull());

  uuid = etcpal::Uuid::FromString(std::string{"Bad string"});
  TEST_ASSERT_TRUE(uuid.IsNull());
}

TEST(etcpal_cpp_uuid, is_null_works)
{
  const etcpal::Uuid null_uuid({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
  TEST_ASSERT_TRUE(null_uuid.IsNull());

  const etcpal::Uuid not_null_uuid({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1});
  TEST_ASSERT_FALSE(not_null_uuid.IsNull());
}

// The pure C tests do a more rigorous test of this functionality. We will just generate one UUID
// and check it for each of these.

TEST(etcpal_cpp_uuid, generates_v1_correctly)
{
  // Only run this test if generate_v1_uuid() is implemented on this platform.
  EtcPalUuid test_uuid;
  if (etcpal_generate_v1_uuid(&test_uuid) == kEtcPalErrNotImpl)
    TEST_IGNORE_MESSAGE("etcpal::Uuid::V1() is not implemented on this platform.");

  const etcpal::Uuid v1 = etcpal::Uuid::V1();
  TEST_ASSERT_FALSE(v1.IsNull());

  // Make sure the variant and version fields are correct. We should always have Variant 1, Version
  // 1.
  TEST_ASSERT_EQUAL_UINT8((v1.get().data[6] & 0xf0u), 0x10u);
  TEST_ASSERT_EQUAL_UINT8((v1.get().data[8] & 0xc0u), 0x80u);
  TEST_ASSERT_EQUAL(v1.version(), etcpal::UuidVersion::kV1);
}

TEST(etcpal_cpp_uuid, generates_v3_correctly)
{
  etcpal::Uuid namespace1(
      {0xbe, 0x88, 0xb7, 0xfe, 0x38, 0x57, 0x40, 0xfa, 0x88, 0x9f, 0x73, 0x06, 0x7d, 0xb6, 0x8a, 0x0e});
  etcpal::Uuid namespace2(
      {0xa9, 0x30, 0x54, 0x86, 0xd7, 0xe8, 0x4c, 0x31, 0xbe, 0xc3, 0x44, 0x20, 0x57, 0xad, 0x3d, 0x17});

  const auto ns1_name1 = etcpal::Uuid::V3(namespace1, "name1");
  const auto ns1_name1_dup = etcpal::Uuid::V3(namespace1, "name1");
  const auto ns1_name2 = etcpal::Uuid::V3(namespace1, "name2");
  const auto ns2_name1 = etcpal::Uuid::V3(namespace2, "name1");
  const auto ns2_name2 = etcpal::Uuid::V3(namespace2, "name2");

  TEST_ASSERT_FALSE(ns1_name1 == ns2_name1);
  TEST_ASSERT_FALSE(ns1_name1 == ns1_name2);
  TEST_ASSERT_FALSE(ns2_name1 == ns2_name2);
  TEST_ASSERT_TRUE(ns1_name1 == ns1_name1_dup);

  // Make sure the version is correct
  TEST_ASSERT_EQUAL(ns1_name1.version(), etcpal::UuidVersion::kV3);
  TEST_ASSERT_EQUAL(ns1_name1_dup.version(), etcpal::UuidVersion::kV3);
  TEST_ASSERT_EQUAL(ns1_name2.version(), etcpal::UuidVersion::kV3);
  TEST_ASSERT_EQUAL(ns2_name1.version(), etcpal::UuidVersion::kV3);
  TEST_ASSERT_EQUAL(ns2_name2.version(), etcpal::UuidVersion::kV3);

  // Make sure the Variant Version bits are correct.
  // We should always have Variant 1, Version 3.
  TEST_ASSERT_EQUAL_UINT8((ns1_name1.data()[6] & 0xf0u), 0x30u);
  TEST_ASSERT_EQUAL_UINT8((ns1_name1.data()[8] & 0xc0u), 0x80u);
  TEST_ASSERT_EQUAL_UINT8((ns1_name2.data()[6] & 0xf0u), 0x30u);
  TEST_ASSERT_EQUAL_UINT8((ns1_name2.data()[8] & 0xc0u), 0x80u);
  TEST_ASSERT_EQUAL_UINT8((ns2_name1.data()[6] & 0xf0u), 0x30u);
  TEST_ASSERT_EQUAL_UINT8((ns2_name1.data()[8] & 0xc0u), 0x80u);
  TEST_ASSERT_EQUAL_UINT8((ns2_name2.data()[6] & 0xf0u), 0x30u);
  TEST_ASSERT_EQUAL_UINT8((ns2_name2.data()[8] & 0xc0u), 0x80u);
  TEST_ASSERT_EQUAL_UINT8((ns1_name1_dup.data()[6] & 0xf0u), 0x30u);
  TEST_ASSERT_EQUAL_UINT8((ns1_name1_dup.data()[8] & 0xc0u), 0x80u);
}

TEST(etcpal_cpp_uuid, generates_v4_correctly)
{
  // Only run this test if generate_v4_uuid() is implemented on this platform.
  EtcPalUuid test_uuid;
  if (etcpal_generate_v4_uuid(&test_uuid) == kEtcPalErrNotImpl)
    TEST_IGNORE_MESSAGE("etcpal::Uuid::V4() is not implemented on this platform.");

  const etcpal::Uuid v4 = etcpal::Uuid::V4();
  TEST_ASSERT_FALSE(v4.IsNull());

  // Make sure the variant and version fields are correct. We should always have Variant 1, Version
  // 4.
  TEST_ASSERT_EQUAL_UINT8((v4.get().data[6] & 0xf0u), 0x40u);
  TEST_ASSERT_EQUAL_UINT8((v4.get().data[8] & 0xc0u), 0x80u);
  TEST_ASSERT_EQUAL(v4.version(), etcpal::UuidVersion::kV4);
}

TEST(etcpal_cpp_uuid, generates_v5_correctly)
{
  etcpal::Uuid namespace1(
      {0xce, 0xb3, 0x21, 0xa9, 0x59, 0x14, 0x47, 0xf4, 0xaa, 0xcc, 0xd6, 0x99, 0x96, 0x53, 0x77, 0xd5});
  etcpal::Uuid namespace2(
      {0xa6, 0xfe, 0x6a, 0xb6, 0xf4, 0xc3, 0x40, 0x9b, 0xb1, 0x0d, 0xa6, 0x4d, 0x3e, 0x62, 0xa9, 0xcc});

  const auto ns1_name1 = etcpal::Uuid::V5(namespace1, "name1");
  const auto ns1_name1_dup = etcpal::Uuid::V5(namespace1, "name1");
  const auto ns1_name2 = etcpal::Uuid::V5(namespace1, "name2");
  const auto ns2_name1 = etcpal::Uuid::V5(namespace2, "name1");
  const auto ns2_name2 = etcpal::Uuid::V5(namespace2, "name2");

  TEST_ASSERT_FALSE(ns1_name1 == ns2_name1);
  TEST_ASSERT_FALSE(ns1_name1 == ns1_name2);
  TEST_ASSERT_FALSE(ns2_name1 == ns2_name2);
  TEST_ASSERT_TRUE(ns1_name1 == ns1_name1_dup);

  // Make sure the version is correct
  TEST_ASSERT_EQUAL(ns1_name1.version(), etcpal::UuidVersion::kV5);
  TEST_ASSERT_EQUAL(ns1_name1_dup.version(), etcpal::UuidVersion::kV5);
  TEST_ASSERT_EQUAL(ns1_name2.version(), etcpal::UuidVersion::kV5);
  TEST_ASSERT_EQUAL(ns2_name1.version(), etcpal::UuidVersion::kV5);
  TEST_ASSERT_EQUAL(ns2_name2.version(), etcpal::UuidVersion::kV5);

  // Make sure the Variant Version bits are correct.
  // We should always have Variant 1, Version 5.
  TEST_ASSERT_EQUAL_UINT8((ns1_name1.data()[6] & 0xf0u), 0x50u);
  TEST_ASSERT_EQUAL_UINT8((ns1_name1.data()[8] & 0xc0u), 0x80u);
  TEST_ASSERT_EQUAL_UINT8((ns1_name2.data()[6] & 0xf0u), 0x50u);
  TEST_ASSERT_EQUAL_UINT8((ns1_name2.data()[8] & 0xc0u), 0x80u);
  TEST_ASSERT_EQUAL_UINT8((ns2_name1.data()[6] & 0xf0u), 0x50u);
  TEST_ASSERT_EQUAL_UINT8((ns2_name1.data()[8] & 0xc0u), 0x80u);
  TEST_ASSERT_EQUAL_UINT8((ns2_name2.data()[6] & 0xf0u), 0x50u);
  TEST_ASSERT_EQUAL_UINT8((ns2_name2.data()[8] & 0xc0u), 0x80u);
  TEST_ASSERT_EQUAL_UINT8((ns1_name1_dup.data()[6] & 0xf0u), 0x50u);
  TEST_ASSERT_EQUAL_UINT8((ns1_name1_dup.data()[8] & 0xc0u), 0x80u);
}

TEST(etcpal_cpp_uuid, generates_os_preferred_correctly)
{
  // Only run this test if generate_os_preferred_uuid() is implemented on this platform.
  EtcPalUuid test_uuid;
  if (etcpal_generate_os_preferred_uuid(&test_uuid) == kEtcPalErrNotImpl)
    TEST_IGNORE_MESSAGE("etcpal::Uuid::OsPreferred() is not implemented on this platform.");

  const etcpal::Uuid os_preferred = etcpal::Uuid::OsPreferred();
  // There is really not much we can test here besides that it's not a null UUID
  TEST_ASSERT_FALSE(os_preferred.IsNull());
}

TEST(etcpal_cpp_uuid, generates_device_correctly)
{
  const uint8_t                mac[6]{0x00, 0xc0, 0x16, 0x01, 0x02, 0x03};
  const std::array<uint8_t, 6> mac_array{0x00, 0xc0, 0x16, 0x01, 0x02, 0x03};

  const etcpal::Uuid dev_1 = etcpal::Uuid::Device("Test Device", mac, 20);
  const etcpal::Uuid dev_1_dup = etcpal::Uuid::Device("Test Device", mac_array, 20);
  const etcpal::Uuid dev_2 = etcpal::Uuid::Device("Test Device", mac, 21);

  TEST_ASSERT_FALSE(dev_1.IsNull());
  TEST_ASSERT_FALSE(dev_1_dup.IsNull());
  TEST_ASSERT_FALSE(dev_2.IsNull());

  TEST_ASSERT_TRUE(dev_1 == dev_1_dup);
  TEST_ASSERT_TRUE(dev_1 != dev_2);
  TEST_ASSERT_TRUE(dev_1_dup != dev_2);
}

TEST(etcpal_cpp_uuid, equality_operators_work)
{
  const etcpal::Uuid uuid_1({20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35});
  const etcpal::Uuid uuid_1_dup(uuid_1);
  const etcpal::Uuid uuid_2({0, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35});

  TEST_ASSERT_TRUE(uuid_1 == uuid_1_dup);
  TEST_ASSERT_TRUE(uuid_1_dup == uuid_1);
  TEST_ASSERT_FALSE(uuid_1 != uuid_1_dup);
  TEST_ASSERT_FALSE(uuid_1_dup != uuid_1);
  TEST_ASSERT_TRUE(uuid_1 != uuid_2);
  TEST_ASSERT_TRUE(uuid_2 != uuid_1);
  TEST_ASSERT_FALSE(uuid_1 == uuid_2);
  TEST_ASSERT_FALSE(uuid_2 == uuid_1);
}

TEST(etcpal_cpp_uuid, comparison_operators_work)
{
  // uuid_2 should be greater than uuid_1
  const etcpal::Uuid uuid_1({20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35});
  const etcpal::Uuid uuid_1_dup(uuid_1);
  const etcpal::Uuid uuid_2({20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 36});

  TEST_ASSERT_FALSE(uuid_1 < uuid_1_dup);
  TEST_ASSERT_FALSE(uuid_1 > uuid_1_dup);
  TEST_ASSERT_TRUE(uuid_1 <= uuid_1_dup);
  TEST_ASSERT_TRUE(uuid_1 >= uuid_1_dup);

  TEST_ASSERT_TRUE(uuid_1 < uuid_2);
  TEST_ASSERT_TRUE(uuid_2 > uuid_1);
  TEST_ASSERT_FALSE(uuid_1 > uuid_2);
  TEST_ASSERT_FALSE(uuid_2 < uuid_1);

  TEST_ASSERT_TRUE(uuid_1 <= uuid_2);
  TEST_ASSERT_TRUE(uuid_2 >= uuid_1);
  TEST_ASSERT_FALSE(uuid_1 >= uuid_2);
  TEST_ASSERT_FALSE(uuid_2 <= uuid_1);
}

// Test the operators that compare the C type EtcPalUuid with the C++ type etcpal::Uuid
TEST(etcpal_cpp_uuid, special_equality_operators_work)
{
  const etcpal::Uuid uuid_1({20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35});
  const EtcPalUuid   c_uuid_1_dup = {20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35};
  const EtcPalUuid   c_uuid_2 = {0, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35};

  TEST_ASSERT_TRUE(uuid_1 == c_uuid_1_dup);
  TEST_ASSERT_TRUE(c_uuid_1_dup == uuid_1);
  TEST_ASSERT_FALSE(uuid_1 != c_uuid_1_dup);
  TEST_ASSERT_FALSE(c_uuid_1_dup != uuid_1);
  TEST_ASSERT_TRUE(uuid_1 != c_uuid_2);
  TEST_ASSERT_TRUE(c_uuid_2 != uuid_1);
  TEST_ASSERT_FALSE(uuid_1 == c_uuid_2);
  TEST_ASSERT_FALSE(c_uuid_2 == uuid_1);
}

TEST(etcpal_cpp_uuid, accessors_work)
{
  const std::array<uint8_t, 16> uuid_data = {0x00, 0x00, 0x11, 0x11, 0x22, 0x22, 0x33, 0x33,
                                             0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb};
  const etcpal::Uuid            uuid(uuid_data.data());

  TEST_ASSERT_EQUAL_UINT32(uuid.time_low(), 0x00001111u);
  TEST_ASSERT_EQUAL_UINT16(uuid.time_mid(), 0x2222u);
  TEST_ASSERT_EQUAL_UINT16(uuid.time_hi_and_version(), 0x3333u);
  TEST_ASSERT_TRUE(
      (uuid.clock_seq_and_node() == std::array<uint8_t, 8>{0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb}));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(uuid.data(), uuid_data.data(), 16);
}

TEST_GROUP_RUNNER(etcpal_cpp_uuid)
{
  RUN_TEST_CASE(etcpal_cpp_uuid, default_constructor_works);
  RUN_TEST_CASE(etcpal_cpp_uuid, data_constructor_works);
  RUN_TEST_CASE(etcpal_cpp_uuid, copy_constructors_work);
  RUN_TEST_CASE(etcpal_cpp_uuid, assignment_operators_work);
  RUN_TEST_CASE(etcpal_cpp_uuid, to_string_works);
  RUN_TEST_CASE(etcpal_cpp_uuid, from_string_works);
  RUN_TEST_CASE(etcpal_cpp_uuid, is_null_works);
  RUN_TEST_CASE(etcpal_cpp_uuid, generates_v1_correctly);
  RUN_TEST_CASE(etcpal_cpp_uuid, generates_v3_correctly);
  RUN_TEST_CASE(etcpal_cpp_uuid, generates_v4_correctly);
  RUN_TEST_CASE(etcpal_cpp_uuid, generates_v5_correctly);
  RUN_TEST_CASE(etcpal_cpp_uuid, generates_os_preferred_correctly);
  RUN_TEST_CASE(etcpal_cpp_uuid, generates_device_correctly);
  RUN_TEST_CASE(etcpal_cpp_uuid, equality_operators_work);
  RUN_TEST_CASE(etcpal_cpp_uuid, comparison_operators_work);
  RUN_TEST_CASE(etcpal_cpp_uuid, special_equality_operators_work);
  RUN_TEST_CASE(etcpal_cpp_uuid, accessors_work);
}
}
