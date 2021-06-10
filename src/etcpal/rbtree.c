/* etcpal_rbtree.c: C-language red-black tree implementation. This module is
 * based heavily on a public-domain red-black tree implementation. The license
 * of that implementation follows as it was when retrieved from Github on
 * 2018-02-28.
 *
 ******************************************************************************
 * Based on Julienne Walker's <http://eternallyconfuzzled.com/> etcpal_rbtree
 * implementation.
 *
 * Modified by Mirek Rusin <http://github.com/mirek/etcpal_rbtree>.
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

#include "etcpal/rbtree.h"

#include <stdbool.h>
#include "etcpal/common.h"

/* etcpal_rbnode */

static EtcPalRbNode* etcpal_rbnode_alloc(EtcPalRbTree* tree)
{
  if (tree && tree->alloc_f)
    return tree->alloc_f();
  return NULL;
}

/**
 * @brief Initialize a red-black tree node.
 *
 * This function must be called on a new node before inserting it manually using
 * etcpal_rbtree_insert_node(). When using etcpal_rbtree_insert(), this function is called on the new
 * node automatically.
 *
 * @param[in] self The node to be initialized.
 * @param[in] value Pointer to the value to assign to the node.
 * @return Pointer to the node that was initialized.
 */
EtcPalRbNode* etcpal_rbnode_init(EtcPalRbNode* self, void* value)
{
  if (self)
  {
    self->red = 1;
    self->link[0] = self->link[1] = NULL;
    self->value = value;
  }
  return self;
}

static EtcPalRbNode* rb_node_create(EtcPalRbTree* tree, void* value)
{
  return etcpal_rbnode_init(etcpal_rbnode_alloc(tree), value);
}

static void rb_node_dealloc(EtcPalRbNode* self, const EtcPalRbTree* tree)
{
  if (self && tree && tree->dealloc_f)
  {
    tree->dealloc_f(self);
  }
}

static int rb_node_is_red(const EtcPalRbNode* self)
{
  return self ? self->red : 0;
}

static EtcPalRbNode* rb_node_rotate(EtcPalRbNode* self, int dir)
{
  EtcPalRbNode* result = NULL;
  if (self)
  {
    result = self->link[!dir];
    self->link[!dir] = result->link[dir];
    result->link[dir] = self;
    self->red = 1;
    result->red = 0;
  }
  return result;
}

static EtcPalRbNode* rb_node_rotate2(EtcPalRbNode* self, int dir)
{
  EtcPalRbNode* result = NULL;
  if (self)
  {
    self->link[!dir] = rb_node_rotate(self->link[!dir], !dir);
    result = rb_node_rotate(self, dir);
  }
  return result;
}

/* etcpal_rbtree - default callbacks */

/**
 * @brief The default node comparison callback.
 *
 * This function can be supplied as an argument to any function that takes a
 * #EtcPalRbTreeNodeCmpFunc. Simply compares the pointer addresses of the two node values.
 */
int etcpal_rbtree_node_cmp_ptr_cb(const EtcPalRbTree* self, const void* a, const void* b)
{
  ETCPAL_UNUSED_ARG(self);
  return (a > b) - (a < b);
}

/**
 * @brief The default node deallocation callback.
 *
 * This function can be supplied as an argument to any function that takes a #EtcPalRbTreeNodeFunc.
 * Simply deallocates the node using the tree's dealloc_f.
 */
void etcpal_rbtree_node_dealloc_cb(const EtcPalRbTree* self, EtcPalRbNode* node)
{
  rb_node_dealloc(node, self);
}

/* etcpal_rbtree */

/**
 * @brief Initialize a red-black tree node.
 *
 * This function must be called on a new red-black tree before performing any other operations on
 * it.
 *
 * @param[in] self The tree to be initialized.
 * @param[in] node_cmp_cb A function to use for comparing values in the tree.
 * @param[in] alloc_f A function to use for allocating new node structures.
 * @param[in] dealloc_f A function to use for deallocating node structures.
 * @return Pointer to the tree that was initialized.
 */
EtcPalRbTree* etcpal_rbtree_init(EtcPalRbTree*           self,
                                 EtcPalRbTreeNodeCmpFunc node_cmp_cb,
                                 EtcPalRbNodeAllocFunc   alloc_f,
                                 EtcPalRbNodeDeallocFunc dealloc_f)
{
  if (self)
  {
    self->root = NULL;
    self->size = 0;
    self->cmp = node_cmp_cb ? node_cmp_cb : etcpal_rbtree_node_cmp_ptr_cb;
    self->alloc_f = alloc_f;
    self->dealloc_f = dealloc_f;
  }
  return self;
}

