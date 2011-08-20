
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <SDL/SDL.h>

#include "video_player.h"
#include "calibration.h"
#include "pulse_listener.h"


/* global rendering variables */
sdl_frame_info sfi;
video_player_info vpi;
pulse_listener pl;
SDL_Rect rect43;
SDL_Rect rect169;
SDL_Surface * surf43;
SDL_Surface * surf169;

uint64_t next_video_id = 0;


typedef struct {
    const char * name;
    int volume;
    int timeout_ms;
    double ratio;      
} video_entry;


video_entry video_db[] = 
{
    { "data/videos/TP01.avi", 30, 50 * 1000, 16.0 / 9.0 },
    { "data/videos/TP02.avi", 35, 50 * 1000, 4.0 / 3.0 },
    { "data/videos/TP03.avi", 30, 50 * 1000, 16.0 / 9.0 },
};


uint32_t stop_video_player(uint32_t timeout_ms, void * par)
{
    uint64_t video_id = (uint64_t)par;
    
    SDL_Event event;
    event.type = VIDEO_PLAYBACK_TIMEOUT_EVENT;
    event.user.code = video_id;
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
        sfi.vid_rect = rect43;
    }
    else
    {
        // we have a problem
        printf("Failed to find rendering rectangle for proportion %g.\n", vid->ratio);
        assert(0);
    }
    
    // clear surface
    SDL_FillRect(sfi.screen, 0, 0x00000000);
    SDL_Flip(sfi.screen);

    vp_prepare_media(&vpi, vid->name);
    vp_play_with_timeout(&vpi, vid->timeout_ms, vid->volume);
    
    // make sure the stop 
    uint64_t current_video_id = ++next_video_id;
    SDL_TimerID timer_id = SDL_AddTimer(vid->timeout_ms, stop_video_player, (void*)current_video_id);
    
    int done = 0;
    int quit = 0;
    SDL_Event event;
    while(!done)
    { 
        // Keys: enter (fullscreen), space (pause), escape (quit)
        SDL_WaitEvent(&event);

        switch(event.type)
        {
            case SDL_QUIT:
                done = 1;
                quit = 1;
                break;

            case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_RETURN)
                {
                    // we can remove the time here, because this is the main thread
                    SDL_RemoveTimer(timer_id);
                    done = 1;
                    printf("Forced video stop from keyboard.\n");
                }
                else
                    printf("Some key pressed.\n");
                break;

            case VIDEO_PLAYBACK_TIMEOUT_EVENT:
                // we could be catching a timeout belonging to a previous
                // video that was forcedly stopped from kbd, make sure this
                // is for us
                if(event.user.code == current_video_id)
                {
                    done = 1;
                    printf("Received TIMEOUT_EVENT for my video.\n");
                }
                else
                    printf("Received TIMEOUT_EVENT from another video.\n");
                break;
        }
    }  // wait for events until done
    
    printf("Stopping video NOW.\n");

    // stop the video player
    vp_stop(&vpi);

    printf("Clearing screen.\n");

    // clear surface
    SDL_FillRect(sfi.screen, 0, 0x00000000);
    SDL_Flip(sfi.screen);
    
    return quit;
}



int main(int argc, char ** argv)
{
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTTHREAD | SDL_INIT_TIMER) == -1)
    {
        printf("cannot initialize SDL\n");
        return -1;
    }
    
    // initialize screen rectangle
    sfi.scr_rect.x = 0; sfi.scr_rect.y = 0;
    sfi.scr_rect.w = 1024; sfi.scr_rect.h = 768;   
       
    // options taken from example @ libvlc
    int options = SDL_ANYFORMAT | SDL_DOUBLEBUF;
    sfi.screen = SDL_SetVideoMode(sfi.scr_rect.w, sfi.scr_rect.h, 0, options);

    // initialize vpi structure and start video system
    vpi.sfi = &sfi;
    vp_initialize(&vpi);
    
    // initialize pulse listener system
    pulse_listener_initialize(&pl);
    
    // Phase 1 screen calibration for screen rectangle 4:3
    rect43.x = 100; rect43.y = 100; rect43.w = 640; rect43.h = 480;
    if(calibrate_rendering_target_rect(sfi.screen, &rect43, 4.0/3.0))
        return 0;
    
    // Phase 2 screen calibration for screen rectangle 16:9
    rect169.x = 100; rect169.y = 100; rect169.w = 160*4; rect169.h = 90*4;
    if(calibrate_rendering_target_rect(sfi.screen, &rect169, 16.0/9.0))
        return 0;
    
    // we construct 4:3 and 16:9 surft occurred in video playback, reqaces
    surf43 = SDL_CreateRGBSurface(SDL_SWSURFACE, rect43.w, rect43.h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0);
    surf169 = SDL_CreateRGBSurface(SDL_SWSURFACE, rect169.w, rect169.h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0);
    
    // Phase 3: sound level calibration
    play_video_from_database(&video_db[2]);
    
    for(int i = 0; i < 3; i++)
    {
        printf("Waiting for pulse.\n");
        // wait for a pulse (black screen)
        listen_for_pulse(&pl, 1000);
        
        // play a video
        printf("Playing video.\n");
        if(play_video_from_database(&video_db[i]))
            return 1;
        
        // wait while displaying message
        
    }
    
    printf("Cleaning up.\n");
    
    vp_cleanup(&vpi);
    pulse_listener_shutdown(&pl);
    
    return 0;
}
