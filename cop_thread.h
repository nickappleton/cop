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

/* C Compiler, OS and Platform Abstractions - Trivial Threading Support.
 *
 * This is not trying to create a fully-featured threading API - it is just
 * a minimal set that should have reasonable performance on most platforms.
 * Hence the two function thread API. */

#ifndef COP_THREAD_H
#define COP_THREAD_H

#include <stdlib.h>
#include <assert.h>
#include "cop_attributes.h"

/* Error codes returned by this library. */
#define COP_THREADERR_RESOURCE      (1)
#define COP_THREADERR_RANGE         (2)
#define COP_THREADERR_PERMISSIONS   (3)
#define COP_THREADERR_DEADLOCK      (4)
#define COP_THREADERR_UNKNOWN       (5)
#define COP_THREADERR_OUT_OF_MEMORY (6)

typedef void *(*cop_threadproc)(void *argument);

/* Define the cop_thread and cop_mutex types which are used by the library.
 * These are exposed so they can be placed on the stack, but you should not
 * use any of the members directly as it will obviously break the platform-
 * independent-ness of your code. They are defined at this point so we can
 * declare the API below. */
#if defined(_MSC_VER)
#include "windows.h"
typedef CRITICAL_SECTION   cop_mutex;
typedef CONDITION_VARIABLE cop_cond;
struct cop_thread_arg {
	void           *arg;
	void           *ret;
	cop_threadproc  proc;
};
typedef struct {
	HANDLE                handle;
	struct cop_thread_arg ret;
} cop_thread;
#else
#include <errno.h>
#include <pthread.h>
typedef pthread_cond_t  cop_cond;
typedef pthread_t       cop_thread;
typedef pthread_mutex_t cop_mutex;
#endif

/* The mutex API consists of the following 5 functions
 *
 *   cop_mutex_create   Initializes the supplied mutex object. This function
 *                      returns zero on success. It may fail if the system
 *                      does not have enough resources. The resources for the
 *                      mutex are not released until cop_mutex_destroy is
 *                      called.
 *   cop_mutex_destroy  Destroys the supplied mutex object. You may not use
 *                      any other mutex functions (apart from create) after
 *                      calling this function.
 *   cop_mutex_lock     Lock the supplied mutex object. If the mutex is
 *                      already held, the function blocks until the lock is
 *                      released and acquired by the calling thread.
 *   cop_mutex_unlock   Unlock the supplied locked mutex object. It is
 *                      undefined to call this on a mutex which has not been
 *                      locked.
 *   cop_mutex_trylock  Try to lock the supplied mutex object. If the mutex is
 *                      already held, the function immediately returns 0.
 *                      Otherwise, the lock was able to be obtained and the
 *                      function returns 1. */
static int cop_mutex_create(cop_mutex *mutex);
static void cop_mutex_destroy(cop_mutex *mutex);
static void cop_mutex_lock(cop_mutex *mutex);
static void cop_mutex_unlock(cop_mutex *mutex);
static int cop_mutex_trylock(cop_mutex *mutex);

/* The threading API consists of the following two functions.
 *
 *   cop_thread_create  Create and immediately start a new thread with the
 *                      entry-point of thread_proc. One pointer argument may
 *                      be passed to the thread using "argument". stack_size
 *                      is an optional argument to specify the amount of
 *                      stack memory which should be supplied to the thread,
 *                      if it has a value of zero, a system default will be
 *                      used. If "create_detached" is non-zero, the caller is
 *                      indicating that they do not want to use
 *                      cop_thread_join() to wait for the thread to complete
 *                      and that the resources allocated to the thread should
 *                      be destroyed as soon as the thread completes. This
 *                      implies that it is undefined to call cop_thread_join()
 *                      on a detached thread.
 *   cop_thread_join    Block the calling thread until the supplied thread
 *                      completes execution. This will also free all resources
 *                      allocated to the thread once it has completed. There
 *                      may only be one thread which is blocked using this
 *                      function. It is undefined to use this method on a
 *                      thread that was created as detached.
 *
 * The thread entry-point prototype is cop_threadproc. */
static int cop_thread_create(cop_thread *thread, cop_threadproc thread_proc, void *argument, size_t stack_size, int create_detached);
static int cop_thread_join(cop_thread thread, void **value);

/*****************************************************************************
 * IMPLEMENTATIONS
 ****************************************************************************/

#if defined(_MSC_VER)

static COP_ATTR_UNUSED COP_ATTR_ALWAYSINLINE int cop_mutex_create(cop_mutex *mutex)
{
	InitializeCriticalSection(mutex);
	return 0;
}

static COP_ATTR_UNUSED COP_ATTR_ALWAYSINLINE void cop_mutex_destroy(cop_mutex *mutex)
{
	DeleteCriticalSection(mutex);
}

