
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#include "video_player.h"
#include "calibration.h"
#include "pulse_listener.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* global rendering variables */
sdl_frame_info sfi;
video_player_info vpi;
pulse_listener pl;
SDL_Rect rect43;
SDL_Rect rect169;
SDL_Surface * surf43;
SDL_Surface * surf169;
TTF_Font * marquee_font;

uint64_t next_scene_id = 0;


/******     RENDERING AND TIMING PARAMETERS    ******/

#define MARQUEE_FONT_SIZE 40
#define INITIAL_PULSE_TIMEOUT (30 * 1000)
#define REGULAR_PULSE_TIMEOUT 3000
#define VIDEO_DURATION (480*1000)
#define REST_MESSAGE_TIMEOUT 5000
#define REST_DURATION (600*1000)
#define VIDEO_SCENES 100


typedef struct {
    const char * filename;
    int volume;
    int aspect_num;      
    int aspect_den;
} video_entry;


video_entry video_db[] = 
{
  { "data/videos/TP01.avi",  35, 16, 9 },
  { "data/videos/TP02.avi",  35, 4, 3 },
  { "data/videos/TP03.avi",  45,  4, 3 },
  { "data/videos/TP04.avi",  45,  4, 3 },
  { "data/videos/TP05.avi", 220, 16, 9 },
  { "data/videos/TP06.avi",  45, 16, 9 },
  { "data/videos/TP07.avi",  40, 16, 9 },
  { "data/videos/TP08.avi",  35, 16, 9 },
  { "data/videos/TP09.avi",  55, 16, 9 },
  { "data/videos/TP10.avi",  35,  4, 3 }
};


void clear_screen()
{
    // clear surface
    SDL_FillRect(sfi.screen, 0, 0x00000000);
    SDL_Flip(sfi.screen);
}


// a general function that notifies an event loop that it's time is up
uint32_t scene_timeout_callback(uint32_t timeout_ms, void * par)
{
    uint64_t video_id = (uint64_t)par;
    
    SDL_Event event;
    event.type = SCENE_TIMEOUT_EVENT;
    event.user.code = video_id;
    SDL_PushEvent(&event);
    
    // do not re-schedule the timer
    return 0;
}


void render_marquee_text(const char * text, SDL_Rect * tgt_rect, 
			 uint32_t timeout_ms)
{
  // clear the screen
  SDL_FillRect(sfi.screen, NULL, 0x00000000);

  SDL_Color white = {255, 255, 255};
  SDL_Surface * text_surf = TTF_RenderText_Solid(marquee_font, text, white);
  if(text_surf)
  {
      SDL_Rect rect;
      rect.x = tgt_rect->x + (tgt_rect->w - text_surf->w) / 2;
      rect.y = tgt_rect->y + (tgt_rect->h - text_surf->h) / 2;
      SDL_BlitSurface(text_surf, NULL, sfi.screen, &rect);
      SDL_FreeSurface(text_surf);
  }
  else
    printf("[marquee] text did not render!\n");

  SDL_Flip(sfi.screen);

  uint64_t current_scene_id = ++next_scene_id;
  SDL_TimerID timer_id = 0;
  if(timeout_ms > 0)
    timer_id = SDL_AddTimer(timeout_ms, scene_timeout_callback,
			    (void*)current_scene_id);

  int done = 0;
  SDL_Event event;
  while(!done)
  {
      SDL_WaitEvent(&event);
      
      switch(event.type)
	{
	case SDL_KEYDOWN:
	  if(event.key.keysym.sym == SDLK_RETURN)
	  {
	      // we can remove the time here, because this is the main thread
	      SDL_RemoveTimer(timer_id);
	      done = 1;
	      printf("[marquee] forced video stop from keyboard.\n");
	  }
	  break;

	case SCENE_TIMEOUT_EVENT:
	  if(event.user.code == current_scene_id)
	  {
	      done = 1;
	      printf("[marquee] reached timeout.\n");
	  }
	  break;

	case SDL_QUIT:
	  done = 1;
	  break;
	}
  }

  // clear the screen
  clear_screen();
}


