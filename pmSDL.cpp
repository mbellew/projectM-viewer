//
//  pmSDL.cpp
//  SDLprojectM
//
//  Created by Mischa Spiegelmock on 2017-09-18.
//

#include <src/libprojectM/Renderer/Pipeline.hpp>
#include <src/libprojectM/Renderer/Waveform.hpp>
#include <src/libprojectM/Renderer/MilkdropWaveform.hpp>
#include "pmSDL.hpp"
#include "SvgShape.h"
#include "BurningManShape.h"
#include "DancingManShape.h"


Shape shapes[5];
MilkdropWaveform waves[5];
BurningManShape *bm = new BurningManShape();
DancingManShape *dm = new DancingManShape();


enum ShapeType {Shape_, MilkdropWaveform_,SVG_};
struct ShapeWrapper
{
    ShapeWrapper(){}
    ShapeWrapper(Shape *shape) : shape(shape), type(Shape_){}
    ShapeWrapper(MilkdropWaveform *shape) : shape(shape), type(MilkdropWaveform_){}
    ShapeWrapper(SvgShape *shape) : shape(shape), type(SVG_){}
    ShapeType type;
    RenderItem *shape;
    void setXY(float x, float y)
    {
        switch (type)
        {
            case Shape_:
                ((Shape *) shape)->x = x;
                ((Shape *) shape)->y = y;
                break;
            case MilkdropWaveform_:
                ((MilkdropWaveform *) shape)->x = x;
                ((MilkdropWaveform *) shape)->y = y;
                break;
            case SVG_:
                ((SvgShape *)shape)->x = x;
                ((SvgShape *)shape)->y = y;
                break;
        }
    }
    void setRGB(float r, float g, float b, float a)
    {
        switch (type)
        {
            case Shape_:
                ((Shape *) shape)->r = r;
                ((Shape *) shape)->g = g;
                ((Shape *) shape)->b = b;
                ((Shape *) shape)->a = a;
                break;
            case MilkdropWaveform_:
                ((MilkdropWaveform *) shape)->r = r;
                ((MilkdropWaveform *) shape)->g = g;
                ((MilkdropWaveform *) shape)->b = b;
                ((MilkdropWaveform *) shape)->r = a;
                break;
        }
    }
};
std::vector<ShapeWrapper> currentShapes(0);


void projectMSDL::audioInputCallbackF32(void *userdata, unsigned char *stream, int len) {
    projectMSDL *app = (projectMSDL *) userdata;
    //    printf("LEN: %i\n", len);
    // stream is (i think) samples*channels floats (native byte order) of len BYTES
    app->pcm()->addPCMfloat((float *)stream, len/sizeof(float));
}

void projectMSDL::audioInputCallbackS16(void *userdata, unsigned char *stream, int len) {
    //    printf("LEN: %i\n", len);
    projectMSDL *app = (projectMSDL *) userdata;
    short pcm16[2][512];

    for (int i = 0; i < 512; i++) {
        for (int j = 0; j < app->audioChannelsCount; j++) {
            pcm16[j][i] = stream[i+j];
        }
    }
    app->pcm()->addPCM16(pcm16);
}


SDL_AudioDeviceID projectMSDL::selectAudioInput(int count) {
    // ask the user which capture device to use
    // printf("Please select which audio input to use:\n");
    int ret = 0;
    printf("Detected devices:\n");
    for (int i = 0; i < count; i++) {
        const char *name = SDL_GetAudioDeviceName(i, true);
        printf("  %i: 🎤%s\n", i, SDL_GetAudioDeviceName(i, true));
        if (NULL != strstr(name,"PnP"))
            ret = i;
        if (NULL != strstr(name,"Bose"))
            ret = i;
    }
    return ret;
}


