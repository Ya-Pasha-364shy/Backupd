#ifndef HELPERS_HASH_TABLE_H
#define HELPERS_HASH_TABLE_H

#define HASH_ITEM_MAX_SIZE_STR (512)
#define HASH_KEY_MAX_LEN       (10000)
#define HASH_TABLE_MAX_SIZE    HASH_KEY_MAX_LEN

typedef unsigned int hash_t;

typedef enum
{
	HASH_TABLE_INVALID = -1,
	HASH_TABLE_INIT    =  0,
	HASH_TABLE_MAX     =  HASH_TABLE_MAX_SIZE,
} hash_table_size_t;

typedef struct hash_item_s
{
	hash_t key;
	char * value;

	void * previous;
	void * next;
} hash_item_t;

typedef struct hash_table_s
{
	hash_item_t **    table;
	hash_item_t *     node;

	hash_table_size_t size;
	int               count;
} hash_table_t;

hash_item_t * hash_table_search_item(hash_table_t * this, hash_t key, char * value);
unsigned int hash_table_insert_item(hash_table_t * this, char * value);
hash_table_t * hash_table_create(hash_table_size_t size);
void hash_table_free(hash_table_t * this);

#endif // HELPERS_HASH_TABLE_H