/* Iterate the tree in a binary search pattern as far as it can go. Returns the final comparison result. */
static int rb_iter_binary_search(EtcPalRbIter* self, EtcPalRbTree* tree, const void* value)
{
  int cmp = 0;

  self->tree = tree;
  self->node = tree->root;
  self->top = 0;

  if (self->node)
  {
    cmp = tree->cmp(tree, self->node->value, value);

    while ((cmp != 0) && (self->node->link[cmp < 0] != NULL))
    {
      /* If the tree supports duplicates, they should be chained to the right subtree for this to
       * work */
      self->path[self->top++] = self->node;
      self->node = self->node->link[cmp < 0];

      cmp = tree->cmp(tree, self->node->value, value);
    }
  }

  return cmp;
}

/**
 * @brief Find a value in a red-black tree.
 *
 * Uses the #EtcPalRbTreeNodeCmpFunc provided in etcpal_rbtree_init() to compare values. Lookup
 * guaranteed in log(n) time.
 *
 * @param[in] self Tree in which to find the value.
 * @param[in] value Value to find.
 * @return Pointer to the value (value found) or NULL (value not found).
 */
void* etcpal_rbtree_find(EtcPalRbTree* self, const void* value)
{
  void*        result = NULL;
  EtcPalRbIter tree_iter;

  etcpal_rbiter_init(&tree_iter);

  int cmp = rb_iter_binary_search(&tree_iter, self, value);

  if ((cmp == 0) && (tree_iter.node))
  {
    result = tree_iter.node->value;
  }
  else
  {
    result = NULL;
  }

  return result;
}

/**
 * @brief Insert a new value into a red-black tree.
 *
 * If the value did not already exist in the tree, a new node is allocated using the
 * #EtcPalRbNodeAllocFunc provided in etcpal_rbtree_init(). Uses the #EtcPalRbTreeNodeCmpFunc
 * provided in etcpal_rbtree_init() to compare values. Insertion guaranteed in log(n) time.
 *
 * If this function returns #kEtcPalErrOk, all iterators created using the etcpal_rbiter_*()
 * functions are invalidated.
 *
 * @param[in] self Tree in which to insert the value.
 * @param[in] value Value to insert.
 * @return #kEtcPalErrOk: the value was inserted.
 * @return #kEtcPalErrExists: the value already existed in the tree.
 * @return #kEtcPalErrNoMem: Couldn't allocate new node.
 * @return #kEtcPalErrInvalid: Invalid argument provided.
 */
etcpal_error_t etcpal_rbtree_insert(EtcPalRbTree* self, void* value)
{
  void* found_val = etcpal_rbtree_find(self, value);
  if (found_val)
    return kEtcPalErrExists;

  EtcPalRbNode* new_node = rb_node_create(self, value);
  if (new_node)
  {
    etcpal_error_t insert_res = etcpal_rbtree_insert_node(self, new_node);
    if (insert_res == kEtcPalErrOk)
      return kEtcPalErrOk;

    rb_node_dealloc(new_node, self);
    return insert_res;
  }
  else
  {
    return kEtcPalErrNoMem;
  }
}

/**
 * @brief Insert a node containing a new value into a red-black tree.
 *
 * The node is supplied by the caller and its memory must remain valid as long as it remains in the
 * tree. Uses the #EtcPalRbTreeNodeCmpFunc provided in etcpal_rbtree_init() to compare values.
 * Insertion guaranteed in log(n) time.
 *
 * If this function returns #kEtcPalErrOk, all iterators created using the etcpal_rbiter_*()
 * functions are invalidated.
 *
 * @param[in] self Tree in which to insert the value.
 * @param[in] node Node containing value to insert. Must have been previously initialized using
 *                 etcpal_rbnode_init().
 * @return #kEtcPalErrOk: The value was inserted.
 * @return #kEtcPalErrExists: The value already existed in the tree.
 * @return #kEtcPalErrInvalid: Invalid argument provided.
 */
