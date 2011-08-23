/* 
 * File:   event_logger.h
 * Author: martin
 *
 * Created on 21. srpen 2011, 9:17
 */

#ifndef EVENT_LOGGER_H
#define	EVENT_LOGGER_H

#ifdef	__cplusplus
extern "C" {
#endif

    
    typedef enum {
        
        LOGEVENT_SYSTEM_STARTED,             // 0
        LOGEVENT_CALIBRATION_STARTED,        // 1
        LOGEVENT_EXPERIMENT_STARTED,         // 2
        LOGEVENT_EXPERIMENT_ENDED,           // 3
                
        LOGEVENT_MARQUEE_START,              // 4
        LOGEVENT_MARQUEE_DONE,               // 5
                
        LOGEVENT_VIDEO_FRAME_DISPLAYED,      // 6
        LOGEVENT_VIDEO_TIMEOUT,              // 7
        LOGEVENT_VIDEO_FORCED_STOP,          // 8
        LOGEVENT_WAITING_FOR_PULSE,          // 9
        LOGEVENT_PULSE_ACQUIRED,             // 10
        LOGEVENT_PULSE_TIMED_OUT,            // 11
               
    } logged_event_code;

    
    typedef struct {
        uint64_t timestamp;
        logged_event_code ev_code;
        int event_id;
    } log_entry;
    
    
#define LOG_ENTRIES_PER_SEGMENT 100000

    typedef struct log_segment {
        log_entry log_entries[LOG_ENTRIES_PER_SEGMENT];
        int next_log_pos;
        struct log_segment* next_log_segment;
    } log_segment;
    
    
    typedef struct {
        SDL_mutex * log_mutex;
        log_segment * first;
        log_segment * current;
    } event_logger;

    
    void event_logger_initialize();
    event_logger * event_logger_singleton();
    void event_logger_log_with_timestamp(logged_event_code ev_code, int event_id);
    void event_logger_save_log(char * file_name);
    void event_logger_shutdown();
    

#ifdef	__cplusplus
}
#endif

#endif	/* EVENT_LOGGER_H */

