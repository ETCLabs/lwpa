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

#include "etcpal/rbtree.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "etcpal/common.h"
#include "unity_fixture.h"
#include "fff.h"

// Disable sprintf() warning on Windows/MSVC
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#define INT_ARRAY_SIZE 100
#define RANDOM_INT_IN_ARRAY() (int)(rand() / ((RAND_MAX + 1.0) / INT_ARRAY_SIZE))

static int incrementing_int_array[INT_ARRAY_SIZE];
static int random_int_array[INT_ARRAY_SIZE];

EtcPalRbNode node_pool[INT_ARRAY_SIZE];
size_t       next_node_index;

// Private function prototypes

static EtcPalRbNode* get_node();
static int           int_compare(const EtcPalRbTree* self, const void* value_a, const void* value_b);
static void          populate_int_arrays();
static void          initialize_tree_with_random_even_ints(EtcPalRbTree* self);
static void          test_bound(int is_lower_bound);

FAKE_VALUE_FUNC(EtcPalRbNode*, node_alloc);
FAKE_VOID_FUNC(node_dealloc, EtcPalRbNode*);
FAKE_VOID_FUNC(clear_func, const EtcPalRbTree*, EtcPalRbNode*);

// Very very dumb "allocation" of nodes but all we need for these tests - each call returns the
// next one from the array, freeing does nothing.
EtcPalRbNode* get_node(void)
{
  return &node_pool[next_node_index++];
}

int int_compare(const EtcPalRbTree* self, const void* value_a, const void* value_b)
{
  ETCPAL_UNUSED_ARG(self);
  int a = *(const int*)value_a;
  int b = *(const int*)value_b;
  return (a > b) - (a < b);
}

void populate_int_arrays()
{
  // Initialize two arrays of monotonically increasing ints
  for (int i = 0; i < INT_ARRAY_SIZE; ++i)
  {
    incrementing_int_array[i] = i;
    random_int_array[i] = i;
  }

  // Shuffle the random array
  for (int i = 0; i < INT_ARRAY_SIZE - 1; ++i)
  {
    // srand() is taken care of at the entry point.
    int j = i + rand() / (RAND_MAX / (INT_ARRAY_SIZE - i) + 1);
    int swap_val = random_int_array[j];
    random_int_array[j] = random_int_array[i];
    random_int_array[i] = swap_val;
  }
}

void initialize_tree_with_random_even_ints(EtcPalRbTree* self)
{
  TEST_ASSERT_NOT_NULL(etcpal_rbtree_init(self, int_compare, node_alloc, node_dealloc));

  // Insert even values between 0 and 98 (inclusive) into the tree at random.
  for (int i = 0; i < INT_ARRAY_SIZE; ++i)
  {
    if ((random_int_array[i] % 2) == 0)
    {
      TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_rbtree_insert(self, &random_int_array[i]));
    }
  }
}

