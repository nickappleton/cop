/* Copyright (c) 2016 Nick Appleton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE. */

/* C Compiler, OS and Platform Abstractions - Threading Support. */

#ifndef COP_THREAD_H_
#define COP_THREAD_H_

#include "cop_attributes.h"

#define COP_THREADERR_RESOURCE      (1)
#define COP_THREADERR_RANGE         (2)
#define COP_THREADERR_PERMISSIONS   (3)
#define COP_THREADERR_DEADLOCK      (4)
#define COP_THREADERR_UNKNOWN       (5)
#define COP_THREADERR_OUT_OF_MEMORY (6)

typedef void *(*cop_threadproc)(void *argument);

#if 0
int
cop_thread_create
	(struct cop_thread *thread
	,cop_threadproc     thread_proc
	,void              *argument
	,size_t             stack_size
	,int                create_detached
	);

/* It is undefined to call cop_thread_join if the given thread was created
 * using the create_detached flag. The value pointer supplied may be NULL if
 * the return value from the thread is not interesting. */
int
cop_thread_join
	(cop_thread   thread
	,void        **value
	);

void
cop_thread_destroy
	(cop_thread thread
	);

int
cop_mutex_create
	(cop_mutex *mutex
	);

void
cop_mutex_destroy
	(cop_mutex *mutex
	);

void
cop_mutex_lock
	(cop_mutex *mutex
	);

void
cop_mutex_unlock
	(cop_mutex *mutex
	);

int
cop_mutex_trylock
	(cop_mutex *mutex
	);
#endif

/*****************************************************************************
 * IMPLEMENTATIONS
 ****************************************************************************/

#if defined(_MSC_VER)

#include "windows.h"
#include <assert.h>

typedef CRITICAL_SECTION cop_mutex;

struct cop_thread_arg {
	void           *arg;
	void           *ret;
	cop_threadproc  proc;
};

typedef struct {
	HANDLE                handle;
	struct cop_thread_arg ret;
} cop_thread;

static COP_ATTR_UNUSED COP_ATTR_ALWAYSINLINE int cop_mutex_create(cop_mutex *mutex)
{
	InitializeCriticalSection(mutex);
	return 0;
}

static COP_ATTR_ALWAYSINLINE void cop_mutex_destroy(cop_mutex *mutex)
{
	DeleteCriticalSection(mutex);
}

static COP_ATTR_ALWAYSINLINE void cop_mutex_lock(cop_mutex *mutex)
{
	EnterCriticalSection(mutex);
}

static COP_ATTR_ALWAYSINLINE void cop_mutex_unlock(cop_mutex *mutex)
{
	LeaveCriticalSection(mutex);
}

static COP_ATTR_ALWAYSINLINE int
cop_mutex_trylock(cop_mutex *mutex)
{
	return TryEnterCriticalSection(mutex);
}

static DWORD __stdcall cop_win_thread_proc(LPVOID param)
{
	struct cop_thread_arg *arg = param;
	arg->ret = arg->proc(arg->arg);
	return 0;
}

static
int
cop_thread_create
	(cop_thread     *thread
	,cop_threadproc  thread_proc
	,void            *argument
	,size_t           stack_size
	,int              create_detached
	)
{
	thread->ret.arg  = argument;
	thread->ret.proc = thread_proc;
	thread->handle   = CreateThread(NULL, stack_size, cop_win_thread_proc, &(thread->ret), 0, NULL);
	return (thread->handle == NULL);
}

static int cop_thread_join(cop_thread thread, void **value)
{
	DWORD err = WaitForSingleObject(thread.handle, INFINITE);
	if (err == WAIT_OBJECT_0) {
		if (value != NULL) {
			*value = thread.ret.ret;
		}
		return 0;
	}
	assert(err == WAIT_FAILED);
	return COP_THREADERR_UNKNOWN;
}

