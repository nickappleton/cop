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

/* C Compiler, OS and Platform Abstractions - Log Support.
 *
 * Logging is always disgusting. This is not meant to be some API which
 * everything uses. This should be used sparingly in modules which have many
 * possible user-related failure modes.
 *
 * Logging is achieved via a log interface which handles log events. Your
 * application is responsible for proving the callback to deal with log events
 * so you can make your log system as crazy-insane as you please.
 *
 * Software modules which wants to use this API as their logging mechanism
 * should:
 *
 *   * Provide a pointer to a cop_log_iface struct in all functions which
 *     require logging.
 *   * Should never assume that cop_log_iface is always not-NULL unless you
 *     explicitly document it on your API. This just means that you should
 *     check that your log interface is not-NULL before calling the log
 *     function.
 *   * Should preferably use the functions/function-like-macros documented in
 *     this API to log information rather than call the log function pointer
 *     directly. This is because we try to enable the compiler to completely
 *     remove particular logging calls during compilation if the user does not
 *     care about logging (i.e. notification of failure is good enough with
 *     no reasoning).
 *
 * The logging APIs provided by this interface are:
 *
 *   void cop_log_error(void *context, const char *format, ...)
 *   void cop_log_warning(void *context, const char *format, ...)
 *   void cop_log_debug(void *context, const char *format, ...)
 *   void cop_log_trace(void *context, const char *format, ...)
 *   void cop_log_error_tags(void *context, const char *tags, const char *format, ...)
 *   void cop_log_warning_tags(void *context, const char *tags, const char *format, ...)
 *   void cop_log_debug_tags(void *context, const char *tags, const char *format, ...)
 *   void cop_log_trace_tags(void *context, const char *tags, const char *format, ...)
 *
 * These are all implemented as either functions or function-like-macros. Thus
 * you should make no assumptions as to whether expressions will be evaluated
 * or not. They are designed to be completely compiled out if the builder
 * wishes to remove them via macros.
 *
 * - cop_log_trace will exist if COP_LOG_TRACE_ENABLE is defined and and is
 *   not zero.
 * - cop_log_debug will exist if COP_LOG_DEBUG_ENABLE is defined and not zero
 *   or if COP_LOG_DEBUG_ENABLE is not defined and either NDEBUG is not
 *   defined or cop_log_trace is enabled.
 * - cop_log_warning will exist if COP_LOG_WARNING_ENABLE is not defined or is
 *   defined to be zero.
 * - cop_log_error will exist if COP_LOG_ERROR_ENABLE is not defined or is
 *   defined to be zero.
 *
 * Tags is a filter string which may be passed into the interface. It is
 * designed to carry keywords (separated by colons ":"). It is useful when
 * you might want to look at the trace of a component, but don't want the log
 * to be polluted with the trace of other data. If you supply tags, it must
 * be a fixed string as the interface may at some point use the preprocessor
 * to prepend additional tags (like compilation filename) in particular
 * builds. */

#ifndef COP_LOG_H
#define COP_LOG_H

#include <stdarg.h>
#include <stddef.h> /* for NULL */
#include "cop_attributes.h"

/* An event used during development that is likely to be noise to even a
 * normal developer. */
#define COP_LOG_TYPE_TRACE   (0)

/* An event that is likely to be interesting in debug builds but would be
 * noise to an end user. */
#define COP_LOG_TYPE_DEBUG   (1)

/* An event which an end user may want to know about or which is likely to
 * result in an error occuring at a later point. */
#define COP_LOG_TYPE_WARNING (2)

/* An event which signifies that something that needed to work which
 * failed. */
#define COP_LOG_TYPE_ERROR   (3)

/* The log structure which all error messages will be passed to. */
struct cop_log_iface {
	void  *context;
	void (*log)(void *context, int log_type, const char *tags, const char *format, va_list args);

};

/* Control which debug functions will be enabled. */
#ifndef COP_LOG_TRACE_ENABLE
#define COP_LOG_TRACE_ENABLE    (0)
#endif
#ifndef COP_LOG_DEBUG_ENABLE
#if !defined(NDEBUG) || COP_LOG_TRACE_ENABLE != 0
#define COP_LOG_DEBUG_ENABLE    (1)
#else
#define COP_LOG_DEBUG_ENABLE    (0)
#endif
#endif
#ifndef COP_LOG_WARNING_ENABLE
#define COP_LOG_WARNING_ENABLE  (1)
#endif
#ifndef COP_LOG_ERROR_ENABLE
#define COP_LOG_ERROR_ENABLE    (1)
#endif

/* If this macro is defined and set and the compiler can get the information,
 * the filename of the source file and the line number where the log call was
 * made will be added as a tag to the log call in the form:
 *     filename(line number) */
#ifndef COP_LOG_ENABLE_FILE_TAG
#define COP_LOG_ENABLE_FILE_TAG (1)
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)

/* C99 - implement log helpers using macros. */

static COP_ATTR_UNUSED void cop_log__c99(struct cop_log_iface *log, int log_type, const char *tags, const char *format, ...)
{
	if (log != NULL) {
		va_list vl;
		va_start(vl, format);
		log->log(log->context, log_type, tags, format, vl);
		va_end(vl);
	}
}

