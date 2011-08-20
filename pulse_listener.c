

#include "pulse_listener.h"

#include <linux/parport.h>
#include <linux/ppdev.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

void clear_pulse_register(pulse_listener * pl)
{
    int ret;
    ioctl(pl->fd, PPCLRIRQ, &ret);
}

void listener_thread_func(void * data)
{
    pulse_listener * pl = data;
    
    // open access to the parallel port
    pl->fd = open("/dev/parport0", O_RDWR);
    ioctl(pl->fd, PPCLAIM);
    int negot_val = IEEE1284_MODE_COMPAT;
    ioctl(pl->fd, PPNEGOT, &negot_val);

    // clear any pulses
    clear_pulse_register(pl);
    
    // start waiting
    fd_set rfds;
    FD_ZERO (&rfds);
    FD_SET (pl->fd, &rfds);
    int ret_val = select (pl->fd + 1, &rfds, NULL, NULL, NULL);
   
    
}


void initialize_pulse_listener(pulse_listener* pl)
{
    pl->fd = 0;
    pl->semaphore = SDL_CreateSemaphore(0);
    pl->listener_thread = SDL_CreateThread(listener_thread_func, pl);
    pl->timeout_ms = -1;
}
