#include "list.h"

#include <stdlib.h>
#include <string.h>

struct list_t *list_create(void) {
	struct list_t *list = calloc(1, sizeof(struct list_t));
	return list;
}

void list_add(struct list_t *list, void *data, size_t data_size, enum list_pos_t position) {
    /* Sanity check */
    if ((list == NULL) || (data == NULL) || (data_size == 0)) {
        return;
    }

    /* Create new element  */
    struct list_node_t *node = calloc(1, sizeof(struct list_node_t));

    /* Allocate space and copy data to the new element */
	node->data = calloc(1, data_size);
	memcpy(node->data, data, data_size);

    /* Add it to the list */
    switch (position) {
    	case LIST_PREPEND:
    		if ((list->head == NULL) || (list->tail == NULL)) { // Empty list case
    			list->head = node;
    			list->tail = node;
    			node->prev = NULL;
    			node->next = NULL;
    		}
    		else {
    			node->next = list->head;
    			node->prev = NULL;
    			list->head->prev = node;
    			list->head = node;
    		}
    		break;
    	case LIST_APPEND:
    		if ((list->head == NULL) || (list->tail == NULL)) {
    			list->head = node;
				list->tail = node;
				node->prev = NULL;
				node->next = NULL;
    		}
    		else {
    			node->prev = list->tail;
    			node->next = NULL;
    			list->tail->next = node;
    			list->tail = node;
    		}
    		break;
    	default:
    		break;
    }
}

void list_sort(struct list_t *list, bool (*compare)(const void *, const void *)) {
	bool sorted = true;
	struct list_node_t *current;
	struct list_node_t *last = NULL;

	/* Sanity check */
	if ((list == NULL) || (compare == NULL)) {
		return;
	}

	/* Empty list case */
	if (list->head == NULL) {
		return;
	}

	/* Bubble sort algorithm - complexity shouldn't matter in such small datasets */
	do {
		sorted = true;
		current = list->head;

		while (current->next != last) {
			if (compare(current->data, current->next->data)) {
				/* Swap data pointers */
				void *temp = current->data;
				current->data = current->next->data;
				current->next->data = temp;

				sorted = false;
			}
			current = current->next;
		}
		last = current;

	} while (!sorted);
}

void list_traverse(struct list_t *list, void (*callback)(void *, void *), void *data, enum list_dir_t direction) {
    /* Sanity check */
    if ((list == NULL) || (callback == NULL)) {
        return;
    }

    struct list_node_t *curr;

    switch (direction) {
    	case LIST_DIR_FORWARD:
    		curr = list->head;
    		while (curr != NULL) {
    			callback(curr->data, data);
    			curr = curr->next;
    		}
    		break;

    	case LIST_DIR_BACKWARD:
    		curr = list->tail;
			while (curr != NULL) {
				callback(curr->data, data);
				curr = curr->prev;
			}
    	    break;

    	default:
    	    break;
    }
}

void list_destroy(struct list_t *list) {
    /* Sanity check */
    if (list == NULL) {
        return;
    }

    struct list_node_t *next;
    struct list_node_t *curr = list->head;

    while (curr != NULL) {
    	next = curr->next; // Get pointer to next node
    	free(curr->data); // Free memory allocated for data
    	free(curr); // Free memory allocated for node
    	curr = next; // Move to next node
    }

    free(list);
}