#define COP_LOG_STR2(x) #x
#define COP_LOG_STR(x)  COP_LOG_STR2(x)

#if COP_LOG_ENABLE_FILE_TAG
#define COP_LOG_ADDITIONAL_TAG_STR __FILE__ "(" COP_LOG_STR(__LINE__) "):" 
#define COP_LOG_DEFAULT_TAG_ARG    __FILE__ "(" COP_LOG_STR(__LINE__) ")" 
#else
#define COP_LOG_ADDITIONAL_TAG_STR
#define COP_LOG_DEFAULT_TAG_ARG    NULL
#endif

#if COP_LOG_TRACE_ENABLE
#define cop_log_trace(a, ...)           cop_log__c99((a), COP_LOG_TYPE_TRACE, COP_LOG_DEFAULT_TAG_ARG, __VA_ARGS__)
#define cop_log_trace_tags(a, b, ...)   cop_log__c99((a), COP_LOG_TYPE_TRACE, COP_LOG_ADDITIONAL_TAG_STR b, __VA_ARGS__)
#else
#define cop_log_trace(a, ...)
#define cop_log_trace_tags(a, b, ...)
#endif
#if COP_LOG_DEBUG_ENABLE
#define cop_log_debug(a, ...)           cop_log__c99((a), COP_LOG_TYPE_DEBUG, COP_LOG_DEFAULT_TAG_ARG, __VA_ARGS__)
#define cop_log_debug_tags(a, b, ...)   cop_log__c99((a), COP_LOG_TYPE_DEBUG, COP_LOG_ADDITIONAL_TAG_STR b, __VA_ARGS__)
#else
#define cop_log_debug(a, ...)
#define cop_log_debug_tags(a, b, ...)
#endif
#if COP_LOG_WARNING_ENABLE
#define cop_log_warning(a, ...)         cop_log__c99((a), COP_LOG_TYPE_WARNING, COP_LOG_DEFAULT_TAG_ARG, __VA_ARGS__)
#define cop_log_warning_tags(a, b, ...) cop_log__c99((a), COP_LOG_TYPE_WARNING, COP_LOG_ADDITIONAL_TAG_STR b, __VA_ARGS__)
#else
#define cop_log_warning(a, ...)
#define cop_log_warning_tags(a, b, ...)
#endif
#if COP_LOG_ERROR_ENABLE
#define cop_log_error(a, ...)           cop_log__c99((a), COP_LOG_TYPE_ERROR, COP_LOG_DEFAULT_TAG_ARG, __VA_ARGS__)
#define cop_log_error_tags(a, b, ...)   cop_log__c99((a), COP_LOG_TYPE_ERROR, COP_LOG_ADDITIONAL_TAG_STR b, __VA_ARGS__)
#else
#define cop_log_error(a, ...)
#define cop_log_error_tags(a, b, ...)
#endif

#else

/* Not C99 - fallback on inline function implementations. */

#define LOG_ENABLE_FN(lfn_name, lfn_type) \
static COP_ATTR_UNUSED void lfn_name(struct cop_log_iface *log, const char *format, ...) \
{ \
	if (log != NULL) { \
		va_list vl; \
		va_start(vl, format); \
		log->log(log->context, lfn_type, NULL, format, vl); \
		va_end(vl); \
	} \
} \
static COP_ATTR_UNUSED void lfn_name ## _tags(struct cop_log_iface *log, const char *tags, const char *format, ...) \
{ \
	if (log != NULL) { \
		va_list vl; \
		va_start(vl, format); \
		log->log(log->context, lfn_type, tags, format, vl); \
		va_end(vl); \
	} \
}

#define LOG_DISABLE_FN(lfn_name) \
static COP_ATTR_UNUSED void lfn_name(struct cop_log_iface *log, const char *format, ...) \
{ \
	(void)log; \
	(void)format; \
} \
static COP_ATTR_UNUSED void lfn_name ## _tags(struct cop_log_iface *log, const char *tags, const char *format, ...) \
{ \
	(void)log; \
	(void)tags; \
	(void)format; \
}

#if COP_LOG_TRACE_ENABLE
LOG_ENABLE_FN(cop_log_trace, COP_LOG_TYPE_TRACE)
#else
LOG_DISABLE_FN(cop_log_trace)
#endif
#if COP_LOG_DEBUG_ENABLE
LOG_ENABLE_FN(cop_log_debug, COP_LOG_TYPE_DEBUG)
#else
LOG_DISABLE_FN(cop_log_debug)
#endif
#if COP_LOG_WARNING_ENABLE
LOG_ENABLE_FN(cop_log_warning, COP_LOG_TYPE_WARNING)
#else
LOG_DISABLE_FN(cop_log_warning)
#endif
#if COP_LOG_ERROR_ENABLE
LOG_ENABLE_FN(cop_log_error, COP_LOG_TYPE_ERROR)
#else
LOG_DISABLE_FN(cop_log_error)
#endif

#undef LOG_ENABLE_FN
#undef LOG_DISABLE_FN

#endif

#endif /* COP_LOG_H */
