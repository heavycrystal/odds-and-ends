//  A simple CHIP-8 emulator written by Kevin K. Biju
//  Usage: simply pass the ROM as the LAST command line argument, must be of size 0x800 or less according with the CHIP-8 specification.
//  Requires SDL2 to be installed. Enjoy!

#include    <cstdlib>
#include    <climits>
#include    <cassert>

#include    <iostream>
#include    <fstream>
#include    <stack>
#include    <random>
#include    <chrono>
#include    <thread>

#include    <SDL2/SDL.h>

typedef     uint8_t                 u8;
typedef     uint16_t                u16;

#if         CHAR_BIT != 8
#error      "CHAR_BIT not 8, refusing to compile."
#endif

#define     WINDOW_NAME             "Project Vanderblit"
#define     SCREEN_WIDTH            64
#define     SCREEN_HEIGHT           32
#define     SCALING_FACTOR          16
#define     SCALING_MODE            "linear"
#define     MAX_RECURSION_DEPTH     16  
#define     CPU_FREQUENCY           1000
#define     INST_PER_CLOCK          1
#define     CLIPPING_GRAPHICS       1

#define     INIT_SDL_ERROR          "SDL2 initialization failed. Error encountered was: " << SDL_GetError() << "\n"
#define     INVALID_OPCODE_ERROR    "Invalid opcode encountered: " << std::hex << HEX0 << std::hex << HEX1 << " " << std::hex << HEX2 << std::hex << HEX3 << "\n"
#define     STACK_OVERFLOW_ERROR    "Call stack overflow, verify program correctness?" << "\n"

#define     BIT_CUT(input, cut, count)  (((input) >> (cut)) & ((1 << (count)) - 1))
#define     CMP_HEX(location, val)      (instruction_hex[location ## u] == 0x ## val)

#define     HEX0                    instruction_hex[0u]
#define     HEX1                    instruction_hex[1u]
#define     HEX2                    instruction_hex[2u]
#define     HEX3                    instruction_hex[3u]

#define     _X                      HEX1
#define     _Y                      HEX2
#define     _N                      HEX3
#define     _NN                     BIT_CUT(instruction, 0, 8)
#define     _NNN                    BIT_CUT(instruction, 0, 12)

