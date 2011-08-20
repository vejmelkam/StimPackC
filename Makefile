

CFLAGS+=-std=c99
CFLAGS+=-g
SOURCES=stim_pack.c video_player.c calibration.c pulse_listener.c
DRAW_DIR=$(HOME)/Lib/SDL_Draw

all: stim_pack


stim_pack: $(SOURCES)
	gcc -o stim_pack $(CFLAGS) $(SOURCES) -lvlc -lSDL $(DRAW_DIR)/lib/libSDL_draw.a
	
	
clean:
	rm stim_pack
	rm *.o
