#include "ta_asset_watcher.h"
#include "tinycthread/source/tinycthread.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

typedef struct ta_asset_watcher_thread_args {
    LPCTSTR lpDir;
} ta_asset_watcher_thread_args;

int ta_asset_watcher_watch(ta_asset_watcher_thread_args *args);

void ta_asset_watcher_init(const char *data_path)
{
    thrd_t asset_watcher_thread = { 0 };
    ta_asset_watcher_thread_args *args = dlb_calloc(1, sizeof(*args));
    args->lpDir = data_path;
    int result = thrd_create(&asset_watcher_thread, ta_asset_watcher_watch, args);
}

static int ta_asset_watch_handle_change(HANDLE hDirectory)
{
    DWORD bytesReturned = 0;
    char *buffer[1024] = { 0 };
    DWORD success = ReadDirectoryChangesW(
        hDirectory,
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

    if (success) {
        if (bytesReturned) {
            FILE_NOTIFY_INFORMATION *info = (void *)buffer;
            printf("[ASSET_WATCHER] bytes_returned = %u\n", bytesReturned);
            for (;;) {
                const char *action_str = 0;
                switch (info->Action) {
                    case FILE_ACTION_ADDED           : action_str = "File created"; break;
                    case FILE_ACTION_REMOVED         : action_str = "File removed"; break;
                    case FILE_ACTION_MODIFIED        : action_str = "File modified"; break;
                    case FILE_ACTION_RENAMED_OLD_NAME: action_str = "File renamed from"; break;
                    case FILE_ACTION_RENAMED_NEW_NAME: action_str = "File renamed to"; break;
                }
                if (action_str) {
                    printf("[ASSET_WATCHER] %s %.*ls\n", action_str, info->FileNameLength, info->FileName);
                } else {
                    printf("[ASSET_WATCHER] UNKOWN (%u) %.*ls\n", info->Action, info->FileNameLength, info->FileName);
                }
                if (!info->NextEntryOffset) {
                    break;
                }
                info = (FILE_NOTIFY_INFORMATION *)((char *)info + info->NextEntryOffset);
            }
            printf("---------\n");
        } else {
            // Nothing we can do about this.. always happens on the first call, annoyingly
            printf("[ASSET_WATCHER] WARNING: ReadDirectoryChangesW failed, buffer too small or too big.\n");
        }
    } else {
        DWORD err = GetLastError();
        printf("ERROR: ReadDirectoryChangesW failed with error code: %u\n", err);
        return -1;
    }
    return 0;
}

int ta_asset_watcher_watch(ta_asset_watcher_thread_args *args)
{
    int err = 0;

    // Open directory handle
    HANDLE hDirectory = CreateFile(
        args->lpDir,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    // Watch the directory for file changes
    if (hDirectory != INVALID_HANDLE_VALUE) {
        for (;;) {
            DWORD handle_error = ta_asset_watch_handle_change(hDirectory);
            if (handle_error) {
                err = -1;
                break;
            }
        }
    } else {
        printf("ERROR: FindFirstChangeNotification function returned an invalid handle.\n");
        err = -2;
    }

    dlb_free(args);
    return err;
}