#pragma once

#include <stddef.h>
#include <stdbool.h>

struct list_node_t {
    void *data;
    struct list_node_t *prev;
    struct list_node_t *next;
};

struct list_t {
	struct list_node_t *head;
	struct list_node_t *tail;
};

enum list_dir_t {
	LIST_DIR_FORWARD,
	LIST_DIR_BACKWARD
};

enum list_pos_t {
	LIST_PREPEND,
	LIST_APPEND
};

struct list_t *list_create(void);

void list_add(struct list_t *list, void *data, size_t data_size, enum list_pos_t position);
void list_sort(struct list_t *list, bool (*compare)(const void *, const void *));
void list_traverse(struct list_t *list, void (*callback)(void *, void *), void *data, enum list_dir_t direction);

void list_destroy(struct list_t *list);
