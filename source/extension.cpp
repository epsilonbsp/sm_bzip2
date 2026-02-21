// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 EpsilonBSP

#include "extension.h"

#include <stdio.h>
#include <string.h>
#include <vector>

extern "C" {
    #include "bzlib.h"
}

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

Bzip2 g_Bzip2; // Global singleton for extension's main interface

SMEXT_LINK(&g_Bzip2);

#define BZIP2_BUFFER_SIZE 65536

enum Bzip2_Error {
    BZIP2_OK                 = 0,
    BZIP2_ERR_SRC_OPEN       = 1, // failed to open source file
    BZIP2_ERR_DEST_OPEN      = 2, // failed to open destination file
    BZIP2_ERR_MEM            = 3, // memory allocation failure
    BZIP2_ERR_DATA           = 4, // data integrity (CRC) error
    BZIP2_ERR_DATA_MAGIC     = 5, // not a valid bzip2 stream
    BZIP2_ERR_IO             = 6, // I/O error during read/write
    BZIP2_ERR_UNEXPECTED_EOF = 7, // truncated bzip2 stream
    BZIP2_ERR_WRITE          = 8, // failed to write output file
    BZIP2_ERR_UNKNOWN        = 9, // other unexpected bzlib error
};

static Bzip2_Error MapBZError(int bz_error) {
    switch (bz_error) {
        case BZ_MEM_ERROR:        return BZIP2_ERR_MEM;
        case BZ_DATA_ERROR:       return BZIP2_ERR_DATA;
        case BZ_DATA_ERROR_MAGIC: return BZIP2_ERR_DATA_MAGIC;
        case BZ_IO_ERROR:         return BZIP2_ERR_IO;
        case BZ_UNEXPECTED_EOF:   return BZIP2_ERR_UNEXPECTED_EOF;
        default:                  return BZIP2_ERR_UNKNOWN;
    }
}

/**
 * native Bzip2_Error Bzip2_DecompressFile(const char[] src, const char[] dest);
 *
 * Decompresses a .bz2 file at src and writes the result to dest.
 * Returns BZIP2_OK on success, or a Bzip2_Error code on failure.
 */
static cell_t Native_Bzip2_DecompressFile(IPluginContext *p_context, const cell_t *params) {
    char *src, *dest;
    p_context->LocalToString(params[1], &src);
    p_context->LocalToString(params[2], &dest);

    FILE *file_in = fopen(src, "rb");

    if (!file_in) {
        return BZIP2_ERR_SRC_OPEN;
    }

    FILE *file_out = fopen(dest, "wb");

    if (!file_out) {
        fclose(file_in);

        return BZIP2_ERR_DEST_OPEN;
    }

    int bz_error;
    BZFILE *bz_file = BZ2_bzReadOpen(&bz_error, file_in, 0, 0, NULL, 0);

    if (bz_error != BZ_OK) {
        fclose(file_in);
        fclose(file_out);

        return MapBZError(bz_error);
    }

    char buf[BZIP2_BUFFER_SIZE];
    Bzip2_Error error_code = BZIP2_OK;

    while (bz_error == BZ_OK) {
        int bytes_read = BZ2_bzRead(&bz_error, bz_file, buf, sizeof(buf));

        if (bz_error != BZ_OK && bz_error != BZ_STREAM_END) {
            error_code = MapBZError(bz_error);

            break;
        }

        if (bytes_read > 0) {
            if (fwrite(buf, 1, bytes_read, file_out) != (size_t)bytes_read) {
                error_code = BZIP2_ERR_WRITE;

                break;
            }
        }
    }

    BZ2_bzReadClose(&bz_error, bz_file);
    fclose(file_in);
    fclose(file_out);

    if (error_code != BZIP2_OK) {
        remove(dest);
    }

    return (cell_t)error_code;
}

/**
 * native Bzip2_Error Bzip2_CompressFile(const char[] src, const char[] dest, int block_size = 9);
 *
 * Compresses a file at src and writes a .bz2 file to dest.
 * block_size controls compression level (1–9, higher = better compression).
 * Returns BZIP2_OK on success, or a Bzip2_Error code on failure.
 */