static COP_ATTR_UNUSED COP_ATTR_ALWAYSINLINE void cop_mutex_lock(cop_mutex *mutex)
{
	EnterCriticalSection(mutex);
}

static COP_ATTR_UNUSED COP_ATTR_ALWAYSINLINE void cop_mutex_unlock(cop_mutex *mutex)
{
	LeaveCriticalSection(mutex);
}

static COP_ATTR_UNUSED COP_ATTR_ALWAYSINLINE int cop_mutex_trylock(cop_mutex *mutex)
{
	return TryEnterCriticalSection(mutex);
}

static DWORD __stdcall cop_win_thread_proc(LPVOID param)
{
	struct cop_thread_arg *arg = param;
	arg->ret = arg->proc(arg->arg);
	return 0;
}

static COP_ATTR_UNUSED int cop_thread_create(cop_thread *thread, cop_threadproc thread_proc, void *argument, size_t stack_size, int create_detached)
{
	thread->ret.arg  = argument;
	thread->ret.proc = thread_proc;
	thread->handle   = CreateThread(NULL, stack_size, cop_win_thread_proc, &(thread->ret), 0, NULL);
	if (thread->handle != NULL) {
		if (create_detached) {
			CloseHandle(thread->handle);

			/* We don't have to set the handle to invalid, but this is an
			 * almost zero cost way to assert if the caller called "join" on
			 * a detached thread. */
			thread->handle = NULL;
		}
		return 0;
	}
	return COP_THREADERR_UNKNOWN;
}

static COP_ATTR_UNUSED int cop_thread_join(cop_thread thread, void **value)
{
	DWORD err;
	assert(thread.handle != NULL && "join called on either a detached thread or invalid thread object");
	err = WaitForSingleObject(thread.handle, INFINITE);
	CloseHandle(thread.handle);
	if (err == WAIT_OBJECT_0) {
		if (value != NULL) {
			*value = thread.ret.ret;
		}
		return 0;
	}
	assert(err == WAIT_FAILED);
	return COP_THREADERR_UNKNOWN;
}

#else

/* Use pthreads. */

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

static COP_ATTR_UNUSED int cop_cond_create(cop_cond *cond)
{
	switch (pthread_cond_init(cond, NULL)) {
		case EBUSY:
		case EINVAL:
			/* Return values of EBUSY or EINVAL indicate that the condition
			 * variable was used in a stupid way. We abort here as a
			 * courtesy. */
			abort();
		case 0:
			return 0;
		default:
			return -1;
	}
}

static COP_ATTR_UNUSED void cop_cond_destroy(cop_cond *cond)
{
	if (pthread_cond_destroy(cond) != 0) {
		/* All the possible wait return values indicate that the condition
		 * variable was used in a stupid way. We abort here as a courtesy. */
		abort();
	}
}

static COP_ATTR_UNUSED void cop_cond_wait(cop_cond *cond, cop_mutex *mutex)
{
	if (pthread_cond_wait(cond, mutex) != 0) {
		/* All the possible wait return values indicate that the condition
		 * variable was used in a stupid way. We abort here as a courtesy. */
		abort();
	}
}

static COP_ATTR_UNUSED void cop_cond_signal(cop_cond *cond)
{
	if (pthread_cond_signal(cond) != 0) {
		/* All the possible wait return values indicate that the condition
		 * variable was used in a stupid way. We abort here as a courtesy. */
		abort();
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

static COP_ATTR_UNUSED int cop_thread_create(cop_thread *thread, cop_threadproc thread_proc, void *argument, size_t stack_size, int create_detached)
{
	pthread_attr_t pattr;
	int err, err2;
	if ((err = pthread_attr_init(&pattr)) != 0) {
		assert(err == ENOMEM); (void)err;
		return COP_THREADERR_RESOURCE;
	}
	if (stack_size != 0 && (err = pthread_attr_setstacksize(&pattr, stack_size)) != 0) {
		assert(err == EINVAL);
		err = COP_THREADERR_RANGE;
	}
	if (err == 0 && (pthread_attr_setdetachstate(&pattr, create_detached ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE)) != 0) {
		err = COP_THREADERR_UNKNOWN;
	}
	if (err == 0 && (err = pthread_create(thread, &pattr, thread_proc, argument)) != 0) {
		if (err == EPERM)
			err = COP_THREADERR_PERMISSIONS;
		else {
			assert(err == EAGAIN);
			err = COP_THREADERR_RESOURCE;
		}
	}
	err2 = pthread_attr_destroy(&pattr);
	assert(err2 == 0); (void)err2;
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

#endif

#endif /* COP_THREAD_H */