/* Pass in 0 to test upper_bound, or 1 to test lower_bound. */
void test_bound(int is_lower_bound)
{
  // Initialize the tree and iterator. Randomly initialize the tree with even numbers between 0 and 98 inclusive.
  EtcPalRbTree tree;
  EtcPalRbIter iter;
  initialize_tree_with_random_even_ints(&tree);
  etcpal_rbiter_init(&iter);

  const char* function_name_log_str = (is_lower_bound ? "lower_bound" : "upper_bound");

  // Test bound of ints from 0 to 99 inclusive.
  for (int i = 0; i < INT_ARRAY_SIZE; ++i)
  {
    char test_error_msg[100];
    sprintf(test_error_msg, "The %s function failed a test on iteration %d.", function_name_log_str, i);

    int expected_value = -1;

    if (is_lower_bound)
    {
      // Even numbers should have equivalent lower bounds.
      // The lower bounds of the odd numbers should be the even number immediately after (if any).
      expected_value = ((i % 2) == 0) ? (i) : (i + 1);
    }
    else
    {
      // The upper_bound should be the even number immediately after.
      expected_value = ((i % 2) == 0) ? (i + 2) : (i + 1);
    }

    int* bound = (int*)(is_lower_bound ? etcpal_rbiter_lower_bound(&iter, &tree, &i)
                                       : etcpal_rbiter_upper_bound(&iter, &tree, &i));

    // Check that this value can be found with etcpal_rbtree_find.
    if (bound)
    {
      TEST_ASSERT_NOT_NULL_MESSAGE(etcpal_rbtree_find(&tree, bound), test_error_msg);
    }

    // Check the return value against the expected result.
    if (expected_value >= INT_ARRAY_SIZE)  // Expect NULL.
    {
      TEST_ASSERT_NULL_MESSAGE(bound, test_error_msg);
    }
    else  // Expect non-NULL.
    {
      TEST_ASSERT_NOT_NULL_MESSAGE(bound, test_error_msg);

      if (bound)
      {
        TEST_ASSERT_EQUAL_MESSAGE(*bound, expected_value, test_error_msg);
      }
    }

    // Check the iterator.
    int* last = NULL;
    int* val = bound;
    for (int j = 0; (j < 10) && (val != NULL); ++j, (val = (int*)etcpal_rbiter_prev(&iter)))
    {
      TEST_ASSERT_NOT_NULL(iter.tree);
      TEST_ASSERT_NOT_NULL(iter.node);

      if (val)
      {
        TEST_ASSERT_EQUAL(*val % 2, 0);

        if (last)
        {
          TEST_ASSERT_EQUAL(*last - *val, 2);
        }
      }

      last = val;
    }

    if (val == NULL)
    {
      // Should be the same as when calling rbiter_next at the end, or rbiter_prev at the beginning.
      TEST_ASSERT_NOT_NULL(iter.tree);
      TEST_ASSERT_NULL(iter.node);
      TEST_ASSERT_EQUAL(iter.top, 0);
    }
  }

  // Clear the tree.
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_rbtree_clear(&tree));
}

// The tests themselves

TEST_GROUP(etcpal_rbtree);

TEST_SETUP(etcpal_rbtree)
{
  RESET_FAKE(node_alloc);
  RESET_FAKE(node_dealloc);
  RESET_FAKE(clear_func);

  node_alloc_fake.custom_fake = get_node;

  populate_int_arrays();
  next_node_index = 0;
}

TEST_TEAR_DOWN(etcpal_rbtree)
{
}

TEST(etcpal_rbtree, insert_node_functions_work)
{
  EtcPalRbTree tree;
  TEST_ASSERT_NOT_NULL(etcpal_rbtree_init(&tree, int_compare, NULL, NULL));

  // Point each node at its respective value and insert it into the tree
  for (int i = 0; i < INT_ARRAY_SIZE; ++i)
  {
    EtcPalRbNode* node = &node_pool[i];
    TEST_ASSERT_NOT_NULL(etcpal_rbnode_init(node, &random_int_array[i]));
    TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_rbtree_insert_node(&tree, node));
  }
  TEST_ASSERT_EQUAL_UINT(INT_ARRAY_SIZE, etcpal_rbtree_size(&tree));
  TEST_ASSERT_GREATER_THAN(0, etcpal_rbtree_test(&tree, tree.root));

  // Find a random number
  int to_find = RANDOM_INT_IN_ARRAY();

  char test_error_msg[100];
  sprintf(test_error_msg, "Couldn't find value %d in the tree.", to_find);

  int* found = (int*)etcpal_rbtree_find(&tree, &to_find);
  TEST_ASSERT_NOT_NULL_MESSAGE(found, test_error_msg);
  TEST_ASSERT_EQUAL(*found, to_find);

  // Try removing one item
  int to_remove = RANDOM_INT_IN_ARRAY();
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_rbtree_remove_with_cb(&tree, &to_remove, clear_func));
  TEST_ASSERT_EQUAL(to_remove, *((int*)(clear_func_fake.arg1_val->value)));

  // Clear the tree
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_rbtree_clear_with_cb(&tree, clear_func));
  TEST_ASSERT_EQUAL_UINT(0u, etcpal_rbtree_size(&tree));
  TEST_ASSERT_EQUAL_UINT(clear_func_fake.call_count, INT_ARRAY_SIZE);
}

