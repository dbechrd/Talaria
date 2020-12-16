#include "ta_asset_watcher.h"
#include "tinycthread/source/tinycthread.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

static ta_watcher_result ta_asset_watcher_wait_changes(ta_asset_watcher *watcher, HANDLE handle)
{
    DWORD bytesReturned = 0;
    char *buffer[1024] = { 0 };
    DWORD success = ReadDirectoryChangesW(
        handle,
        buffer,
        sizeof(buffer),
        TRUE,
        FILE_NOTIFY_CHANGE_FILE_NAME
        //| FILE_NOTIFY_CHANGE_DIR_NAME
        //| FILE_NOTIFY_CHANGE_ATTRIBUTES
        //| FILE_NOTIFY_CHANGE_SIZE
        | FILE_NOTIFY_CHANGE_LAST_WRITE
        //| FILE_NOTIFY_CHANGE_LAST_ACCESS
        //| FILE_NOTIFY_CHANGE_CREATION
        //| FILE_NOTIFY_CHANGE_SECURITY
        ,
        &bytesReturned,
        NULL,
        NULL
    );
    if (!success) {
        DWORD err = GetLastError();
        printf("[ASSET_WATCHER] ERROR: ReadDirectoryChangesW failed with error code: %lu\n", err);
        return TA_WATCHER_ERR_READ_DIRECTORY_CHANGES;
    }

    if (bytesReturned == 0) {
        printf("[ASSET_WATCHER] WARNING: ReadDirectoryChangesW failed, buffer too small or too big.\n");
        return TA_WATCHER_ERR_BUFFER_SIZE_OUT_OF_RANGE;
    }

    //printf("[ASSET_WATCHER] bytes_returned = %u\n", bytesReturned);

    FILE_NOTIFY_INFORMATION *info = (FILE_NOTIFY_INFORMATION *)buffer;
    for (;;) {
#if 0
        const char *action_str = 0;
        switch (info->Action) {
            case FILE_ACTION_ADDED           : action_str = "File created     "; break;
            case FILE_ACTION_REMOVED         : action_str = "File removed     "; break;
            case FILE_ACTION_MODIFIED        : action_str = "File modified    "; break;
            case FILE_ACTION_RENAMED_OLD_NAME: action_str = "File renamed from"; break;
            case FILE_ACTION_RENAMED_NEW_NAME: action_str = "File renamed to  "; break;
        }

        // NOTE: This doesn't work properly because %.*s doesn't work properly for wide char strings?
        if (action_str) {
            printf("[ASSET_WATCHER] %s '%.*ls'\n", action_str, info->FileNameLength, info->FileName);
        } else {
            printf("[ASSET_WATCHER] UNKOWN (%u) '%.*ls'\n", info->Action, info->FileNameLength, info->FileName);
        }
#endif

        if (info->Action == FILE_ACTION_ADDED
            || info->Action == FILE_ACTION_MODIFIED
            || info->Action == FILE_ACTION_RENAMED_NEW_NAME)
        {
            DLB_ASSERT(info->FileNameLength);
            DLB_ASSERT(info->FileName);

            // If multi-bype size of locale is > 1, the file buffer below could overflow
            DLB_ASSERT(MB_CUR_MAX == 1);

            // NOTE: If not empty slot is found, we simply discard the change notification. I don't know how to resize
            // the buffer in a thread-safe way, and this isn't a vital thing to detect.
            bool found_slot = false;
            for (size_t i = 0; i < ARRAY_SIZE(watcher->changes); ++i) {
                if (watcher->changes[i].path == 0) {
                    found_slot = true;

                    bool is_file = false;
                    int file_len = info->FileNameLength / sizeof(wchar_t);
                    char *file = (char *)dlb_calloc(1, file_len + 1);
                    for (int j = 0; j < file_len; ++j) {
                        int result = wctomb(&file[j], info->FileName[j]);
                        if (result == -1) {
                            file[j] = '?';
                        } else if (file[j] == '\\') {
                            file[j] = '/';
                        }
                        if (file[j] == '.') {
                            is_file = true;
                        }
                    }

                    //size_t dir_len = strlen(watcher->dir_path);
                    //size_t path_len = dir_len + file_len + 1;
                    //char *path = dlb_calloc(1, path_len);
                    //snprintf(path, path_len, "%s%.*s", watcher->dir_path, (int)file_len, file);
                    //watcher->changed_files[i] = path;

                    if (is_file) {
                        watcher->changes[i].path = file;
                        watcher->changes[i].frame_num = tg_game.frame_num;
                    }
                    break;
                }
            }

            if (!found_slot) {
                printf("[ASSET_WATCHER] WARNING: buffer full, discarding change notification\n");
            }
        }

        if (!info->NextEntryOffset) {
            break;
        }
        info = (FILE_NOTIFY_INFORMATION *)((char *)info + info->NextEntryOffset);
    }
    return TA_WATCHER_SUCCESS;
}

static ta_watcher_result ta_open_directory(const char *path, HANDLE *handle)
{
    // Open directory handle
    HANDLE hnd = CreateFile(
        path,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );
    if (hnd == INVALID_HANDLE_VALUE) {
        return TA_WATCHER_ERR_INVALID_HANDLE;
    }
    *handle = hnd;
    return TA_WATCHER_SUCCESS;
}

static int ta_asset_watcher_watch(void *arg)
{
    ta_watcher_result err;
    ta_asset_watcher *watcher = (ta_asset_watcher *)arg;

    HANDLE handle;
    err = ta_open_directory(watcher->dir_path, &handle);

    // Watch the directory for file changes
    while (!err) {
        err = ta_asset_watcher_wait_changes(watcher, handle);
    }

    switch (err) {
        case TA_WATCHER_ERR_INVALID_HANDLE:
            printf("[ASSET_WATCHER] ERROR: Failed to open directory handle.\n");
            break;
        case TA_WATCHER_ERR_READ_DIRECTORY_CHANGES:
            printf("[ASSET_WATCHER] ERROR: Failed to read directory changes.\n");
            break;
        case TA_WATCHER_ERR_BUFFER_SIZE_OUT_OF_RANGE:
            printf("[ASSET_WATCHER] ERROR: Failed to populate change buffer due to size (out of range).\n");
            break;
        default:
            break;
    }

    return (int)err;
}

void ta_asset_watcher_init(ta_asset_watcher *watcher, const char *directory, size_t directory_len)
{
    DLB_ASSERT(directory);
    DLB_ASSERT(directory_len);
    watcher->dir_path = directory;
    DLB_ASSERT(watcher->dir_path[directory_len - 1] == '/');  // Directory must end with slash

    thrd_t asset_watcher_thread = { 0 };
    int result = thrd_create(&asset_watcher_thread, ta_asset_watcher_watch, watcher);
    UNUSED(result);
}