int projectMSDL::openAudioInput() {
    // get audio driver name (static)
    const char* driver_name = SDL_GetCurrentAudioDriver();
    SDL_Log("Using audio driver: %s\n", driver_name);

    // get audio input device
    int i, count = SDL_GetNumAudioDevices(true);  // capture, please
    if (count == 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "No audio capture devices found");
        SDL_Quit();
    }
    for (i = 0; i < count; i++) {
        SDL_Log("Found audio capture device %d: %s", i, SDL_GetAudioDeviceName(i, true));
    }

    SDL_AudioDeviceID selectedAudioDevice = 0;  // device to open
    if (count > 1) {
        // need to choose which input device to use
        selectedAudioDevice = selectAudioInput(count);
        if (selectedAudioDevice > count) {
            SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "No audio input device specified.");
            SDL_Quit();
        }
    }

    // params for audio input
    SDL_AudioSpec want, have;

    // requested format
    SDL_zero(want);
    want.freq = 48000;
    want.format = AUDIO_F32;  // float
    want.channels = 2;
    want.samples = 512;
    want.callback = projectMSDL::audioInputCallbackF32;
    want.userdata = this;

    audioDeviceID = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(selectedAudioDevice, true), true, &want, &have, 0);

    if (audioDeviceID == 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to open audio capture device: %s", SDL_GetError());
        SDL_Quit();
    }

    // read characteristics of opened capture device
    SDL_Log("Opened audio capture device %i: %s", audioDeviceID, SDL_GetAudioDeviceName(selectedAudioDevice, true));
    SDL_Log("Sample rate: %i, frequency: %i, channels: %i, format: %i", have.samples, have.freq, have.channels, have.format);
    audioChannelsCount = have.channels;
    audioSampleRate = have.freq;
    audioSampleCount = have.samples;
    audioFormat = have.format;
    audioInputDevice = audioDeviceID;
    return 1;
}

void projectMSDL::beginAudioCapture() {
    // allocate a buffer to store PCM data for feeding in
    unsigned int maxSamples = audioChannelsCount * audioSampleCount;
    pcmBuffer = (unsigned char *) malloc(maxSamples);
    SDL_PauseAudioDevice(audioDeviceID, false);
    pcm()->initPCM(2048);
}

void projectMSDL::endAudioCapture() {
    free(pcmBuffer);
    SDL_PauseAudioDevice(audioDeviceID, true);
}

void projectMSDL::maximize() {
    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm) != 0) {
        SDL_Log("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
        return;
    }

    SDL_SetWindowSize(win, dm.w, dm.h);
    resize(dm.w, dm.h);
}

void projectMSDL::toggleFullScreen() {
    maximize();
    if (isFullScreen) {
        SDL_SetWindowFullscreen(win, 0); // SDL_WINDOW_FULLSCREEN_DESKTOP);
        isFullScreen = false;
        SDL_ShowCursor(true);
    } else {
        SDL_ShowCursor(false);
        SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN);
        isFullScreen = true;
    }
}

void projectMSDL::keyHandler(SDL_Event *sdl_evt) {
    projectMEvent evt;
    projectMKeycode key;
    projectMModifier mod;
    SDL_Keymod sdl_mod = (SDL_Keymod) sdl_evt->key.keysym.mod;
    SDL_Keycode sdl_keycode = sdl_evt->key.keysym.sym;

    // handle keyboard input (for our app first, then projectM)
    switch (sdl_keycode) {
        case SDLK_f:
            if (sdl_mod & KMOD_LGUI || sdl_mod & KMOD_RGUI || sdl_mod & KMOD_LCTRL) {
                // command-f: fullscreen
                toggleFullScreen();
                return; // handled
            }
            break;
    }

    // translate into projectM codes and perform default projectM handler
    evt = sdl2pmEvent(sdl_evt);
    key = sdl2pmKeycode(sdl_keycode);
    mod = sdl2pmModifier(sdl_mod);
    key_handler(evt, key, mod);
}