TEST(etcpal_rbtree, insert_functions_work)
{
  EtcPalRbTree tree;
  TEST_ASSERT_NOT_NULL(etcpal_rbtree_init(&tree, int_compare, node_alloc, node_dealloc));

  // Insert each value into the tree; dynamic alloc functions should be called.
  for (int i = 0; i < INT_ARRAY_SIZE; ++i)
    TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_rbtree_insert(&tree, &random_int_array[i]));
  TEST_ASSERT_EQUAL_UINT(INT_ARRAY_SIZE, etcpal_rbtree_size(&tree));
  TEST_ASSERT_EQUAL_UINT(INT_ARRAY_SIZE, node_alloc_fake.call_count);
  TEST_ASSERT_GREATER_THAN(0, etcpal_rbtree_test(&tree, tree.root));

  // Find a random number
  int to_find = RANDOM_INT_IN_ARRAY();

  char test_error_msg[100];
  sprintf(test_error_msg, "Couldn't find value %d in the tree.", to_find);

  int* found = (int*)etcpal_rbtree_find(&tree, &to_find);
  TEST_ASSERT_NOT_NULL_MESSAGE(found, test_error_msg);
  TEST_ASSERT_EQUAL(*found, to_find);

  // Make sure removing something that wasn't in the tree fails.
  int not_in_tree = INT_ARRAY_SIZE;
  TEST_ASSERT_EQUAL(kEtcPalErrNotFound, etcpal_rbtree_remove(&tree, &not_in_tree));
  TEST_ASSERT_EQUAL_UINT(node_dealloc_fake.call_count, 0u);

  // Clear the tree
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_rbtree_clear(&tree));
  TEST_ASSERT_EQUAL_UINT(0u, etcpal_rbtree_size(&tree));
  TEST_ASSERT_EQUAL_UINT(node_dealloc_fake.call_count, INT_ARRAY_SIZE);
}

TEST(etcpal_rbtree, insert_should_fail_if_element_already_exists)
{
  EtcPalRbTree tree;
  TEST_ASSERT_NOT_NULL(etcpal_rbtree_init(&tree, int_compare, node_alloc, node_dealloc));

  // Insert a few values into the tree. Enough so we have some branches to traverse
  for (int i = 0; i < 10; ++i)
    TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_rbtree_insert(&tree, &incrementing_int_array[i]));
  TEST_ASSERT_EQUAL_UINT(10u, etcpal_rbtree_size(&tree));

  // Try inserting a duplicate value - should fail with error 'already exists'
  int duplicate = 5;
  TEST_ASSERT_EQUAL(kEtcPalErrExists, etcpal_rbtree_insert(&tree, &duplicate));

  // Remove the offending value and try inserting it again, should succeed
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_rbtree_remove(&tree, &duplicate));
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_rbtree_insert(&tree, &duplicate));
}

TEST(etcpal_rbtree, insert_failure_should_not_leak_memory)
{
  EtcPalRbTree tree;
  TEST_ASSERT_NOT_NULL(etcpal_rbtree_init(&tree, int_compare, node_alloc, node_dealloc));

  // Insert a value, then try to insert it again
  int val = 0;
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_rbtree_insert(&tree, &val));
  TEST_ASSERT_EQUAL(kEtcPalErrExists, etcpal_rbtree_insert(&tree, &val));

  // Alloc should only have been called once - failed insert should not alloc or dealloc
  TEST_ASSERT_EQUAL_UINT(node_alloc_fake.call_count, 1u);
  TEST_ASSERT_EQUAL_UINT(node_dealloc_fake.call_count, 0u);

  // Now clear the tree
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_rbtree_clear(&tree));

  // The alloc and dealloc count should be equal - no memory should be leaked
  TEST_ASSERT_EQUAL_UINT(node_alloc_fake.call_count, node_dealloc_fake.call_count);
}