static cell_t Native_Bzip2_CompressFile(IPluginContext *p_context, const cell_t *params) {
    char *src, *dest;
    p_context->LocalToString(params[1], &src);
    p_context->LocalToString(params[2], &dest);

    int block_size = (int)params[3];

    if (block_size < 1 || block_size > 9) {
        p_context->ThrowNativeError("Invalid block_size %d: must be between 1 and 9", block_size);

        return 0;
    }

    FILE *file_in = fopen(src, "rb");

    if (!file_in) {
        return BZIP2_ERR_SRC_OPEN;
    }

    FILE *file_out = fopen(dest, "wb");

    if (!file_out) {
        fclose(file_in);

        return BZIP2_ERR_DEST_OPEN;
    }

    int bz_error;
    BZFILE *bz_file = BZ2_bzWriteOpen(&bz_error, file_out, block_size, 0, 30);

    if (bz_error != BZ_OK) {
        fclose(file_in);
        fclose(file_out);

        return MapBZError(bz_error);
    }

    char buf[BZIP2_BUFFER_SIZE];
    Bzip2_Error error_code = BZIP2_OK;

    while (!feof(file_in)) {
        size_t bytes_read = fread(buf, 1, sizeof(buf), file_in);

        if (bytes_read == 0) {
            if (ferror(file_in)) {
                error_code = BZIP2_ERR_IO;
            }

            break;
        }

        BZ2_bzWrite(&bz_error, bz_file, buf, (int)bytes_read);

        if (bz_error != BZ_OK) {
            error_code = MapBZError(bz_error);
            break;
        }
    }

    BZ2_bzWriteClose(&bz_error, bz_file, error_code != BZIP2_OK ? 1 : 0, NULL, NULL);
    fclose(file_in);
    fclose(file_out);

    if (error_code != BZIP2_OK) {
        remove(dest);
    }

    return (cell_t)error_code;
}

enum Bzip2_Task_Type {
    Bzip2_Task_Decompress,
    Bzip2_Task_Compress,
};

struct Bzip2_Task : public IThread {
    Bzip2_Task_Type      type;
    char                src[PLATFORM_MAX_PATH];
    char                dest[PLATFORM_MAX_PATH];
    int                 block_size; // compress only
    Bzip2_Error         result;
    cell_t              data;
    IChangeableForward* p_forward;

    Bzip2_Task(Bzip2_Task_Type type_, const char *src_, const char *dest_, int block_size_, cell_t data_, IChangeableForward *p_forward)
        : type(type_), block_size(block_size_), result(BZIP2_OK), data(data_), p_forward(p_forward)
    {
        strncpy(src, src_, sizeof(src) - 1);
        src[sizeof(src) - 1] = '\0';
        strncpy(dest, dest_, sizeof(dest) - 1);
        dest[sizeof(dest) - 1] = '\0';
    }

    void RunThread(IThreadHandle *p_handle) override {
        if (type == Bzip2_Task_Decompress) {
            result = RunDecompress();
        } else {
            result = RunCompress();
        }
    }

    void OnTerminate(IThreadHandle *p_handle, bool cancel) override {
        // Called on the worker thread — queue ourselves for main-thread dispatch.
        g_Bzip2.QueueCompletedTask(this);
    }

private:
    Bzip2_Error RunDecompress() {
        FILE *file_in = fopen(src, "rb");
        if (!file_in) return BZIP2_ERR_SRC_OPEN;

        FILE *file_out = fopen(dest, "wb");
        if (!file_out) { fclose(file_in); return BZIP2_ERR_DEST_OPEN; }

        int bz_error;
        BZFILE *bz_file = BZ2_bzReadOpen(&bz_error, file_in, 0, 0, NULL, 0);

        if (bz_error != BZ_OK) {
            fclose(file_in);
            fclose(file_out);

            return MapBZError(bz_error);
        }

        char buf[BZIP2_BUFFER_SIZE];
        Bzip2_Error error_code = BZIP2_OK;

        while (bz_error == BZ_OK) {
            int bytes_read = BZ2_bzRead(&bz_error, bz_file, buf, sizeof(buf));

            if (bz_error != BZ_OK && bz_error != BZ_STREAM_END) {
                error_code = MapBZError(bz_error);

                break;
            }

            if (bytes_read > 0) {
                if (fwrite(buf, 1, bytes_read, file_out) != (size_t)bytes_read) {
                    error_code = BZIP2_ERR_WRITE;

                    break;
                }
            }
        }

        BZ2_bzReadClose(&bz_error, bz_file);
        fclose(file_in);
        fclose(file_out);

        if (error_code != BZIP2_OK) remove(dest);

        return error_code;
    }

    Bzip2_Error RunCompress() {
        FILE *file_in = fopen(src, "rb");
        if (!file_in) return BZIP2_ERR_SRC_OPEN;

        FILE *file_out = fopen(dest, "wb");
        if (!file_out) { fclose(file_in); return BZIP2_ERR_DEST_OPEN; }

        int bz_error;
        BZFILE *bz_file = BZ2_bzWriteOpen(&bz_error, file_out, block_size, 0, 30);

        if (bz_error != BZ_OK) {
            fclose(file_in);
            fclose(file_out);

            return MapBZError(bz_error);
        }

        char buf[BZIP2_BUFFER_SIZE];
        Bzip2_Error error_code = BZIP2_OK;

        while (!feof(file_in)) {
            size_t bytes_read = fread(buf, 1, sizeof(buf), file_in);

            if (bytes_read == 0) {
                if (ferror(file_in)) error_code = BZIP2_ERR_IO;

                break;
            }

            BZ2_bzWrite(&bz_error, bz_file, buf, (int)bytes_read);

            if (bz_error != BZ_OK) {
                error_code = MapBZError(bz_error);

                break;
            }
        }

        BZ2_bzWriteClose(&bz_error, bz_file, error_code != BZIP2_OK ? 1 : 0, NULL, NULL);
        fclose(file_in);
        fclose(file_out);

        if (error_code != BZIP2_OK) remove(dest);

        return error_code;
    }
};

