#include "render.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>

#include <assert.h>
#include <stdbool.h>
#include <malloc.h>

#define alloc(type) (malloc(sizeof(type)));

struct RenderWindow
{
    SDL_Window *sdl_window;
    Render *r; // reference to render created for the window
};

#define TEXT_BUFFER_SIZE 128

struct Render
{
    SDL_Renderer *sdl_render;
    RenderCursorState cursor_state;
    int cursor_x, cursor_y;
    int pixel_scale;
    RenderWindow *window;

    // input system data, maybe unused
    bool input_enabled;
    struct InputData
    {
        int keys_held[64]; // max keys on compact keyboard

        size_t text_len;
        char text_buffer[TEXT_BUFFER_SIZE]; // store text for text input

        // joystick data
    } input;
};

struct RenderTexture
{
    SDL_Texture *sdl_texture;
};

struct RenderFont
{
    TTF_Font *sdl_font;
};

struct RenderText
{
    SDL_Texture *sdl_texture;
    float aspect_ratio;
};

struct RenderButton
{
    uint8_t padding, borderWidth;
    RenderColour borderColour, backgroundColour;
    RenderText *buttonText;
    RenderRect position;
};

static bool render_initialized = false;
static unsigned render_count   = 0;
static unsigned window_count   = 0;

// helpers

static SDL_Rect convert_rect(const RenderRect *rect, size_t pixel_scale)
{
    return (SDL_Rect){
        .x = rect->x * pixel_scale,
        .y = rect->y * pixel_scale,
        .w = rect->w * pixel_scale,
        .h = rect->h * pixel_scale,
    };
}

static size_t calculate_pixel_scale(const RenderWindow *w, const Render *r)
{
    // find window pixel scale
    int window_width;
    int render_width;
    SDL_GetWindowSize(w->sdl_window, &window_width, NULL);
    SDL_GetRendererOutputSize(r->sdl_render, &render_width, NULL);
    return render_width / window_width;
}

RenderResult render_init()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    {
        printf("Failed to init SDL, ERR: %s\n", SDL_GetError());
        SDL_Quit();
        return RENDER_FAILURE;
    }
    int want_flags = IMG_INIT_PNG | IMG_INIT_JPG;

    int init_flags = IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
    if ((init_flags & want_flags) != want_flags)
    {
        printf("Failed to init SDL_IMG, ERR: %s\n", SDL_GetError());
        SDL_Quit();
        return RENDER_FAILURE;
    }
    if (TTF_Init() != 0)
    {
        printf("Failed to init SDL_TTF, ERR: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return RENDER_FAILURE;
    }

    render_initialized = true;
    return RENDER_SUCCESS;
}

void render_quit()
{

    if (render_count)
    {
        printf(
            "There are %u renders that have not been distroyed. The render "
            "should not be quit before all renders and windows have been "
            "destroyed.\n",
            render_count);
    }
    if (window_count)
    {
        printf(
            "There are %u windows that have not been distroyed. The render "
            "should not be quit before all renders and windows have been "
            "destroyed.\n",
            window_count);
    }
    render_initialized = false;
    TTF_Quit();
    IMG_Quit();
    SDL_TLSCleanup();
    SDL_Quit();
}

bool render_is_initialized() { return render_initialized; }

