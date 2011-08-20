

#include <assert.h>
#include <memory.h>
#include <malloc.h>
#include <stdlib.h>
#include <math.h>

#include "video_player.h"


static void  display_callback(void *data, void *id);
static void  unlock_callback(void *data, void *id, void *const *p_pixels);
static void* lock_callback(void *data, void **p_pixels);


void vp_initialize(video_player_info * vpi)
{       
    int argc = 1;
    char const *argv[] = {
        "--alsa-caching=5000", /* skip any audio track */
        "--no-xlib"
    };    
    vpi->inst = libvlc_new(argc, argv);
    vpi->player = libvlc_media_player_new(vpi->inst);
    vpi->media = 0;
}



void vp_cleanup(video_player_info * vpi)
{
    assert(vpi->player);
    assert(vpi->inst);
    assert(vpi->media == 0);
    
    libvlc_media_player_release(vpi->player);
    vpi->player = 0;
    libvlc_release(vpi->inst);
    vpi->inst = 0;
}


void vp_prepare_media(video_player_info * vpi, const char * filename)
{
    assert(vpi->player);
    assert(vpi->media == 0);
    vpi->media = libvlc_media_new_path(vpi->inst, filename);
    libvlc_media_player_set_media(vpi->player, vpi->media);
    libvlc_media_add_option(vpi->media, "no-video-title-show");
    
    // setup the rendering mechanism
    sdl_frame_info * sfi = vpi->sfi;
    libvlc_video_set_format(vpi->player, "RV32", sfi->vid_rect.w, sfi->vid_rect.h, sfi->vid_rect.w * 4);
    libvlc_video_set_callbacks(vpi->player, lock_callback, unlock_callback, display_callback, vpi);
    
    // we can release media right now as per example
    libvlc_media_release(vpi->media);
}


//FIXME: wtf? release not needed?
void vp_release_media(video_player_info * vpi)
{
    assert(vpi->media);
    vpi->media = 0;
}


void vp_play_with_timeout(video_player_info * vpi, uint32_t timeout_ms, int volume)
{
    assert(vpi->player);
    assert(vpi->media);
    
    // setup required state
    vpi->start_time = SDL_GetTicks();
    vpi->end_time = vpi->start_time + timeout_ms;
    vpi->nominal_volume = volume;
    
    // send commands to media player
    libvlc_media_player_play(vpi->player);
    libvlc_audio_set_volume(vpi->player, volume);
}


void vp_stop(video_player_info* vpi)
{
    assert(vpi->player);
    assert(vpi->media);
    libvlc_media_player_stop(vpi->player);
}


static void *lock_callback(void *data, void **p_pixels)
{
    video_player_info * vpi = data;
    sdl_frame_info * sfi = vpi->sfi;

    SDL_LockMutex(sfi->mutex);
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
    SDL_UnlockMutex(sfi->mutex);

    assert(id == NULL); /* picture identifier, not needed here */

    // do the blit now
    SDL_BlitSurface(sfi->vid_surf, 0, sfi->screen, &sfi->vid_rect);
}

static void display_callback(void *data, void *id)
{
    video_player_info * vpi = data;
    sdl_frame_info * sfi = vpi->sfi;

    /* VLC wants to display the video */
    SDL_Flip(sfi->screen);
    assert(id == NULL);
}
