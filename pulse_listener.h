/* 
 * File:   pulse_listener.h
 * Author: martin
 *
 * Created on 20. srpen 2011, 18:20
 */

#ifndef PULSE_LISTENER_H
#define	PULSE_LISTENER_H

#ifdef	__cplusplus
extern "C" {
#endif

    
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
    
    typedef struct {
        SDL_sem * semaphore;
        SDL_Thread * listener_thread;
        int timeout_ms;
        int fd;
    } pulse_listener;
    
    void initialize_pulse_listener(pulse_listener * pl);
    int wait_for_pulse();
    void finalize_pulse_listener(pulse_listener * pl);


#ifdef	__cplusplus
}
#endif

#endif	/* PULSE_LISTENER_H */

