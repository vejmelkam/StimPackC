
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#include "video_player.h"
#include "calibration.h"
#include "pulse_listener.h"
#include "event_logger.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define PRODUCTION_CODE


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

// from argv
char * subj_id;
int video_sched[4];   // 4 videos
int cal_video;


/******     RENDERING AND TIMING PARAMETERS    ******/

#ifdef PRODUCTION_CODE

#define MARQUEE_FONT_SIZE 40
#define INITIAL_PULSE_TIMEOUT 0
#define REGULAR_PULSE_TIMEOUT 3000
#define VIDEO_DURATION (480*1000)
#define INTERVIDEO_REST_DURATION (30*1000)
#define REST_MESSAGE_TIMEOUT 5000
#define FINAL_REST_DURATION (600*1000)
#define VIDEO_SCENE_COUNT 4

#else

#define MARQUEE_FONT_SIZE 40
#define INITIAL_PULSE_TIMEOUT 1000
#define REGULAR_PULSE_TIMEOUT 3000
#define VIDEO_DURATION (480 * 1000)
#define INTERVIDEO_REST_DURATION (5*1000)
#define REST_MESSAGE_TIMEOUT 2000
#define FINAL_REST_DURATION (2*1000)
#define VIDEO_SCENE_COUNT 10

#endif // PRODUCTION_CODE

typedef struct {
    const char * filename;
    int volume;
    int add_latency;
    int aspect_num;      
    int aspect_den;
} video_entry;


video_entry video_db[] = 
{
  { "data/videos/TP01_640.avi",  25,  0, 16, 9 },
  { "data/videos/TP02_640.avi",  25, 40,  4, 3 },
  { "data/videos/TP03_640.avi",  30, 50,  4, 3 },
  { "data/videos/TP04_640.avi",  30, 70,  4, 3 },
  { "data/videos/TP05_640.avi", 100, 75, 16, 9 },
  { "data/videos/TP06_640.avi",  30, 65, 16, 9 },
  { "data/videos/TP07_640.avi",  25, 70, 16, 9 },
  { "data/videos/TP08_640.avi",  25, 80, 16, 9 },
  { "data/videos/TP09_640.avi",  40, 35, 16, 9 },
  { "data/videos/TP10_640.avi",  25, 75,  4, 3 }
};


void preload_video(const char * filename)
{
    char buf[1000000];
    FILE * f = fopen(filename, "r");
    int max_reads = 0;
    while(!feof(f) && (++max_reads < 100))
        fread(buf, 1000000, 1, f);
    fclose(f);
}


char * make_log_name()
{
    static char buf[200];
    time_t raw_time;
    struct tm * ti;
    time(&raw_time);
    ti = localtime(&raw_time);
    sprintf(buf, "ev_logs/%s-%04d%02d%02d-%02d%02d.log", subj_id,
            ti->tm_year, ti->tm_mon, ti->tm_mday, ti->tm_hour, ti->tm_min);
    return buf;
}


char * string_timestamp()
{
    static char buf[100];
    time_t raw_time;
    struct tm * ti;
    time(&raw_time);
    ti = localtime(&raw_time);
    strftime(buf, 100, "%H:%M:%S", ti);
    return buf;
}


int load_screen_and_rendering_settings()
{
    FILE * f = fopen("display.cfg", "r");
    if(!f)
    {
        // set defaults for screen size
        sfi.scr_rect.x = 0; sfi.scr_rect.y = 0;
        sfi.scr_rect.w = 1024; sfi.scr_rect.h = 768;   
        
        // set defaults for rect 4:3
        rect43.x = 100; rect43.y = 100; rect43.w = 640; rect43.h = 480;
        
        // set defaults for rect 16:9
        rect169.x = 100; rect169.y = 100; rect169.w = 160*4; rect169.h = 90*4;
        
        return 1;
    }
    
    if(fscanf(f, "%hd %hd %hd %hd", &sfi.scr_rect.x, &sfi.scr_rect.y, &sfi.scr_rect.w, &sfi.scr_rect.h) != 4)
    {
        printf("[stimpack] failed to load screen size from config file.");
        return 0;
    }
    
    if(fscanf(f, "%hd %hd %hd %hd", &rect43.x, &rect43.y, &rect43.w, &rect43.h) != 4)
    {
        printf("[stimpack] failed to load 4:3 initial rendering window from config file.");
        return 0;
    }

    if(fscanf(f, "%hd %hd %hd %hd", &rect169.x, &rect169.y, &rect169.w, &rect169.h) != 4)
    {
        printf("[stimpack] failed to load 16:9 initial rendering window from config file.");
        return 0;
    }
    
    fclose(f);
    
    return 1;
}