etcpal_error_t etcpal_rbtree_insert_node(EtcPalRbTree* self, EtcPalRbNode* node)
{
  etcpal_error_t result = kEtcPalErrInvalid;
  if (self && node)
  {
    if (self->root == NULL)
    {
      self->root = node;
      result = kEtcPalErrOk;
    }
    else
    {
      EtcPalRbNode head = {0}; /* False tree root */
      /* Grandparent & parent */
      EtcPalRbNode* g = NULL;
      EtcPalRbNode* t = &head;
      /* Iterator & parent */
      EtcPalRbNode* p = NULL;
      EtcPalRbNode* q = t->link[1] = self->root;
      int           dir = 0;
      int           last = 0;

      /* Search down the tree for a place to insert */
      while (1)
      {
        bool inserted = false;

        if (q == NULL)
        {
          /* Insert node at the first null link. */
          p->link[dir] = q = node;
          inserted = true;
        }
        else if (rb_node_is_red(q->link[0]) && rb_node_is_red(q->link[1]))
        {
          /* Simple red violation: color flip */
          q->red = 1;
          q->link[0]->red = 0;
          q->link[1]->red = 0;
        }

        if (rb_node_is_red(q) && rb_node_is_red(p))
        {
          /* Hard red violation: rotations necessary */
          int dir2 = t->link[1] == g;
          if (q == p->link[last])
            t->link[dir2] = rb_node_rotate(g, !last);
          else
            t->link[dir2] = rb_node_rotate2(g, !last);
        }

        /* Stop working if we inserted a node. This check also disallows duplicates in the tree */
        if (self->cmp(self, q->value, node->value) == 0)
        {
          result = inserted ? kEtcPalErrOk : kEtcPalErrExists;
          break;
        }

        last = dir;
        dir = self->cmp(self, q->value, node->value) < 0;

        /* Move the helpers down */
        if (g != NULL)
          t = g;

        g = p, p = q;
        q = q->link[dir];
      }

      /* Update the root (it may be different) */
      self->root = head.link[1];
    }

    /* Make the root black for simplified logic */
    self->root->red = 0;
    ++self->size;
  }

  return result;
}

/**
 * @brief Remove a value from a red-black tree.
 *
 * The node memory is deallocated using the #EtcPalRbNodeDeallocFunc provided in
 * etcpal_rbtree_init(); the user is responsible for deallocating the value memory. Uses the
 * #EtcPalRbTreeNodeCmpFunc provided in etcpal_rbtree_init() to compare values. Removal guaranteed
 * in log(n) time.
 *
 * If this function returns #kEtcPalErrOk, all iterators created using the etcpal_rbiter_*()
 * functions are invalidated.
 *
 * @param[in] self Tree from which to remove the value.
 * @param[in] value Value to remove.
 * @return #kEtcPalErrOk: The value was removed.
 * @return #kEtcPalErrInvalid: Invalid argument provided.
 * @return #kEtcPalErrNotFound: The value did not exist in the tree.
 */
etcpal_error_t etcpal_rbtree_remove(EtcPalRbTree* self, const void* value)
{
  etcpal_error_t result = kEtcPalErrInvalid;
  if (self)
    result = etcpal_rbtree_remove_with_cb(self, value, etcpal_rbtree_node_dealloc_cb);
  return result;
}

/**
 * @brief Remove a value from a red-black tree, calling back into the application with the node and
 *        value being removed.
 *
 * The user provides a #EtcPalRbTreeNodeFunc callback function and is responsible for deallocating
 * both the node and value memory. Uses the #EtcPalRbTreeNodeCmpFunc provided in
 * etcpal_rbtree_init() to compare values. Removal guaranteed in log(n) time.
 *
 * If this function returns #kEtcPalErrOk, all iterators created using the etcpal_rbiter_*()
 * functions are invalidated.
 *
 * @param[in] self Tree from which to remove the value.
 * @param[in] value Value to remove.
 * @param[in] node_cb Callback function to call with the node and value being removed.
 * @return #kEtcPalErrOk: The value was removed.
 * @return #kEtcPalErrInvalid: Invalid argument provided.
 * @return #kEtcPalErrNotFound: The value did not exist in the tree.
 */