void projectMSDL::addFakePCM() {
    int i;
    short pcm_data[2][512];
    /** Produce some fake PCM data to stuff into projectM */
    for ( i = 0 ; i < 512 ; i++ ) {
        if ( i % 2 == 0 ) {
            pcm_data[0][i] = (float)( rand() / ( (float)RAND_MAX ) * (pow(2,14) ) );
            pcm_data[1][i] = (float)( rand() / ( (float)RAND_MAX ) * (pow(2,14) ) );
        } else {
            pcm_data[0][i] = (float)( rand() / ( (float)RAND_MAX ) * (pow(2,14) ) );
            pcm_data[1][i] = (float)( rand() / ( (float)RAND_MAX ) * (pow(2,14) ) );
        }
        if ( i % 2 == 1 ) {
            pcm_data[0][i] = -pcm_data[0][i];
            pcm_data[1][i] = -pcm_data[1][i];
        }
    }

    /** Add the waveform data */
    pcm()->addPCM16(pcm_data);
}

void projectMSDL::resize(unsigned int width_, unsigned int height_) {
    width = width_;
    height = height_;
    settings.windowWidth = width;
    settings.windowHeight = height;
    projectM_resetGL(width, height);
}

void projectMSDL::pollEvent() {
    SDL_Event evt;

    while (SDL_PollEvent(&evt))
    {
        switch (evt.type)
        {
            case SDL_WINDOWEVENT:
            {
                switch (evt.window.event)
                {
                    case SDL_WINDOWEVENT_RESIZED:
                        resize(evt.window.data1, evt.window.data2);
                        break;
                }
                break;
            }
            case SDL_MOUSEMOTION:
            {
                float x = (float)evt.motion.x / (float)width;
                float y = 1.0f - ((float)evt.motion.y / (float)height);
                currentShapes.clear();
                if (evt.motion.state)
                {
                    ShapeWrapper w = ShapeWrapper(&waves[0]);
                    w.setXY(x, y);
                    currentShapes.push_back(w);
                }
                else
                {
                    ShapeWrapper w = ShapeWrapper(dm);
                    w.setXY(x, y);
                    currentShapes.push_back(w);
                }
                break;
            }
            case SDL_KEYDOWN:
                keyHandler(&evt);
                break;
            case SDL_QUIT:
                done = true;
                break;
            default:
                if (evt.type == CUSTOM_EVENT_TYPE)
                    customEventHandler(&evt);
                break;
        }
    }
}


void projectMSDL::customEventHandler(SDL_Event *evt)
{
    //printf("custom %d %ld %ld\n", evt->user.code, (long)evt->user.data1, (long)evt->user.data2);
    switch (evt->user.code)
    {
        case SETTING_MESH:
        {
            Uint64 x = (Uint64) evt->user.data1;
            Uint64 y = (Uint64) evt->user.data2;
            break;
        }
        case SETTING_FPS:
        case SETTING_TEXTURE_SIZE:
            break;

        case SETTING_WINDOW_SIZE:
        {
            Uint64 width = (Uint64)evt->user.data1;
            Uint64 height = (Uint64)evt->user.data2;
            SDL_SetWindowSize(win, width, height);
            resize(width, height);
            break;
        }
        case SETTING_PRESET_URL:
        case SETTING_TITLE_FONT_URL:
        case SETTING_MENU_FONT_URL:
        case SETTING_SMOOTH_PRESET_DURATION:
        case SETTING_PRESET_DURATION:
        case SETTING_BEAT_SENSITIVITY:
        case SETTING_ASPECT_CORRECTION:
        case SETTING_EASTER_EGG:
        case SETTING_SHUFFLE_ENABLED:
        case SETTING_SOFT_CUT_RATINGS_ENABLED:
            break;

        // COMMANDS
        case SELECT_RANDOM:
            selectRandom(true);
            break;
        case SELECT_NEXT:
            selectNext(true);
            break;
        case SELECT_PREVIOUS:
            selectPrevious(true);
            break;
        case SELECT_PRESET_INDEX:
            selectPreset((int)(Uint64)evt->user.data1, true);
            break;
        case SELECT_PRESET_URL:
        {
            std::string url((char *)evt->user.data2);
            free(evt->user.data2);
            int max = getPlaylistSize();
            for (int i=0 ; i<max ; i++)
            {
                if (url == getPresetURL(i))
                {
                    selectPreset(i, true);
                    break;
                }
            }
            break;
        }
        case SET_FULL_SCREEN:
        {
            Sint64 opt = (Sint64)evt->user.data1;
            if (opt == 1 && !isFullScreen ||
                opt == 0 && isFullScreen ||
                opt == -1)
            {
                toggleFullScreen();
            }
            break;
        }

        case UPDATE_SKELETON:
        {
            Skeleton *skel = (Skeleton *)evt->user.data2;
            dm->update(*skel);
            delete skel;
        }
    }
}


