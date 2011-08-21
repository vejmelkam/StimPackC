


#include <SDL/SDL_mutex.h>

#include "event_logger.h"

#include <assert.h>
#include <malloc.h>
#include <time.h>
#include <stdio.h>


static event_logger * ev_log = 0;


event_logger * event_logger_singleton()
{
    assert(ev_log);
    return ev_log;
}


static void event_logger_add_log_segment()
{
    assert(ev_log);
    
    // allocate a new log segment
    log_segment * ls = (log_segment*)malloc(sizeof(log_segment));
    ls->next_log_pos = 0;
    ls->next_log_segment = 0;
    memset(ls->log_entries, sizeof(ls->log_entries), 0);
    
    if(ev_log->current)
    {
        ev_log->current->next_log_segment = (struct log_segment*)ls;
        ev_log->current = ls;
    }
    else if(!ev_log->first)
    {
        ev_log->first = ls;
        ev_log->current = ls;
    }
    else
        assert(0);
}


void event_logger_initialize()
{
    ev_log = (event_logger*)malloc(sizeof(event_logger));
    ev_log->log_mutex = SDL_CreateMutex();
    ev_log->current = ev_log->first = 0;
    
    event_logger_add_log_segment();
}


void event_logger_log_with_timestamp(logged_event_code ev_code, int event_id)
{
    // get current time
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    
    SDL_LockMutex(ev_log->log_mutex);

    log_segment * ls = ev_log->current;
    if(ls->next_log_pos == LOG_ENTRIES_PER_SEGMENT)
    {
        event_logger_add_log_segment();
        ls = ev_log->current;
    }
    
    log_entry * le = &ls->log_entries[ls->next_log_pos++];
    le->timestamp = (uint64_t)now.tv_sec * 1e6 + now.tv_nsec / 1000;
    le->ev_code = ev_code;
    le->event_id = event_id;

    SDL_UnlockMutex(ev_log->log_mutex);
}


void event_logger_save_log(char * file_name)
{
    FILE * f = fopen(file_name, "w");
    if(!f)
    {
        printf("[event-logger] panic! cannot open log file.\n");
        return;
    }
    
    SDL_LockMutex(ev_log->log_mutex);
    log_segment * ls = ev_log->first;
    
    while(ls)
    {
        for(int i = 0; i < ls->next_log_pos; i++)
        {
            log_entry * le = &ls->log_entries[i];
            fprintf(f, "%lu, %d, %d\n", le->timestamp, le->ev_code, le->event_id);
        }
        
        // move to next log-segment
        ls = ls->next_log_segment;
    }
    
    SDL_UnlockMutex(ev_log->log_mutex);
    fclose(f);
}


void event_logger_shutdown()
{
    log_segment * ls = ev_log->first;
    while(ls)
    {
        log_segment * next = ls->next_log_segment;
        free(ls);
        ls = next;
    }
    
    SDL_DestroyMutex(ev_log->log_mutex);
    ev_log->first = ev_log->current = 0;
    
    free(ev_log);
    ev_log = 0;
}