etcpal_error_t etcpal_rbtree_remove_with_cb(EtcPalRbTree* self, const void* value, EtcPalRbTreeNodeFunc node_cb)
{
  if (!self)
    return kEtcPalErrInvalid;
  if (self->root == NULL)
    return kEtcPalErrNotFound;

  /* SMK added this check, because the removal code seems to fail badly in the case where the node
   * being removed didn't previously exist in the tree. */
  if (NULL == etcpal_rbtree_find(self, value))
    return kEtcPalErrNotFound;

  EtcPalRbNode head = {0}; /* False tree root */
  /* Helpers */
  EtcPalRbNode* q = &head;
  EtcPalRbNode* g = NULL;
  EtcPalRbNode* p = NULL;
  EtcPalRbNode* f = NULL; /* Found item */
  q->link[1] = self->root;

  int dir = 1;

  /* Search and push a red node down to fix red violations as we go */
  while (q->link[dir] != NULL)
  {
    int last = dir;

    /* Move the helpers down */
    g = p, p = q;
    q = q->link[dir];
    dir = self->cmp(self, q->value, value) < 0;

    /* Save the node with matching value and keep going; we'll do removal tasks at the end */
    if (self->cmp(self, q->value, value) == 0)
      f = q;

    /* Push the red node down with rotations and color flips */
    if (!rb_node_is_red(q) && !rb_node_is_red(q->link[dir]))
    {
      if (rb_node_is_red(q->link[!dir]))
      {
        p = p->link[last] = rb_node_rotate(q, dir);
      }
      else if (!rb_node_is_red(q->link[!dir]))
      {
        EtcPalRbNode* s = p->link[!last];
        if (s)
        {
          if (!rb_node_is_red(s->link[!last]) && !rb_node_is_red(s->link[last]))
          {
            /* Color flip */
            p->red = 0;
            s->red = 1;
            q->red = 1;
          }
          else
          {
            int dir2 = g->link[1] == p;
            if (rb_node_is_red(s->link[last]))
              g->link[dir2] = rb_node_rotate2(p, last);
            else if (rb_node_is_red(s->link[!last]))
              g->link[dir2] = rb_node_rotate(p, last);

            /* Ensure correct coloring */
            q->red = g->link[dir2]->red = 1;
            g->link[dir2]->link[0]->red = 0;
            g->link[dir2]->link[1]->red = 0;
          }
        }
      }
    }
  }

  /* Replace and remove the saved node */
  if (f)
  {
    void* tmp = f->value;
    f->value = q->value;
    q->value = tmp;

    p->link[p->link[1] == q] = q->link[q->link[0] == NULL];

    if (node_cb)
      node_cb(self, q);
    q = NULL;
  }

  /* Update the root (it may be different) */
  self->root = head.link[1];

  /* Make the root black for simplified logic */
  if (self->root != NULL)
    self->root->red = 0;

  --self->size;
  return kEtcPalErrOk;
}

/**
 * @brief Clear all values from a red-black tree.
 *
 * The node memory is deallocated using the #EtcPalRbNodeDeallocFunc provided in
 * etcpal_rbtree_init(); the user is responsible for deallocating the value memory.
 *
 * If this function returns #kEtcPalErrOk, all iterators created using the etcpal_rbiter_*()
 * functions are invalidated.
 *
 * @param[in] self Tree to clear.
 * @return #kEtcPalErrOk: The tree was cleared.
 * @return #kEtcPalErrInvalid: Invalid argument provided.
 */
etcpal_error_t etcpal_rbtree_clear(EtcPalRbTree* self)
{
  etcpal_error_t result = kEtcPalErrInvalid;
  if (self)
    result = etcpal_rbtree_clear_with_cb(self, etcpal_rbtree_node_dealloc_cb);
  return result;
}

/**
 * @brief Clear all values from a red-black tree, calling back into the application for each node
 *        and value being removed.
 *
 * The user provides a #EtcPalRbTreeNodeFunc callback function which is called for each node in the
 * tree. The user is responsible for deallocating both the node and value memory.
 *
 * If this function returns #kEtcPalErrOk, all iterators created using the etcpal_rbiter_*()
 * functions are invalidated.
 *
 * @param[in] self Tree to clear.
 * @param[in] node_cb Callback function to call with each node and value being removed.
 * @return #kEtcPalErrOk: The tree was cleared.
 * @return #kEtcPalErrInvalid: Invalid argument provided.
 */
