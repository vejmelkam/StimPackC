

#include "pulse_listener.h"
#include "event_logger.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

#include <SDL/SDL.h>

volatile int done = 0;

void signal_handler(int sig)
{
  printf("[pulse_test] signal received\n");
  done = 1;
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

int main(int argc, char ** argv)
{
    pulse_listener pl;

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTTHREAD | SDL_INIT_TIMER) == -1)
    {
        printf("[pulse_test] cannot initialize SDL\n");
        return -1;
    }

    // install SIGINT handler
    signal(SIGINT, signal_handler);

    // set mode for stdio output
    setvbuf(stdout, 0, _IOLBF, 1024);

    // initialize logging system
    event_logger_initialize();
    event_logger_log_with_timestamp(LOGEVENT_SYSTEM_STARTED, 0);

    // initialize pulse listener system
    pulse_listener_initialize(&pl);

    // start logging pulses right now
    pulse_listener_log_pulses(&pl, 1);

    while(!done)
    {
        printf("[wait-for-pulse] [%s] waiting for pulse ...\n",
	       string_timestamp());

        switch(listen_for_pulse(&pl, 3000))
	{
	
	case PL_REQ_TIMED_OUT:
	    printf("[wait-for-pulse] [%s] timed out waiting for pulse.\n",
		   string_timestamp());
	    break;

	case PL_REQ_PULSE_ACQUIRED:
	    printf("[wait-for-pulse] [%s] pulse received.\n",
		   string_timestamp());
	    break;

	default:
	  printf("No flag set!\n");
	  break;
	}
        
        // we have the info, clear the request flag
        pulse_listener_clear_request(&pl);
    }

    event_logger_save_log("pulse_test.log");    

    pulse_listener_shutdown(&pl);
    event_logger_shutdown();

    printf("[pulse_tester] exiting.\n");

    return 0;
}  
