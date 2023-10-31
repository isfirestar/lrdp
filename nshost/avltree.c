#if !defined __PUBLIC_INTERNEL_DECLARE
#define __PUBLIC_INTERNEL_DECLARE
#endif

#include "avltree.h"

#if !defined (NULL)
#define NULL ((void *)0)
#endif /*!NULL*/

#if !defined MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif /*!MAX*/

#define HEIGHT(t) ((t) == NULL ? -1 : (t)->height)

avltree_node_pt _avlsinglerotateleft(avltree_node_pt tree);
avltree_node_pt _avlsinglerotateright(avltree_node_pt tree);
avltree_node_pt _avldoublerotateleft(avltree_node_pt tree);
avltree_node_pt _avldoublerotateright(avltree_node_pt tree);

PORTABLEIMPL(avltree_node_pt) avlinsert(avltree_node_pt tree, avltree_node_pt node, const avltree_compare_fp compare)
{
    int ret;

    if (unlikely(!node || !compare)) {
        return NULL;
    }

    if (!tree) {
        node->lchild = NULL;
        node->rchild = NULL;
        node->height = 0;
        return node;
    }

    ret = compare(node, tree);
    if (ret < 0) {
        tree->lchild = avlinsert(tree->lchild, node, compare);
        if (HEIGHT(tree->lchild) - HEIGHT(tree->rchild) == 2) {
            ret = compare(node, tree->lchild);
            if (ret < 0) {
                tree = _avlsinglerotateleft(tree);
            } else if (ret > 0) {
                tree = _avldoublerotateleft(tree);
            }
        } else {
            tree->height = MAX(HEIGHT(tree->lchild), HEIGHT(tree->rchild)) + 1;
        }
    } else if (ret > 0) {
        tree->rchild = avlinsert(tree->rchild, node, compare);
        if (HEIGHT(tree->rchild) - HEIGHT(tree->lchild) == 2) {
            ret = compare(node, tree->rchild);
            if (ret < 0) {
                tree = _avldoublerotateright(tree);
            } else if (ret > 0) {
                tree = _avlsinglerotateright(tree);
            }
        } else {
            tree->height = MAX(HEIGHT(tree->lchild), HEIGHT(tree->rchild)) + 1;
        }
    }
    return tree;
}

PORTABLEIMPL(avltree_node_pt) avlremove(avltree_node_pt tree, avltree_node_pt node, avltree_node_pt *rmnode, const avltree_compare_fp compare)
{
    int ret;

    if (unlikely(!tree || !node || !compare)) {
        return NULL;
    }

    if (rmnode) {
        *rmnode = NULL;
    }

    ret = compare(node, tree);
    if (ret == 0) {
        avltree_node_pt tmpnode;
        if (tree->lchild != NULL && tree->rchild != NULL) {
            /* 2 child */
            tmpnode = avlgetmin(tree->rchild);
            tree->rchild = avlremove(tree->rchild, tmpnode, rmnode, compare);
            tmpnode->lchild = tree->lchild;
            tmpnode->rchild = tree->rchild;
            if (rmnode) {
                *rmnode = tree;
            }
            tree = tmpnode;
        } else {
            /* 1 or 0 child */
            if (tree->lchild) {
                tmpnode = tree->lchild;
            } else if (tree->rchild) {
                tmpnode = tree->rchild;
            } else {
                tmpnode = NULL;
            }
            if (rmnode) {
                *rmnode = tree;
            }
            return tmpnode;
        }
    } else if (ret < 0) {
        if (tree->lchild != NULL){
            tree->lchild = avlremove(tree->lchild, node, rmnode, compare);
        }
    } else {
        /* ret > 0 */
        if (tree->rchild != NULL) {
            tree->rchild = avlremove(tree->rchild, node, rmnode, compare);
        }
    }

    if (HEIGHT(tree->lchild) - HEIGHT(tree->rchild) == 2) {
        if (HEIGHT(tree->lchild->lchild) - HEIGHT(tree->rchild) == 1
                && HEIGHT(tree->lchild->rchild) - HEIGHT(tree->rchild) == 1) {
            /*
                    tree --> o
                    /
                   o
                  / \
                 o   o
             */
            tree = _avlsinglerotateleft(tree);
        } else if (HEIGHT(tree->lchild->lchild) - HEIGHT(tree->rchild) == 1) {
            /*
                    tree --> o
                    /
                   o
                  /
                 o
             */
            tree = _avlsinglerotateleft(tree);
        } else if (HEIGHT(tree->lchild->rchild) - HEIGHT(tree->rchild) == 1) {
            /*
                    tree --> o
                    /
                   o
                    \
                     o
             */
            tree = _avldoublerotateleft(tree);
        }
    } else if (HEIGHT(tree->rchild) - HEIGHT(tree->lchild) == 2) {
        if (HEIGHT(tree->rchild->rchild) - HEIGHT(tree->lchild) == 1
                && HEIGHT(tree->rchild->lchild) - HEIGHT(tree->lchild) == 1) {
            /*
                    tree --> o
                     \
                      o
                     / \
                    o   o
             */
            tree = _avlsinglerotateright(tree);
        } else if (HEIGHT(tree->rchild->rchild) - HEIGHT(tree->lchild) == 1) {
            /*
                    tree --> o
                     \
                      o
                       \
                        o
             */
            tree = _avlsinglerotateright(tree);
        } else if (HEIGHT(tree->rchild->lchild) - HEIGHT(tree->lchild) == 1) {
            /*
                    tree --> o
                      \
                       o
                      /
                     o
             */
            tree = _avldoublerotateright(tree);
        }
    }
    tree->height = MAX(HEIGHT(tree->lchild), HEIGHT(tree->rchild)) + 1;
    return tree;
}

