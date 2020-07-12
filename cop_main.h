/* Copyright (c) 2020 Nick Appleton
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

/* Portable implementaiton of main()
 *
 * This header defines the following function-like macro which expands to a
 * C main() entry point:
 *
 *   COP_MAIN(main_fn_)
 *
 * main_fn_ must be a name of a function which has the prototype:
 *
 *   int main_fn(int argc, char *argv[]);
 *
 * The value of argc is the number of entries in argv. Each entry of argv is
 * a UTF-8 encoded Unicode string with the command line argument.
 */

#ifndef COP_MAIN_H
#define COP_MAIN_H

#ifdef WIN32
#include <windows.h>
#define COP_MAIN(main_fn_) \
int main(int argc, char *argv[]) { \
	LPWSTR cmdline = GetCommandLineW(); \
	char   **p_args; \
	char   **p_args2; \
	int nb_args, i, ret; \
	(void)argc; \
	(void)argv; \
	if (cmdline == NULL) \
		abort(); \
	p_args = (char **)CommandLineToArgvW(cmdline, &nb_args); \
	if (p_args == NULL || nb_args < 1) \
		abort(); \
	p_args2 = (char **)malloc(sizeof(char *) * nb_args); \
	if (p_args2 == NULL) \
		abort(); \
	for (i = 0; i < nb_args; i++) { \
		int x = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)(p_args[i]), -1, NULL, 0, NULL, NULL); \
		if (x <= 0) \
			abort(); \
		char *m = (char *)malloc(x); \
		if (m == NULL) \
			abort(); \
		if (x != WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)(p_args[i]), -1, m, x, NULL, NULL)) \
			abort(); \
		p_args[i] = m; \
		p_args2[i] = m; \
	} \
	ret = main_fn_(nb_args, p_args2); \
	for (i = 0; i < nb_args; i++) { \
		free(p_args[i]); \
	} \
	free(p_args2); \
	LocalFree(p_args); \
	return ret; \
}
#endif

#ifndef COP_MAIN
#define COP_MAIN(main_fn_) \
int main(int argc, char *argv[]) { \
	return main_fn_(argc, argv); \
}
#endif /* COP_MAIN */

#endif /* COP_MAIN_H */
