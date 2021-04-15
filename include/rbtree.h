typedef struct rbtree_t rbtree_t;
typedef struct rbnode_t rbnode_t;

rbtree_t *rbtree_create(int (*Compare)(const void *, const void *),
			void (*freeNode)(void *), int replace_flag);
void rbtree_free(rbtree_t *tree);
int rbtree_insert(rbtree_t *tree, void *Data);
void rbtree_delete(rbtree_t *tree, rbnode_t *Z);
rbnode_t *rbtree_find(rbtree_t *tree, const void *Data);
void *rbtree_finddata(rbtree_t *tree, const void *Data);
int rbtree_num_elements(rbtree_t *tree);
void *rbtree_node2data(rbtree_t *tree, rbnode_t *node);
int rbtree_deletebydata(rbtree_t *tree, const void *data);

/* callback order for walking  */
typedef enum { PreOrder, InOrder, PostOrder } RBTREE_ORDER;
int rbtree_walk(rbtree_t *tree, RBTREE_ORDER order, int (*callback)(void *, void *), void *context);
