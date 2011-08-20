/* 
 * File:   video_player.h
 * Author: martin
 *
 * Created on 20. srpen 2011, 10:12
 */

#ifndef VIDEO_PLAYER_H
#define	VIDEO_PLAYER_H

#ifdef	__cplusplus
extern "C" {
#endif


#include <SDL/SDL.h>
#include <vlc/vlc.h>

    
    // frame structure which is passed to callback functions
    typedef struct {
        
        // SDL main screen
        SDL_Rect scr_rect;
        SDL_Surface * screen;

        // current SDL rendering surface
        SDL_Surface * vid_surf;
        SDL_Rect vid_rect;

    } sdl_frame_info;
    
    
    // video player structure containing everything relevant for playback
    typedef struct {
        libvlc_instance_t * inst;
        libvlc_media_player_t * player;
        sdl_frame_info * sfi;
        uint32_t start_time;
        uint32_t end_time;
        int nominal_volume;
        int block_screen_updates;
    } video_player_info;
    

    void vp_initialize(video_player_info * vpi);    
    void vp_cleanup(video_player_info * vpi);
    void vp_prepare_media(video_player_info * vpi, const char * filename);
    void vp_play_with_timeout(video_player_info* vpi, unsigned int timeout_ms, int volume);
    void vp_stop(video_player_info* vpi);
    void vp_release_media(video_player_info * vpi);
    

#define VIDEO_PLAYBACK_TIMEOUT_EVENT (SDL_USEREVENT)
#define VLC_FLIP_REQUEST (SDL_USEREVENT+1)

#ifdef	__cplusplus
}
#endif

#endif	/* VIDEO_PLAYER_H */