static void cop_thread_destroy(cop_thread thread)
{
	CloseHandle(thread.handle);
}

#else

/* Use pthreads. */

#include <errno.h>
#include <assert.h>
#include <pthread.h>

typedef pthread_t       cop_thread;
typedef pthread_mutex_t cop_mutex;

static int cop_translate_err(int err)
{
	switch (err) {
	case 0:
		return 0;
	case EPERM:
		return COP_THREADERR_PERMISSIONS;
	case EAGAIN:
		return COP_THREADERR_RESOURCE;
	case ENOMEM:
		return COP_THREADERR_OUT_OF_MEMORY;
	default:
		return COP_THREADERR_UNKNOWN;
	}
}

static COP_ATTR_UNUSED int cop_mutex_create(cop_mutex *mutex)
{
	return cop_translate_err(pthread_mutex_init(mutex, NULL));
}

static COP_ATTR_UNUSED void cop_mutex_destroy(cop_mutex *mutex)
{
	int err = pthread_mutex_destroy(mutex);
	assert(err == 0);
	(void)err;
}

static COP_ATTR_UNUSED COP_ATTR_ALWAYSINLINE void cop_mutex_lock(cop_mutex *mutex)
{
	int err = pthread_mutex_lock(mutex);
	assert(err == 0);
	(void)err;
}

static COP_ATTR_UNUSED COP_ATTR_ALWAYSINLINE void cop_mutex_unlock(cop_mutex *mutex)
{
	int err = pthread_mutex_unlock(mutex);
	assert(err == 0);
	(void)err;
}

static COP_ATTR_UNUSED COP_ATTR_ALWAYSINLINE int cop_mutex_trylock(cop_mutex *mutex)
{
	int err = pthread_mutex_trylock(mutex);
	assert((err == 0) || (err == EBUSY));
	return (err == 0);
}

static int build_thread
	(cop_thread     *thread
	,pthread_attr_t  *pattr
	,cop_threadproc  thread_proc
	,void            *argument
	,size_t           stack_size
	,int              create_detached
	)
{
	int err;

	if (stack_size != 0) {
		err = pthread_attr_setstacksize(pattr, stack_size);
		if (err) {
			assert(err == EINVAL);
			return COP_THREADERR_RANGE;
		}
	}

	/* The only time we expect setdetachstate to fail is when the supplied
	 * argument is invalid. There is no other defined reason for it to fail.
	 * We assume success in release builds and I think this is OK. */
	err = pthread_attr_setdetachstate(pattr, create_detached ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE);
	assert(err == 0); (void)err;

	err = pthread_create(thread, pattr, thread_proc, argument);
	if (err == EPERM)
		return COP_THREADERR_PERMISSIONS;

	if (err) {
		assert(err == EAGAIN);
		return COP_THREADERR_RESOURCE;
	}

	return 0;
}

static COP_ATTR_UNUSED
int
cop_thread_create
	(cop_thread     *thread
	,cop_threadproc  thread_proc
	,void            *argument
	,size_t           stack_size
	,int              create_detached
	)
{
	pthread_attr_t pattr;
	int err, err2;

	err = pthread_attr_init(&pattr);
	if (err) {
		assert(err == ENOMEM);
		return COP_THREADERR_RESOURCE;
	}

	err = build_thread(thread, &pattr, thread_proc, argument, stack_size, create_detached);

	err2 = pthread_attr_destroy(&pattr);
	assert(err2 == 0);
	(void)err2;

	return err;
}

static COP_ATTR_UNUSED int cop_thread_join(cop_thread thread, void **value)
{
	int err = pthread_join(thread, value);
	if (err) {
		assert(err == EDEADLK);
		return COP_THREADERR_DEADLOCK;
	}
	return 0;
}

static COP_ATTR_UNUSED void cop_thread_destroy(cop_thread thread)
{
	/* No effect for posix threads. */
}

#endif

#endif /* COP_THREAD_H_ */