static cell_t Native_Bzip2_DecompressFileAsync(IPluginContext *p_context, const cell_t *params) {
    char *src, *dest;
    p_context->LocalToString(params[1], &src);
    p_context->LocalToString(params[2], &dest);

    IPluginFunction *pFunc = p_context->GetFunctionById((funcid_t)params[3]);

    if (!pFunc) {
        return p_context->ThrowNativeError("Invalid callback function");
    }

    cell_t data = params[4];

    IChangeableForward *p_forward = forwards->CreateForwardEx(
        NULL, ET_Ignore, 4, NULL,
        Param_Cell,               // Bzip2_Error
        Param_String,             // src
        Param_String,             // dest
        Param_Cell                // any data
    );

    if (!p_forward) {
        return p_context->ThrowNativeError("Failed to create forward");
    }

    p_forward->AddFunction(pFunc);

    Bzip2_Task *task = new Bzip2_Task(Bzip2_Task_Decompress, src, dest, 0, data, p_forward);
    threader->MakeThread(task, Thread_AutoRelease);

    return 0;
}

static cell_t Native_Bzip2_CompressFileAsync(IPluginContext *p_context, const cell_t *params) {
    char *src, *dest;
    p_context->LocalToString(params[1], &src);
    p_context->LocalToString(params[2], &dest);

    IPluginFunction *pFunc = p_context->GetFunctionById((funcid_t)params[3]);

    if (!pFunc) {
        return p_context->ThrowNativeError("Invalid callback function");
    }

    cell_t data = params[4];
    int block_size = (int)params[5];

    if (block_size < 1 || block_size > 9) {
        return p_context->ThrowNativeError("Invalid block_size %d: must be between 1 and 9", block_size);
    }

    IChangeableForward *p_forward = forwards->CreateForwardEx(
        NULL, ET_Ignore, 4, NULL,
        Param_Cell,               // Bzip2_Error
        Param_String,             // src
        Param_String,             // dest
        Param_Cell                // any data
    );

    if (!p_forward) {
        return p_context->ThrowNativeError("Failed to create forward");
    }

    p_forward->AddFunction(pFunc);

    Bzip2_Task *task = new Bzip2_Task(Bzip2_Task_Compress, src, dest, block_size, data, p_forward);
    threader->MakeThread(task, Thread_AutoRelease);

    return 0;
}

static void OnGameFrame(bool simulating) {
    g_Bzip2.ProcessCompletedTasks();
}

static const sp_nativeinfo_t g_Bzip2Natives[] = {
    {"Bzip2_DecompressFile",      Native_Bzip2_DecompressFile},
    {"Bzip2_CompressFile",        Native_Bzip2_CompressFile},
    {"Bzip2_DecompressFileAsync", Native_Bzip2_DecompressFileAsync},
    {"Bzip2_CompressFileAsync",   Native_Bzip2_CompressFileAsync},
    {NULL, NULL},
};

void Bzip2::QueueCompletedTask(Bzip2_Task *task) {
    m_mutex->Lock();
    m_completed_tasks.push_back(task);
    m_mutex->Unlock();
}

void Bzip2::ProcessCompletedTasks() {
    std::vector<Bzip2_Task *> tasks;

    m_mutex->Lock();
    tasks.swap(m_completed_tasks);
    m_mutex->Unlock();

    for (Bzip2_Task *task : tasks) {
        IChangeableForward *p_forward = task->p_forward;

        if (p_forward->GetFunctionCount() > 0) {
            p_forward->PushCell((cell_t)task->result);
            p_forward->PushString(task->src);
            p_forward->PushString(task->dest);
            p_forward->PushCell(task->data);
            p_forward->Execute(NULL);
        }

        forwards->ReleaseForward(p_forward);

        delete task;
    }
}

bool Bzip2::SDK_OnLoad(char *error, size_t maxlen, bool late) {
    m_mutex = threader->MakeMutex();

    if (!m_mutex) {
        snprintf(error, maxlen, "Failed to create mutex");

        return false;
    }

    smutils->AddGameFrameHook(OnGameFrame);

    return true;
}

void Bzip2::SDK_OnUnload() {
    smutils->RemoveGameFrameHook(OnGameFrame);

    // Drain any tasks that finished between the last frame and unload.
    ProcessCompletedTasks();

    m_mutex->DestroyThis();
    m_mutex = nullptr;
}

void Bzip2::SDK_OnAllLoaded() {
    sharesys->AddNatives(myself, g_Bzip2Natives);
}
