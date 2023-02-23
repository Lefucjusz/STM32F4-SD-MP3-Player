/*
 * dir.h
 *
 *  Created on: Feb 18, 2023
 *      Author: lefucjusz
 */

#ifndef DIR_H_
#define DIR_H_

#include "list.h"
#include "fatfs.h"

typedef struct list_t dir_list_t;
typedef struct list_node_t dir_entry_t;

void dir_init(const char *root_path);

int dir_enter(const char *name);
int dir_return(void);

const char *dir_get_fs_path(void);

dir_list_t *dir_list(void);

/* Returns previous element, if there's no such, returns last (looped list) */
dir_entry_t *dir_get_prev(dir_list_t *list, dir_entry_t *current);

/* Returns next element, if there's no such, returns first (looped list) */
dir_entry_t *dir_get_next(dir_list_t *list, dir_entry_t *current);

void dir_list_free(dir_list_t *list);

#endif /* DIR_H_ */
