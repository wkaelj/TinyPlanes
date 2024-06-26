#pragma once

#include <SDL2/SDL_surface.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __GNUC__
#define RENDER_WARN_RESULT [[gnu::warn_unused_result]]
#else
#define RENDER_WARN_RESULT ((void)0)
#endif

#define RENDER_NONULL(...) __attribute__((nonnull(__VA_ARGS__)))

typedef enum
{
    RENDER_SUCCESS = 0,
    RENDER_FAILURE = 1,
} RenderResult;

typedef enum
{
    RENDER_CURSOR_PRESSED,
    RENDER_CURSOR_RELEASED,
    RENDER_CURSOR_DOWN,
    RENDER_CURSOR_UP,
} RenderCursorState;

typedef enum
{
    RENDER_EVENT_NONE = 0,
    RENDER_EVENT_QUIT,
    RENDER_EVENT_WINDOW_RESIZE,
} RenderEvent;

// created in a window, can draw textures and boxes
typedef struct Render Render;

// a window that can be used to create a render
typedef struct RenderWindow RenderWindow;

// a texture loaded from a image file that can be drawn to the screen
typedef struct RenderTexture RenderTexture;

// a font loaded from a .ttf file used to create text
typedef struct RenderFont RenderFont;

// text created from a font that can be drawn to the screen
typedef struct RenderText RenderText;

typedef struct RenderButton RenderButton;

// a x, y, width and height specify an area on the screen or a size
typedef struct
{
    int32_t x, y, w, h;
} RenderRect;

typedef struct
{
    int32_t x, y;
} RenderCoord;

typedef struct RenderColour
{
    unsigned char r, g, b, a;
} RenderColour;

// initialize the render backend
RenderResult render_init();

// quit the render backend and release resources
void render_quit();

// check if the render is initialized
RENDER_WARN_RESULT bool render_is_initialized();

// create a window. NULL for failure
RENDER_WARN_RESULT RenderWindow *
render_create_window(const char *title, int w, int h) RENDER_NONULL(1);

// create a render. NULL for failure
RENDER_WARN_RESULT Render *render_create_render(RenderWindow *window)
    RENDER_NONULL(1);

void render_destroy_render(Render *render) RENDER_NONULL(1);
void render_destroy_window(RenderWindow *window) RENDER_NONULL(1);

RENDER_WARN_RESULT RenderButton *render_create_button(
    const Render *render,
    const RenderFont *font,
    const char *text,
    const RenderRect *position,
    const RenderColour background,
    const RenderColour border,
    const uint8_t border_width,
    const uint8_t padding) RENDER_NONULL(1, 2, 3, 4);

RENDER_WARN_RESULT bool
render_button_hovered(const Render *render, RenderButton *button)
    RENDER_NONULL(1, 2);
RENDER_WARN_RESULT bool
render_button_clicked(const Render *render, RenderButton *button)
    RENDER_NONULL(1, 2);
void render_button_draw(const Render *R, RenderButton *button)
    RENDER_NONULL(1, 2);

void render_destroy_button(RenderButton *button) RENDER_NONULL(1);
void render_button_set_pos(RenderButton *button, uint16_t x, uint16_t y)
    RENDER_NONULL(1);

// get the size of a window. the x, y properties are not used
RenderResult render_get_window_size(const RenderWindow *window, int *w, int *h)
    RENDER_NONULL(1);

RenderResult render_get_render_size(const Render *render, int *w, int *h)
    RENDER_NONULL(1);

// draw a texture to the screen
RenderResult render_draw_texture(
    const Render *render,
    const RenderRect *dst_rect,
    const RenderRect *src_rect,
    const RenderTexture *texture,
    RenderCoord *pivot,
    float angle) RENDER_NONULL(1, 2, 4);

RenderResult render_draw_rect(const Render *render, const RenderRect *rect)
    RENDER_NONULL(1, 2);
RenderResult
render_draw_rects(const Render *render, const RenderRect *rects, size_t n)
    RENDER_NONULL(1, 2);
RenderResult render_draw_text(
    const Render *r, const RenderText *text, const RenderRect *rect)
    RENDER_NONULL(1, 2, 3);
RenderResult render_draw_point(const Render *render, int x, int y)
    RENDER_NONULL(1);
RenderResult render_set_colour(const Render *render, RenderColour c)
    RENDER_NONULL(1);

// clear the screen of any drawings. should be called at the beginning of a
// frame to avoid the last frame staying in undrawn areas
RenderResult render_clear(const Render *render) RENDER_NONULL(1);

// submit render commands to be drawn. nothing is shown until this is called
void render_submit(const Render *render) RENDER_NONULL(1);

// should be called every frame to read mouse input and check for window close
RenderEvent render_poll_events(Render *render) RENDER_NONULL(1);

RENDER_WARN_RESULT RenderTexture *
render_create_texture(const Render *render, const char *texture_path)
    RENDER_NONULL(1, 2);
RenderTexture *
render_create_texture_from_surface(const Render *render, SDL_Surface *surface)
    RENDER_NONULL(1, 2);
RENDER_WARN_RESULT RenderTexture *
render_create_drawable_texture(const Render *render, int w, int h)
    RENDER_NONULL(1);
void render_destroy_texture(RenderTexture *texture) RENDER_NONULL(1);
RenderResult
render_set_texture_alpha(const RenderTexture *texture, uint8_t alpha)
    RENDER_NONULL(1);
RenderResult render_get_texture_size(const RenderTexture *t, int *w, int *h)
    RENDER_NONULL(1);
// set render to draw to a texture, pass NULL to draw to window
RenderResult
render_set_drawing_target(const Render *render, const RenderTexture *texture)
    RENDER_NONULL(1);

// font_path is a path to .ttf font
RENDER_WARN_RESULT RenderFont *
render_create_font(const Render *render, const char *font_path, uint8_t size)
    RENDER_NONULL(1, 2);
void render_destroy_font(RenderFont *font) RENDER_NONULL(1);

RENDER_WARN_RESULT RenderText *render_create_text(
    const Render *render,
    const RenderFont *font,
    const char *text,
    uint8_t r,
    uint8_t g,
    uint8_t b) RENDER_NONULL(1, 2, 3);

void render_destroy_text(RenderText *text);

RENDER_WARN_RESULT float render_text_get_aspect_ratio(const RenderText *text)
    RENDER_NONULL(1);

RenderResult render_get_cursor_pos(const Render *render, int *x, int *y)
    RENDER_NONULL(1);
RENDER_WARN_RESULT RenderCursorState
render_get_cursor_state(const Render *render) RENDER_NONULL(1);

// calculate a rectangle centered around x, y with dimensions w, h
RENDER_WARN_RESULT RenderRect
calculate_rect_centered(int32_t x, int32_t y, uint16_t w, uint16_t h);

// FIXME what am i doing
typedef struct SDL_Renderer SDL_Renderer;
SDL_Renderer *render_internal(const Render *render) RENDER_NONULL(1);
