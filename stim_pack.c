
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <SDL/SDL.h>

#include "video_player.h"
#include "calibration.h"

#define VIDEO_STOPPED_EVENT (SDL_USEREVENT)


/* global rendering variables */
sdl_frame_info sfi;
video_player_info vpi;
SDL_Rect rect43;
SDL_Rect rect169;
SDL_Surface * surf43;
SDL_Surface * surf169;



typedef struct {
    const char * name;
    int volume;
    int timeout_ms;
    double ratio;      
} video_entry;


video_entry video_db[] = 
{
    { "data/videos/TP01.avi", 30, 20 * 1000, 16.0 / 9.0 },
    { "data/videos/TP02.avi", 35, 20 * 1000, 4.0 / 3.0 }
};



uint32_t stop_video_player(uint32_t timeout_ms, void * par)
{
    video_player_info * vpi = par;
    vp_stop(vpi);
    
    SDL_Event event;
    event.type = VIDEO_STOPPED_EVENT;
    SDL_PushEvent(&event);
    
    // do not re-schedule event
    return 0;
}


int play_video_from_database(video_entry * vid)
{
    // set the 16:9 surface for the first video
    if(vid->ratio == 16.0/9.0)
    {
        sfi.vid_surf = surf169;
        sfi.vid_rect = rect169;
    }
    else if(vid->ratio == 4.0/3.0)
    {
        sfi.vid_surf = surf43;
        sfi.vid_rect = rect169;
    }
    else
    {
        // we have a problem
        printf("Failed to find rendering rectangle for proportion %g.\n", vid->ratio);
        assert(0);
    }

    vp_prepare_media(&vpi, vid->name);
    vp_play_with_timeout(&vpi, vid->timeout_ms, vid->volume);
    
    // make sure the stop 
    SDL_AddTimer(vid->timeout_ms, stop_video_player, &vpi);
    
    int done = 0;
    int quit = 0;
    SDL_Event event;
    while(!done)
    { 
        // Keys: enter (fullscreen), space (pause), escape (quit)
        while( SDL_PollEvent( &event ) ) 
        { 
            switch(event.type)
            {
                case SDL_QUIT:
                    done = 1;
                    quit = 1;
                    break;
                
                case VIDEO_STOPPED_EVENT:
                    done = 1;
                    break;
                    
            }
        }
    }

    vp_stop(&vpi);
    vp_release_media(&vpi);
    
    return quit;
}



int main(int argc, char ** argv)
{
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTTHREAD | SDL_INIT_TIMER) == -1)
    {
        printf("cannot initialize SDL\n");
        return -1;
    }
    
    sfi.mutex = SDL_CreateMutex();

    // screen rectangle
    sfi.scr_rect.x = 0; sfi.scr_rect.y = 0;
    sfi.scr_rect.w = 1024; sfi.scr_rect.h = 768;   
       
    // options from example @ libvlc
    int options = SDL_ANYFORMAT | SDL_HWSURFACE | SDL_DOUBLEBUF;
    sfi.screen = SDL_SetVideoMode(sfi.scr_rect.w, sfi.scr_rect.h, 0, options);

    // initialize vpi structure and start video system
    vpi.sfi = &sfi;
    vp_initialize(&vpi);
    
    // Phase 1 screen calibration for screen rectangle 4:3
    rect43.x = 100; rect43.y = 100; rect43.w = 640; rect43.h = 480;
    if(calibrate_rendering_target_rect(sfi.screen, &rect43, 4.0/3.0))
        return 0;
    
    // Phase 2 screen calibration for screen rectangle 16:9
    rect169.x = 100; rect169.y = 100; rect169.w = 160*4; rect169.h = 90*4;
    if(calibrate_rendering_target_rect(sfi.screen, &rect169, 16.0/9.0))
        return 0;
    
    // we directly set the 4:3 surface as the current video surface
    surf43 = SDL_CreateRGBSurface(SDL_SWSURFACE, rect43.w, rect43.h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0);
    surf169 = SDL_CreateRGBSurface(SDL_SWSURFACE, rect169.w, rect169.h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0);
    
    if(play_video_from_database(&video_db[1]))
        return 1;
    
    vp_cleanup(&vpi);
    
    return 0;
}
