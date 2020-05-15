#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#ifdef __cplusplus
extern "C"{
#endif

#include <stdarg.h>

void console_printf(const char *format, ...);

void console_in_foreground();
void console_release_foreground();

void console_init();
void console_deinit();

#ifdef __cplusplus
}
#endif

#endif /* _CONSOLE_H_ */
