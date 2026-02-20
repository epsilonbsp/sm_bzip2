#if defined bzip2_included
    #endinput
#endif
#define bzip2_included

/**
 * Decompresses a .bz2 file to a destination path.
 *
 * @param src   Path to the source .bz2 file.
 * @param dest  Path to write the decompressed output.
 * @return      True on success, false on failure.
 *              On failure a native error is thrown with the reason.
 */
native bool BZ2_ExtractFile(const char[] src, const char[] dest);

/**
 * Compresses a file to a .bz2 archive.
 *
 * @param src       Path to the source file to compress.
 * @param dest      Path to write the compressed .bz2 output.
 * @param blockSize Compression level, 1 (fastest) to 9 (best). Default is 9.
 * @return          True on success, false on failure.
 *                  On failure a native error is thrown with the reason.
 */
native bool BZ2_CompressFile(const char[] src, const char[] dest, int blockSize = 9);

public Extension __ext_bzip2 =
{
    name    = "BZip2 Extension",
    file    = "bzip2.ext",
    autoload = 1,
    required = 1,
};