etcpal_error_t etcpal_rbtree_clear_with_cb(EtcPalRbTree* self, EtcPalRbTreeNodeFunc node_cb)
{
  etcpal_error_t result = kEtcPalErrInvalid;
  if (self && node_cb)
  {
    EtcPalRbNode* node = self->root;
    EtcPalRbNode* save = NULL;

    /* Rotate away the left links so that we can treat this like the destruction of a linked list */
    while (node)
    {
      if (node->link[0] == NULL)
      {
        /* No left links, just kill the node and move on */
        save = node->link[1];
        node_cb(self, node);
        node = NULL;
      }
      else
      {
        /* Rotate away the left link and check again */
        save = node->link[0];
        node->link[0] = save->link[1];
        save->link[1] = node;
      }
      node = save;
    }
    self->root = NULL;
    self->size = 0;
    result = kEtcPalErrOk;
  }
  return result;
}

/**
 * @brief Get the current number of values in a red-black tree.
 * @param[in] self The tree of which to get the size.
 * @return The number of values currently in the tree.
 */
size_t etcpal_rbtree_size(EtcPalRbTree* self)
{
  size_t result = 0;
  if (self)
    result = self->size;
  return result;
}

/**
 * @brief Test the validity of a red-black tree.
 *
 * A debugging function; tests that both basic binary tree rules and specific red-black rules are
 * not violated.
 *
 * @param[in] self Tree to test.
 * @param[in] root Node at which to start the test. All nodes beneath this node will be tested.
 * @return The "black height" of the tree; the number of black nodes present in the traversal
 *         from the root to each leaf. A valid red-black tree requires this number to be the same
 *         for every possible traversal.
 * @return 0: Invalid tree.
 */
int etcpal_rbtree_test(EtcPalRbTree* self, EtcPalRbNode* root)
{
  if (root == NULL)
    return 1;

  EtcPalRbNode* ln = root->link[0];
  EtcPalRbNode* rn = root->link[1];

  /* Consecutive red links */
  if (rb_node_is_red(root))
  {
    if (rb_node_is_red(ln) || rb_node_is_red(rn))
    {
      return 0;
    }
  }

  int lh = etcpal_rbtree_test(self, ln);
  int rh = etcpal_rbtree_test(self, rn);

  /* Invalid binary search tree */
  if ((ln != NULL && self->cmp(self, ln->value, root->value) >= 0) ||
      (rn != NULL && self->cmp(self, rn->value, root->value) <= 0))
  {
    return 0;
  }

  /* Black height mismatch */
  if (lh != 0 && rh != 0 && lh != rh)
  {
    return 0;
  }

  /* Only count black links */
  if (lh != 0 && rh != 0)
    return rb_node_is_red(root) ? lh : lh + 1;

  return 0;
}

/* etcpal_rbiter */

/**
 * @brief Initialize a red-black tree iterator.
 *
 * This function must be called on a new iterator before using any of the other etcpal_rbiter_*
 * functions on it.
 *
 * @param[in] self The iterator to be initialized.
 * @return Pointer to the iterator that was initialized.
 */
EtcPalRbIter* etcpal_rbiter_init(EtcPalRbIter* self)
{
  if (self)
  {
    self->tree = NULL;
    self->node = NULL;
    self->top = 0;
  }
  return self;
}

/* Internal function, init traversal object, dir determines whether to begin traversal at the
 * smallest or largest valued node. */
static void* rb_iter_start(EtcPalRbIter* self, EtcPalRbTree* tree, int dir)
{
  void* result = NULL;
  if (self)
  {
    self->tree = tree;
    self->node = tree->root;
    self->top = 0;

    /* Save the path for later selfersal */
    if (self->node != NULL)
    {
      while (self->node->link[dir] != NULL)
      {
        self->path[self->top++] = self->node;
        self->node = self->node->link[dir];
      }
    }

    result = self->node == NULL ? NULL : self->node->value;
  }
  return result;
}

/* Traverse a red black tree in the user-specified direction (0 asc, 1 desc) */
static void* rb_iter_move(EtcPalRbIter* self, int dir)
{
  if (self->node->link[dir] != NULL)
  {
    /* Continue down this branch */
    self->path[self->top++] = self->node;
    self->node = self->node->link[dir];
    while (self->node->link[!dir] != NULL)
    {
      self->path[self->top++] = self->node;
      self->node = self->node->link[!dir];
    }
  }
  else
  {
    /* Move to the next branch */
    EtcPalRbNode* last = NULL;
    do
    {
      if (self->top == 0)
      {
        self->node = NULL;
        break;
      }
      last = self->node;
      self->node = self->path[--self->top];
    } while (last == self->node->link[dir]);
  }
  return self->node == NULL ? NULL : self->node->value;
}