RenderWindow *render_create_window(const char *title, int w, int h)
{

    if (render_is_initialized() == false)
        return NULL;
    RenderWindow *window = alloc(RenderWindow);
    assert(window);
    window->sdl_window = SDL_CreateWindow(
        title, 0, 0, w, h, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

    if (window->sdl_window)
        window_count++;
    window->r = NULL;
    return window;
}

Render *render_create_render(RenderWindow *window)
{
    Render *render = alloc(Render);
    assert(render);
    render->sdl_render = SDL_CreateRenderer(
        window->sdl_window,
        0,
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (render->sdl_render == NULL)
    {
        free(render);
        return NULL;
    }

    SDL_GetMouseState(&render->cursor_x, &render->cursor_y);

    render_count++;

    render->pixel_scale = calculate_pixel_scale(window, render);

    // if the pixel scale is bigger then 1 resize the window so it appears at
    // normal size
    if (render->pixel_scale > 1)
    {
        int w, h;
        render_get_window_size(window, &w, &h);
        SDL_SetWindowSize(
            window->sdl_window,
            w * render->pixel_scale,
            h * render->pixel_scale);
    }

    window->r      = render;
    render->window = window;

    render->input_enabled = false;
    render->cursor_state  = RENDER_CURSOR_UP;

    return render;
}

void render_destroy_render(Render *render)
{
    SDL_DestroyRenderer(render->sdl_render);
    render->window->r = NULL;
    free(render);
    render_count--;
}

void render_destroy_window(RenderWindow *window)
{
    if (window->r)
    {
        printf("Cannot destroy window, as a render relies on it");
        return;
    }

    SDL_DestroyWindow(window->sdl_window);
    free(window);
    window_count--;
}

RenderResult render_get_window_size(const RenderWindow *window, int *w, int *h)
{
    SDL_GetWindowSize(window->sdl_window, w, h);
    return RENDER_SUCCESS;
}

RenderResult render_get_render_size(const Render *render, int *w, int *h)
{
    render_get_window_size(render->window, w, h);
    return RENDER_SUCCESS;
}

RenderResult render_draw_texture(
    const Render *render,
    const RenderRect *dst_rect,
    const RenderRect *src_rect,
    const RenderTexture *texture,
    RenderCoord *pivot,
    float angle)
{
    assert(texture->sdl_texture);
    SDL_Rect dest = convert_rect(dst_rect, render->pixel_scale);

    return SDL_RenderCopyEx(
               render->sdl_render,
               texture->sdl_texture,
               (SDL_Rect *)src_rect,
               &dest,
               angle * (180.f / M_PI),
               (SDL_Point *)pivot,
               SDL_FLIP_NONE) == 0
               ? RENDER_SUCCESS
               : RENDER_FAILURE;
}

RenderResult render_set_colour(const Render *render, RenderColour c)
{
    return SDL_SetRenderDrawColor(render->sdl_render, c.r, c.g, c.b, c.a) == 0
               ? RENDER_SUCCESS
               : RENDER_FAILURE;
}

RenderResult render_draw_rect(const Render *render, const RenderRect *rect)
{
    SDL_Rect sdl_rect = convert_rect(rect, render->pixel_scale);
    return SDL_RenderFillRect(render->sdl_render, &sdl_rect) == 0
               ? RENDER_SUCCESS
               : RENDER_FAILURE;
}

RenderResult render_draw_point(const Render *render, int x, int y)
{
    return SDL_RenderDrawPoint(render->sdl_render, x, y) == 0 ? RENDER_SUCCESS
                                                              : RENDER_FAILURE;
}

RenderResult
render_draw_rects(const Render *render, const RenderRect *rects, size_t n)
{
    SDL_Rect sdl_rects[n];
    for (size_t i = 0; i < n; i++)
        sdl_rects[i] = convert_rect(&rects[i], render->pixel_scale);

    return SDL_RenderFillRects(render->sdl_render, sdl_rects, n);
}

RenderResult render_draw_text(
    const Render *r, const RenderText *text, const RenderRect *rect)
{
    SDL_Rect sdl_rect = convert_rect(rect, r->pixel_scale);
    return SDL_RenderCopy(r->sdl_render, text->sdl_texture, NULL, &sdl_rect) ==
                   0
               ? RENDER_SUCCESS
               : RENDER_FAILURE;
}

RenderResult render_clear(const Render *render)
{
    return SDL_RenderClear(render->sdl_render) == 0 ? RENDER_SUCCESS
                                                    : RENDER_FAILURE;
}

void render_submit(const Render *render)
{
    SDL_RenderPresent(render->sdl_render);
}

RenderEvent render_poll_events(Render *render)
{

    // update the cursor state
    if (render->cursor_state == RENDER_CURSOR_PRESSED)
        render->cursor_state = RENDER_CURSOR_DOWN;

    if (render->cursor_state == RENDER_CURSOR_RELEASED)
        render->cursor_state = RENDER_CURSOR_UP;

    RenderEvent ret = RENDER_EVENT_NONE;

    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        size_t input_str_len = 0; // for text input case
        switch (e.type)
        {
        case SDL_WINDOWEVENT:
            switch (e.window.event)
            {
            case SDL_WINDOWEVENT_RESIZED:
                render->pixel_scale =
                    calculate_pixel_scale(render->window, render);
                // set return if higher priotety
                if (RENDER_EVENT_WINDOW_RESIZE > ret)
                    ret = RENDER_EVENT_WINDOW_RESIZE;
                break;
            case SDL_WINDOWEVENT_DISPLAY_CHANGED:
                render->pixel_scale =
                    calculate_pixel_scale(render->window, render);
                break;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            render->cursor_state = RENDER_CURSOR_PRESSED;
            break;
        case SDL_MOUSEBUTTONUP:
            render->cursor_state = RENDER_CURSOR_RELEASED;
            break;
        case SDL_MOUSEMOTION:
            render->cursor_x = e.motion.x;
            render->cursor_y = e.motion.y;
            break;
        case SDL_TEXTINPUT:
            if (render->input_enabled == false)
                break;
            input_str_len = strnlen(e.text.text, 32);
            if ((input_str_len + render->input.text_len) > TEXT_BUFFER_SIZE)
            {
                printf("String input buffer overflow, discarding input");
                printf(
                    " strlen input = %lu, strlen buffer = %lu\n",
                    input_str_len,
                    render->input.text_len);
                break;
            }
            strncpy(
                &render->input.text_buffer[render->input.text_len],
                e.text.text,
                TEXT_BUFFER_SIZE - render->input.text_len);
            render->input.text_len += strlen(e.text.text);
            break;
        case SDL_QUIT: ret = RENDER_EVENT_QUIT; break;
        }
    }
    return ret;
}

RenderTexture *
render_create_texture(const Render *render, const char *texture_path)
{
    RenderTexture *t = alloc(RenderTexture);
    assert(t);

    SDL_Surface *s = IMG_Load(texture_path);

    t->sdl_texture = SDL_CreateTextureFromSurface(render->sdl_render, s);
    SDL_SetTextureScaleMode(t->sdl_texture, SDL_ScaleModeBest);

    SDL_FreeSurface(s);
    if (t->sdl_texture == NULL)
    {
        free(t);
        return NULL;
    }
    else
        return t;
}

RenderTexture *
render_create_texture_from_surface(const Render *render, SDL_Surface *surface)
{
    RenderTexture *t = alloc(RenderTexture);
    assert(t);
    t->sdl_texture = SDL_CreateTextureFromSurface(render->sdl_render, surface);
    if (t->sdl_texture == NULL)
    {
        free(t);
        printf("Error creating texture from surface %s\n", SDL_GetError());
        return NULL;
    }
    return t;
}

RenderTexture *
render_create_drawable_texture(const Render *render, int w, int h)
{
    RenderTexture *t = alloc(RenderTexture);
    assert(t);
    t->sdl_texture = SDL_CreateTexture(
        render->sdl_render,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        w,
        h);
    if (t->sdl_texture == NULL)
    {
        free(t);
        return NULL;
    }

    return t;
}
void render_destroy_texture(RenderTexture *texture)
{
    SDL_DestroyTexture(texture->sdl_texture);
    free(texture);
}

RenderResult
render_set_texture_alpha(const RenderTexture *texture, uint8_t alpha)
{
    return SDL_SetTextureAlphaMod(texture->sdl_texture, alpha) == 0
               ? RENDER_SUCCESS
               : RENDER_FAILURE;
}

RenderResult render_get_texture_size(const RenderTexture *t, int *w, int *h)
{
    if (SDL_QueryTexture(t->sdl_texture, NULL, NULL, w, h))
        return RENDER_FAILURE;

    return RENDER_SUCCESS;
}

RenderResult
render_set_drawing_target(const Render *render, const RenderTexture *texture)
{
    return SDL_SetRenderTarget(
               render->sdl_render, texture ? texture->sdl_texture : NULL) == 0
               ? RENDER_SUCCESS
               : RENDER_FAILURE;
}

RenderFont *render_create_font(
    [[maybe_unused]] const Render *render, const char *font_path, uint8_t size)
{
    RenderFont *f = alloc(RenderFont);
    assert(f);
    f->sdl_font = TTF_OpenFont(font_path, size);
    if (f->sdl_font == NULL)
    {
        free(f);
        return NULL;
    }
    return f;
}

void render_destroy_font(RenderFont *font)
{
    TTF_CloseFont(font->sdl_font);
    free(font);
}

RenderText *render_create_text(
    const Render *render,
    const RenderFont *font,
    const char *text,
    uint8_t r,
    uint8_t g,
    uint8_t b)
{
    RenderText *t = alloc(RenderText);
    assert(t);
    SDL_Color colour = {r, g, b, 255};

    SDL_Surface *s = TTF_RenderText_Solid(font->sdl_font, text, colour);
    if (s == NULL)
    {
        SDL_FreeSurface(s);
        free(t);
        return NULL;
    }
    t->sdl_texture = SDL_CreateTextureFromSurface(render->sdl_render, s);
    if (t->sdl_texture == NULL)
    {
        SDL_DestroyTexture(t->sdl_texture);
        SDL_FreeSurface(s);
        free(t);
        return NULL;
    }

    SDL_FreeSurface(s);

    // find aspect ratio
    int w, h;
    TTF_SizeText(font->sdl_font, text, &w, &h);
    t->aspect_ratio = (float)w / (float)h;

    return t;
}

void render_destroy_text(RenderText *text)
{
    SDL_DestroyTexture(text->sdl_texture);
    free(text);
}

float render_text_get_aspect_ratio(const RenderText *text)
{
    return text->aspect_ratio;
}

RenderResult render_get_cursor_pos(const Render *render, int *x, int *y)
{
    if (!((x || y)))
        return RENDER_FAILURE;
    if (x)
        *x = render->cursor_x;
    if (y)
        *y = render->cursor_y;
    return RENDER_SUCCESS;
}

RenderCursorState render_get_cursor_state(const Render *render)
{
    return render->cursor_state;
}

// button code
RenderButton *render_create_button(
    const Render *render,
    const RenderFont *font,
    const char *text,
    const RenderRect *position,
    const RenderColour background,
    const RenderColour border,
    const uint8_t border_width,
    const uint8_t padding)
{
    RenderButton *button = alloc(RenderButton);
    assert(button);

    RenderText *textObj = render_create_text(render, font, text, 255, 255, 255);
    if (textObj == NULL)
    {
        free(button);
        return NULL;
    }

    *button = (RenderButton){
        .padding          = padding,
        .borderWidth      = border_width,
        .borderColour     = border,
        .backgroundColour = background,
        .position         = *position,
        .buttonText       = textObj,
    };

    // set button height for text if no specified
    if (position->h == 0)
        button->position.h =
            position->w * render_text_get_aspect_ratio(textObj);

    return button;
}

RenderRect calculate_rect_centered(int32_t x, int32_t y, uint16_t w, uint16_t h)
{
    return (RenderRect){.x = x - w / 2, .y = y - h / 2, .w = w, .h = h};
}

bool render_button_hovered(const Render *render, RenderButton *button)
{
    int mouseX, mouseY;
    render_get_cursor_pos(render, &mouseX, &mouseY);

    const RenderRect *rect = &button->position;
    RenderRect buttonShape = calculate_rect_centered(
        rect->x,
        rect->y,
        rect->w + button->padding * 2,
        rect->h + button->padding * 2);

    bool withinH =
        mouseY > buttonShape.y && mouseY < (buttonShape.y + buttonShape.h);
    bool withinX =
        mouseX > buttonShape.x && mouseX < (buttonShape.x + buttonShape.w);

    return withinH && withinX;
}

bool render_button_clicked(const Render *render, RenderButton *button)
{

    return render_button_hovered(render, button) &&
           render_get_cursor_state(render) == RENDER_CURSOR_PRESSED;
}

void render_button_set_pos(RenderButton *button, uint16_t x, uint16_t y)
{
    button->position.x = x;
    button->position.y = y;
}

void render_button_draw(const Render *render, RenderButton *button)
{
    // check whether text will be high limited or width limited
    float text_aspect = render_text_get_aspect_ratio(button->buttonText);
    int text_width    = button->position.w;
    if ((float)button->position.w / (float)button->position.h >= text_aspect)
        text_width = button->position.h * text_aspect;

    RenderRect textRect = calculate_rect_centered(
        button->position.x,
        button->position.y,
        text_width,
        text_width / text_aspect);
    RenderRect outerRect = calculate_rect_centered(
        button->position.x,
        button->position.y,
        button->position.w + button->padding * 2,
        button->position.h + button->padding * 2);

    // draw border
    RenderRect borderRect = calculate_rect_centered(
        button->position.x,
        button->position.y,
        outerRect.w + button->borderWidth * 2,
        outerRect.h + button->borderWidth * 2);
    render_set_colour(render, button->borderColour);
    render_draw_rect(render, &borderRect);

    // draw background
    bool hovered               = render_button_hovered(render, button);
    int diff                   = -20;
    RenderColour hoveredColour = {
        button->backgroundColour.r + diff,
        button->backgroundColour.g + diff,
        button->backgroundColour.b + diff,
        button->backgroundColour.a,
    };
    render_set_colour(
        render, hovered ? hoveredColour : button->backgroundColour);
    render_draw_rect(render, &outerRect);

    // draw text
    render_draw_text(render, button->buttonText, &textRect);
}

void render_destroy_button(RenderButton *b)
{
    render_destroy_text(b->buttonText);
    free(b);
}

// INPUT EXTENSION
#define INPUT_ENABLED(render, ret)            \
    do                                        \
        if ((render)->input_enabled == false) \
            return (ret);                     \
    while (0)

RenderResult input_init(Render *r)
{
    if (r == NULL)
        return RENDER_FAILURE;
    if (r->input_enabled == true)
        return RENDER_FAILURE;
    if (r->sdl_render == NULL)
        return RENDER_FAILURE;
    r->input_enabled = true;
    r->input         = (struct InputData){};

    return RENDER_SUCCESS;
}

RenderResult input_start_text_input(const Render *render)
{
    assert(render);
    INPUT_ENABLED(render, RENDER_FAILURE);
    // start listening for text entry if not already
    if (SDL_IsTextInputActive() == SDL_FALSE)
    {
        SDL_StartTextInput();
        return RENDER_FAILURE;
    }

    // reset text buffer
    ((Render *)render)->input.text_len       = 0;
    ((Render *)render)->input.text_buffer[0] = '\0';

    return RENDER_FAILURE;
}

const char *input_get_input_text([[maybe_unused]] const Render *render)
{
    assert(render);
    INPUT_ENABLED(render, NULL);

    return render->input.text_buffer;
}

void input_stop_text_input([[maybe_unused]] const Render *render)
{
    SDL_StopTextInput();
}

bool input_is_key_pressed([[maybe_unused]] const Render *render, int key_code)
{
    // get list of pressed keys
    int num_keys;
    const uint8_t *key_data = SDL_GetKeyboardState(&num_keys);
    return key_data[key_code];
}

int input_get_next_key_code([[maybe_unused]] const Render *render)
{
    // return the next keycode types

    return 0;
}

SDL_Renderer *render_internal(const Render *render)
{
    return render->sdl_render;
}