void projectMSDL::renderFrame() {
    glClearColor( 0.0, 0.0, 0.0, 0.0 );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    projectM::renderFrame();
    glFlush();

    SDL_RenderPresent(rend);
}

projectMSDL::projectMSDL(Settings settings, int flags) : projectM(settings, flags) {
    width = getWindowWidth();
    height = getWindowHeight();
    done = 0;
    isFullScreen = false;
}

projectMSDL::projectMSDL(std::string config_file, int flags) : projectM(config_file, flags) {
    width = getWindowWidth();
    height = getWindowHeight();
    done = 0;
    isFullScreen = false;
}

void projectMSDL::init(SDL_Window *window, SDL_Renderer *renderer) {
    win = window;
    rend = renderer;
    CUSTOM_EVENT_TYPE = SDL_RegisterEvents(1);
    selectRandom(true);
    projectM_resetGL(width, height);
}

void projectMSDL::pushEvent(enum customEvents code, int data1, void *data2)
{
    SDL_Event event;
    SDL_zero(event);
    event.type = CUSTOM_EVENT_TYPE;
    event.user.code = code;
    event.user.data1 = (void *)(Uint64)data1;
    event.user.data2 = (void *)data2;
    SDL_PushEvent(&event);
}

inline float constrain(float x)
{
    return x<0.0f ? 0.0f : x>1.0f ? 1.0f : x;
}


