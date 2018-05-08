//
//  pmSDL.hpp
//  SDLprojectM
//
//  Created by Mischa Spiegelmock on 2017-09-18.
//

#ifndef pmSDL_hpp
#define pmSDL_hpp

#include "projectM-opengl.h"
#include <projectM.hpp>
#include <sdltoprojectM.h>
#include <iostream>
#include <sys/stat.h>
#include <SDL2/SDL.h>
#include <Renderable.hpp>
#include "SvgShape.h"


// DATADIR_PATH should be set by the root Makefile if this is being
// built with autotools.
#ifndef DATADIR_PATH
    #ifdef DEBUG
        #define DATADIR_PATH "."
        #warning "DATADIR_PATH is not defined - falling back to ./"
    #else
        #define DATADIR_PATH "/usr/local/share/projectM"
        #warning "DATADIR_PATH is not defined - falling back to /usr/local/share/projectM"
    #endif
#endif

const float FPS = 60;


enum customEvents
{
    // SETTINGS
    SETTING_MESH, /* x, y */
    SETTING_FPS,
    SETTING_TEXTURE_SIZE,
    SETTING_WINDOW_SIZE,  /* width, height */
    SETTING_PRESET_URL,
    SETTING_TITLE_FONT_URL,
    SETTING_MENU_FONT_URL,
    SETTING_SMOOTH_PRESET_DURATION,
    SETTING_PRESET_DURATION,
    SETTING_BEAT_SENSITIVITY,
    SETTING_ASPECT_CORRECTION,
    SETTING_EASTER_EGG,
    SETTING_SHUFFLE_ENABLED,
    SETTING_SOFT_CUT_RATINGS_ENABLED,
    // COMMANDS
    SELECT_PRESET_INDEX,
    SELECT_PRESET_URL,
    SELECT_NEXT,
    SELECT_PREVIOUS,
    SELECT_RANDOM,
    SET_FULL_SCREEN, /* 1=FULL, 0=DESKTOP, -1=TOGGLE */
    // POINTS
    CLEAR_POINTS,
    ADD_POINT,       /* x*1000, y*1000 */
    ADD_LINE,         /* (index, index) */

    // SKELETON TRACKING
    UPDATE_SKELETON   /* id, Skeleton * */
};


class projectMSDL : public projectM {
public:
    bool done;

    projectMSDL(Settings settings, int flags);
    projectMSDL(std::string config_file, int flags);
    void init(SDL_Window *window, SDL_Renderer *renderer);
    int openAudioInput();
    void beginAudioCapture();
    void endAudioCapture();
    void toggleFullScreen();
    void resize(unsigned int width, unsigned int height);
    void renderFrame();
    void pollEvent();
    void maximize();

    void pushEvent(enum customEvents code, int data1=0, void *data2=NULL);

private:
    int CUSTOM_EVENT_TYPE=0;

    SDL_Window *win;
    SDL_Renderer *rend;
    bool isFullScreen;
    projectM::Settings settings;
    SDL_AudioDeviceID audioInputDevice;
    unsigned int width, height;

    // audio input device characteristics
    unsigned short audioChannelsCount;
    unsigned short audioSampleRate;
    unsigned short audioSampleCount;
    SDL_AudioFormat audioFormat;
    SDL_AudioDeviceID audioDeviceID;
    unsigned char *pcmBuffer;  // pre-allocated buffer for audioInputCallback

    static void audioInputCallbackF32(void *userdata, unsigned char *stream, int len);
    static void audioInputCallbackS16(void *userdata, unsigned char *stream, int len);


    void addFakePCM();
    void keyHandler(SDL_Event *);
    void customEventHandler(SDL_Event *);
    SDL_AudioDeviceID selectAudioInput(int count);

    Pipeline &modifyPipeline(Pipeline &presetPipeline, PipelineContext &pipelineContext);
};


#endif /* pmSDL_hpp */