int play_video(const char * fname, int volume, int timeout_ms, int anum, int aden)
{
    // set the 16:9 surface for the first video
    if(anum == 16 && aden == 9)
    {
        sfi.vid_surf = surf169;
        sfi.vid_rect = rect169;
    }
    else if(anum == 4 && aden == 3)
    {
        sfi.vid_surf = surf43;
        sfi.vid_rect = rect43;
    }
    else
    {
        // we have a problem
      printf("Failed to find rendering rectangle for proportion %d:%d.\n", anum, aden);
        assert(0);
    }
    
    // clear surface
    clear_screen();

    vp_prepare_media(&vpi, fname);
    vp_play_with_timeout(&vpi, timeout_ms, volume);
    
    // make sure the stop 
    uint64_t current_scene_id = ++next_scene_id;
    SDL_TimerID timer_id = SDL_AddTimer(timeout_ms, scene_timeout_callback,
					(void*)current_scene_id);
    
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

            case SCENE_TIMEOUT_EVENT:
                // we could be catching a timeout belonging to a previous
                // video that was forcedly stopped from kbd, make sure this
                // is for us
                if(event.user.code == current_scene_id)
                {
                    done = 1;
                    printf("[video] received timeout event.\n");
                }
                break;
        }
    }  // wait for events until done
    
    // stop the video player
    vp_stop(&vpi);

    
    return quit;
}


int main(int argc, char ** argv)
{
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTTHREAD | SDL_INIT_TIMER) == -1)
    {
        printf("cannot initialize SDL\n");
        return -1;
    }

    if(TTF_Init() == -1)
    {
      printf("Failed to initialize font subsystem!\n");
    }

    // load font
    marquee_font = TTF_OpenFont("fonts/LiberationSans-Bold.ttf",
				MARQUEE_FONT_SIZE);
    if(!marquee_font)
      printf("Could not open font!\n");

    // initialize screen rectangle
    sfi.scr_rect.x = 0; sfi.scr_rect.y = 0;
    sfi.scr_rect.w = 1024; sfi.scr_rect.h = 768;   

    // options taken from example @ libvlc
    int options = SDL_ANYFORMAT | SDL_DOUBLEBUF | SDL_NOFRAME;
    sfi.screen = SDL_SetVideoMode(sfi.scr_rect.w, sfi.scr_rect.h, 0, options);

    // hide the cursor
    SDL_ShowCursor(SDL_DISABLE);

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
    video_entry * ve = &video_db[6];
    if(play_video(ve->filename, ve->volume, VIDEO_DURATION,
		  ve->aspect_num, ve->aspect_den))
      return 1;
   
    // Phase 4: we are ready to experiment
    render_marquee_text("Ready", &rect43, 0);

    // wait for first pulse
    printf("[wait-for-pulse] listening on parallel port [timeout %d ms].\n", INITIAL_PULSE_TIMEOUT);
    listen_for_pulse(&pl, INITIAL_PULSE_TIMEOUT);

    // run through the cycle many times
    for(int i = 0; i < VIDEO_SCENES; i++)
    {
        // select new video
	ve = &video_db[i % 10];

	// wait for pulse
	printf("[wait-for-pulse] listening on parallel port [timeout %d ms].\n", REGULAR_PULSE_TIMEOUT);
	switch(listen_for_pulse(&pl, REGULAR_PULSE_TIMEOUT))
	{
	case PL_REQ_TIMED_OUT:
	  printf("[wait-for-pulse] timed out waiting for pulse.\n");
	  break;

	case PL_REQ_PULSE_ACQUIRED:
	  printf("[wait-for-pulse] pulse received.\n");
	  break;
	}
        
	// play a video
       	if(play_video(ve->filename, ve->volume, VIDEO_DURATION, 
		      ve->aspect_num, ve->aspect_den))
            return 1;

        // 30 seconds relaxed cross fixation if not after last video
	if(i < VIDEO_SCENES - 1)
	  render_marquee_text("+", &rect43, INITIAL_PULSE_TIMEOUT);
    }

    // show message that rest is coming up
    printf("[experiment] videos are done, showing rest banner.\n");
    render_marquee_text("10MIN KLID", &rect43, REST_MESSAGE_TIMEOUT);

    // render the cross for the resting state
    printf("[experiment] entering rest stage.\n");
    render_marquee_text("+", &rect43, REST_DURATION);
    
    // we're done
    printf("Experiment completed, cleaning up.\n");
    
    // cleanup the system
    vp_cleanup(&vpi);
    pulse_listener_shutdown(&pl);

    // shutdown font lib
    TTF_CloseFont(marquee_font); marquee_font = 0;
    TTF_Quit();

    return 0;
}
