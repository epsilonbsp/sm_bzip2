// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 EpsilonBSP

#if defined _bzip2_included
    #endinput
#endif
#define _bzip2_included

enum BZ2Error {
    BZ2_OK              = 0,  /**< Success */
    BZ2_ERR_SRC_OPEN    = 1,  /**< Failed to open source file */
    BZ2_ERR_DEST_OPEN   = 2,  /**< Failed to open destination file */
    BZ2_ERR_MEM         = 3,  /**< Memory allocation failure */
    BZ2_ERR_DATA        = 4,  /**< Data integrity (CRC) error */
    BZ2_ERR_DATA_MAGIC  = 5,  /**< Not a valid bzip2 stream */
    BZ2_ERR_IO          = 6,  /**< I/O error during read/write */
    BZ2_ERR_UNEXPECTED_EOF = 7, /**< Truncated bzip2 stream */
    BZ2_ERR_WRITE       = 8,  /**< Failed to write output file */
    BZ2_ERR_UNKNOWN     = 9,  /**< Other unexpected error */
};

/**
 * Decompresses a .bz2 file to a destination path.
 *
 * @param src   Path to the source .bz2 file.
 * @param dest  Path to write the decompressed output.
 * @return      BZ2_OK on success, or a BZ2Error code indicating the failure reason.
 */
native BZ2Error BZ2_DecompressFile(const char[] src, const char[] dest);

/**
 * Compresses a file to a .bz2 archive.
 *
 * @param src       Path to the source file to compress.
 * @param dest      Path to write the compressed .bz2 output.
 * @param blockSize Compression level, 1 (fastest) to 9 (best). Default is 9.
 * @return          BZ2_OK on success, or a BZ2Error code indicating the failure reason.
 *                  Throws a native error if blockSize is out of range.
 */
native BZ2Error BZ2_CompressFile(const char[] src, const char[] dest, int blockSize = 9);

/**
 * Called when an async bzip2 operation completes.
 *
 * @param error     BZ2_OK on success, otherwise the failure reason.
 * @param src       Source path passed to the async native.
 * @param dest      Destination path passed to the async native.
 * @param data      User data value passed to the async native.
 */
typedef BZ2Callback = function void(BZ2Error error, const char[] src, const char[] dest, any data);

/**
 * Decompresses a .bz2 file on a background thread, then calls back on the main thread.
 *
 * @param src       Path to the source .bz2 file.
 * @param dest      Path to write the decompressed output.
 * @param callback  Function to call when the operation finishes.
 * @param data      Optional value forwarded to the callback unchanged.
 */
native void BZ2_DecompressFileAsync(const char[] src, const char[] dest, BZ2Callback callback, any data = 0);

/**
 * Compresses a file on a background thread, then calls back on the main thread.
 *
 * @param src       Path to the source file to compress.
 * @param dest      Path to write the compressed .bz2 output.
 * @param callback  Function to call when the operation finishes.
 * @param data      Optional value forwarded to the callback unchanged.
 * @param blockSize Compression level, 1 (fastest) to 9 (best). Default is 9.
 */
native void BZ2_CompressFileAsync(const char[] src, const char[] dest, BZ2Callback callback, any data = 0, int blockSize = 9);

public Extension __ext_bzip2 =
{
    name    = "BZip2 Extension",
    file    = "bzip2.ext",
    autoload = 1,
    required = 1,
};
