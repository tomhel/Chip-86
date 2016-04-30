#
# Makefile for Chip-86
#
# Compiler: GNU GCC
#

CPP = g++
CPPFLAGS = -m32 -ansi -Wall
GL_CFLAGS = -lGL
SDL_CFLAGS = $(shell sdl-config --cflags)
SDL_LDFLAGS = $(shell sdl-config --libs)
OPTIMIZE = -O3 -fomit-frame-pointer -w
OUT = chip86


all: clean $(OUT)

$(OUT): main.o Translator.o TranslationCache.o CodeGenerator.o RegTracker.o
	$(CPP) $(OPTIMIZE) $(CPPFLAGS) $(SDL_CFLAGS) $(SDL_LDFLAGS) $(GL_CFLAGS) main.o Translator.o TranslationCache.o CodeGenerator.o RegTracker.o -o $(OUT)

main.o: main.cpp Translator.o TranslationCache.o Chip8def.h
	$(CPP) $(OPTIMIZE) $(CPPFLAGS) -c main.cpp

Translator.o: Translator.cpp Translator.h CodeGenerator.o RegTracker.o CodeBlock.h x86def.h Chip8def.h
	$(CPP) $(OPTIMIZE) $(CPPFLAGS) -c Translator.cpp
	
TranslationCache.o: TranslationCache.cpp TranslationCache.h CodeBlock.h
	$(CPP) $(OPTIMIZE) $(CPPFLAGS) -c TranslationCache.cpp
	
RegTracker.o: RegTracker.cpp RegTracker.h CodeGenerator.o Chip8def.h x86def.h
	$(CPP) $(OPTIMIZE) $(CPPFLAGS) -c RegTracker.cpp

CodeGenerator.o: CodeGenerator.cpp CodeGenerator.h x86def.h
	$(CPP) $(OPTIMIZE) $(CPPFLAGS) -c CodeGenerator.cpp

clean:
	@$(RM) main.o Translator.o TranslationCache.o CodeGenerator.o RegTracker.o $(OUT)