void save_screen_and_rendering_settings()
{
    FILE * f = fopen("display.cfg", "w");
    fprintf(f, "%hd %hd %hd %hd\n", sfi.scr_rect.x, sfi.scr_rect.y, sfi.scr_rect.w, sfi.scr_rect.h);
    fprintf(f, "%hd %hd %hd %hd\n", rect43.x, rect43.y, rect43.w, rect43.h);
    fprintf(f, "%hd %hd %hd %hd\n", rect169.x, rect169.y, rect169.w, rect169.h);
    fclose(f);
}


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
  
  printf("[marquee] [%s] displaying message [%s] now.\n", string_timestamp(), text);
  event_logger_log_with_timestamp(LOGEVENT_MARQUEE_START, 0);

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
	      printf("[marquee] [%s] forced stop from keyboard.\n", string_timestamp());
	  }
	  break;

	case SCENE_TIMEOUT_EVENT:
	  if(event.user.code == current_scene_id)
	  {
	      done = 1;
	      printf("[marquee] [%s] reached timeout.\n", string_timestamp());
	  }
	  break;

	case SDL_QUIT:
	  done = 1;
	  break;
	}
  }
  
  event_logger_log_with_timestamp(LOGEVENT_MARQUEE_DONE, 0);
  // we do not clear screen after marquee
}


int play_video(const char * fname, int volume, int timeout_ms, int anum, int aden, int log_frames)
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
        // we have a serious problem
        printf("[stimpack] failed to find rendering rectangle for proportion %d:%d.\n", anum, aden);
        assert(0);
    }
    
    // clear surface
    clear_screen();
    
    vp_prepare_media(&vpi, fname);
    vp_play_with_timeout(&vpi, timeout_ms, volume, log_frames);
    
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
                    event_logger_log_with_timestamp(LOGEVENT_VIDEO_FORCED_STOP, 0);
                    // we can remove the time here, because this is the main thread
                    SDL_RemoveTimer(timer_id);
                    done = 1;
		    if(log_frames)
		      printf("[video] [%s] forced video stop from keyboard.\n", string_timestamp());
                    
                }
                break;

            case SCENE_TIMEOUT_EVENT:
                // we could be catching a timeout belonging to a previous
                // video that was forcedly stopped from kbd, make sure this
                // is for us
                if(event.user.code == current_scene_id)
                {
                    event_logger_log_with_timestamp(LOGEVENT_VIDEO_TIMEOUT, 0);
                    done = 1;
		    if(log_frames)
		      printf("[video] [%s] received timeout event.\n", string_timestamp());
                }
                break;
        }
    }  // wait for events until done
    
    // stop the video player
    vp_stop(&vpi);
    
    return quit;
}



