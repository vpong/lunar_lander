#ifndef SCREEN_H_
#define SCREEN_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

class Screen {
    private:
        SDL_Window* window;
    public:
        static const int WIDTH = 640;
        static const int HEIGHT = 480;
        SDL_Renderer* renderer;
        TTF_Font* font;

        Screen();
        ~Screen();
        void flip();
        void clear();
};

#endif
