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

#include <stdio.h>

#include "cJSON/cJSON.h"

#include "mem-mgr.h"
#include "json.h"
#include "pool-alloc.h"
#include "err.h"
#include "util.h"

#define JSON_ALLOC_16    0
#define JSON_ALLOC_32    1
#define JSON_ALLOC_64    2
#define JSON_ALLOC_128   3
#define JSON_ALLOC_256   4
#define JSON_ALLOC_CNT   5

/*************************************************************************************************
 * types/globals
 */
struct json_mgr
{
    struct pool_alloc_ts buffs[JSON_ALLOC_CNT];
};

static struct json_mgr* g_json = NULL;

/*************************************************************************************************/
INLINE void* json_alloc_putsize(void* ptr, uint sz)
{
    *((uint*)ptr) = sz;
    return ((uint8*)ptr + sizeof(uint));
}

INLINE uint json_alloc_getsize(void* ptr, void** preal_ptr)
{
    uint8* bptr = (uint8*)ptr - sizeof(uint);
    uint sz = *((uint*)bptr);
    *preal_ptr = bptr;
    return sz;
}

INLINE uint json_choose_alloc(size_t sz)
{
    if (sz <= 16)
        return JSON_ALLOC_16;
    else if (sz > 16 && sz <= 32)
        return JSON_ALLOC_32;
    else if (sz > 32 && sz <=64)
        return JSON_ALLOC_64;
    else if (sz > 64 && sz <= 128)
        return JSON_ALLOC_128;
    else if (sz > 128 && sz <= 256)
        return JSON_ALLOC_256;
    else
        return INVALID_INDEX;
}

/*************************************************************************************************/
/* callbacks for memory allocation/deallocation */
static void* json_malloc(size_t size)
{
    ASSERT(size < UINT32_MAX);

    size += sizeof(uint);    /* to keep the size */
    uint a_idx = json_choose_alloc(size);
    void* ptr;
    if (a_idx != INVALID_INDEX)
        ptr = mem_pool_alloc_ts(&g_json->buffs[a_idx]);
    else
        ptr = mem_alloc(size, __FILE__, __LINE__, 0);
    return json_alloc_putsize(ptr, (uint)size);
}

static void json_free(void* p)
{
    uint sz = json_alloc_getsize(p, &p);
    uint a_idx = json_choose_alloc(sz);

    if (a_idx != INVALID_INDEX)
        mem_pool_free_ts(&g_json->buffs[a_idx], p);
    else
        mem_free(p);
}

/* */
static result_t json_create_buffs()
{
    if (IS_FAIL(mem_pool_create_ts(mem_heap(), &g_json->buffs[JSON_ALLOC_16], 16, 1024, 0)))
        return RET_OUTOFMEMORY;
    if (IS_FAIL(mem_pool_create_ts(mem_heap(), &g_json->buffs[JSON_ALLOC_32], 32, 1024, 0)))
        return RET_OUTOFMEMORY;
    if (IS_FAIL(mem_pool_create_ts(mem_heap(), &g_json->buffs[JSON_ALLOC_64], 64, 1024, 0)))
        return RET_OUTOFMEMORY;
    if (IS_FAIL(mem_pool_create_ts(mem_heap(), &g_json->buffs[JSON_ALLOC_128], 128, 512, 0)))
        return RET_OUTOFMEMORY;
    if (IS_FAIL(mem_pool_create_ts(mem_heap(), &g_json->buffs[JSON_ALLOC_256], 256, 512, 0)))
        return RET_OUTOFMEMORY;

    return RET_OK;
}

static void json_destroy_buffs()
{
    for (uint i = 0; i < JSON_ALLOC_CNT; i++)
        mem_pool_destroy_ts(&g_json->buffs[i]);
}