int parse_argument_line(int argc, char ** argv)
{
    // parse parameters from command line
    if(argc != 7)
    {
        printf("Usage: stim_pack subj_id cal_video v1 v2 v3 v4\n");
        return 0;
    }

    // read & check args
    subj_id = argv[1];
    cal_video = atoi(argv[2]);
    video_sched[0] = atoi(argv[3]);
    video_sched[1] = atoi(argv[4]);
    video_sched[2] = atoi(argv[5]);
    video_sched[3] = atoi(argv[6]);
    
    if(cal_video < 1 || cal_video > 10)
    {
        printf("Invalid calibration video selected %d, must be 1..10\n",
	       cal_video);
        return -1;
    }
    else
    {
      //        printf("[stimpack] [%s] pre-loading calibration video [id %d]\n",
      //       string_timestamp(), cal_video);
	//	preload_video(video_db[cal_video-1].filename);
    }
    
    int ok = 1;
    for(int i = 0; i < 4; i++)
    {
        if(video_sched[i] < 1 || video_sched[i] > 10)
        {
            printf("Invalid video %d selected (value is %d), must be 1..10\n",
		   i+1, video_sched[i]);
            ok = 0;
        }
	else
	{
	  //	    printf("[stimpack] [%s] pre-loading video %d [id %d]\n",
	  //	   string_timestamp(), i + 1, video_sched[i]);
	    //	    preload_video(video_db[i].filename);
	}
    }

    return ok;
}