#define     SDL_KEYDOWN(key)        case SDLK_ ## key: \
                                    keyboard_press[0x ## key] = true; \
                                    last_key_pressed = 0x ## key; \
                                    break
#define     SDL_KEYUP(key)          case SDLK_ ## key: \
                                    keyboard_press[0x ## key] = false; \
                                    break

#define     vX                      chip8_cpu_registers.v[_X]
#define     vY                      chip8_cpu_registers.v[_Y]
#define     vF                      chip8_cpu_registers.v[0xF]

static std::random_device                   hrng;
static std::mt19937                         engine;
static std::uniform_int_distribution<u8>    generator(0x00, 0xFF);
static std::stack<u16>                      call_stack;

struct  chip8_cpu_registers_struct
{
    u8          v[16] = { };

    u16         i = 0x0000;     
    u16         pc = 0x0200;

    u8          delay_timer = 0x00;
    u8          sound_timer = 0x00;
}       chip8_cpu_registers;

u8      memory[0x1000] = {  0xF0, 0x90, 0x90, 0x90, 0xF0,   //  0
                            0x20, 0x60, 0x20, 0x20, 0x70,   //  1
                            0xF0, 0x10, 0xF0, 0x80, 0xF0,   //  2
                            0xF0, 0x10, 0xF0, 0x10, 0xF0,   //  3
                            0x90, 0x90, 0xF0, 0x10, 0x10,   //  4
                            0xF0, 0x80, 0xF0, 0x10, 0xF0,   //  5
                            0xF0, 0x80, 0xF0, 0x90, 0xF0,   //  6
                            0xF0, 0x10, 0x20, 0x40, 0x40,   //  7
                            0xF0, 0x90, 0xF0, 0x90, 0xF0,   //  8
                            0xF0, 0x90, 0xF0, 0x10, 0xF0,   //  9
                            0xF0, 0x90, 0xF0, 0x90, 0x90,   //  A
                            0xE0, 0x90, 0xE0, 0x90, 0xE0,   //  B
                            0xF0, 0x80, 0x80, 0x80, 0xF0,   //  C
                            0xE0, 0x90, 0x90, 0x90, 0xE0,   //  D
                            0xF0, 0x80, 0xF0, 0x80, 0xF0,   //  E
                            0xF0, 0x80, 0xF0, 0x80, 0x80    //  F   
                        };
bool    keyboard_press[16] = { };
u8      last_key_pressed = 0xFF;
u8      screen_buffer[SCREEN_WIDTH][SCREEN_HEIGHT] = { };

struct  sdl2_internals_struct {
    SDL_Renderer*   renderer = nullptr;
	SDL_Window*     window = nullptr;
}       sdl2_internals;

void    event_listener(void) {
    SDL_Event   event;

    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case    SDL_QUIT:
                exit(0);
            case    SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    SDL_KEYDOWN(0);
                    SDL_KEYDOWN(1);
                    SDL_KEYDOWN(2);
                    SDL_KEYDOWN(3);
                    SDL_KEYDOWN(4);
                    SDL_KEYDOWN(5);
                    SDL_KEYDOWN(6);
                    SDL_KEYDOWN(7);
                    SDL_KEYDOWN(8);
                    SDL_KEYDOWN(9);
                    SDL_KEYDOWN(a);
                    SDL_KEYDOWN(b);
                    SDL_KEYDOWN(c);
                    SDL_KEYDOWN(d);
                    SDL_KEYDOWN(e);
                    SDL_KEYDOWN(f);
                    default:
                        break;
                }
                break;                
            case    SDL_KEYUP:
                switch(event.key.keysym.sym) {
                    SDL_KEYUP(0);
                    SDL_KEYUP(1);
                    SDL_KEYUP(2);
                    SDL_KEYUP(3);
                    SDL_KEYUP(4);
                    SDL_KEYUP(5);
                    SDL_KEYUP(6);
                    SDL_KEYUP(7);
                    SDL_KEYUP(8);
                    SDL_KEYUP(9);
                    SDL_KEYUP(a);
                    SDL_KEYUP(b);
                    SDL_KEYUP(c);
                    SDL_KEYUP(d);
                    SDL_KEYUP(e);
                    SDL_KEYUP(f);
                    default:
                        break;
                }
                break;                                                                                                                                                                                                                                                                             
            default:
                break;    
                
        }
    }
}

