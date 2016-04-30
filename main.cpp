/************************************************************
  **** main.cpp (CHIP-8 emulator with dynarec)
   ***
    ** Author:
     *   Tommy Hellstrom
     *
     * Description:
     *   Simple CHIP-8 emulator using dynamic translation to x86
     *
     * Revision history:
     *   When         Who       What
     *   20090531     me        created/modified original
     *
     * License information:
     *   GPLv3
     *
     ********************************************************/

#include <cstdio>
#include <cstring>
#include <ctime>
#include <stdint.h>

#ifndef _WINDOWS
# include <SDL/SDL.h>
# include <SDL/SDL_opengl.h>
#else
# include "SDL_win/include/SDL.h"
# include "SDL_win/include/SDL_opengl.h"
#endif

#include "Chip8def.h"
#include "Translator.h"
#include "TranslationCache.h"

#define WINDOW_WIDTH  512
#define WINDOW_HEIGHT 256

#define SCALE_WIDTH  8
#define SCALE_HEIGHT 8

#define COL_PIX_ON_R  50 / 255.0
#define COL_PIX_ON_G 205 / 255.0
#define COL_PIX_ON_B  50 / 255.0

#define COL_PIX_OFF_R 0
#define COL_PIX_OFF_G 0
#define COL_PIX_OFF_B 0

#define ARG_OPCOUNT 10

#define APP_NAME         "Chip-86"
#define APP_VERSION      "1.1"
#define APP_BINARY_NAME  "chip86"
#define APP_WINDOW_TITLE "Chip-86"

//Font sprites that is copied to chip8 memory
const uint8_t C8_FONT[] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

//Chip8 variabels
static uint32_t gC8_pc;
static uint32_t gC8_seedRng;
static uint32_t gC8_newFrame;
static uint32_t gC8_addressReg;
static uint32_t gC8_stack[C8_STACK_DEPTH];
static uint32_t *gC8_stackPointer;
static uint8_t  gC8_regs[C8_GPREG_COUNT];
static uint8_t  gC8_memory[C8_MEMSIZE];
static uint8_t  gC8_screen[C8_RES_HEIGHT][C8_RES_WIDTH];
static uint8_t  gC8_keys[C8_KEY_COUNT];
static uint8_t  gC8_delaytimer;
static uint8_t  gC8_soundtimer;

/**
 * Fetch the instruction, pointed to by the PC
 *
 * RETURNS
 * next instruction
 */
inline uint32_t c8_getOpcode()
{
    return (gC8_memory[gC8_pc] << 8) | gC8_memory[gC8_pc + 1];
}

/**
 * Reset the Chip8 system
 */
void c8_reset()
{
    gC8_pc = C8_PC_START;
    gC8_addressReg = 0;
    gC8_delaytimer = 0;
    gC8_soundtimer = 0;
    gC8_newFrame = 0;
    gC8_stackPointer = gC8_stack;
    gC8_seedRng = time(NULL);
    memset(gC8_regs, 0, sizeof(gC8_regs));
    //memset(gC8_memory, 0, sizeof(gC8_memory));
    memset(gC8_keys, 0, sizeof(gC8_keys));
    memset(gC8_screen, 0, sizeof(gC8_screen));
    memcpy(gC8_memory, C8_FONT, sizeof(C8_FONT));
}

/**
 * Decrease chip8 timers
 */
void c8_decreaseTimers()
{
    if (gC8_delaytimer > 0)
        gC8_delaytimer--;

    if (gC8_soundtimer > 0)
        gC8_soundtimer--;
}

/**
 * Play beep (sound)
 */
void c8_beep()
{
    //not implemented
}

/**
 * Load a Chip8 rom
 *
 * PARAMS
 * pFile filepath
 *
 * RETURNS
 * true if successful, otherwise false
 */
