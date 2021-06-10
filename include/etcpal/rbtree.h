/* etcpal/rbtree.h: C-language red-black tree implementation. .his module is
 * based heavily on a public-domain red-black tree implementation. The license
 * of that implementation follows as it was when retrieved from Github on
 * 2018-02-28.
 *
 ******************************************************************************
 * Based on Julienne Walker's <http://eternallyconfuzzled.com/> rb_tree
 * implementation.
 *
 * Modified by Mirek Rusin <http://github.com/mirek/rb_tree>.
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */

#ifndef ETCPAL_RBTREE_H_
#define ETCPAL_RBTREE_H_

#include <stddef.h>
#include "etcpal/error.h"

/**
 * @defgroup etcpal_rbtree rbtree (Red-Black Trees)
 * @ingroup etcpal_core
 * @brief A red-black tree implementation.
 *
 * ```c
 * #include "etcpal/rbtree.h"
 * ```
 *
 * A red-black tree is a popular design for a self-balancing binary search tree. Based on a
 * public-domain red-black tree implementation; see the header file for details.
 *
 * *If you have access to C++, just use std::map or std::set. They're much nicer than this module.*
 *
 * Red-black tree nodes store arbitrary data as a `void*` pointer, which are compared with a
 * user-provided #EtcPalRbTreeNodeCmpFunc. The module user must also manage memory allocation for
 * each node of the red-black tree by providing an #EtcPalRbNodeAllocFunc and
 * #EtcPalRbNodeDeallocFunc, which must allocate and deallocate #EtcPalRbNode instances. Note: if
 * you don't have access to a malloc() implementation, @ref etcpal_mempool is a convenient way to
 * allocate nodes.
 *
 * @code
 * typedef struct MyStruct
 * {
 *   int key;
 * } MyStruct;
 *
 * int compare_func(const EtcPalRbTree* self, const void* value_a, const void* value_b)
 * {
 *   const MyStruct* a = (const MyStruct*)value_a;
 *   const MyStruct* b = (const MyStruct*)value_b;
 *   return (a->key > b->key) - (a->key < b->key);
 * }
 *
 * EtcPalRbNode* node_alloc_func()
 * {
 *   return (EtcPalRbNode*)malloc(sizeof(EtcPalRbNode)); // Or some other method you define
 * }
 *
 * void node_dealloc_func(EtcPalRbNode* node)
 * {
 *   free(node); // Or some other method you define
 * }
 *
 * EtcPalRbTree tree;
 * etcpal_rbtree_init(&tree, compare_func, node_alloc_func, node_dealloc_func);
 *
 * MyStruct struct_1;
 * struct_1.key = 20;
 * etcpal_rbtree_insert(&tree, &struct_1);
 *
 * MyStruct struct_2;
 * struct_2.key = 30;
 * etcpal_rbtree_insert(&tree, &struct_2);
 *
 * EXPECT_EQ(etcpal_rbtree_size(&tree), 2);
 *
 * MyStruct* found = etcpal_rbtree_find(&struct_1); // found == &struct_1
 * found = etcpal_rbtree_find(&struct_2); // found == &struct_2
 *
 * etcpal_error_t insert_res = etcpal_rbtree_insert(&tree, &struct_1); // insert_res == kEtcPalErrExists
 *
 * EtcPalRbIter tree_iter;
 * etcpal_rbiter_init(&tree_iter);
 * MyStruct* iter_val = etcpal_rbiter_first(&iter, &tree); // iter_val == &struct_1
 * iter_val = etcpal_rbiter_next(&tree_iter); // iter_val == &struct_2
 * iter_val = etcpal_rbiter_next(&tree_iter); // iter_val == NULL
 *
 * etcpal_rbtree_remove(&tree, &struct_1);
 * found = etcpal_rbtree_find(&tree, &struct_1); // found == NULL
 * EXPECT_EQ(etcpal_rbtree_size(&tree), 1);
 *
 * etcpal_rbtree_clear(&tree);
 * found = etcpal_rbtree_find(&tree, &struct_2); // found == NULL
 * EXPECT_EQ(etcpal_rbtree_size(&tree), 0);
 *
 * @endcode
 *
 * @{
 */

/** The tallest allowable tree that can be iterated over. */
#ifndef ETCPAL_RB_ITER_MAX_HEIGHT
#define ETCPAL_RB_ITER_MAX_HEIGHT 64
#endif

/** @cond rb_struct_typedefs */
typedef struct EtcPalRbNode EtcPalRbNode;
typedef struct EtcPalRbTree EtcPalRbTree;
/** @endcond */

/**
 * @name Callback Functions
 * @{
 */

/**
 * @brief A function type that compares two value instances contained in a tree.
 *
 * A default, etcpal_rbtree_node_cmp_ptr_cb(), is provided which simply compares the pointer (address) of
 * each value.
 *
 * @param[in] self The tree in which two values are being compared.
 * @param[in] value_a The first node being compared.
 * @param[in] value_b The second node being compared.
 * @return < 0: value_a is less than value_b
 * @return 0: value_a is equal to value_b
 * @return > 0: value_a is greater than value_b
 */
typedef int (*EtcPalRbTreeNodeCmpFunc)(const EtcPalRbTree* self, const void* value_a, const void* value_b);

