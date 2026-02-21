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

#define BZ2_BUFFER_SIZE 65536

enum BZ2Error {
    BZ2_OK                 = 0,
    BZ2_ERR_SRC_OPEN       = 1, // failed to open source file
    BZ2_ERR_DEST_OPEN      = 2, // failed to open destination file
    BZ2_ERR_MEM            = 3, // memory allocation failure
    BZ2_ERR_DATA           = 4, // data integrity (CRC) error
    BZ2_ERR_DATA_MAGIC     = 5, // not a valid bzip2 stream
    BZ2_ERR_IO             = 6, // I/O error during read/write
    BZ2_ERR_UNEXPECTED_EOF = 7, // truncated bzip2 stream
    BZ2_ERR_WRITE          = 8, // failed to write output file
    BZ2_ERR_UNKNOWN        = 9, // other unexpected bzlib error
};

static BZ2Error MapBZError(int bzError) {
    switch (bzError) {
        case BZ_MEM_ERROR:        return BZ2_ERR_MEM;
        case BZ_DATA_ERROR:       return BZ2_ERR_DATA;
        case BZ_DATA_ERROR_MAGIC: return BZ2_ERR_DATA_MAGIC;
        case BZ_IO_ERROR:         return BZ2_ERR_IO;
        case BZ_UNEXPECTED_EOF:   return BZ2_ERR_UNEXPECTED_EOF;
        default:                  return BZ2_ERR_UNKNOWN;
    }
}

/**
 * native BZ2Error BZ2_DecompressFile(const char[] src, const char[] dest);
 *
 * Decompresses a .bz2 file at src and writes the result to dest.
 * Returns BZ2_OK on success, or a BZ2Error code on failure.
 */
static cell_t Native_BZ2_DecompressFile(IPluginContext *pContext, const cell_t *params) {
    char *src, *dest;
    pContext->LocalToString(params[1], &src);
    pContext->LocalToString(params[2], &dest);

    FILE *inFile = fopen(src, "rb");

    if (!inFile) {
        return BZ2_ERR_SRC_OPEN;
    }

    FILE *outFile = fopen(dest, "wb");

    if (!outFile) {
        fclose(inFile);

        return BZ2_ERR_DEST_OPEN;
    }

    int bzError;
    BZFILE *bzFile = BZ2_bzReadOpen(&bzError, inFile, 0, 0, NULL, 0);

    if (bzError != BZ_OK) {
        fclose(inFile);
        fclose(outFile);

        return MapBZError(bzError);
    }

    char buf[BZ2_BUFFER_SIZE];
    BZ2Error errorCode = BZ2_OK;

    while (bzError == BZ_OK) {
        int bytesRead = BZ2_bzRead(&bzError, bzFile, buf, sizeof(buf));

        if (bzError != BZ_OK && bzError != BZ_STREAM_END) {
            errorCode = MapBZError(bzError);

            break;
        }

        if (bytesRead > 0) {
            if (fwrite(buf, 1, bytesRead, outFile) != (size_t)bytesRead) {
                errorCode = BZ2_ERR_WRITE;

                break;
            }
        }
    }

    BZ2_bzReadClose(&bzError, bzFile);
    fclose(inFile);
    fclose(outFile);

    if (errorCode != BZ2_OK) {
        remove(dest);
    }

    return (cell_t)errorCode;
}

/**
 * native BZ2Error BZ2_CompressFile(const char[] src, const char[] dest, int blockSize = 9);
 *
 * Compresses a file at src and writes a .bz2 file to dest.
 * blockSize controls compression level (1–9, higher = better compression).
 * Returns BZ2_OK on success, or a BZ2Error code on failure.
 */
