

#include "pulse_listener.h"

#include <linux/parport.h>
#include <linux/ppdev.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

void clear_pulse_register(pulse_listener * pl)
{
    int ret;
    ioctl(pl->fd, PPCLRIRQ, &ret);
}


int listener_thread_func(void * data)
{
    pulse_listener * pl = data;
    
    // open access to the parallel port
    pl->fd = open("/dev/parport0", O_RDWR);
    ioctl(pl->fd, PPCLAIM);
    int negot_val = IEEE1284_MODE_COMPAT;
    ioctl(pl->fd, PPNEGOT, &negot_val);
    
    // clear any pulses
    clear_pulse_register(pl);

    pl->exit_flag = 0;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 20 * 1000;
    while(!pl->exit_flag)
    {
        // start waiting
        fd_set rfds;
        FD_ZERO (&rfds);
        FD_SET (pl->fd, &rfds);
        int ret_val = select (pl->fd + 1, &rfds, NULL, NULL, &timeout);

        // if we were signalled, activate the semaphore
        if(ret_val > 0)
        {
            SDL_LockMutex(pl->semaphore_mutex);
            if(pl->serviced_how == PL_REQ_NOT_SERVICED)
            {
                // we have acquired a pulse
                pl->serviced_how = PL_REQ_PULSE_ACQUIRED;
                               
                // we post the finding to the wait_for_pulse() method via semaphore
                SDL_SemPost(pl->semaphore);
            }
            SDL_UnlockMutex(pl->semaphore_mutex);
            
            printf("Have pulse from /dev/parport0.\n");
        }
    }
    
    return 0;
}


void pulse_listener_initialize(pulse_listener* pl)
{
    pl->fd = -1;
    pl->semaphore = SDL_CreateSemaphore(0);
    pl->semaphore_mutex = SDL_CreateMutex();
    pl->exit_flag = 0;
    pl->listener_thread = SDL_CreateThread(listener_thread_func, pl);
    pl->serviced_how = PL_REQ_NOT_SERVICED;
    pl->timer = 0;
}


uint32_t pulse_listener_timed_out(uint32_t timeout_ms, void * data)
{
    pulse_listener * pl = data;
    
    
    printf("pulse_listener_timed_out: in function.\n");
   
    SDL_LockMutex(pl->semaphore_mutex);
    if(pl->serviced_how == PL_REQ_NOT_SERVICED)
    {
        pl->serviced_how = PL_REQ_TIMED_OUT;
        SDL_SemPost(pl->semaphore);

        printf("pulse_listener_timed_out: i'm signalling semaphore.\n");
        fflush(stdout);
    }
    SDL_UnlockMutex(pl->semaphore_mutex);
    
    // we do not wish to renew the timer
    return 0;
}


int listen_for_pulse(pulse_listener * pl, uint32_t timeout_ms)
{
    clear_pulse_register(pl);
    
    // initialize service variables
    pl->serviced_how = PL_REQ_NOT_SERVICED;
    pl->timer = 0;
    
    // add time-out notification
    if(timeout_ms > 0)
        pl->timer = SDL_AddTimer(timeout_ms, pulse_listener_timed_out, pl);
    
    // we wait for the semaphore indefinitely
    SDL_SemWait(pl->semaphore);

    printf("Returning from listen_for_pulse() after semaphore cleared.\n");
    fflush(stdout);

    return pl->serviced_how;
}


void pulse_listener_shutdown(pulse_listener* pl)
{
    // set exit flag
    pl->exit_flag = 1;
    int status;
    SDL_WaitThread(pl->listener_thread, &status);
    
    // clean up after ourselves
    SDL_DestroyMutex(pl->semaphore_mutex); 
    pl->semaphore_mutex = 0;
    SDL_DestroySemaphore(pl->semaphore);
    
    close(pl->fd);
}

