

#include <assert.h>
#include <memory.h>
#include <malloc.h>
#include <stdlib.h>
#include <math.h>

#include "video_player.h"
#include "event_logger.h"

static void  display_callback(void *data, void *id);
static void  unlock_callback(void *data, void *id, void *const *p_pixels);
static void* lock_callback(void *data, void **p_pixels);

void vp_initialize(video_player_info * vpi)
{       
    char const *argv[] = {
        "--alsa-caching=20000", /* skip any audio track */
        "--no-xlib",
        "--verbose=1"
    };    
    int argc = sizeof(argv) / sizeof(const char*);
    printf("Have %d arguments to libvlc_new().\n", argc);
    vpi->inst = libvlc_new(argc, argv);
    vpi->player = libvlc_media_player_new(vpi->inst);
}



void vp_cleanup(video_player_info * vpi)
{
//    assert(vpi->player);
    assert(vpi->inst);
    
//    libvlc_media_player_release(vpi->player);
//    vpi->player = 0;
    libvlc_release(vpi->inst);
    vpi->inst = 0;
}


void vp_prepare_media(video_player_info * vpi, const char * filename)
{
//    assert(vpi->player);
    
    // make ourselves a new player
    vpi->player = libvlc_media_player_new(vpi->inst);
    
    libvlc_media_t * media = libvlc_media_new_path(vpi->inst, filename);
    libvlc_media_player_set_media(vpi->player, media);
    libvlc_media_add_option(media, "no-video-title-show");
    
    // setup the rendering mechanism
    sdl_frame_info * sfi = vpi->sfi;
    libvlc_video_set_format(vpi->player, "RV32", sfi->vid_rect.w, sfi->vid_rect.h, sfi->vid_rect.w * 4);
    libvlc_video_set_callbacks(vpi->player, lock_callback, unlock_callback, display_callback, vpi);
    
    // we can release media right now as per example
    libvlc_media_release(media);
}


void vp_play_with_timeout(video_player_info * vpi, uint32_t timeout_ms, int volume, int log_frames)
{
    assert(vpi->player);
    
    // setup required state
    vpi->block_screen_updates = 0;
    vpi->start_time = SDL_GetTicks();
    vpi->end_time = vpi->start_time + timeout_ms;
    vpi->nominal_volume = volume;
    vpi->current_frame_ndx = 0;
    vpi->log_frames = log_frames;
    
    // send commands to media player
    libvlc_media_player_play(vpi->player);
    libvlc_audio_set_volume(vpi->player, volume);
}


void vp_stop(video_player_info* vpi)
{
    assert(vpi->player);
    //    printf("Stopping player %ux vp_stop.\n", (void*)vpi->player);
    libvlc_media_player_release(vpi->player);
    vpi->block_screen_updates = 1;
    vpi->player = 0;
    //    printf("Player stopped in vp_stop.\n");
}


static void *lock_callback(void *data, void **p_pixels)
{
    video_player_info * vpi = data;
    sdl_frame_info * sfi = vpi->sfi;

    SDL_LockSurface(sfi->vid_surf);
    *p_pixels = sfi->vid_surf->pixels;
    return NULL; /* picture identifier, not needed here */
}


static void unlock_callback(void *data, void *id, void *const *p_pixels)
{
    video_player_info * vpi = data;
    sdl_frame_info * sfi = vpi->sfi;

    /* Insert fadeout effect here */
    uint32_t *pixels = *p_pixels;
    int x, y, i;
    SDL_Rect rect = sfi->vid_rect;
    uint32_t now = SDL_GetTicks();
    
    if(vpi->end_time - now < 2000)
    {
        // create 256 byte lookup table for fast fadeout
        double factor = (vpi->end_time - now) / 2000.0;
        uint8_t lookup[256];
        for(i = 0; i < 256; i++)
            lookup[i] = (uint8_t)lrint(round(factor * i));
        
        uint32_t val;
        uint8_t r, g, b;
        for(y = 0; y < rect.h; y++)
        {
            for(x = 0; x < rect.w; x++)
            {
                val = pixels[y * rect.w + x];
                r = (val & 0x00ff0000) >> 16;
                g = (val & 0x0000ff00) >> 8;
                b = (val & 0x000000ff);
                pixels[y * rect.w + x] = (lookup[r] << 16) + (lookup[g] << 8) + lookup[g];
            }
        }
        
        // and reduce the volume
        int new_vol = lrint(vpi->nominal_volume * factor * factor);
        libvlc_audio_set_volume(vpi->player, new_vol);
    }
    
    SDL_UnlockSurface(sfi->vid_surf);

    assert(id == NULL); /* picture identifier, not needed here */

    // do the blit now
    if(!vpi->block_screen_updates)
        SDL_BlitSurface(sfi->vid_surf, 0, sfi->screen, &sfi->vid_rect);
}

static void display_callback(void *data, void *id)
{
    video_player_info * vpi = data;
    sdl_frame_info * sfi = vpi->sfi;
    
    /* VLC wants to display the video */
    if(!vpi->block_screen_updates)
    {
        if(vpi->log_frames)
                event_logger_log_with_timestamp(LOGEVENT_VIDEO_FRAME_DISPLAYED, vpi->current_frame_ndx++);
        SDL_Flip(sfi->screen);
    }
    
    assert(id == NULL);
}
