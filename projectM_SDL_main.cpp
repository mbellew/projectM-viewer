//
//  main.cpp
//  projectM-sdl
//
//  Created by Mischa Spiegelmock on 6/3/15.
//
//  This is an implementation of projectM using libSDL2

#include "pmSDL.hpp"
#include "PresetChooser.hpp"
#include <cstdio>
#include "DancingManShape.h"

void *handleConsole(void *);
inline Uint64 min(Uint64 a,Uint64 b) {return a<b?a:b;}


// return path to config file to use
std::string getConfigFilePath() {
    const char *path = DATADIR_PATH;
    struct stat sb = {};
    
    // check if datadir exists.
    // it should exist if this application was installed. if not, assume we're developing it and use currect directory
    if (stat(path, &sb) != 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "Could not open datadir path %s.\n", DATADIR_PATH);
    }
    
    std::string configFilePath = path;
    configFilePath += "/config.inp";
    
    // check if config file exists
    if (stat(configFilePath.c_str(), &sb) != 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "No config file %s found. Using development settings.\n", configFilePath.c_str());
        return "";
    }
    
    return configFilePath;
}


inline bool streq(char *a,char *b) { return 0==strcmp(a,b); }


// HANDLE CONSOLE

// TODO: add _IO_FILE *input_dev to projectMSDL
char *command_input_device = nullptr;
char *spotify_dbus_device = nullptr;
char *persee_device = nullptr;

void *handleConsole(void *pv)
{
    auto *app = (projectMSDL *)pv;

    _IO_FILE *in = stdin;
    if (nullptr != command_input_device)
    {
        in = fopen(command_input_device,"r");
        if (in == nullptr)
        {
            perror("commands");
            return nullptr;
        }
    }

    char str[4*1024];
    while (fgets(str, sizeof(str)-1, in))
    {
        size_t len = strlen(str);
        if (len > 0 && str[len-1]=='\n')
            str[--len] = 0;
        if (len == 0)
            continue;
        if (str[0] >= '0' and str[0] <= '9')
        {
            int i = atoi(str);
            app->pushEvent(SELECT_PRESET_INDEX, i);
        }
        else if (streq(str,"toggleFullScreen"))
        {
            app->pushEvent(SET_FULL_SCREEN, -1);
        }
        else if (streq(str,"selectRandom"))
        {
            app->pushEvent(SELECT_RANDOM);
        }
        else if (streq(str,"selectPrevious"))
        {
            app->pushEvent(SELECT_PREVIOUS);
        }
        else if (streq(str,"selectNext"))
        {
            app->pushEvent(SELECT_NEXT);
        }
        else if (str[0] == '/')
        {
            char *cp = strcpy((char*)malloc(strlen(str)),str);
            app->pushEvent(SELECT_PRESET_URL, 0, cp);
        }
    }
    return nullptr;
}


void *handleSpotify(void *pv)
{
    auto *app = (projectMSDL *)pv;

    _IO_FILE *in = stdin;
    if (nullptr == spotify_dbus_device)
        return nullptr;

    in = fopen(spotify_dbus_device,"r");
    if (in == nullptr)
    {
        perror("spotify");
        return nullptr;
    }

    Uint64 last_track_ticks = 0;
    char last_track_title[200] = "";
    char str[8*1024];

    while (fgets(str, sizeof(str)-1, in))
    {
        size_t len = strlen(str);
        if (len > 0 && str[len-1]=='\n')
            str[--len] = 0;
        if (len == 0)
            continue;

        char title[200] = "";
        char *property = strstr(str,"'xesam:title': <'");
        if (nullptr != property)
        {
            char *title_ptr = property + strlen("'xesam:title': <'");
            char *end = strstr(title_ptr, "'>");
            if (nullptr != end)
            {
                strncpy(title, title_ptr, min(end - title_ptr,sizeof(title)));
                title[end - title_ptr] = 0;
            }
        }

        long microseconds = -1;
        property = strstr(str,"'mpris:length': <uint64 ");
        if (nullptr != property)
        {
            char *digits = property+strlen("'mpris:length': <uint64 ");
            char *end = strstr(digits,">");
            if (nullptr != end)
                microseconds = atol(digits);
        }

        // avoid duplicates
        Uint64 ticks = SDL_GetTicks();
        if (ticks - last_track_ticks < 1000)
            continue;
        if (0==strcmp(title, last_track_title))
            continue;
        strcpy(last_track_title,title);
        last_track_ticks = ticks;
        printf("[SPOTIFY] title: %s duration: %d\n", title, (int)(microseconds/1000000));
        if (strlen(title)==0)
            printf("%s\n",str);
        app->pushEvent(SELECT_RANDOM);
    }
    return nullptr;
}


bool parseFloat(char **inout, float *f)
{
    char *start = *inout;
    char *end;
    *f = strtof(start, &end);
    if (end == start)
        return false;
    *inout = end;
    return true;
}


bool parsePair(char **inout, float *xy)
{
    char *p = *inout;
    if (*p++ != '(') return false;
    long x, y;
    parseFloat(&p, &xy[0]);
    if (*p++ != ',') return false;
    parseFloat(&p, &xy[1]);
    if (*p++ != ')') return false;
    *inout = p;
    return true;
}