static cell_t Native_BZ2_CompressFile(IPluginContext *pContext, const cell_t *params) {
    char *src, *dest;
    pContext->LocalToString(params[1], &src);
    pContext->LocalToString(params[2], &dest);

    int blockSize = (int)params[3];

    if (blockSize < 1 || blockSize > 9) {
        pContext->ThrowNativeError("Invalid blockSize %d: must be between 1 and 9", blockSize);

        return 0;
    }

    FILE *inFile = fopen(src, "rb");

    if (!inFile) {
        return BZ2_ERR_SRC_OPEN;
    }

    FILE *outFile = fopen(dest, "wb");

    if (!outFile) {
        fclose(inFile);

        return BZ2_ERR_DEST_OPEN;
    }

    int bzError;
    BZFILE *bzFile = BZ2_bzWriteOpen(&bzError, outFile, blockSize, 0, 30);

    if (bzError != BZ_OK) {
        fclose(inFile);
        fclose(outFile);

        return MapBZError(bzError);
    }

    char buf[BZ2_BUFFER_SIZE];
    BZ2Error errorCode = BZ2_OK;

    while (!feof(inFile)) {
        size_t bytesRead = fread(buf, 1, sizeof(buf), inFile);

        if (bytesRead == 0) {
            if (ferror(inFile)) {
                errorCode = BZ2_ERR_IO;
            }

            break;
        }

        BZ2_bzWrite(&bzError, bzFile, buf, (int)bytesRead);

        if (bzError != BZ_OK) {
            errorCode = MapBZError(bzError);
            break;
        }
    }

    BZ2_bzWriteClose(&bzError, bzFile, errorCode != BZ2_OK ? 1 : 0, NULL, NULL);
    fclose(inFile);
    fclose(outFile);

    if (errorCode != BZ2_OK) {
        remove(dest);
    }

    return (cell_t)errorCode;
}

enum BZ2TaskType {
    BZ2Task_Decompress,
    BZ2Task_Compress,
};

struct BZ2Task : public IThread {
    BZ2TaskType     type;
    char            src[PLATFORM_MAX_PATH];
    char            dest[PLATFORM_MAX_PATH];
    int             blockSize; // compress only
    BZ2Error        result;
    cell_t          data;
    IChangeableForward *pForward;

    BZ2Task(BZ2TaskType type_, const char *src_, const char *dest_, int blockSize_, cell_t data_, IChangeableForward *pForward_)
        : type(type_), blockSize(blockSize_), result(BZ2_OK), data(data_), pForward(pForward_)
    {
        strncpy(src, src_, sizeof(src) - 1);
        src[sizeof(src) - 1] = '\0';
        strncpy(dest, dest_, sizeof(dest) - 1);
        dest[sizeof(dest) - 1] = '\0';
    }

    void RunThread(IThreadHandle *pHandle) override {
        if (type == BZ2Task_Decompress) {
            result = RunDecompress();
        } else {
            result = RunCompress();
        }
    }

    void OnTerminate(IThreadHandle *pHandle, bool cancel) override {
        // Called on the worker thread — queue ourselves for main-thread dispatch.
        g_Bzip2.QueueCompletedTask(this);
    }

private:
    BZ2Error RunDecompress() {
        FILE *inFile = fopen(src, "rb");
        if (!inFile) return BZ2_ERR_SRC_OPEN;

        FILE *outFile = fopen(dest, "wb");
        if (!outFile) { fclose(inFile); return BZ2_ERR_DEST_OPEN; }

        int bzError;
        BZFILE *bzFile = BZ2_bzReadOpen(&bzError, inFile, 0, 0, NULL, 0);

        if (bzError != BZ_OK) {
            fclose(inFile);
            fclose(outFile);

            return MapBZError(bzError);
        }

        char buf[BZ2_BUFFER_SIZE];
        BZ2Error errorCode = BZ2_OK;

        while (bzError == BZ_OK) {
            int bytesRead = BZ2_bzRead(&bzError, bzFile, buf, sizeof(buf));

            if (bzError != BZ_OK && bzError != BZ_STREAM_END) {
                errorCode = MapBZError(bzError);

                break;
            }

            if (bytesRead > 0) {
                if (fwrite(buf, 1, bytesRead, outFile) != (size_t)bytesRead) {
                    errorCode = BZ2_ERR_WRITE;

                    break;
                }
            }
        }

        BZ2_bzReadClose(&bzError, bzFile);
        fclose(inFile);
        fclose(outFile);

        if (errorCode != BZ2_OK) remove(dest);

        return errorCode;
    }

    BZ2Error RunCompress() {
        FILE *inFile = fopen(src, "rb");
        if (!inFile) return BZ2_ERR_SRC_OPEN;

        FILE *outFile = fopen(dest, "wb");
        if (!outFile) { fclose(inFile); return BZ2_ERR_DEST_OPEN; }

        int bzError;
        BZFILE *bzFile = BZ2_bzWriteOpen(&bzError, outFile, blockSize, 0, 30);

        if (bzError != BZ_OK) {
            fclose(inFile);
            fclose(outFile);

            return MapBZError(bzError);
        }

        char buf[BZ2_BUFFER_SIZE];
        BZ2Error errorCode = BZ2_OK;

        while (!feof(inFile)) {
            size_t bytesRead = fread(buf, 1, sizeof(buf), inFile);

            if (bytesRead == 0) {
                if (ferror(inFile)) errorCode = BZ2_ERR_IO;

                break;
            }

            BZ2_bzWrite(&bzError, bzFile, buf, (int)bytesRead);

            if (bzError != BZ_OK) {
                errorCode = MapBZError(bzError);

                break;
            }
        }

        BZ2_bzWriteClose(&bzError, bzFile, errorCode != BZ2_OK ? 1 : 0, NULL, NULL);
        fclose(inFile);
        fclose(outFile);

        if (errorCode != BZ2_OK) remove(dest);

        return errorCode;
    }
};

