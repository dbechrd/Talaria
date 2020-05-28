#pragma once

typedef enum ta_watcher_result {
    TA_WATCHER_SUCCESS                      = 0,
    TA_WATCHER_ERR_INVALID_HANDLE           = -1,
    TA_WATCHER_ERR_READ_DIRECTORY_CHANGES   = -2,
    TA_WATCHER_ERR_BUFFER_SIZE_OUT_OF_RANGE = -3,
} ta_watcher_result;

typedef struct ta_asset_watcher {
    const char *dir_path;     // directory path to watch for file changes
    char *changed_files[16];  // relative name of files with detected changes
} ta_asset_watcher;

void ta_asset_watcher_init(ta_asset_watcher *watcher);