// TODO handle multiple skeletons (enter,leave)
void *handlePersee(void *pv)
{
    auto *app = (projectMSDL *) pv;

    _IO_FILE *in = stdin;
    if (nullptr == persee_device)
        return nullptr;

    while (1)
    {
        in = fopen(persee_device, "r");
        if (in == nullptr) {
            perror("persee");
            return nullptr;
        }

        char str[8 * 1024];

        while (fgets(str, sizeof(str) - 1, in)) {
            char *p = str;
            size_t len = strlen(str);
            if (len > 0 && str[len - 1] == '\n')
                str[--len] = 0;
            if (len < 50 || str[0] != '[' || str[len - 1] != ']')
                continue;

            Skeleton *s = new Skeleton();
            Skeleton &skel = *s;
            if (*p++ != '[') continue;
            if (!parsePair(&p, skel.left_foot) || *p++ != ',') continue;
            if (!parsePair(&p, skel.left_knee) || *p++ != ',') continue;
            if (!parsePair(&p, skel.left_hip) || *p++ != ',') continue;
            if (!parsePair(&p, skel.left_shoulder) || *p++ != ',') continue;
            if (!parsePair(&p, skel.left_elbow) || *p++ != ',') continue;
            if (!parsePair(&p, skel.left_wrist) || *p++ != ',') continue;

            if (!parsePair(&p, skel.right_foot) || *p++ != ',') continue;
            if (!parsePair(&p, skel.right_knee) || *p++ != ',') continue;
            if (!parsePair(&p, skel.right_hip) || *p++ != ',') continue;
            if (!parsePair(&p, skel.right_shoulder) || *p++ != ',') continue;
            if (!parsePair(&p, skel.right_elbow) || *p++ != ',') continue;
            if (!parsePair(&p, skel.right_wrist)) continue;

            skel.scale_by(1.0f / 2000.0f, 0.5, 0.5);
            //printf("%f %f\n", skel.left_foot[0], skel.right_wrist[1]);
            app->pushEvent(UPDATE_SKELETON, 0, s);
        }
        fclose(in);
    }
}


int main(int argc, char *argv[])
{
    for (int i=1 ; i<argc ; i++)
    {
        char *arg = argv[i];
        if (0==strncmp(arg,"--commands=",strlen("--commands=")))
            command_input_device=arg+strlen("--commands=");
        if (0==strncmp(arg,"--spotify=",strlen("--spotify=")))
            spotify_dbus_device=arg+strlen("--spotify=");
        if (0==strncmp(arg,"--persee=",strlen("--persee=")))
            persee_device=arg+strlen("--persee=");
    }

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    if (! SDL_VERSION_ATLEAST(2, 0, 5)) {
        SDL_Log("SDL version 2.0.5 or greater is required. You have %i.%i.%i", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
        return 1;
    }

    // default window size to usable bounds (e.g. minus menubar and dock)
    SDL_Rect initialWindowBounds;
#if SDL_VERSION_ATLEAST(2, 0, 5)
    // new and better
    SDL_GetDisplayUsableBounds(0, &initialWindowBounds);
#else
    SDL_GetDisplayBounds(0, &initialWindowBounds);
#endif
    int width = initialWindowBounds.w / 2;
    int height = initialWindowBounds.h / 2;

    SDL_Window *win = SDL_CreateWindow("projectM", 0, 0, width, height, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *rend = SDL_CreateRenderer(win, 0, SDL_RENDERER_ACCELERATED);
    if (! rend) {
        fprintf(stderr, "Failed to create renderer: %s\n", SDL_GetError());
        SDL_Quit();
    }
    SDL_SetWindowTitle(win, "projectM Visualizer");

    projectMSDL *app;

    // load configuration file
    std::string configFilePath = getConfigFilePath();

    if (false && ! configFilePath.empty()) {
        // found config file, use it
        app = new projectMSDL(configFilePath, 0);
    } else {
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Config file not found, using development settings\n");
        float heightWidthRatio = (float)height / (float)width;
        projectM::Settings settings;
        settings.windowWidth = width;
        settings.windowHeight = height;
        settings.meshX = 300;
        settings.meshY = settings.meshX * heightWidthRatio;
        settings.fps   = FPS;
        settings.smoothPresetDuration = 1; // seconds
        settings.presetDuration = 30; // seconds
        settings.beatSensitivity = 0.8;
        settings.aspectCorrection = 1;
        settings.shuffleEnabled = 01;
        settings.softCutRatingsEnabled = 1; // ???
        // get path to our app, use CWD for presets/fonts/etc
        std::string base_path = SDL_GetBasePath();
        settings.presetURL = base_path + "presets/presets_tryptonaut";
        settings.menuFontURL = base_path + "fonts/Vera.ttf";
        settings.titleFontURL = base_path + "fonts/Vera.ttf";
        // init with settings
        app = new projectMSDL(settings, 0);
    }
    app->init(win, rend);
    app->changePresetDuration(600);
    printf("playlist size=%d\n", app->getPlaylistSize());

    // get an audio input device
    app->openAudioInput();
    app->beginAudioCapture();

    pthread_t tidConsole=0, tidSpotify=0, tidPersee;
    pthread_create(&tidConsole, nullptr, &handleConsole, app);
    pthread_create(&tidSpotify, nullptr, &handleSpotify, app);
    pthread_create(&tidPersee, nullptr,  &handlePersee, app);

    // standard main loop
    const Uint32 frame_delay = 1000/FPS;
    Uint32 last_time = SDL_GetTicks();
    app->resize(width,height);
    while (! app->done) {
        app->renderFrame();
        app->pollEvent();
        Uint32 elapsed = SDL_GetTicks() - last_time;
        if (elapsed < frame_delay)
            SDL_Delay(frame_delay - elapsed);
        last_time = SDL_GetTicks();
    }

    delete app;

    return PROJECTM_SUCCESS;
}