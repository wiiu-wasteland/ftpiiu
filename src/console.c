/*
 * console.c
 * Console output
 */

#include <coreinit/memfrmheap.h>
#include <coreinit/memheap.h>
#include <coreinit/screen.h>
#include <coreinit/thread.h>
#include <coreinit/cache.h>
#include <coreinit/time.h>
#include <malloc.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#define CONSOLE_CALLBACK_PRIORITY	(100)
#define CONSOLE_MAX_LINES_TV		(27)
#define CONSOLE_MAX_LINES_DRC		(18)
#define CONSOLE_FRAME_HEAP_TAG		(0x46545055)

static MEMHeapHandle MEM1Heap;

static void *sBufferTV, *sBufferDRC;
static uint32_t sBufferSizeTV, sBufferSizeDRC;

static char *consoleArrayTv[CONSOLE_MAX_LINES_TV] = {0};
static char *consoleArrayDrc[CONSOLE_MAX_LINES_DRC] = {0};

static void console_refresh()
{
	if (!in_foreground)
		return;

	OSScreenClearBufferEx(SCREEN_TV, 0);
	OSScreenClearBufferEx(SCREEN_DRC, 0);
	
	for(int i = 0; i < CONSOLE_MAX_LINES_TV; i++)
	{
		if(consoleArrayTv[i])
			OSScreenPutFontEx(SCREEN_TV, 0, i, consoleArrayTv[i]);
	}
	
	for(int i = 0; i < CONSOLE_MAX_LINES_DRC; i++)
	{
		if(consoleArrayDrc[i])
			OSScreenPutFontEx(SCREEN_DRC, 0, i, consoleArrayDrc[i]);
	}
	
	DCFlushRange(sBufferTV, sBufferSizeTV);
	DCFlushRange(sBufferDRC, sBufferSizeDRC);
	OSScreenFlipBuffersEx(SCREEN_TV);
	OSScreenFlipBuffersEx(SCREEN_DRC);
}

void console_clear()
{
	for(int i = 0; i < CONSOLE_MAX_LINES_TV; i++)
	{
		if(!consoleArrayTv[i])
			continue;
		free(consoleArrayTv[i]);
		consoleArrayTv[i] = NULL;
	}

	for(int i = 0; i < CONSOLE_MAX_LINES_DRC; i++)
	{
		if(!consoleArrayDrc[i])
			continue;
		free(consoleArrayDrc[i]);
		consoleArrayDrc[i] = NULL;
	}
}

void console_printf(const char *format, ...)
{
	char * tmp = NULL;

	va_list va;
	va_start(va, format);
	if((vasprintf(&tmp, format, va) >= 0) && tmp)
	{
		if(consoleArrayTv[0])
			free(consoleArrayTv[0]);
		if(consoleArrayDrc[0])
			free(consoleArrayDrc[0]);

		for(int i = 1; i < CONSOLE_MAX_LINES_TV; i++)
			consoleArrayTv[i-1] = consoleArrayTv[i];

		for(int i = 1; i < CONSOLE_MAX_LINES_DRC; i++)
			consoleArrayDrc[i-1] = consoleArrayDrc[i];

		if(strlen(tmp) > 79)
			tmp[79] = 0;

		consoleArrayTv[CONSOLE_MAX_LINES_TV-1] = strdup(tmp);
		consoleArrayDrc[CONSOLE_MAX_LINES_DRC-1] = (tmp);
	}
	va_end(va);

	console_refresh();
}

static void console_acquire_foreground()
{
	if (in_foreground)
		return;

	/* Reinitialize OSScreen after entering foreground */
	MEM1Heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
	MEMRecordStateForFrmHeap(MEM1Heap, CONSOLE_FRAME_HEAP_TAG);
	
	OSScreenInit();
	
	sBufferSizeTV = OSScreenGetBufferSizeEx(SCREEN_TV);
	sBufferSizeDRC = OSScreenGetBufferSizeEx(SCREEN_DRC);
	
	sBufferTV = MEMAllocFromFrmHeapEx(MEM1Heap, sBufferSizeTV, 4);
	sBufferDRC = MEMAllocFromFrmHeapEx(MEM1Heap, sBufferSizeDRC, 4);
	
	OSScreenSetBufferEx(SCREEN_TV, sBufferTV);
	OSScreenSetBufferEx(SCREEN_DRC, sBufferDRC);
	
	OSScreenEnableEx(SCREEN_TV, 1);
	OSScreenEnableEx(SCREEN_DRC, 1);

	in_foreground = 1;

	/* Rerender screen content */
	console_refresh();
}

static void console_release_foreground()
{
	if (!in_foreground)
		return;

	/* Deinitialize OSScreen before entering background */
	MEMFreeByStateToFrmHeap(MEM1Heap, CONSOLE_FRAME_HEAP_TAG);

	in_foreground = 0;
}

void console_init()
{
	/* Initialize console content */
	for(int i = 0; i < CONSOLE_MAX_LINES_TV; i++)
		consoleArrayTv[i] = NULL;

	for(int i = 0; i < CONSOLE_MAX_LINES_DRC; i++)
		consoleArrayDrc[i] = NULL;

	/* Run OSScreen inialization */
	console_in_foreground();
}

void console_deinit()
{
	console_release_foreground();

	/* Cleanup console content */
	for(int i = 0; i < CONSOLE_MAX_LINES_TV; i++)
	{
		if(!consoleArrayTv[i])
			continue;
		free(consoleArrayTv[i]);
		consoleArrayTv[i] = NULL;
	}

	for(int i = 0; i < CONSOLE_MAX_LINES_DRC; i++)
	{
		if(!consoleArrayDrc[i])
			continue;
		free(consoleArrayDrc[i]);
		consoleArrayDrc[i] = NULL;
	}
}
