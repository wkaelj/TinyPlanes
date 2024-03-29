#pragma once

/*
 * This defines an extension of the render
 * that can handle complex user input.
 * the base render can only deal with mouse input;
 * This defines functions to deal with
 * [ ] Keyboard keys held
 * [ ] Text input
 * [ ] Controller Input
 */

#include "render.h"

// enable the input functions for r
RenderResult input_init(Render *r);

// start ignoring key inputs and instead take text input into a buffer
// retrieve the buffer with input_get_input_text
RenderResult input_start_text_input(const Render *render);

void input_stop_text_input(const Render *render);

// returns NULL if input is not started
// - returns whatever has been typed since string input started
// - starting text input again will delete the string returned
RENDER_WARN_RESULT const char *input_get_input_text(const Render *render);

RENDER_WARN_RESULT bool
input_is_key_pressed(const Render *render, int key_code);

// useful for binding game controlls, starts listening for the next key press
RENDER_WARN_RESULT int input_get_next_key_code(const Render *render);