void    instruction_parse_and_execute(void) {
    u16 instruction = (memory[chip8_cpu_registers.pc] << 8) + memory[chip8_cpu_registers.pc + 1];
    u8  instruction_hex[4u] = { (u8)BIT_CUT(instruction, 12, 4), (u8)BIT_CUT(instruction, 8, 4), (u8)BIT_CUT(instruction, 4, 4), (u8)BIT_CUT(instruction, 0, 4) };

    if(CMP_HEX(0, 0) && CMP_HEX(1, 0) && CMP_HEX(2, E) && CMP_HEX(3, 0)) {
        memset(screen_buffer, 0, sizeof(bool) * SCREEN_WIDTH * SCREEN_HEIGHT);
    }
    else if(CMP_HEX(0, 0) && CMP_HEX(1, 0) && CMP_HEX(2, E) && CMP_HEX(3, E)) {
        chip8_cpu_registers.pc = call_stack.top();
        call_stack.pop();          
    }
    else if(CMP_HEX(0, 1)) {
        /*  since PC is incremented by 2 always, offsetting */
        chip8_cpu_registers.pc = _NNN - 2;      
    }
    else if(CMP_HEX(0, 2)) {
        if(call_stack.size() > MAX_RECURSION_DEPTH) {
            std::cerr << STACK_OVERFLOW_ERROR;
            exit(1);
        }
        call_stack.push(chip8_cpu_registers.pc);
        chip8_cpu_registers.pc = _NNN - 2;        
    }
    else if(CMP_HEX(0, 3)) {
        if(vX == _NN) {
            chip8_cpu_registers.pc = chip8_cpu_registers.pc + 2;
        }
    }
    else if(CMP_HEX(0, 4)) {
        if(vX != _NN) {
            chip8_cpu_registers.pc = chip8_cpu_registers.pc + 2;
        }        
    }
    else if(CMP_HEX(0, 5))
    {
        if(vX == vY) {
            chip8_cpu_registers.pc = chip8_cpu_registers.pc + 2;
        }        
    }
    else if(CMP_HEX(0, 6)) {
        vX = _NN;
    }
    else if(CMP_HEX(0, 7)) {
        vX = vX + _NN;
    }
    else if(CMP_HEX(0, 8) && CMP_HEX(3, 0)) {
        vX = vY;        
    } 
    else if(CMP_HEX(0, 8) && CMP_HEX(3, 1)) {
        vX = vX | vY;        
        vF = 0;
    }  
    else if(CMP_HEX(0, 8) && CMP_HEX(3, 2)) {
        vX = vX & vY;   
        vF = 0;     
    }
    else if(CMP_HEX(0, 8) && CMP_HEX(3, 3)) {
        vX = vX ^ vY; 
        vF = 0;       
    }
    else if(CMP_HEX(0, 8) && CMP_HEX(3, 4)) {
        vF = ((0xFF - vX) < vY);
        vX = vX + vY;        
    }
    else if(CMP_HEX(0, 8) && CMP_HEX(3, 5)) {
        vF = (vY <= vX);
        vX = vX - vY;        
    }
    else if(CMP_HEX(0, 8) && CMP_HEX(3, 6)) {
        vF = BIT_CUT(vY, 0, 1);
        vX = vY >> 1;
    }
    else if(CMP_HEX(0, 8) && CMP_HEX(3, 7)) {
        vF = (vY >= vX);
        vX = vY - vX; 
    }
    else if(CMP_HEX(0, 8) && CMP_HEX(3, E)) {
        vF = BIT_CUT(vY, 7, 1);
        vX = vY << 1;
    }
    else if(CMP_HEX(0, 9)) {
        if(vX != vY) {
            chip8_cpu_registers.pc = chip8_cpu_registers.pc + 2;
        } 
    }
    else if(CMP_HEX(0, A)) {
        chip8_cpu_registers.i = _NNN;
    }
    else if(CMP_HEX(0, B)) {
        chip8_cpu_registers.pc = chip8_cpu_registers.v[0x0] + _NNN - 2;
    }
    else if(CMP_HEX(0, C)) {
        vX = (generator(engine) & _NN);
    }
    else if(CMP_HEX(0, D)) {
        for(u8 i = 0; i < _N; i++) {
            for(u8 j = 0; j < 8; j++) {
#if CLIPPING_GRAPHICS
                if((vY + i) >= SCREEN_HEIGHT) {
                    continue;
                }
#endif
                if(memory[chip8_cpu_registers.i] & (1 << (7 - j))) {
                    vF = 1;
                }
                screen_buffer[(vX + j) % SCREEN_WIDTH][(vY + i) % SCREEN_HEIGHT] = screen_buffer[(vX + j) % SCREEN_WIDTH][(vY + i) % SCREEN_HEIGHT] ^ 
                ((memory[chip8_cpu_registers.i + i] & (1 << (7 - j))) >> (7 - j));
            }    
        }
    }
    else if(CMP_HEX(0, E) && CMP_HEX(2, 9) && CMP_HEX(3, E)) {
        if(keyboard_press[vX] == true) {
            chip8_cpu_registers.pc = chip8_cpu_registers.pc + 2;
        }       
    }  
    else if(CMP_HEX(0, E) && CMP_HEX(2, A) && CMP_HEX(3, 1)) {
        if(keyboard_press[vX] == false) {
            chip8_cpu_registers.pc = chip8_cpu_registers.pc + 2;
        }     
    }
    else if(CMP_HEX(0, F) && CMP_HEX(2, 0) && CMP_HEX(3, 7)) {
        vX = chip8_cpu_registers.delay_timer;
    } 
    else if(CMP_HEX(0, F) && CMP_HEX(2, 0) && CMP_HEX(3, A)) {
        u8 cached_last_key_pressed = last_key_pressed;
        while(last_key_pressed == cached_last_key_pressed) {
            event_listener();
        }
        vX = last_key_pressed;
    } 
    else if(CMP_HEX(0, F) && CMP_HEX(2, 1) && CMP_HEX(3, 5)) {
        chip8_cpu_registers.delay_timer = vX;
    } 
    else if(CMP_HEX(0, F) && CMP_HEX(2, 1) && CMP_HEX(3, 8)) {
        chip8_cpu_registers.sound_timer = vX;
    } 
    else if(CMP_HEX(0, F) && CMP_HEX(2, 1) && CMP_HEX(3, E)) {
        chip8_cpu_registers.i = chip8_cpu_registers.i + vX;
    }
    else if(CMP_HEX(0, F) && CMP_HEX(2, 2) && CMP_HEX(3, 9)) {
        chip8_cpu_registers.i = (0x5 * vX);
    }
    else if(CMP_HEX(0, F) && CMP_HEX(2, 3) && CMP_HEX(3, 3)) {
        memory[chip8_cpu_registers.i] = (vX / 100);
        memory[chip8_cpu_registers.i + 1] = ((vX / 10) % 10);
        memory[chip8_cpu_registers.i + 2] = (vX % 10);
    } 
    else if(CMP_HEX(0, F) && CMP_HEX(2, 5) && CMP_HEX(3, 5)) {
        for(u8 i = 0; i <= _X; i++) {
            memory[chip8_cpu_registers.i + i] = chip8_cpu_registers.v[i];
        }
        chip8_cpu_registers.i++;
    }  
    else if(CMP_HEX(0, F) && CMP_HEX(2, 6) && CMP_HEX(3, 5)) {
        for(u8 i = 0; i <= _X; i++) {
            chip8_cpu_registers.v[i] = memory[chip8_cpu_registers.i + i];
        }
        chip8_cpu_registers.i++;
    }
    else {
        std::cerr << INVALID_OPCODE_ERROR;
        exit(1);
    }

    chip8_cpu_registers.pc = chip8_cpu_registers.pc + 2;
    return;                                                          
}