Pipeline &projectMSDL::modifyPipeline(Pipeline &pipeline, PipelineContext &pipelineContext)
{

    if (currentShapes.empty())
    {
        Shape &shape = shapes[0];
        shape.enabled = true;
        shape.additive = true;
        shape.sides = 5;
        shape.r = 1;
        shape.a = 0.5;
        shape.r2 = 1;
        shape.a2 = 1;
        shape.border_b=1;
        shape.border_a=1;
        shape.radius = 0.2;
        shape.thickOutline = true;

        MilkdropWaveform &wave = waves[0];
        wave.r = 0.0;
        wave.g = 1.0;
        wave.b = 0.5;
        wave.a = 1.0;
        wave.mystery = 0;
        wave.mode = Blob3; //RadialBlob; // Circle;
        wave.modulateAlphaByVolume = false;
        wave.maximizeColors = true;
        wave.scale = 1.0;
        wave.dots = false;
        wave.thick = false;
        wave.additive = true;
        wave.smoothing = 0.5;
        wave.modOpacityStart = 0.7;
        wave.modOpacityEnd = 1.3;

        bm->r = 1.0;
        bm->border_r = 1.0;
        bm->border_a = 1.0;
        bm->radius = 0.2;

        dm->r = 1.0;
        dm->border_r = 1.0;
        dm->border_a = 1.0;
        dm->radius = 0.2;

        //currentShapes.push_back(ShapeWrapper(&shapes[0]));
        currentShapes.push_back(ShapeWrapper(dm));
        currentShapes.push_back(ShapeWrapper(&waves[0]));
    }

    float time = pipelineContext.time;
    float r = 0.5 + 0.45*(0.5*sin(time*0.701)+ 0.3*cos(time*0.438));
    float g = 0.5 - 0.4*(0.5*sin(time*4.782)+0.5*cos(time*0.722));
    float b = 0.5 + 0.4*sin(time*1.931);

    for (std::vector<ShapeWrapper>::iterator it = currentShapes.begin() ;
            it < currentShapes.end() ; it++)
    {
        it->setRGB(r,g,b,0.8);
        pipeline.drawables.push_back(it->shape);
    }
    return pipeline;
}


 /**

<?xml version="1.0" encoding="utf-8"?>
<!-- Generator: Adobe Illustrator 13.0.0, SVG Export Plug-In  -->
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd" [
	<!ENTITY ns_flows "http://ns.adobe.com/Flows/1.0/">
]>
<svg version="1.1"
	 xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:a="http://ns.adobe.com/AdobeSVGViewerExtensions/3.0/"
	 x="0px" y="0px" width="457px" height="707px" viewBox="-0.648 -0.055 457 707" enable-background="new -0.648 -0.055 457 707"
	 xml:space="preserve">
<defs>
</defs>
<path d="M82.46,703.835c-11.455-5.389-16.368-19.048-10.979-30.502l0,0c0-0.011,1.329-2.82,3.786-8.281l0,0
	c2.445-5.473,5.981-13.485,10.218-23.513l0,0c8.475-20.083,19.808-48.279,31.109-80.505l0,0
	c22.667-64.311,44.863-145.374,44.706-207.181l0,0c0-6.6-0.235-12.96-0.736-19.037l0,0c-3.998-51.687-33.68-116.585-66.664-171.523
	l0,0C61.048,108.202,25.479,62.461,11.535,45.17l0,0c-3.99-4.95-6.102-7.442-6.113-7.454l0,0
	c-8.178-9.661-6.967-24.128,2.706-32.295l0,0c9.672-8.178,24.128-6.967,32.306,2.705l0,0c0.414,0.501,38.336,45.339,77.964,107.565
	l0,0c39.486,62.473,81.403,141.388,87.849,215.373l0,0c0.604,7.465,0.89,15.075,0.89,22.789l0,0
	c-0.154,71.918-23.963,155.472-47.283,222.351l0,0c-23.404,66.723-46.679,116.182-46.892,116.652l0,0
	c-3.906,8.304-12.148,13.166-20.751,13.166l0,0C88.939,706.022,85.616,705.317,82.46,703.835L82.46,703.835z"/>
<path d="M342.478,692.851c-0.201-0.47-23.49-49.919-46.892-116.652l0,0c-23.334-66.879-47.143-150.433-47.3-222.339l0,0
	c0-7.75,0.297-15.357,0.906-22.799l0,0c6.452-73.988,48.375-152.887,87.857-215.362l0,0C376.689,53.472,414.6,8.634,415.014,8.122
	l0,0l0.011,0.011c8.17-9.672,22.624-10.881,32.299-2.717l0,0c9.657,8.178,10.878,22.634,2.702,32.307l0,0
	c0,0.011-2.126,2.504-6.11,7.453l0,0c-3.989,4.936-9.747,12.201-16.687,21.339l0,0c-13.888,18.278-32.577,44.066-51.508,73.798l0,0
	c-38.017,59.257-76.341,135.358-80.84,194.51l0,0c-0.498,6.099-0.755,12.461-0.755,19.036l0,0
	c-0.106,54.023,16.804,122.966,36.293,182.32l0,0c19.428,59.459,41.167,109.735,49.762,128.867l0,0
	c2.445,5.473,3.771,8.281,3.788,8.293l0,0c5.389,11.454,0.471,25.102-10.989,30.502l0,0c-3.156,1.482-6.469,2.188-9.736,2.188l0,0
	C354.637,706.028,346.383,701.154,342.478,692.851L342.478,692.851z"/>
<path d="M207.821,203.555L146.225,95.76c-5.324-9.322-3.494-21.02,4.415-28.266l0,0l61.597-56.466
	c8.769-8.049,22.201-8.049,30.969,0l0,0l61.597,56.466c7.907,7.235,9.736,18.944,4.415,28.266l0,0L247.62,203.555
	c-4.062,7.104-11.711,11.541-19.897,11.541l0,0C219.542,215.096,211.884,210.658,207.821,203.555L207.821,203.555z M195.111,88.914
	l32.606,57.07l32.617-57.081l-32.611-29.895L195.111,88.914L195.111,88.914z"/>
</svg>

 **/
