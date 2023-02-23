/*
 * dir.c
 *
 *  Created on: Feb 18, 2023
 *      Author: lefucjusz
 */
#include "dir.h"
#include <sys/syslimits.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

static char path[PATH_MAX];
static size_t depth;

static int path_append(const char *name) {
	const size_t cur_path_len = strlen(path);
	const size_t space_left = sizeof(path) - cur_path_len;
	if (strlen(name) >= space_left) {
		return -ENAMETOOLONG;
	}

	snprintf(path + cur_path_len, space_left, "/%s", name);

	return 0;
}

static int path_remove(void) {
	if (depth == 0) {
		return -ENOENT;
	}

	char *last_slash = strrchr(path, '/');
	*last_slash = '\0';

	return 0;
}

static bool compare_ascending(const void *val1, const void *val2) {
	const FILINFO *fno1 = (FILINFO *)val1;
	const FILINFO *fno2 = (FILINFO *)val2;

	return (strcmp(fno1->fname, fno2->fname) > 0);
}

void dir_init(const char *root_path) {
	strncpy(path, root_path, sizeof(path));
	depth = 0;
}

int dir_enter(const char *name) {
	const int ret = path_append(name);
	if (ret) {
		return ret;
	}
	depth++;

	return 0;
}

int dir_return(void) {
	const int ret = path_remove();
	if (ret) {
		return ret;
	}
	depth--;

	return 0;
}

const char *dir_get_fs_path(void) {
	return path;
}

dir_list_t *dir_list(void) {
	DIR dir;
	FILINFO fno;
	FRESULT ret;
	dir_list_t *list = list_create();

	ret = f_opendir(&dir, path);
	if (ret != FR_OK) {
		return NULL;
	}

	while (1) {
		ret = f_readdir(&dir, &fno);
		if ((ret != FR_OK) || (fno.fname[0] == '\0')) {
			break;
		}

		list_add(list, &fno, sizeof(FILINFO), LIST_PREPEND);
	}

	/* Sort alphabetically ascending */
	list_sort(list, compare_ascending);

	f_closedir(&dir);
	return list;
}

dir_entry_t *dir_get_prev(dir_list_t *list, dir_entry_t *current) {
	if ((list == NULL) || (current == NULL)) {
		return NULL;
	}

	dir_entry_t *entry = list->head;
	do {
		if (entry->data == current->data) {
			break;
		}
		entry = entry->next;

	} while (entry != NULL);

	if ((entry == NULL) || (entry->prev == NULL)) {
		return list->tail;
	}
	return entry->prev;
}

dir_entry_t *dir_get_next(dir_list_t *list, dir_entry_t *current) {
	if ((list == NULL) || (current == NULL)) {
		return NULL;
	}

	dir_entry_t *entry = list->head;
	do {
		if (entry->data == current->data) {
			break;
		}
		entry = entry->next;

	} while (entry != NULL);

	if ((entry == NULL) || (entry->next == NULL)) {
		return list->head;
	}
	return entry->next;
}

void dir_list_free(dir_list_t *list) {
	list_destroy(list);
}
