#include "command.h"

#include <string.h>
#include <stdio.h>

EngineCommand gCommands[MAX_COMMANDS] = { 0 };
int32_t       gCommandsCount          = 0;

static bool          running  = true;
static SDL_Window   *window   = NULL;
static SDL_Renderer *renderer = NULL;

void native_clear_color_buffer( VM *vm )
{
	Value R = vm->stack[vm->sp--];
	Value G = vm->stack[vm->sp--];
	Value B = vm->stack[vm->sp--];
	Value A = vm->stack[vm->sp--];

	gCommands[gCommandsCount++] = (EngineCommand){
	    .cType = CT_Clear,
	    .data  = { .clearCommand = (SDL_Color){ R.data.i, G.data.i, B.data.i, A.data.i } },
	};
}

void native_draw_text( VM *vm )
{
	if ( vm->sp < 5 )
	{
		vm_error( vm, "Stack underflow in native_draw_text" );
		return;
	}

	Value TextIndex = vm->stack[vm->sp--];

	Value Y = vm->stack[vm->sp--];
	Value X = vm->stack[vm->sp--];

	Value FontSize = vm->stack[vm->sp--];
	Value A        = vm->stack[vm->sp--];
	Value B        = vm->stack[vm->sp--];
	Value G        = vm->stack[vm->sp--];
	Value R        = vm->stack[vm->sp--];

	if ( TextIndex.type != VAL_INT || FontSize.type != VAL_INT || X.type != VAL_INT || Y.type != VAL_INT )
	{
		vm_error( vm, "native_draw_text expects string, x, y, fontSize" );
		return;
	}

	gCommands[gCommandsCount++] = (EngineCommand){

	    .cType = CT_DrawText,
	    .data =
	        {
	            .drawTextCommand =
	                {
	                    .color    = (SDL_Color){ R.data.i, G.data.i, B.data.i, A.data.i },
	                    .x        = X.data.i,
	                    .y        = Y.data.i,
	                    .fontSize = FontSize.data.i,
	                    .text     = vm->string_table[TextIndex.data.i],
	                },
	        },
	};
}

void native_draw_rect( VM *vm )
{
	Value X = vm->stack[vm->sp--];
	Value Y = vm->stack[vm->sp--];

	Value W = vm->stack[vm->sp--];
	Value H = vm->stack[vm->sp--];

	Value R = vm->stack[vm->sp--];
	Value G = vm->stack[vm->sp--];
	Value B = vm->stack[vm->sp--];
	Value A = vm->stack[vm->sp--];

	gCommands[gCommandsCount++] = (EngineCommand){

	    .cType = CT_DrawRect,
	    .data =
	        {
	            .drawRectCommand =
	                {
	                    .color = (SDL_Color){ R.data.i, G.data.i, B.data.i, A.data.i },
	                    .x     = X.data.i,
	                    .y     = Y.data.i,
	                    .w     = W.data.i,
	                    .h     = H.data.i,
	                },
	        },
	};
}

void native_handle_events()
{
	SDL_Event event;
	while ( SDL_PollEvent( &event ) )
	{
		if ( event.type == SDL_EVENT_QUIT )
		{
			running = false;
		}
	}
}

bool key_down[SDL_SCANCODE_COUNT]            = { 0 };
bool key_down_last_frame[SDL_SCANCODE_COUNT] = { 0 };
bool key_pressed[SDL_SCANCODE_COUNT]         = { 0 };

void native_update_scancodes_input_state()
{
	const bool *kb_state = SDL_GetKeyboardState( NULL );
	for ( int32_t i = 0; i < SDL_SCANCODE_COUNT; ++i )
	{
		key_down[i]            = kb_state[i];
		key_pressed[i]         = key_down[i] && !key_down_last_frame[i];
		key_down_last_frame[i] = key_down[i];
	}
}

void native_set_mouse_position_to_vm( VM *vm )
{
	float x, y;
	SDL_GetMouseState( &x, &y );

	vm->globals[0] = make_int( y );
	vm->globals[1] = make_int( x );
}