TEST(etcpal_rbtree, iterators_work_as_expected)
{
  EtcPalRbTree tree;
  TEST_ASSERT_NOT_NULL(etcpal_rbtree_init(&tree, int_compare, node_alloc, node_dealloc));

  // Insert each value into the tree; dynamic alloc functions should be called.
  for (int i = 0; i < INT_ARRAY_SIZE; ++i)
    TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_rbtree_insert(&tree, &random_int_array[i]));
  TEST_ASSERT_EQUAL_UINT(INT_ARRAY_SIZE, etcpal_rbtree_size(&tree));

  // Initialize an iterator
  EtcPalRbIter iter;
  TEST_ASSERT_EQUAL_PTR(&iter, etcpal_rbiter_init(&iter));

  // Get the first value.
  int* val = (int*)etcpal_rbiter_first(&iter, &tree);
  // Although the elements were inserted in random order, the tree should be sorted so 0 should be
  // the first value.
  TEST_ASSERT_NOT_NULL(val);
  TEST_ASSERT_EQUAL(*val, 0);

  // Test iterating through the tree in forward order.
  int num_iterations = 1;
  int last_val = *val;
  while ((val = (int*)etcpal_rbiter_next(&iter)) != NULL)
  {
    ++num_iterations;
    // Each value given by the iterator should be the next one in order in the tree.
    TEST_ASSERT_EQUAL(*val, last_val + 1);
    last_val = *val;
  }
  TEST_ASSERT_EQUAL(num_iterations, INT_ARRAY_SIZE);

  // Get the last value.
  val = (int*)etcpal_rbiter_last(&iter, &tree);
  TEST_ASSERT_NOT_NULL(val);
  TEST_ASSERT_EQUAL(*val, INT_ARRAY_SIZE - 1);

  // Test iterating through the tree in reverse order.
  num_iterations = 1;
  last_val = *val;
  while ((val = (int*)etcpal_rbiter_prev(&iter)) != NULL)
  {
    ++num_iterations;
    // Each value given by the iterator should be the previous one in order in the tree.
    TEST_ASSERT_EQUAL(*val, last_val - 1);
    last_val = *val;
  }
  TEST_ASSERT_EQUAL(num_iterations, INT_ARRAY_SIZE);
}

TEST(etcpal_rbtree, max_height_is_within_bounds)
{
  EtcPalRbTree tree;
  TEST_ASSERT_NOT_NULL(etcpal_rbtree_init(&tree, int_compare, node_alloc, node_dealloc));

  // Insert monotonically incrementing values into the tree. In a traditional binary tree, this
  // would result in a worst-case unbalanced tree of height INT_ARRAY_SIZE. In the red-black tree,
  // the maximum height should be determined by the formula 2 * log2(INT_ARRAY_SIZE + 1).
  for (int i = 0; i < INT_ARRAY_SIZE; ++i)
    TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_rbtree_insert(&tree, &incrementing_int_array[i]));
  TEST_ASSERT_EQUAL_UINT(INT_ARRAY_SIZE, etcpal_rbtree_size(&tree));

  // Get the height of the tree and compare it against the theoretical maximum.
  EtcPalRbIter iter;
  size_t       max_height = 0;
  TEST_ASSERT_NOT_NULL(etcpal_rbiter_init(&iter));
  TEST_ASSERT_NOT_NULL(etcpal_rbiter_first(&iter, &tree));
  do
  {
    if (iter.top > max_height)
      max_height = iter.top;
  } while (NULL != etcpal_rbiter_next(&iter));
  max_height += 1;

  // http://www.doctrina.org/maximum-height-of-red-black-tree.html
  size_t theoretical_max_height = 2 * (size_t)(log(INT_ARRAY_SIZE + 1) / log(2));

  TEST_ASSERT_LESS_OR_EQUAL_UINT(theoretical_max_height, max_height);
}

TEST(etcpal_rbtree, lower_bound_works)
{
  // Pass in 1 to test lower_bound.
  test_bound(1);
}

TEST(etcpal_rbtree, upper_bound_works)
{
  // Pass in 0 to test upper_bound.
  test_bound(0);
}

TEST_GROUP_RUNNER(etcpal_rbtree)
{
  RUN_TEST_CASE(etcpal_rbtree, insert_node_functions_work);
  RUN_TEST_CASE(etcpal_rbtree, insert_functions_work);
  RUN_TEST_CASE(etcpal_rbtree, insert_should_fail_if_element_already_exists);
  RUN_TEST_CASE(etcpal_rbtree, insert_failure_should_not_leak_memory);
  RUN_TEST_CASE(etcpal_rbtree, iterators_work_as_expected);
  RUN_TEST_CASE(etcpal_rbtree, max_height_is_within_bounds);
  RUN_TEST_CASE(etcpal_rbtree, lower_bound_works);
  RUN_TEST_CASE(etcpal_rbtree, upper_bound_works);
}