int main(int argc, char ** argv)
{
    // set mode for stdio output
    setvbuf(stdout, 0, _IOLBF, 1024);
    
    // parse arguments
    if(!parse_argument_line(argc, argv))
        return -1;
    
    printf("[stimpack] [%s] stim_pack C version is starting up.\n",
	   string_timestamp());

#ifdef PRODUCTION_CODE
    printf("[stimpack] ***** this version is production code *****\n");
#else
    printf("[stimpack] ***** THIS VERSION IS TEST CODE, NOT FOR USE *****\n");
#endif

    printf("Subject id: %s cal_video: %d schedule: %d %d %d %d\n",
	   subj_id, cal_video, video_sched[0],
	   video_sched[1], video_sched[2], video_sched[3]);

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTTHREAD | SDL_INIT_TIMER) == -1)
    {
        printf("[stimpack] cannot initialize SDL\n");
        return -1;
    }

    if(TTF_Init() == -1)
    {
      printf("[stimpack] failed to initialize TTF font subsystem!\n");
      return -1;
    }
    
    printf("[stimpack] SDL initialized, TTF initialized\n");
    
    // initialize logging system
    event_logger_initialize();
    event_logger_log_with_timestamp(LOGEVENT_SYSTEM_STARTED, 0);
    
    // load font
    marquee_font = TTF_OpenFont("fonts/LiberationSans-Bold.ttf",
				MARQUEE_FONT_SIZE);
    if(!marquee_font)
    {
        printf("[stimpack] [%s] cannot open required font!\n",
	       string_timestamp());
	return -1;
    }
    else
        printf("[stimpack] [%s] loaded liberation-sans font.\n", string_timestamp());

    // load from file or set defaults if no file found
    load_screen_and_rendering_settings();

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
    if(calibrate_rendering_target_rect(sfi.screen, &rect43, 4.0/3.0))
        return 0;
    
    // Phase 2 screen calibration for screen rectangle 16:9
    if(calibrate_rendering_target_rect(sfi.screen, &rect169, 16.0/9.0))
        return 0;
    
    // we construct 4:3 and 16:9 surft occurred in video playback, reqaces
    surf43 = SDL_CreateRGBSurface(SDL_SWSURFACE, rect43.w, rect43.h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0);
    surf169 = SDL_CreateRGBSurface(SDL_SWSURFACE, rect169.w, rect169.h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0);
    
    // save whatever we have calibrated for next session
    printf("[stimpack] [%s] saving calibration results.\n", string_timestamp());
    save_screen_and_rendering_settings();

    // Phase 3: sound level calibration
    printf("[stimpack] [%s] playing calibration video %d.\n", string_timestamp(), cal_video);
    fprintf(stderr, "[stimpack] [%s] playing calibration video %d, press Enter to terminate.\n", string_timestamp(), cal_video);

    video_entry * ve = &video_db[cal_video - 1];
    if(play_video(ve->filename, ve->volume, VIDEO_DURATION,
		  ve->aspect_num, ve->aspect_den, 0))
      return 1;
   
    // Phase 4: we are ready to experiment
    printf("[stimpack] [%s] ready for experiment, press Enter to begin.\n", string_timestamp());
    render_marquee_text("Ready", &rect43, 0);

    // we start logging pulses now
    pulse_listener_log_pulses(&pl, 1);
    event_logger_log_with_timestamp(LOGEVENT_EXPERIMENT_STARTED, 0);
    
    // run through the cycle many times
    for(int i = 0; i < VIDEO_SCENE_COUNT; i++)
    {
        // select new video (video_sched contains 1-based indices)
#ifdef PRODUCTION_CODE
        assert(0 <= i && i < 10);
	int video_ndx = video_sched[i] - 1;
	ve = &video_db[video_ndx];
#else
	int video_ndx = i % 10;
        ve = &video_db[i % 10];
#endif
       	if(play_video(ve->filename, ve->volume, 10, ve->aspect_num, 
		      ve->aspect_den, 0))
            return 1;

	// wait for pulse
        uint32_t pulse_timeout = i == 0? INITIAL_PULSE_TIMEOUT : REGULAR_PULSE_TIMEOUT;
	printf("[wait-for-pulse] [%s] listening on parallel port [timeout %u ms].\n",
	       string_timestamp(), pulse_timeout);

	// wait for a new pulse
	switch(listen_for_pulse(&pl, pulse_timeout))
	{
	case PL_REQ_TIMED_OUT:
	  printf("[wait-for-pulse] [%s] timed out waiting for pulse.\n",
		 string_timestamp());
	  break;

	case PL_REQ_PULSE_ACQUIRED:
	  printf("[wait-for-pulse] [%s] pulse received.\n", string_timestamp());
	  break;
	}

        // we have the info, clear the request flag
	pulse_listener_clear_request(&pl);

	// delay to equalize videos
	printf("[stimpack] waiting %d ms before playing.\n", ve->add_latency);
	SDL_Delay(ve->add_latency);

	// play a video
        fprintf(stderr, "[stimpack] [%s] playing video number %d with id %d\n",
		string_timestamp(), i+1, video_ndx+1);
        printf("[video] [%s] playing video number %d, with id %d, timeout %d, volume %d.\n",
	       string_timestamp(), i+1, video_ndx+1, VIDEO_DURATION, ve->volume);

       	if(play_video(ve->filename, ve->volume, VIDEO_DURATION, 
		      ve->aspect_num, ve->aspect_den, 1))
            return 1;

        // 30 seconds relaxed cross fixation if not after last video
        // after last video, we go directly to rest message
	if(i < VIDEO_SCENE_COUNT - 1)
	  render_marquee_text("+", &rect43, INTERVIDEO_REST_DURATION);
    }

    // show message that rest is coming up
    printf("[stimpack] [%s] videos are done, showing rest banner.\n", string_timestamp());
    render_marquee_text("10MIN KLID", &rect43, REST_MESSAGE_TIMEOUT);

    // render the cross for the resting state
    printf("[stimpack] [%s] entering REST stage.\n", string_timestamp());
    render_marquee_text("+", &rect43, FINAL_REST_DURATION);
    
    // stop listening for pulses
    pulse_listener_log_pulses(&pl, 0);

    // we're done
    event_logger_log_with_timestamp(LOGEVENT_EXPERIMENT_ENDED, 0);
    printf("[stimpack] [%s] experiment completed, cleaning up.\n", string_timestamp());
    
    // save the event log
    char * log_name = make_log_name();
    printf("[stimpack] [%s] saving log to file [%s].\n", string_timestamp(), log_name);
    event_logger_save_log(log_name);
    
    // cleanup the system
    vp_cleanup(&vpi);
    pulse_listener_shutdown(&pl);

    // shutdown font lib
    TTF_CloseFont(marquee_font); marquee_font = 0;
    TTF_Quit();
    
    printf("[stimpack] [%s] bye bye.\n", string_timestamp());
    
    return 0;
}