/**
 * @brief A function type to be called for each node in a tree.
 *
 * Usually provided on a tree-wide clear or destroy operation; in this case, it should provide any
 * deallocation necessary for the node structure and its value. A default,
 * etcpal_rbtree_node_dealloc_cb(), is provided which simply calls the tree's
 * #EtcPalRbNodeDeallocFunc on the node.
 *
 * @param[in] self The tree in which the node resides.
 * @param[in] node The node for which an action should be performed.
 */
typedef void (*EtcPalRbTreeNodeFunc)(const EtcPalRbTree* self, EtcPalRbNode* node);

/**
 * @brief A function type to allocate a new node.
 *
 * The user provides the allocation method for new nodes, whether this be malloc() or some more
 * static method. A function of this type is saved by the tree struct and called on a call to
 * etcpal_rbtree_insert().
 *
 * @return Pointer to the newly allocated node.
 */
typedef EtcPalRbNode* (*EtcPalRbNodeAllocFunc)(void);

/**
 * @brief A function type to deallocate a node.
 *
 * The user provides the deallocation method for nodes, whether this be free() or some more static
 * method. A function of this type is saved by the tree struct and called on calls to
 * etcpal_rbtree_remove() and etcpal_rbtree_clear().
 *
 * @param[in] node Pointer to node to deallocate.
 */
typedef void (*EtcPalRbNodeDeallocFunc)(EtcPalRbNode* node);

/**
 * @}
 */

/** A red-black tree node. */
struct EtcPalRbNode
{
  int           red;     /**< The node color: red (1), black (0) */
  EtcPalRbNode* link[2]; /**< Child node links: left [0], right [1] */
  void*         value;   /**< The value object represented by this node. */
};

/**
 * @brief A red-black tree.
 *
 * Initialize using etcpal_rbtree_init() before carrying out any other operation on the tree.
 */
struct EtcPalRbTree
{
  EtcPalRbNode*           root;      /**< The root node of the tree. */
  EtcPalRbTreeNodeCmpFunc cmp;       /**< A function to use for comparing two nodes. */
  size_t                  size;      /**< The current count of nodes in the tree. */
  EtcPalRbNodeAllocFunc   alloc_f;   /**< A function to use for allocating a new node.*/
  EtcPalRbNodeDeallocFunc dealloc_f; /**< A function to use for deallocating a node. */
  void*                   info;      /**< User provided, not used by etcpal_rbtree. */
};

/**
 * @brief A red-black tree iterator.
 *
 * Initialize using etcpal_rbiter_init() before carrying out any other operation on the iterator.
 */
typedef struct EtcPalRbIter
{
  EtcPalRbTree* tree;                            /**< The tree being iterated over. */
  EtcPalRbNode* node;                            /**< The current node. */
  EtcPalRbNode* path[ETCPAL_RB_ITER_MAX_HEIGHT]; /**< The traversal path to the current node. */
  size_t        top;                             /**< Top of the traversal stack. */
  void*         info;                            /**< User provided, not used by etcpal_rbiter. */
} EtcPalRbIter;

#ifdef __cplusplus
extern "C" {
#endif

int  etcpal_rbtree_node_cmp_ptr_cb(const EtcPalRbTree* self, const void* a, const void* b);
void etcpal_rbtree_node_dealloc_cb(const EtcPalRbTree* self, EtcPalRbNode* node);

EtcPalRbNode* etcpal_rbnode_init(EtcPalRbNode* self, void* value);

EtcPalRbTree*  etcpal_rbtree_init(EtcPalRbTree*           self,
                                  EtcPalRbTreeNodeCmpFunc node_cmp_cb,
                                  EtcPalRbNodeAllocFunc   alloc_f,
                                  EtcPalRbNodeDeallocFunc dealloc_f);
void*          etcpal_rbtree_find(EtcPalRbTree* self, const void* value);
etcpal_error_t etcpal_rbtree_insert(EtcPalRbTree* self, void* value);
etcpal_error_t etcpal_rbtree_remove(EtcPalRbTree* self, const void* value);
etcpal_error_t etcpal_rbtree_clear(EtcPalRbTree* self);
size_t         etcpal_rbtree_size(EtcPalRbTree* self);

etcpal_error_t etcpal_rbtree_insert_node(EtcPalRbTree* self, EtcPalRbNode* node);
etcpal_error_t etcpal_rbtree_remove_with_cb(EtcPalRbTree* self, const void* value, EtcPalRbTreeNodeFunc node_cb);
etcpal_error_t etcpal_rbtree_clear_with_cb(EtcPalRbTree* self, EtcPalRbTreeNodeFunc node_cb);

int etcpal_rbtree_test(EtcPalRbTree* self, EtcPalRbNode* root);

EtcPalRbIter* etcpal_rbiter_init(EtcPalRbIter* self);
void*         etcpal_rbiter_first(EtcPalRbIter* self, EtcPalRbTree* tree);
void*         etcpal_rbiter_last(EtcPalRbIter* self, EtcPalRbTree* tree);
void*         etcpal_rbiter_next(EtcPalRbIter* self);
void*         etcpal_rbiter_prev(EtcPalRbIter* self);
void*         etcpal_rbiter_lower_bound(EtcPalRbIter* self, EtcPalRbTree* tree, const void* value);
void*         etcpal_rbiter_upper_bound(EtcPalRbIter* self, EtcPalRbTree* tree, const void* value);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ETCPAL_RBTREE_H_ */