void native_process_frame( VM *vm )
{
	native_handle_events();
	native_update_scancodes_input_state();
	native_set_mouse_position_to_vm( vm );

	if ( !running )
	{
		vm->running = 0;
	}

	if ( key_pressed[SDL_SCANCODE_R] )
	{
		printf( "R Pressed\n" );
		vm_reset( vm );
	}

	for ( size_t i = 0; i < gCommandsCount; i++ )
	{
		EngineCommand cmd = gCommands[i];
		switch ( cmd.cType )
		{
		case CT_Clear:
		{
			SDL_Color color = cmd.data.clearCommand.color;
			SDL_SetRenderDrawColor( renderer, color.r, color.g, color.b, color.a );
			SDL_RenderClear( renderer );
		}
		break;

		case CT_DrawRect:
		{
			SDL_Color color = cmd.data.drawRectCommand.color;
			SDL_SetRenderDrawColor( renderer, color.r, color.g, color.b, color.a );

			const SDL_FRect rect = {
			    .h = cmd.data.drawRectCommand.h,
			    .w = cmd.data.drawRectCommand.w,
			    .x = cmd.data.drawRectCommand.x,
			    .y = cmd.data.drawRectCommand.y,
			};
			SDL_RenderFillRect( renderer, &rect );
		}
		break;

		case CT_DrawText:
		{
			const char *str      = cmd.data.drawTextCommand.text;
			int         x        = cmd.data.drawTextCommand.x;
			int         y        = cmd.data.drawTextCommand.y;
			int         fontSize = cmd.data.drawTextCommand.fontSize;
			SDL_Color   color    = cmd.data.drawTextCommand.color;

			SDL_SetRenderDrawColor( renderer, color.r, color.g, color.b, color.a );
			float scale = fontSize / SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;

			SDL_SetRenderScale( renderer, scale, scale );
			SDL_RenderDebugText( renderer, (float)x / scale, (float)y / scale, str );
			SDL_SetRenderScale( renderer, 1, 1 );
		}
		break;

		default:
			break;
		}
	}

	SDL_RenderTexture( renderer, NULL, NULL, NULL );
	SDL_RenderPresent( renderer );

	gCommandsCount = 0;
}

void native_randint( VM *vm )
{
	Value Max = vm->stack[vm->sp--];
	Value Min = vm->stack[vm->sp--];

	if ( Max.type != VAL_INT || Min.type != VAL_INT )
	{
		vm_error( vm, "native_randint expects min and max" );
		return;
	}

	vm->stack[++vm->sp] = make_int( Min.data.i + rand() % ( Max.data.i - Min.data.i + 1 ) );
}

void native_make_window( VM *vm )
{
	if ( vm->sp < 1 )
	{
		vm_error( vm, "Stack underflow in native_make_window" );
		return;
	}

	Value height = vm->stack[vm->sp--];
	Value width  = vm->stack[vm->sp--];

	if ( width.type != VAL_INT || height.type != VAL_INT )
	{
		vm_error( vm, "make_window expects two integers, width and height" );
		return;
	}

	SDL_WindowFlags FLAGS = SDL_WINDOW_OPENGL;
	if ( !SDL_CreateWindowAndRenderer( "A Window", width.data.i, height.data.i, FLAGS, &window, &renderer ) )
	{
		SDL_LogError( SDL_LOG_CATEGORY_APPLICATION, "Error: %s\n", SDL_GetError() );
	}

	printf( "[VM] Window Was created (%dx%d)!\n", width.data.i, height.data.i );
}

void native_print( VM *vm )
{
	if ( vm->sp < 0 )
	{
		vm_error( vm, "Stack underflow in native_print" );
		return;
	}

	Value val = vm->stack[vm->sp--];

	switch ( val.type )
	{
	case VAL_INT:
		printf( "%d\n", val.data.i );
		break;
	case VAL_FLOAT:
		printf( "%f\n", val.data.f );
		break;
	case VAL_BOOL:
		printf( "%s\n", val.data.boolean ? "true" : "false" );
		break;
	case VAL_PTR:
		printf( "Pointer: %p\n", val.data.ptr );
		break;
	case VAL_STRING:
		printf( "%s\n", val.data.str );
		break;
	case VAL_NULL:
		printf( "null\n" );
		break;
	}
}

void native_concat( VM *vm )
{
	if ( vm->sp < 1 )
	{
		vm_error( vm, "Stack underflow in native_concat" );
		return;
	}

	Value b = vm->stack[vm->sp--];
	Value a = vm->stack[vm->sp--];

	if ( a.type != VAL_STRING || b.type != VAL_STRING )
	{
		vm_error( vm, "native_concat expects two strings" );
		return;
	}

	size_t len_a  = strlen( a.data.str );
	size_t len_b  = strlen( b.data.str );
	char  *result = gc_alloc( vm, len_a + len_b + 1, VAL_STRING );

	if ( !result )
	{
		vm_error( vm, "Failed to allocate memory for string concatenation" );
		return;
	}

	strcpy( result, a.data.str );
	strcat( result, b.data.str );

	Value result_val;
	result_val.type     = VAL_STRING;
	result_val.data.str = result;

	vm->stack[++vm->sp] = result_val;
}