bool c8_loadRom(const char *const pFile)
{
    c8_reset();

    FILE *const pIn = fopen(pFile, "rb");

    if(pIn == NULL)
        return false;

    fseek(pIn, 0, SEEK_END);
    const unsigned int fsize = ftell(pIn);
    rewind(pIn);

    if(fsize > C8_MEMSIZE - C8_PC_START)
    {
        fclose(pIn);
        return false;
    }

    if(fread(&gC8_memory[C8_PC_START], 1, fsize, pIn) != fsize)
    {
        fclose(pIn);
        return false;
    }

    fclose(pIn);

    return true;
}

/**
 * Handle input
 *
 * PARAMS
 * rDelay   ref. to delay variable
 * rOpcount ref. to opcount variable
 * rEvent   SDL event
 */
void handleInput(int &rDelay, int &rOpCount, const SDL_Event &rEvent)
{
	switch(rEvent.type)
	{
	    case SDL_KEYDOWN:
            switch(rEvent.key.keysym.sym)
            {
                case SDLK_x: gC8_keys[0] = 1; break;
                case SDLK_1: gC8_keys[1] = 1; break;
                case SDLK_2: gC8_keys[2] = 1; break;
                case SDLK_3: gC8_keys[3] = 1; break;
                case SDLK_q: gC8_keys[4] = 1; break;
                case SDLK_w: gC8_keys[5] = 1; break;
                case SDLK_e: gC8_keys[6] = 1; break;
                case SDLK_a: gC8_keys[7] = 1; break;
                case SDLK_s: gC8_keys[8] = 1; break;
                case SDLK_d: gC8_keys[9] = 1; break;
                case SDLK_z: gC8_keys[10] = 1; break;
                case SDLK_c: gC8_keys[11] = 1; break;
                case SDLK_4: gC8_keys[12] = 1; break;
                case SDLK_r: gC8_keys[13] = 1; break;
                case SDLK_f: gC8_keys[14] = 1; break;
                case SDLK_v: gC8_keys[15] = 1; break;
                case SDLK_PAGEDOWN: rDelay++; break;
                case SDLK_PAGEUP: if(rDelay > 0) rDelay--; break;
                case SDLK_HOME: rOpCount++; break;
                case SDLK_END: if(rOpCount > 1) rOpCount--; break;
                default:;
            }
            break;

        case SDL_KEYUP:
            switch(rEvent.key.keysym.sym)
            {
                case SDLK_x: gC8_keys[0] = 0; break;
                case SDLK_1: gC8_keys[1] = 0; break;
                case SDLK_2: gC8_keys[2] = 0; break;
                case SDLK_3: gC8_keys[3] = 0; break;
                case SDLK_q: gC8_keys[4] = 0; break;
                case SDLK_w: gC8_keys[5] = 0; break;
                case SDLK_e: gC8_keys[6] = 0; break;
                case SDLK_a: gC8_keys[7] = 0; break;
                case SDLK_s: gC8_keys[8] = 0; break;
                case SDLK_d: gC8_keys[9] = 0; break;
                case SDLK_z: gC8_keys[10] = 0; break;
                case SDLK_c: gC8_keys[11] = 0; break;
                case SDLK_4: gC8_keys[12] = 0; break;
                case SDLK_r: gC8_keys[13] = 0; break;
                case SDLK_f: gC8_keys[14] = 0; break;
                case SDLK_v: gC8_keys[15] = 0; break;
                default:;
            }
    }
}

/**
 * Renders a new frame
 */
