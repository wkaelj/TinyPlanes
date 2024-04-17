#pragma once

#include <types.h>

typedef enum UI_AnchorPos
{
    UI_ANCHOR_TOP_LEFT,
    UI_ANCHOR_TOP_RIGHT,
    UI_ANCHOR_BOTTOM_LEFT,
    UI_ANCHOR_BOTTOM_RIGHT,
} UI_AnchorPos;

// used to anchor an element to a corner of the screen
struct UI_Anchor
{
    UI_AnchorPos position;
    vec2 offset;
};

// ui element types
struct UI_TextBox;
struct UI_TextInputBox;
struct UI_Image;

typedef struct UI_Element UI_Element;