PORTABLEIMPL(avltree_node_pt) avlsearch(avltree_node_pt tree, avltree_node_pt node, const avltree_compare_fp compare)
{
    int ret;
    avltree_node_pt t;

    if ( unlikely(!node || !compare) ) {
        return NULL;
    }

    t = tree;
    while (t != NULL) {
        ret = compare(node, t);
        if (ret < 0) {
            t = t->lchild;
        } else if (ret > 0) {
            t = t->rchild;
        } else {
            return t;
        }
    }
    return NULL;
}

PORTABLEIMPL(avltree_node_pt) avlgetmin(avltree_node_pt tree)
{
    avltree_node_pt t;

    if (!tree) {
        return NULL;
    }

    t = tree;
    while (t->lchild != NULL) {
        t = t->lchild;
    }
    return t;
}

PORTABLEIMPL(avltree_node_pt) avlgetmax(avltree_node_pt tree)
{
    avltree_node_pt t;

    if (!tree) {
        return NULL;
    }

    t = tree;
    while (t->rchild != NULL) {
        t = t->rchild;
    }
    return t;
}

avltree_node_pt _avlsinglerotateleft(avltree_node_pt tree)
{
    avltree_node_pt newtree;

    /*
            tree --> 1                  newtree --> 2
            /                              / \
           2        ---->                 3   1
          /
         3
     */
    /* 2 */
    newtree = tree->lchild;
    /* 1 */
    tree->lchild = newtree->rchild;
    tree->height = MAX(HEIGHT(tree->lchild), HEIGHT(tree->rchild)) + 1;
    /* 2 */
    newtree->rchild = tree;
    newtree->height = MAX(HEIGHT(newtree->lchild), HEIGHT(newtree->rchild)) + 1;
    return newtree;
}

avltree_node_pt _avlsinglerotateright(avltree_node_pt tree)
{
    avltree_node_pt newtree;

    /*
            tree --> 1                  newtree --> 2
             \                            / \
              2    ---->                 1   3
               \
                3
     */
    /* 2 */
    newtree = tree->rchild;
    /* 1 */
    tree->rchild = newtree->lchild;
    tree->height = MAX(HEIGHT(tree->lchild), HEIGHT(tree->rchild)) + 1;
    /* 2 */
    newtree->lchild = tree;
    newtree->height = MAX(HEIGHT(newtree->lchild), HEIGHT(newtree->rchild)) + 1;
    return newtree;
}

avltree_node_pt _avldoublerotateleft(avltree_node_pt tree)
{
    avltree_node_pt newtree;

    /*
            tree --> 1                  newtree --> 3
            /                              / \
           2        ---->                 2   1
            \
             3
     */
    /* 3 */
    newtree = tree->lchild->rchild;
    /* 2 */
    tree->lchild->rchild = newtree->lchild;
    tree->lchild->height = MAX(HEIGHT(tree->lchild->lchild), HEIGHT(tree->lchild->rchild)) + 1;
    /* 3 */
    newtree->lchild = tree->lchild;
    /* 1 */
    tree->lchild = newtree->rchild;
    tree->height = MAX(HEIGHT(tree->lchild), HEIGHT(tree->rchild)) + 1;
    /* 3 */
    newtree->rchild = tree;
    newtree->height = MAX(HEIGHT(newtree->lchild), HEIGHT(newtree->rchild)) + 1;
    return newtree;
}

avltree_node_pt _avldoublerotateright(avltree_node_pt tree)
{
    avltree_node_pt newtree;

    /*
            tree --> 1                  newtree --> 3
            \                            / \
             2    ---->                 1   2
            /
           3
     */
    /* 3 */
    newtree = tree->rchild->lchild;
    /* 2 */
    tree->rchild->lchild = newtree->rchild;
    tree->rchild->height = MAX(HEIGHT(tree->rchild->lchild), HEIGHT(tree->rchild->rchild)) + 1;
    /* 3 */
    newtree->rchild = tree->rchild;
    /* 1 */
    tree->rchild = newtree->lchild;
    tree->height = MAX(HEIGHT(tree->lchild), HEIGHT(tree->rchild)) + 1;
    /* 3 */
    newtree->lchild = tree;
    newtree->height = MAX(HEIGHT(newtree->lchild), HEIGHT(newtree->rchild)) + 1;
    return newtree;
}

avltree_node_pt avllowerbound(avltree_node_pt tree, avltree_node_pt node, const avltree_compare_fp compare)
{
    int ret;
    avltree_node_pt cursor;

    ret = compare(node, tree);
    if (ret < 0) {
        if (tree->lchild) {
            cursor = avllowerbound(tree->lchild, node, compare);
            return cursor ? cursor : tree;
        } else {
            return tree;
        }
    } else if (ret > 0) {
        return tree->rchild ? avllowerbound(tree->rchild, node, compare) : NULL;
    } else {
        return tree;
    }
}

avltree_node_pt avlupperbound(avltree_node_pt tree, avltree_node_pt node, const avltree_compare_fp compare)
{
    int ret;
    avltree_node_pt cursor;

    ret = compare(node, tree);
    if (ret < 0) {
        if (tree->lchild) {
            cursor = avlupperbound(tree->lchild, node, compare);
            return cursor ? cursor : tree;
        } else {
            return tree;
        }
    }

    return tree->rchild ? avlupperbound(tree->rchild, node, compare) : NULL;
}
