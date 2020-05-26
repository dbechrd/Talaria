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

static void ta_asset_watcher_hotload_shader(const char *path)
{
    UNUSED(path);
}

static void ta_asset_watcher_hotload_texture(const char *path)
{
    UNUSED(path);
}

static int ta_asset_watch_handle_change(HANDLE hDirectory)
{
    DWORD bytesReturned = 0;
    WCHAR buffer[256] = { 0 };
    DWORD success = ReadDirectoryChangesW(
        hDirectory,
        buffer,
        sizeof(buffer),
        TRUE,
        FILE_NOTIFY_CHANGE_LAST_WRITE,
        &bytesReturned,
        NULL,
        NULL);

    if (success) {
        if (bytesReturned) {
            FILE_NOTIFY_INFORMATION *info = (FILE_NOTIFY_INFORMATION *)buffer;
            //printf("[ASSET_WATCHER] bytes_returned = %u\n", bytesReturned);
            for (;;) {
                const char *action_str = 0;
                switch (info->Action) {
                    case FILE_ACTION_ADDED           : action_str = "FILE_ACTION_ADDED"; break;
                    case FILE_ACTION_REMOVED         : action_str = "FILE_ACTION_REMOVED"; break;
                    case FILE_ACTION_MODIFIED        : action_str = "FILE_ACTION_MODIFIED"; break;
                    case FILE_ACTION_RENAMED_OLD_NAME: action_str = "FILE_ACTION_RENAMED_OLD_NAME"; break;
                    case FILE_ACTION_RENAMED_NEW_NAME: action_str = "FILE_ACTION_RENAMED_NEW_NAME"; break;
                }
                if (action_str) {
                    printf("[ASSET_WATCHER] %s %.*ls\n", action_str, info->FileNameLength, info->FileName);
                }
                if (!info->NextEntryOffset) {
                    break;
                }
                info += info->NextEntryOffset;
            }
        } else {
            // Nothing we can do about this.. always happens on the first call, annoyingly
            printf("[ASSET_WATCHER] WARNING: ReadDirectoryChangesW failed, buffer was not big enough.\n");
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

    DWORD dwWaitStatus;
    HANDLE hDirectory;
    TCHAR lpDrive[4];
    TCHAR lpFile[_MAX_FNAME];
    TCHAR lpExt[_MAX_EXT];

    _tsplitpath_s(args->lpDir, lpDrive, 4, NULL, 0, lpFile, _MAX_FNAME, lpExt, _MAX_EXT);

    lpDrive[2] = (TCHAR)'\\';
    lpDrive[3] = (TCHAR)'\0';

    // Watch the directory for file changes
    hDirectory = FindFirstChangeNotification(
        args->lpDir,                    // directory to watch
        TRUE,                           // watch the subtree
        FILE_NOTIFY_CHANGE_LAST_WRITE   // watch file writes
    );

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

    FindCloseChangeNotification(hDirectory);
    //thrd_exit(-1);
    return err;
}