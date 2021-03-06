/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#if MULTI_CONSOLE_API

#include <assert.h>

#include <drivers/console.h>

console_t *console_list;
uint8_t console_state = CONSOLE_FLAG_BOOT;

int console_register(console_t *console)
{
	IMPORT_SYM(console_t *, __STACKS_START__, stacks_start)
	IMPORT_SYM(console_t *, __STACKS_END__, stacks_end)

	/* Assert that the struct is not on the stack (common mistake). */
	assert((console < stacks_start) || (console >= stacks_end));
	/* Assert that we won't make a circle in the list. */
	assert(!console_is_registered(console));

	console->next = console_list;
	console_list = console;

	/* Return 1 for convenient tail-calling from console_xxx_register(). */
	return 1;
}

console_t *console_unregister(console_t *to_be_deleted)
{
	console_t **ptr;

	assert(to_be_deleted != NULL);

	for (ptr = &console_list; *ptr != NULL; ptr = &(*ptr)->next)
		if (*ptr == to_be_deleted) {
			*ptr = (*ptr)->next;
			return to_be_deleted;
		}

	return NULL;
}

int console_is_registered(console_t *to_find)
{
	console_t *console;

	assert(to_find != NULL);

	for (console = console_list; console != NULL; console = console->next)
		if (console == to_find)
			return 1;

	return 0;
}

void console_switch_state(unsigned int new_state)
{
	console_state = new_state;
}

void console_set_scope(console_t *console, unsigned int scope)
{
	assert(console != NULL);

	console->flags = (console->flags & ~CONSOLE_FLAG_SCOPE_MASK) | scope;
}

int console_putc(int c)
{
	int err = ERROR_NO_VALID_CONSOLE;
	console_t *console;

	for (console = console_list; console != NULL; console = console->next)
		if (console->flags & console_state) {
			int ret = console->putc(c, console);
			if ((err == ERROR_NO_VALID_CONSOLE) || (ret < err))
				err = ret;
		}

	return err;
}

int console_getc(void)
{
	int err = ERROR_NO_VALID_CONSOLE;
	console_t *console;

	do {	/* Keep polling while at least one console works correctly. */
		for (console = console_list; console != NULL;
		     console = console->next)
			if (console->flags & console_state) {
				int ret = console->getc(console);
				if (ret >= 0)
					return ret;
				if (err != ERROR_NO_PENDING_CHAR)
					err = ret;
			}
	} while (err == ERROR_NO_PENDING_CHAR);

	return err;
}

int console_flush(void)
{
	int err = ERROR_NO_VALID_CONSOLE;
	console_t *console;

	for (console = console_list; console != NULL; console = console->next)
		if (console->flags & console_state) {
			int ret = console->flush(console);
			if ((err == ERROR_NO_VALID_CONSOLE) || (ret < err))
				err = ret;
		}

	return err;
}

#endif	/* MULTI_CONSOLE_API */
