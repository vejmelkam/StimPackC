/* 
 * File:   calibration.h
 * Author: martin
 *
 * Created on 20. srpen 2011, 17:34
 */

#ifndef CALIBRATION_H
#define	CALIBRATION_H

#ifdef	__cplusplus
extern "C" {
#endif

    // calibrate the rendering rectangle for videos with given ratio (if non-zero, quit requested)
    int calibrate_rendering_target_rect(SDL_Surface * surf, SDL_Rect * rect, double ratio);


#ifdef	__cplusplus
}
#endif

#endif	/* CALIBRATION_H */