result_t json_init()
{
    if (g_json != NULL)
        return RET_FAIL;
    g_json = (struct json_mgr*)ALLOC(sizeof(struct json_mgr), 0);
    if (g_json == NULL)
        return RET_OUTOFMEMORY;

    memset(g_json, 0x00, sizeof(struct json_mgr));

    if (IS_FAIL(json_create_buffs()))
        return RET_OUTOFMEMORY;

    cJSON_Hooks hooks;
    hooks.malloc_fn = json_malloc;
    hooks.free_fn = json_free;
    cJSON_InitHooks(&hooks);

    return RET_OK;
}

void json_release()
{
    if (g_json != NULL) {
        json_destroy_buffs();
        cJSON_InitHooks(NULL);

        FREE(g_json);
        g_json = NULL;
    }
}

json_t json_parsefile(const char* filepath)
{
    ASSERT(g_json);
    
    char* buff = util_readtextfile(filepath, mem_heap());
    if (buff == NULL)   {
        err_printf(__FILE__, __LINE__, "JSON parse failed: could not open file '%s'", filepath);
        return NULL;
    }

    json_t jroot = json_parsestring(buff);
    FREE(buff);

    return jroot;
}

json_t json_parsefilef(file_t f, struct allocator* tmp_alloc)
{
    ASSERT(g_json);

    /* read file and put it's data into a buffer */
    size_t size = fio_getsize(f);
    if (size == 0)         {
        err_printf(__FILE__, __LINE__, "JSON load failed: zero size file '%s'", fio_getpath(f));
        return NULL;
    }

    char* buffer = (char*)A_ALLOC(tmp_alloc, size+1, 0);
    if (buffer == NULL)    {
        err_printn(__FILE__, __LINE__, RET_OUTOFMEMORY);
        return NULL;
    }

    fio_read(f, buffer, size, 1);
    buffer[size] = 0;
    json_t j = json_parsestring(buffer);
    A_FREE(tmp_alloc, buffer);

    if (j == NULL) {
        err_printf(__FILE__, __LINE__, "JSON parse '%s' failed: '%s'", fio_getpath(f),
            cJSON_GetErrorPtr());
        return NULL;
    }
    return j;
}

json_t json_parsestring(const char* str)
{
    ASSERT(g_json);

    json_t j = cJSON_Parse(str);
    if (j == NULL)     {
        err_printf(__FILE__, __LINE__, "JSON parse failed: %s", cJSON_GetErrorPtr());
        return NULL;
    }

    return j;
}

result_t json_savetofile(json_t j, const char* filepath, int trim)
{
    ASSERT(g_json);

    /* open file for writing */
    FILE* f = fopen(filepath, "wb");
    if (f == NULL)    {
        err_printf(__FILE__, __LINE__, "JSON write error: could not open file '%s' for writing.",
                   filepath);
        return RET_FILE_ERROR;
    }

    char* buffer = trim ? cJSON_PrintUnformatted(j) : cJSON_Print(j);
    if (buffer == NULL) {
        err_printn(__FILE__, __LINE__, RET_OUTOFMEMORY);
        return RET_OUTOFMEMORY;
    }

    fwrite(buffer, 1, strlen(buffer) + 1, f);
    json_free(buffer);
    return RET_OK;
}

result_t json_savetofilef(json_t j, file_t f, int trim)
{
    ASSERT(g_json);
    ASSERT(fio_isopen(f));

    char* buffer = trim ? cJSON_PrintUnformatted(j) : cJSON_Print(j);
    if (buffer == NULL) {
        err_printn(__FILE__, __LINE__, RET_OUTOFMEMORY);
        return RET_OUTOFMEMORY;
    }

    fio_write(f, buffer, 1, strlen(buffer)+1);
    json_free(buffer);
    return RET_OK;
}

char* json_savetobuffer(json_t j, size_t* outsize, int trim)
{
    ASSERT(g_json);

    char* buffer = trim ? cJSON_PrintUnformatted(j) : cJSON_Print(j);
    if (buffer == NULL)
        return NULL;

    *outsize = strlen(buffer);
    return buffer;
}

