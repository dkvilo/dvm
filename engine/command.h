#ifndef __ME_ENGINE_COMMAND_H_
#define __ME_ENGINE_COMMAND_H_

#include <stdint.h>
#include <SDL3/SDL.h>

#include "../vm/vm.h"

#define MAX_COMMANDS 1024

typedef enum CommandType
{
	CT_Clear,
	CT_DrawRect,
	CT_DrawText
} CommandType;

typedef struct ClearCommand
{
	SDL_Color color;
} ClearCommand;

typedef struct DrawRectCommand
{
	SDL_Color color;
	int       x, y;
	int       w, h;
} DrawRectCommand;

typedef struct DrawTextCommand
{
	SDL_Color   color;
	int         fontSize;
	int         x, y;
	const char *text;
} DrawTextCommand;

typedef struct EngineCommand
{
	CommandType cType;
	union
	{
		ClearCommand    clearCommand;
		DrawRectCommand drawRectCommand;
		DrawTextCommand drawTextCommand;
	} data;
} EngineCommand;

void native_clear_color_buffer( VM *vm );

void native_draw_text( VM *vm );

void native_draw_rect( VM *vm );

void native_process_frame( VM *vm );

void native_randint( VM *vm );

void native_make_window( VM *vm );

void native_print( VM *vm );

void native_concat( VM *vm );

#endif // __ME_ENGINE_RENDER_COMMAND_H_