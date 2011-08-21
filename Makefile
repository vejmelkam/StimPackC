

CFLAGS+=-std=c99 -Wall -D_XOPEN_SOURCE=500
CFLAGS+=-g
SOURCES=stim_pack.c video_player.c calibration.c pulse_listener.c event_logger.c
DRAW_DIR=$(HOME)/Lib/SDL_Draw

all: stim_pack pulse_test


stim_pack: $(SOURCES)
	gcc -o stim_pack $(CFLAGS) $(SOURCES) -lvlc -lSDL -lSDL_ttf $(DRAW_DIR)/lib/libSDL_draw.a


pulse_test: $(SOURCES) pulse_test.c
	gcc -o pulse_test $(CFLAGS) pulse_test.c pulse_listener.c event_logger.c -lSDL


clean:
	rm stim_pack
	rm pulse_test

