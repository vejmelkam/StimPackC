

#include "SDL_draw.h"
#include <math.h>
#include <SDL/SDL.h>


void draw_navigation(SDL_Surface * s, SDL_Rect * rect, uint32_t target, uint32_t guides, uint32_t border)
{
    // border around entire SDL surface
    Draw_Rect(s, 0, 0, s->w, s->h, border);
    Draw_Rect(s, 1, 1, s->w - 2, s->h - 2, border);
    
    // yellow navigation lines to cue the render area
    Draw_HLine(s, 0, rect->y, s->w - 1, guides);
    Draw_HLine(s, 0, rect->y + rect->h, s->w - 1, guides);
    
    Draw_VLine(s, rect->x, 0, s->h - 1, guides);
    Draw_VLine(s, rect->x + rect->w, 0, s->h - 1, guides);
    
    // draw target rendering area
    Draw_Rect(s, rect->x, rect->y, rect->w + 1, rect->h + 1, target);
    Draw_Rect(s, rect->x+1, rect->y+1, rect->w - 1, rect->h - 1, target);
}


int calibrate_rendering_target_rect(SDL_Surface * surf, SDL_Rect * rect, double ratio)
{
    SDL_PixelFormat * fmt = surf->format;
    uint32_t black_col = SDL_MapRGB(fmt, 0, 0, 0);
    uint32_t red_col = SDL_MapRGB(fmt, 255, 40, 40);
    uint32_t green_col = SDL_MapRGB(fmt, 40, 255, 40);
    uint32_t yellow_col = SDL_MapRGB(fmt, 255, 255, 40);
    
    // blacken screen
    SDL_FillRect(surf, NULL, black_col);
    
    // make sure height is computed correctly
    rect->h = lrint(rect->w / ratio);
    
    // draw the rectangle with regular colors
    draw_navigation(surf, rect, red_col, yellow_col, green_col);
    SDL_Flip(surf);
    
    int quit = 0;
    int done = 0;
    SDL_Event event;
    SDL_Rect r2;
    while(!done)
    {
        if(!SDL_PollEvent(&event))
        {
            SDL_Delay(10);
            continue;
        }
        
        switch(event.type)
        {
            case SDL_KEYDOWN:

                // copy rectangle
                r2 = *rect;

                switch(event.key.keysym.sym)
                {
                    case SDLK_LEFT:
                        if(r2.x >= 10)
                        {
                            r2.x -= 10;
                            break;
                        } else continue;

                    case SDLK_RIGHT:
                        if(r2.x + r2.w < surf->w - 10)
                        {
                            r2.x += 10;
                            break;
                        } else continue;

                    case SDLK_UP:
                        if(r2.y >= 10)
                        {
                            r2.y -= 10;
                            break;
                        } else continue;

                    case SDLK_DOWN:
                        if(r2.y + r2.h < surf->h - 10)
                        {
                            r2.y += 10;
                            break;
                        } else continue;

                    case SDLK_w:
                        r2.w += 10;
                        r2.x -= 5;
                        r2.h = lrint(r2.w / ratio);
                        r2.y -= (r2.h - rect->h) / 2;

                        // if update takes us outside surface area, cancel movement
                        if(r2.x < 0 || r2.y < 0 || r2.x+r2.w >= surf->w || r2.y + r2.h >= surf->h)
                            continue;
                        break;

                    case SDLK_s:
                        r2.w -= 10;
                        r2.x += 5;
                        r2.h = lrint(r2.w / ratio);
                        r2.y -= (r2.h - rect->h) / 2;

                        // if update takes us outside surface area, cancel movement
                        if(r2.x < 0 || r2.y < 0 || r2.x+r2.w >= surf->w || r2.y + r2.h >= surf->h)
                            continue;
                        break;

                    case SDLK_RETURN:
                        printf("Exiting calibration procedure with rect (x=%d,y=%d,w=%d,h=%d).\n",
                                rect->x, rect->y, rect->w, rect->h);
                        done = 1;
                        break;

                    default:
                        break;
                }

                // erase in last position, draw in new position
                draw_navigation(surf, rect, black_col, black_col, black_col);
                draw_navigation(surf, &r2, red_col, yellow_col, green_col);
                *rect = r2;
                SDL_Flip(surf);
                break;

            case SDL_QUIT:
                done = 1;
                quit = 1;
                printf("Exiting calibration procedure with QUIT intention.\n");
                break;

            default:
                break;

        } // switch evtype
    } // wait while not done
    
    // cleanup after calibration screen
    SDL_FillRect(surf, NULL, black_col);
    SDL_Flip(surf);
    
    return quit;
}

