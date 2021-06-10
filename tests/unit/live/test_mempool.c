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

#include "etcpal/mempool.h"
#include "unity_fixture.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define ALLOC_TEST_MEMP_SIZE 500
#define ALLOC_TEST_MEMP_ARR_SIZE 5

typedef struct TestElem
{
  int  val1;
  char val2;
} TestElem;

ETCPAL_MEMPOOL_DEFINE(alloc_test, TestElem, ALLOC_TEST_MEMP_SIZE);
ETCPAL_MEMPOOL_DEFINE_ARRAY(alloc_array_test, TestElem, ALLOC_TEST_MEMP_ARR_SIZE, ALLOC_TEST_MEMP_SIZE);

TestElem* test_arr[ALLOC_TEST_MEMP_SIZE];
size_t    index_arr[ALLOC_TEST_MEMP_SIZE];

static void create_shuffled_index_array(size_t* array, size_t size)
{
  for (size_t i = 0; i < size; ++i)
    array[i] = i;

  for (size_t i = 0; i < size - 1; ++i)
  {
    // srand() is taken care of at the entry point.
    // Clang analyzer false positive: NOLINTNEXTLINE(clang-analyzer-core.DivideZero)
    size_t j = (size_t)(i + rand()) / (size_t)(RAND_MAX / (size - i) + 1);
    size_t swap_val = array[j];
    array[j] = array[i];
    array[i] = swap_val;
  }
}

TEST_GROUP(etcpal_mempool);

TEST_SETUP(etcpal_mempool)
{
  memset(test_arr, 0, sizeof test_arr);
  memset(index_arr, 0, sizeof index_arr);
}

TEST_TEAR_DOWN(etcpal_mempool)
{
}

TEST(etcpal_mempool, alloc_and_free_works)
{
  // Initialize the pool.
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_mempool_init(alloc_test));
  TEST_ASSERT_EQUAL_UINT(ALLOC_TEST_MEMP_SIZE, etcpal_mempool_size(alloc_test));

  // Allocate the entire pool.
  for (size_t i = 0; i < ALLOC_TEST_MEMP_SIZE; ++i)
  {
    TestElem* elem = (TestElem*)etcpal_mempool_alloc(alloc_test);
    TEST_ASSERT(elem);
    test_arr[i] = elem;
  }
  TEST_ASSERT_EQUAL_UINT(ALLOC_TEST_MEMP_SIZE, etcpal_mempool_used(alloc_test));

  // Free the elements back in random order.
  // Generate and shuffle the index array.
  create_shuffled_index_array(index_arr, ALLOC_TEST_MEMP_SIZE);
  for (size_t i = 0; i < ALLOC_TEST_MEMP_SIZE; ++i)
  {
    etcpal_mempool_free(alloc_test, test_arr[index_arr[i]]);
  }
  TEST_ASSERT_EQUAL_UINT(0u, etcpal_mempool_used(alloc_test));

  // Make sure we can allocate the entire pool again.
  for (size_t i = 0; i < ALLOC_TEST_MEMP_SIZE; ++i)
  {
    test_arr[i] = (TestElem*)etcpal_mempool_alloc(alloc_test);
    TEST_ASSERT(test_arr[i]);
  }
}

TEST(etcpal_mempool, alloc_and_free_array_works)
{
  // Initialize the pool.
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_mempool_init(alloc_array_test));
  TEST_ASSERT_EQUAL_UINT(ALLOC_TEST_MEMP_SIZE, etcpal_mempool_size(alloc_array_test));

  // Allocate the entire pool.
  for (size_t i = 0; i < ALLOC_TEST_MEMP_SIZE; ++i)
  {
    TestElem* elem_arr = (TestElem*)etcpal_mempool_alloc(alloc_array_test);
    TEST_ASSERT(elem_arr);

    // Write to each spot in the array - failure could be caught by a segfault or similar
    for (size_t j = 0; j < ALLOC_TEST_MEMP_ARR_SIZE; ++j)
    {
      elem_arr[j].val1 = 1;
      elem_arr[j].val2 = 2;
    }

    test_arr[i] = elem_arr;
  }
  TEST_ASSERT_EQUAL_UINT(ALLOC_TEST_MEMP_SIZE, etcpal_mempool_used(alloc_array_test));

  // Free the elements back in random order.
  // Generate and shuffle the index array.
  create_shuffled_index_array(index_arr, ALLOC_TEST_MEMP_SIZE);
  for (size_t i = 0; i < ALLOC_TEST_MEMP_SIZE; ++i)
  {
    etcpal_mempool_free(alloc_array_test, test_arr[index_arr[i]]);
  }
  TEST_ASSERT_EQUAL_UINT(0u, etcpal_mempool_used(alloc_array_test));

  // Make sure we can allocate the entire pool again.
  for (size_t i = 0; i < ALLOC_TEST_MEMP_SIZE; ++i)
  {
    test_arr[i] = (TestElem*)etcpal_mempool_alloc(alloc_array_test);

    TEST_ASSERT(test_arr[i]);

    // Make sure the sentinel values we set before are still there.
    for (size_t j = 0; j < ALLOC_TEST_MEMP_ARR_SIZE; ++j)
    {
      TEST_ASSERT_EQUAL(test_arr[i][j].val1, 1);
      TEST_ASSERT_EQUAL(test_arr[i][j].val2, 2);
    }
  }
}

TEST_GROUP_RUNNER(etcpal_mempool)
{
  RUN_TEST_CASE(etcpal_mempool, alloc_and_free_works);
  RUN_TEST_CASE(etcpal_mempool, alloc_and_free_array_works);
}