void renderFrame()
{
    glClearColor(COL_PIX_OFF_R, COL_PIX_OFF_G, COL_PIX_OFF_B, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    for(int y = 0; y < C8_RES_HEIGHT; y++)
        for(int x = 0; x < C8_RES_WIDTH; x++)
            if(gC8_screen[y][x] == C8_PIXEL_ON)
            {
                const int xx = x * SCALE_WIDTH;
                const int yy = y * SCALE_HEIGHT;

                glBegin(GL_QUADS);
                    glColor3f(COL_PIX_ON_R, COL_PIX_ON_G, COL_PIX_ON_B);
                    glVertex2f(xx, yy);
                    glVertex2f(xx + SCALE_WIDTH, yy);
                    glVertex2f(xx + SCALE_WIDTH, yy + SCALE_HEIGHT);
                    glVertex2f(xx, yy + SCALE_HEIGHT);
                glEnd();
            }

    glFlush();
}

/**
 * Main emulationloop
 *
 * PARAMS
 * delay    delayvalue
 * opcount  number of opcodes to execute between delays
 */
void dispatchLoop(int delay, int opcount)
{
    CodeBlock *ptr;
    SDL_Event event;
    TranslationCache cache;
    Translator dynarec(gC8_regs, &gC8_seedRng, &gC8_addressReg,
                       &gC8_delaytimer, &gC8_soundtimer, &gC8_newFrame,
                       gC8_keys, gC8_memory, gC8_screen, &gC8_stackPointer);

    for(;;)
    {
        const unsigned int future = SDL_GetTicks() + delay;

        if(gC8_newFrame == NEW_FRAME)
           renderFrame();

        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
                return;

            if(event.active.state & SDL_APPINPUTFOCUS)
                SDL_GL_SwapBuffers();

            handleInput(delay, opcount, event);
        }

        if(gC8_newFrame == NEW_FRAME)
        {
            SDL_GL_SwapBuffers();
            gC8_newFrame = NO_NEW_FRAME;
        }

        if(cache.executeN(gC8_pc, opcount))
        {
            c8_decreaseTimers();
            //c8_beep();

            while(SDL_GetTicks() < future)
                SDL_Delay(0);
        }
        else
        {
            while(dynarec.emit(c8_getOpcode(), gC8_pc)) ;

            while(dynarec.getCodeBlock(&ptr))
                cache.insert(ptr);
        }
    }
}

/**
 * Init OpenGL
 */
void initGL()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, 1);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    //glDisable(GL_CULL_FACE);
    //glDisable(GL_DITHER);
    //glDisable(GL_BLEND);
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef (0.375, 0.375, 0);
    glClearColor(COL_PIX_OFF_R, COL_PIX_OFF_G, COL_PIX_OFF_B, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glShadeModel(GL_FLAT);
}

/**
 * Create SDL window
 */
bool createSDLWindow()
{
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
	{
	    fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	if(SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 16, SDL_OPENGL) == NULL)
	{
	    fprintf(stderr, "Video initialization failed: %s\n", SDL_GetError());
		return false;
	}

    initGL();
    SDL_WM_SetCaption(APP_WINDOW_TITLE, NULL);

	return true;
}

/**
 * Print helptext
 */
void printHelp()
{
    printf("%s v%s\n\n", APP_NAME, APP_VERSION);
    printf("%s is a Chip-8 emulator with dynamic translation to x86.\n", APP_NAME);
    printf("Written in C++ by Tommy Hellstrom at the University of Gavle,\n");
    printf("Sweden, 2009.\n\n");
    printf("USAGE:\n");
    printf("\t%s file speed [tune]\n\n", APP_BINARY_NAME);
    printf("WHERE:\n");
    printf("\tfile\n");
    printf("\t  is the rom to load.\n\n");
    printf("\tspeed\n");
    printf("\t  is a non-negative integer that controls\n");
    printf("\t  the emulation speed. Zero (0) equals maximum\n");
    printf("\t  speed. For most roms 5 to 20 are good values.\n");
    printf("\ttune\n");
    printf("\t  is a positive integer that can be used to\n");
    printf("\t  finetune the emulation. This argument controls\n");
    printf("\t  emulation speed and smoothness.\n");
    printf("\t  The argument is optional, default value is %u.\n", ARG_OPCOUNT);
    printf("\t  For most roms 5 to 20 are good values.\n");
}

/**
 * Main...
 */
int main(int argc, char *argv[])
{
    if(argc != 3 && argc != 4)
    {
        printHelp();
        return 0;
    }

    const int delay = atoi(argv[2]);

    int opcount;

    if(argc == 4)
        opcount = atoi(argv[3]);
    else
        opcount = ARG_OPCOUNT;

    if (!c8_loadRom(argv[1]))
    {
        printf("Could not open file!\n");
        return 0;
    }

    if(!createSDLWindow())
        return 0;

    dispatchLoop(delay, opcount);

    SDL_Quit();
    return 0;
}