static cell_t Native_BZ2_DecompressFileAsync(IPluginContext *pContext, const cell_t *params) {
    char *src, *dest;
    pContext->LocalToString(params[1], &src);
    pContext->LocalToString(params[2], &dest);

    IPluginFunction *pFunc = pContext->GetFunctionById((funcid_t)params[3]);

    if (!pFunc) {
        return pContext->ThrowNativeError("Invalid callback function");
    }

    cell_t data = params[4];

    IChangeableForward *pForward = forwards->CreateForwardEx(
        NULL, ET_Ignore, 4, NULL,
        Param_Cell,   // BZ2Error
        Param_String, // src
        Param_String, // dest
        Param_Cell    // any data
    );

    if (!pForward) {
        return pContext->ThrowNativeError("Failed to create forward");
    }

    pForward->AddFunction(pFunc);

    BZ2Task *task = new BZ2Task(BZ2Task_Decompress, src, dest, 0, data, pForward);
    threader->MakeThread(task, Thread_AutoRelease);

    return 0;
}

static cell_t Native_BZ2_CompressFileAsync(IPluginContext *pContext, const cell_t *params) {
    char *src, *dest;
    pContext->LocalToString(params[1], &src);
    pContext->LocalToString(params[2], &dest);

    IPluginFunction *pFunc = pContext->GetFunctionById((funcid_t)params[3]);

    if (!pFunc) {
        return pContext->ThrowNativeError("Invalid callback function");
    }

    cell_t data = params[4];
    int blockSize = (int)params[5];

    if (blockSize < 1 || blockSize > 9) {
        return pContext->ThrowNativeError("Invalid blockSize %d: must be between 1 and 9", blockSize);
    }

    IChangeableForward *pForward = forwards->CreateForwardEx(
        NULL, ET_Ignore, 4, NULL,
        Param_Cell,   // BZ2Error
        Param_String, // src
        Param_String, // dest
        Param_Cell    // any data
    );

    if (!pForward) {
        return pContext->ThrowNativeError("Failed to create forward");
    }

    pForward->AddFunction(pFunc);

    BZ2Task *task = new BZ2Task(BZ2Task_Compress, src, dest, blockSize, data, pForward);
    threader->MakeThread(task, Thread_AutoRelease);

    return 0;
}

static void OnGameFrame(bool simulating) {
    g_Bzip2.ProcessCompletedTasks();
}

static const sp_nativeinfo_t g_BZ2Natives[] = {
    {"BZ2_DecompressFile",      Native_BZ2_DecompressFile},
    {"BZ2_CompressFile",        Native_BZ2_CompressFile},
    {"BZ2_DecompressFileAsync", Native_BZ2_DecompressFileAsync},
    {"BZ2_CompressFileAsync",   Native_BZ2_CompressFileAsync},
    {NULL, NULL},
};

void Bzip2::QueueCompletedTask(BZ2Task *task) {
    m_Mutex->Lock();
    m_CompletedTasks.push_back(task);
    m_Mutex->Unlock();
}

void Bzip2::ProcessCompletedTasks() {
    std::vector<BZ2Task *> tasks;

    m_Mutex->Lock();
    tasks.swap(m_CompletedTasks);
    m_Mutex->Unlock();

    for (BZ2Task *task : tasks) {
        IChangeableForward *pForward = task->pForward;

        if (pForward->GetFunctionCount() > 0) {
            pForward->PushCell((cell_t)task->result);
            pForward->PushString(task->src);
            pForward->PushString(task->dest);
            pForward->PushCell(task->data);
            pForward->Execute(NULL);
        }

        forwards->ReleaseForward(pForward);

        delete task;
    }
}

bool Bzip2::SDK_OnLoad(char *error, size_t maxlen, bool late) {
    m_Mutex = threader->MakeMutex();

    if (!m_Mutex) {
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

    m_Mutex->DestroyThis();
    m_Mutex = nullptr;
}

void Bzip2::SDK_OnAllLoaded() {
    sharesys->AddNatives(myself, g_BZ2Natives);
}
