/***********************************************************************************
 * Copyright (c) 2012, Sepehr Taghdisian
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 ***********************************************************************************/

#include "dhcore/err.h"
#include "dhcore/mem-mgr.h"
#include "dhcore/zip.h"
#include "miniz/miniz.h"

/* */
size_t zip_compressedsize(size_t src_size)
{
    return (size_t)compressBound((mz_ulong)src_size);
}

size_t zip_compress(void* dest_buffer, size_t dest_size,
                    const void* buffer, size_t size, enum compress_mode mode)
{
    int c_level;
    switch (mode)   {
        case COMPRESS_NORMAL:   c_level = Z_DEFAULT_COMPRESSION;    break;
        case COMPRESS_FAST:     c_level = Z_BEST_SPEED;             break;
        case COMPRESS_BEST:     c_level = Z_BEST_COMPRESSION;       break;
        case COMPRESS_NONE:     c_level = Z_NO_COMPRESSION;         break;
        default:				c_level = Z_DEFAULT_COMPRESSION;	break;
    }

    uLongf dsize = (uLongf)dest_size;
    int r = compress2((Bytef*)dest_buffer, &dsize, (const Bytef*)buffer, (uLongf)size, c_level);
    return (r == Z_OK) ? (size_t)dsize : 0;
}

size_t zip_decompress(void* dest_buffer, size_t dest_size, const void* buffer, size_t size)
{
    uLongf dsize = (uLongf)dest_size;
    int r = uncompress((Bytef*)dest_buffer, &dsize, (Bytef*)buffer, (uLongf)size);
    return (r == Z_OK) ? (size_t)dsize : 0;
}

zip_t zip_open(const char *filepath)
{
    mz_zip_archive *zip = (mz_zip_archive*)ALLOC(sizeof(mz_zip_archive), 0);
    if (zip == NULL)
        return NULL;
    memset(zip, 0x00, sizeof(mz_zip_archive));

    if (!mz_zip_reader_init_file(zip, filepath, 0)) {
        FREE(zip);
        return NULL;
    }

    return zip;
}

zip_t zip_open_mem(const char *buff, size_t buff_sz)
{
    mz_zip_archive *zip = (mz_zip_archive*)ALLOC(sizeof(mz_zip_archive), 0);
    if (zip == NULL)
        return NULL;
    memset(zip, 0x00, sizeof(mz_zip_archive));

    if (!mz_zip_reader_init_mem(zip, buff, buff_sz, 0)) {
        FREE(zip);
        return NULL;
    }

    return zip;
}

void zip_close(zip_t zip)
{
    ASSERT(zip);
    mz_zip_reader_end(zip);
}

file_t zip_getfile(zip_t zip, const char *filepath, struct allocator *alloc)
{
    int idx = mz_zip_reader_locate_file(zip, filepath, NULL, 0);
    if (idx == -1)
        return NULL;

    mz_zip_archive_file_stat stat;
    mz_zip_reader_file_stat(zip, idx, &stat);

    void *buff = A_ALLOC(alloc, (size_t)stat.m_uncomp_size, 0);
    if (buff == NULL)
        return NULL;
    if (!mz_zip_reader_extract_to_mem(zip, idx, buff, (size_t)stat.m_uncomp_size, 0))   {
        A_FREE(alloc, buff);
        return NULL;
    }

    return fio_attachmem(alloc, buff, (size_t)stat.m_uncomp_size, filepath, 0);
}
