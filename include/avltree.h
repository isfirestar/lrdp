#if !defined (_AVLTREE_HEADER_ANDERSON_20120216)
#define _AVLTREE_HEADER_ANDERSON_20120216

#include "compiler.h"

/* avl search tree, by neo-anderson 2012-05-05 copyright(C) shunwang Co,.Ltd*/
struct avlnode {
    struct avlnode *lchild, *rchild;
    int height;
} __POSIX_TYPE_ALIGNED__;

typedef struct avlnode avltree_node_t, *avltree_node_pt;

/**
 *  the compare function for comparing two nodes.
 *
 *  left > right -> 1
 *  left < right -> -1
 *  left = right -> 0
 */
typedef int( *avltree_compare_fp)(const void *left, const void *right);

#define avl_simple_compare(left, right, field)   \
    ( ((left)->field > (right)->field) ? (1) : ( ((left)->field < (right)->field) ? (-1) : (0) ) )

#define avl_type_compare(type, leaf, field, left, right) \
    avl_simple_compare(container_of(left, type, leaf), container_of(right, type, leaf), field)


/**
 *  insert a node into an AVL tree.
 *
 *  @param tree The root node of the AVL tree.
 *  @param node The node to insert.
 *  @param compare The comparison function to use for comparing nodes.
 *  @return The root node of the AVL tree after the node has been inserted.
 */
PORTABLEAPI(avltree_node_pt) avlinsert(avltree_node_pt tree, avltree_node_pt node, const avltree_compare_fp compare);

/**
 *  remove a node from an AVL tree.
 *
 *  @param tree The root node of the AVL tree.
 *  @param node The node to remove.
 *  @param rmnode The node to remove from the AVL tree.
 *  @param compare The comparison function to use for comparing nodes.
 *  @return The root node of the AVL tree after the node has been removed.
 *
 *  notes that @rmnode can be NULL, if it is NULL, funcion ignore the removed itme.
 */
PORTABLEAPI(avltree_node_pt) avlremove(avltree_node_pt tree, avltree_node_pt node, avltree_node_pt *rmnode, const avltree_compare_fp compare);

/**
 * search a node in an AVL tree.
 *
 * @param tree The root node of the AVL tree.
 * @param node The node to search for.
 * @param compare The comparison function to use for comparing nodes.
 */
PORTABLEAPI(avltree_node_pt) avlsearch(avltree_node_pt tree, avltree_node_pt node, const avltree_compare_fp compare);

/**
 * get the minimum/maximum node in an AVL tree.
 */
PORTABLEAPI(avltree_node_pt) avlgetmin(avltree_node_pt tree);
PORTABLEAPI(avltree_node_pt) avlgetmax(avltree_node_pt tree);

/**
 * Finds the lower/upper bound node in an AVL tree for a given node using the provided comparison function.
 *
 * @param tree The root node of the AVL tree.
 * @param node The node to find the upper bound for.
 * @param compare The comparison function to use for comparing nodes.
 * @return The lower/upper bound node in the AVL tree for the given node, or NULL if no such node exists.
 *
 *  @avllowerbound function return the first node that is not less than the given node.
 *  @avlupperbound function return the first node that is greater than the given node.
 *  for example:
 *  set = { 1, 4, 7, 10, 13, 16, 19, 22, 25 }
 *  avllowerbound(set, 7) == 7
 *  avlupperbound(set, 7) == 10
 */
PORTABLEAPI(avltree_node_pt) avllowerbound(avltree_node_pt tree, avltree_node_pt node, const avltree_compare_fp compare);
PORTABLEAPI(avltree_node_pt) avlupperbound(avltree_node_pt tree, avltree_node_pt node, const avltree_compare_fp compare);

#endif /*_AVLTREE_HEADER_ANDERSON_20120216*/
