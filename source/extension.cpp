// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 EpsilonBSP

#include "extension.h"
#include <stdio.h>
#include <string.h>

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

/**
 * native bool BZ2_DecompressFile(const char[] src, const char[] dest);
 *
 * Decompresses a .bz2 file at src and writes the result to dest.
 * Returns true on success, false on failure.
 */
static cell_t Native_BZ2_DecompressFile(IPluginContext *pContext, const cell_t *params) {
    char *src, *dest;
    pContext->LocalToString(params[1], &src);
    pContext->LocalToString(params[2], &dest);

    FILE *inFile = fopen(src, "rb");

    if (!inFile) {
        return 0;
    }

    FILE *outFile = fopen(dest, "wb");

    if (!outFile) {
        fclose(inFile);

        return 0;
    }

    int bzError;
    BZFILE *bzFile = BZ2_bzReadOpen(&bzError, inFile, 0, 0, NULL, 0);

    if (bzError != BZ_OK) {
        fclose(inFile);
        fclose(outFile);

        return 0;
    }

    char buf[BZ2_BUFFER_SIZE];
    bool success = true;

    while (bzError == BZ_OK) {
        int bytesRead = BZ2_bzRead(&bzError, bzFile, buf, sizeof(buf));

        if (bzError != BZ_OK && bzError != BZ_STREAM_END) {
            success = false;

            break;
        }

        if (bytesRead > 0) {
            if (fwrite(buf, 1, bytesRead, outFile) != (size_t)bytesRead) {
                success = false;

                break;
            }
        }
    }

    BZ2_bzReadClose(&bzError, bzFile);
    fclose(inFile);
    fclose(outFile);

    if (!success) {
        remove(dest);
    }

    return success ? 1 : 0;
}

/**
 * native bool BZ2_CompressFile(const char[] src, const char[] dest, int blockSize = 9);
 *
 * Compresses a file at src and writes a .bz2 file to dest.
 * blockSize controls compression level (1–9, higher = better compression).
 * Returns true on success, false on failure.
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
        return 0;
    }

    FILE *outFile = fopen(dest, "wb");

    if (!outFile) {
        fclose(inFile);

        return 0;
    }

    int bzError;
    BZFILE *bzFile = BZ2_bzWriteOpen(&bzError, outFile, blockSize, 0, 30);

    if (bzError != BZ_OK) {
        fclose(inFile);
        fclose(outFile);

        return 0;
    }

    char buf[BZ2_BUFFER_SIZE];
    bool success = true;

    while (!feof(inFile)) {
        size_t bytesRead = fread(buf, 1, sizeof(buf), inFile);

        if (bytesRead == 0) {
            if (ferror(inFile)) {
                success = false;
            }

            break;
        }

        BZ2_bzWrite(&bzError, bzFile, buf, (int)bytesRead);

        if (bzError != BZ_OK) {
            success = false;

            break;
        }
    }

    BZ2_bzWriteClose(&bzError, bzFile, success ? 0 : 1, NULL, NULL);
    fclose(inFile);
    fclose(outFile);

    if (!success) {
        remove(dest);
    }

    return success ? 1 : 0;
}

static const sp_nativeinfo_t g_BZ2Natives[] = {
    {"BZ2_DecompressFile", Native_BZ2_DecompressFile},
    {"BZ2_CompressFile", Native_BZ2_CompressFile},
    {NULL, NULL},
};

void Bzip2::SDK_OnAllLoaded() {
    sharesys->AddNatives(myself, g_BZ2Natives);
}
