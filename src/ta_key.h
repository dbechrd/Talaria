#pragma once
#include "dlb/dlb_types.h"
#include "SDL/SDL_keyboard.h"

struct ta_event;
struct ta_keybind;

enum {
    SDL_SCANCODE_MOUSE_LEFT = SDL_NUM_SCANCODES + 1,
    SDL_SCANCODE_MOUSE_RIGHT,
    SDL_SCANCODE_MOUSE_MIDDLE,
    SDL_SCANCODE_MOUSE_X1,
    SDL_SCANCODE_MOUSE_X2,
    TA_KEY_COUNT
};

typedef s32 ta_key;

typedef struct ta_key_state {
    u8     down;                    // button is currently down
    u8     changed;                 // state changed since last frame
    double last_change_ms;          // time of last state change in milliseconds
    const struct ta_keybind *lock;  // pointer to keybind currently consuming this key, if any
} ta_key_state;

// Reset key state changed flags
void ta_key_reset_frame   ();

// Query key states
bool ta_key_down          (ta_key key);
bool ta_key_pressed       (ta_key key);
bool ta_key_released      (ta_key key);

// Mutexes for keybind handling
bool ta_key_available     (const struct ta_keybind *keybind, ta_key key);
void ta_key_lock          (const struct ta_keybind *keybind, ta_key key);
void ta_key_unlock        (const struct ta_keybind *keybind, ta_key key);
void ta_key_unlock_all    ();

// Handle key events
void ta_key_event         (struct ta_event *event);