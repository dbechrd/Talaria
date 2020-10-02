#pragma once

typedef enum ta_watcher_result {
    TA_WATCHER_SUCCESS                      = 0,
    TA_WATCHER_ERR_INVALID_HANDLE           = -1,
    TA_WATCHER_ERR_READ_DIRECTORY_CHANGES   = -2,
    TA_WATCHER_ERR_BUFFER_SIZE_OUT_OF_RANGE = -3,
} ta_watcher_result;

typedef struct ta_asset_change_record {
    char *path;     // relative name of files with detected changes
    u64 frame_num;  // frame # change was detected on (used to delay handling to allow file handle to close)
} ta_asset_change_record;

typedef struct ta_asset_watcher {
    const char *dir_path;                // directory path to watch for file changes
    ta_asset_change_record changes[16];  // unhandled changes buffer
} ta_asset_watcher;

void ta_asset_watcher_init(ta_asset_watcher *watcher, const char *directory, size_t directory_len);