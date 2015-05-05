/*
 * Copyright (C) 2014  Raphaël Poggi <poggi.raph@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef TASK_H
#define TASK_H

#include <list.h>
#include <queue.h>
#include <stddef.h>

#define NR_TASK	8

#define TASK_RUNNING		0
#define TASK_RUNNABLE		1
#define TASK_STOPPED		2
#define TASK_INTERRUPTIBLE	3
#define TASK_BLOCKED		4

#define TASK_STACK_START	0x10008000
#define TASK_STACK_OFFSET	0x00001000

struct registers
{
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
	unsigned int r12;
	unsigned int sp;
	unsigned int lr;
	unsigned int pc;

};

struct task
{
	unsigned int state;
	unsigned int pid;
	unsigned int priority;
	unsigned int start_stack;
	void (*func)(void);
	struct registers *regs;
	struct entry list_entry;
};

struct list runnable_tasks;

struct registers task_regs[NR_TASK];
struct task *task[NR_TASK];

void task_init(void);
void add_task(void (*func)(void), unsigned int priority);
void first_switch_task(int index_task);
void switch_task(struct task *task);
struct task *get_current_task(void);
struct task *find_next_task(void);

#endif /* TASK_H */