void    screen_render(SDL_Renderer* renderer) {
    for(int i = 0; i < SCREEN_WIDTH; i++) {
        for(int j = 0; j < SCREEN_HEIGHT; j++) {
            if(screen_buffer[i][j] == 1) {
                for(int k = 0; k < SCALING_FACTOR; k++) {
                    for(int l = 0; l < SCALING_FACTOR; l++) {
                        SDL_RenderDrawPoint(renderer, (SCALING_FACTOR * i) + k, (SCALING_FACTOR * j) + l);
                    }
                }
            }
        }
    }
}

void    timer_audio_thread(void) {
    while(true) {
        if(chip8_cpu_registers.delay_timer > 0) {
            chip8_cpu_registers.delay_timer--;
        }
        if(chip8_cpu_registers.sound_timer > 0) {
            chip8_cpu_registers.sound_timer--;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));        
    }
}

int     main(int argc, char** argv) {
    std::ifstream file_handle(argv[argc - 1u], std::ios::in | std::ios::binary);

    file_handle.ignore(std::numeric_limits<std::streamsize>::max());
    std::streamsize size = file_handle.gcount();
    assert(size < (0x1000 - 0x200));
    file_handle.clear();  
    file_handle.seekg(0, std::ios_base::beg);
    u8* file_data = &memory[0x200];
    file_handle.read(reinterpret_cast<char*>(file_data), size);

    engine.seed(hrng());

    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << INIT_SDL_ERROR;
        exit(1);     
    }
    sdl2_internals.window = SDL_CreateWindow(WINDOW_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
        SCREEN_WIDTH * SCALING_FACTOR, SCREEN_HEIGHT * SCALING_FACTOR, 0);  
    if(sdl2_internals.window == nullptr) {
        std::cerr << INIT_SDL_ERROR;
        exit(1);     
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, SCALING_MODE);
    sdl2_internals.renderer = SDL_CreateRenderer(sdl2_internals.window, -1, SDL_RENDERER_ACCELERATED);
    if(sdl2_internals.renderer == nullptr) {
        std::cerr << INIT_SDL_ERROR;
        exit(1);        
    }

    std::thread thread_dispatch(timer_audio_thread);

    while(true) {
        SDL_SetRenderDrawColor(sdl2_internals.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(sdl2_internals.renderer);
        SDL_SetRenderDrawColor(sdl2_internals.renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);   
        screen_render(sdl2_internals.renderer); 

        for(int i = 0; i < INST_PER_CLOCK; i++) {
            event_listener();
            instruction_parse_and_execute();
        }    
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / CPU_FREQUENCY));  
        SDL_RenderPresent(sdl2_internals.renderer);
    }         
}




