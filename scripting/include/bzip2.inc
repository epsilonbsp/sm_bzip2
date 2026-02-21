// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 EpsilonBSP

#if defined _bzip2_included
    #endinput
#endif
#define _bzip2_included

enum Bzip2_Error {
    BZIP2_OK                 = 0, /**< Success */
    BZIP2_ERR_SRC_OPEN       = 1, /**< Failed to open source file */
    BZIP2_ERR_DEST_OPEN      = 2, /**< Failed to open destination file */
    BZIP2_ERR_MEM            = 3, /**< Memory allocation failure */
    BZIP2_ERR_DATA           = 4, /**< Data integrity (CRC) error */
    BZIP2_ERR_DATA_MAGIC     = 5, /**< Not a valid bzip2 stream */
    BZIP2_ERR_IO             = 6, /**< I/O error during read/write */
    BZIP2_ERR_UNEXPECTED_EOF = 7, /**< Truncated bzip2 stream */
    BZIP2_ERR_WRITE          = 8, /**< Failed to write output file */
    BZIP2_ERR_UNKNOWN        = 9, /**< Other unexpected error */
};

/**
 * Decompresses a .bz2 file to a destination path.
 *
 * @param src   Path to the source .bz2 file.
 * @param dest  Path to write the decompressed output.
 * @return      BZIP2_OK on success, or a Bzip2_Error code indicating the failure reason.
 */
native Bzip2_Error Bzip2_DecompressFile(const char[] src, const char[] dest);

/**
 * Compresses a file to a .bz2 archive.
 *
 * @param src       Path to the source file to compress.
 * @param dest      Path to write the compressed .bz2 output.
 * @param block_size Compression level, 1 (fastest) to 9 (best). Default is 9.
 * @return          BZIP2_OK on success, or a Bzip2_Error code indicating the failure reason.
 *                  Throws a native error if block_size is out of range.
 */
native Bzip2_Error Bzip2_CompressFile(const char[] src, const char[] dest, int block_size = 9);

/**
 * Called when an async bzip2 operation completes.
 *
 * @param error     BZIP2_OK on success, otherwise the failure reason.
 * @param src       Source path passed to the async native.
 * @param dest      Destination path passed to the async native.
 * @param data      User data value passed to the async native.
 */
typedef BZ2Callback = function void(Bzip2_Error error, const char[] src, const char[] dest, any data);

/**
 * Decompresses a .bz2 file on a background thread, then calls back on the main thread.
 *
 * @param src       Path to the source .bz2 file.
 * @param dest      Path to write the decompressed output.
 * @param callback  Function to call when the operation finishes.
 * @param data      Optional value forwarded to the callback unchanged.
 */
native void Bzip2_DecompressFileAsync(const char[] src, const char[] dest, BZ2Callback callback, any data = 0);

/**
 * Compresses a file on a background thread, then calls back on the main thread.
 *
 * @param src       Path to the source file to compress.
 * @param dest      Path to write the compressed .bz2 output.
 * @param callback  Function to call when the operation finishes.
 * @param data      Optional value forwarded to the callback unchanged.
 * @param block_size Compression level, 1 (fastest) to 9 (best). Default is 9.
 */
native void Bzip2_CompressFileAsync(const char[] src, const char[] dest, BZ2Callback callback, any data = 0, int block_size = 9);

public Extension __ext_bzip2 = {
    name     = "Bzip2 Extension",
    file     = "bzip2.ext",
    autoload = 1,
    required = 1,
};
