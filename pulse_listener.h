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
    
    typedef enum
    {
        PL_REQ_NO_REQUEST,
        PL_REQ_NOT_SERVICED,
        PL_REQ_TIMED_OUT,
        PL_REQ_PULSE_ACQUIRED
    } serviced_info;

    typedef struct {
        SDL_sem * semaphore;
        SDL_Thread * listener_thread;
        SDL_mutex * service_mutex;
        int fd;
        int exit_flag;
        volatile serviced_info serviced_how;
        SDL_TimerID timer;
        int log_pulses;
    } pulse_listener;
    
    void pulse_listener_initialize(pulse_listener * pl);
    
    // returns 1 if pulse was registered and 0 if the waiting thread timed out
    int listen_for_pulse(pulse_listener * pl, uint32_t timeout_ms);
    
    void pulse_listener_clear_request(pulse_listener * pl);
    
    // 1 means yes, 0 means no
    void pulse_listener_log_pulses(pulse_listener * pl, int flag);
    
    void pulse_listener_shutdown(pulse_listener * pl);


#ifdef	__cplusplus
}
#endif

#endif	/* PULSE_LISTENER_H */

