#include <raylib.h>

#include "engine/engine.h"

#include "engine/command.c"
#include "engine/engine.c"
#include "vm/vm.c"

int main( int argc, char **argv )
{
	engine_run( argc, argv );
	return 0;
}