/**
 * @brief Point a red-black tree iterator at the first value in the tree.
 *
 * The first value is the lowest, as determined by the #EtcPalRbTreeNodeCmpFunc provided in
 * etcpal_rbtree_init(). Use etcpal_rbiter_next() to get the next higher value.
 *
 * @param[in] self Iterator to point at the first value.
 * @param[in] tree Tree of which to get the first value.
 * @return Pointer to the first value or NULL (the tree was empty or invalid).
 */
void* etcpal_rbiter_first(EtcPalRbIter* self, EtcPalRbTree* tree)
{
  return rb_iter_start(self, tree, 0);
}

/**
 * @brief Point a red-black tree iterator at the last value in the tree.
 *
 * The last value is the highest, as determined by the #EtcPalRbTreeNodeCmpFunc provided in
 * etcpal_rbtree_init(). Use etcpal_rbiter_prev() to get the next lower value.
 *
 * @param[in] self Iterator to point at the last value.
 * @param[in] tree Tree of which to get the last value.
 * @return Pointer to the last value or NULL (the tree was empty or invalid).
 */
void* etcpal_rbiter_last(EtcPalRbIter* self, EtcPalRbTree* tree)
{
  return rb_iter_start(self, tree, 1);
}

/**
 * @brief Advance a red-black tree iterator.
 *
 * Gets the next higher value in the tree as determined by the #EtcPalRbTreeNodeCmpFunc provided in
 * etcpal_rbtree_init().
 *
 * @param[in] self Iterator to advance.
 * @return Pointer to next higher value, or NULL (the end of the tree has been reached).
 */
void* etcpal_rbiter_next(EtcPalRbIter* self)
{
  return rb_iter_move(self, 1);
}

/**
 * @brief Reverse-advance a red-black tree iterator.
 *
 * Gets the next lower value in the tree as determined by the #EtcPalRbTreeNodeCmpFunc provided in
 * etcpal_rbtree_init().
 *
 * @param[in] self Iterator to reverse-advance.
 * @return Pointer to next lower value, or NULL (the beginning of the tree has been reached).
 */
void* etcpal_rbiter_prev(EtcPalRbIter* self)
{
  return rb_iter_move(self, 0);
}

/**
 * @brief Point a red-black tree iterator to the lower-bound of a value.
 *
 * Gets the first value in the tree that is not considered to go before the given value.
 *
 * @param[in] self Iterator to modify.
 * @param[in] tree Tree of which to get the lower bound.
 * @param[in] value The value to compare against to determine the lower bound.
 * @return Pointer to the lower bound value, or NULL (the end of the tree has been reached).
 */
void* etcpal_rbiter_lower_bound(EtcPalRbIter* self, EtcPalRbTree* tree, const void* value)
{
  void* result = NULL;

  if (tree && self)
  {
    int cmp = rb_iter_binary_search(self, tree, value);

    if (self->node)
    {
      if (cmp < 0)
      {
        // The current node goes before the value. The next node (if any) goes after the value.
        result = etcpal_rbiter_next(self);
      }
      else
      {
        // If cmp == 0, then the current node is equal to the value. If cmp > 0, then the current node goes after the
        // value. In either case, the previous node (if any) goes before the value.
        result = self->node->value;
      }
    }
  }

  return result;
}

/**
 * @brief Point a red-black tree iterator to the upper-bound of a value.
 *
 * Gets the first value in the tree that is considered to go after the given value.
 *
 * @param[in] self Iterator to modify.
 * @param[in] tree Tree of which to get the upper bound.
 * @param[in] value The value to compare against to determine the upper bound.
 * @return Pointer to the upper bound value, or NULL (the end of the tree has been reached).
 */
void* etcpal_rbiter_upper_bound(EtcPalRbIter* self, EtcPalRbTree* tree, const void* value)
{
  void* result = NULL;

  if (tree && self)
  {
    int cmp = rb_iter_binary_search(self, tree, value);

    if (self->node)
    {
      if (cmp > 0)
      {
        // The current node goes after the value. The previous node (if any) goes before the value.
        result = self->node->value;
      }
      else
      {
        // If cmp == 0, then the current node is equal to the value, but not after. If cmp < 0, then the current node
        // goes before the value. In either case, the next node goes after the value (there are no duplicates).
        result = etcpal_rbiter_next(self);
      }
    }
  }

  return result;
}
