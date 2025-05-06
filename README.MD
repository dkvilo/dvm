# DVM Fantasy Console

Work in progress

To see sample programs check scripts


# Hello, World
```asm
    ; STORE STRING IN STRINGS TABLE WITH INDEX 0
    ; THIS WILL PUT INDEX INTO STACK SO YOU DON'T NEED TO PUSH_INT 0
    ; IF YOU IMMEDIATELY ARE CALLING THE CONSUMER FUNCTION
    ;
    PUSH_STRING "Hello, Sailor!"
    
    ; CALL ENGINE `print` WHERE `0` IS ADDRESS OF FUNCTION AND `1` IS NUMBER OF ARGUMENTS
    ; PUSH_INT  0 IS NO NEEDED IN THIS CASE
    ;
    CALL_NATIVE 0  1 

    HALT
```


# Compiling Examples 
```sh
./bin/dvmc ./scripts/hello-world.dvm ./bin/hello-world
./bin/dvm ./bin/hello-world
```

# Dependencies
- Raylib - Window, handling input and drawing