void json_deletebuffer(char* buffer)
{
    ASSERT(g_json);

    json_free(buffer);
}

void json_destroy(json_t j)
{
    ASSERT(g_json);

    ASSERT(j != NULL);
    cJSON_Delete(j);
}

void json_seti(json_t j, int n)
{
    (j)->valueint = n;
}

void json_setf(json_t j, float f)
{
    (j)->valuedouble = f;
}

void json_sets(json_t j, const char* str)
{
    strcpy((j)->string, str);
}

void json_setb(json_t j, int b)
{
    (j)->valueint = b;
}

int json_geti_child(json_t parent, const char* name, int def_value)
{
    json_t j = json_getitem(parent, name);
    if (j != NULL)  {
        return json_geti(j);
    }   else    {
        return def_value;
    }
}

float json_getf_child(json_t parent, const char* name, float def_value)
{
    json_t j = json_getitem(parent, name);
    if (j != NULL)  {
        return json_getf(j);
    }   else    {
        return def_value;
    }
}

const char* json_gets_child(json_t parent, const char* name, const char* def_value)
{
    json_t j = json_getitem(parent, name);
    if (j != NULL)  {
        return json_gets(j);
    }   else    {
        return def_value;
    }
}

int json_getb_child(json_t parent, const char* name, int def_value)
{
    json_t j = json_getitem(parent, name);
    if (j != NULL)  {
        return json_getb(j);
    }   else    {
        return def_value;
    }
}

int json_geti(json_t j)
{
    return (j)->valueint;
}

float json_getf(json_t j)
{
    return (float)(j)->valuedouble;
}

const char* json_gets(json_t j)
{
    return (j)->valuestring;
}

int json_getb(json_t j)
{
    return ((j)->valueint ? TRUE : FALSE);
}

enum json_type json_gettype(json_t j)
{
    int t = (j)->type;
    if (t == 0)
        return JSON_BOOL;
    return (enum json_type)t;
}

int json_getarr_count(json_t j)
{
    return cJSON_GetArraySize(j);
}

json_t json_getarr_item(json_t j, int idx)
{
    return cJSON_GetArrayItem(j, idx);
}

json_t json_getitem(json_t j, const char* name)
{
    return cJSON_GetObjectItem(j, name);
}

json_t json_create_null()
{
    return cJSON_CreateNull();
}

json_t json_create_obj()
{
    return cJSON_CreateObject();
}

json_t json_create_bool(int b)
{
    return cJSON_CreateBool(b);
}

json_t json_create_num(fl64 n)
{
    return cJSON_CreateNumber(n);
}

json_t json_create_str(const char* str)
{
    return cJSON_CreateString(str);
}

json_t json_create_arr()
{
    return cJSON_CreateArray();
}

json_t json_create_arri(int* nums, int count)
{
    return cJSON_CreateIntArray(nums, count);
}

json_t json_create_arrf(float* nums, int count)
{
    return cJSON_CreateFloatArray(nums, count);
}

json_t json_create_arrs(const char** strs, int count)
{
    return cJSON_CreateStringArray(strs, count);
}

void json_additem_toarr(json_t arr, json_t item)
{
    cJSON_AddItemToArray(arr, item);
}

void json_additem_toobj(json_t obj, const char* name, json_t item)
{
    cJSON_AddItemToObject(obj, name, item);
}

void json_additem_toobj_nodup(json_t obj, const char* name, json_t item)
{
    if (json_getitem(obj, name) == NULL)
        json_additem_toobj(obj, name, item);
    else
        json_replaceitem_inobj(obj, name, item);
}

void json_replaceitem_inobj(json_t obj, const char* name, json_t item)
{
    cJSON_ReplaceItemInObject(obj, name, item);
}

void json_replaceitem_inarr(json_t obj, int idx, json_t item)
{
    cJSON_ReplaceItemInArray(obj, idx, item);
}