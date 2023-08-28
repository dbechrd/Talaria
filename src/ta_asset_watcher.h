#pragma once
#include "dlb/dlb_types.h"
#include "SDL/SDL_thread.h"
#include "SDL/SDL_mutex.h"

typedef enum ta_watcher_result {
    TA_WATCHER_SUCCESS                      = 0,
    TA_WATCHER_ERR_INVALID_HANDLE           = -1,
    TA_WATCHER_ERR_READ_DIRECTORY_CHANGES   = -2,
    TA_WATCHER_ERR_BUFFER_SIZE_OUT_OF_RANGE = -3,
} ta_watcher_result;

typedef struct ta_asset_change_record {
    char *path;             // relative name of files with detected changes
    double changed_at_ms;   // elapsed_ms when was detected (used to delay handling to allow file handle to close)
} ta_asset_change_record;

typedef struct ta_asset_watcher {
    SDL_Thread *thread;                 // asset watcher thread
    SDL_mutex *mutex;                   // mutex to be used for all access to this data structure
    bool signal_exit;                   // if true, main thread is requesting asset watcher to clean up for exit

    // Protected by mutex, *must* lock before accessing this buffer
    ta_asset_change_record *changes;    // unhandled changes buffer

    // NOTE: This is not protected by the mutex, it should only be set once before the thread is created
    const char *dir_path;               // directory path to watch for file changes
} ta_asset_watcher;

void ta_asset_watcher_start(ta_asset_watcher *watcher, const char *directory, size_t directory_len);
void ta_asset_watcher_stop(ta_asset_watcher *watcher);