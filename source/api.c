/*
 * API module - RomM API wrapper with HTTP and JSON parsing
 */

#include "api.h"
#include "cJSON/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>

#define MAX_URL_LEN 512
#define MAX_RESPONSE_SIZE (512 * 1024)  // 512KB max response
#define DEBUG_BODY_PREVIEW_LEN 500      // Max chars to show for response body

static char baseUrl[256] = "";
static char authHeader[512] = "";
static int debugLevel = API_DEBUG_OFF;

// Base64 encoding table
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void base64_encode(const char *input, char *output, size_t outlen) {
    size_t i, j;
    size_t len = strlen(input);
    
    for (i = 0, j = 0; i < len && j < outlen - 4;) {
        uint32_t octet_a = i < len ? (unsigned char)input[i++] : 0;
        uint32_t octet_b = i < len ? (unsigned char)input[i++] : 0;
        uint32_t octet_c = i < len ? (unsigned char)input[i++] : 0;
        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;
        
        output[j++] = base64_table[(triple >> 18) & 0x3F];
        output[j++] = base64_table[(triple >> 12) & 0x3F];
        output[j++] = base64_table[(triple >> 6) & 0x3F];
        output[j++] = base64_table[triple & 0x3F];
    }
    output[j] = '\0';
    
    // Add padding based on input length
    size_t remainder = len % 3;
    if (remainder == 1 && j >= 2) {
        output[j - 1] = '=';
        output[j - 2] = '=';
    } else if (remainder == 2 && j >= 1) {
        output[j - 1] = '=';
    }
}

void api_init(void) {
    debugLevel = API_DEBUG_OFF;
}

void api_exit(void) {
    // Nothing to cleanup yet
}

void api_set_debug_level(int level) {
    if (level < API_DEBUG_OFF) level = API_DEBUG_OFF;
    if (level > API_DEBUG_BODIES) level = API_DEBUG_BODIES;
    debugLevel = level;
}

int api_get_debug_level(void) {
    return debugLevel;
}

void api_set_base_url(const char *url) {
    strncpy(baseUrl, url, sizeof(baseUrl) - 1);
    // Remove trailing slash if present
    size_t len = strlen(baseUrl);
    if (len > 0 && baseUrl[len - 1] == '/') {
        baseUrl[len - 1] = '\0';
    }
}

void api_set_auth(const char *username, const char *password) {
    if (!username || !password || username[0] == '\0') {
        authHeader[0] = '\0';
        return;
    }
    
    char credentials[256];
    snprintf(credentials, sizeof(credentials), "%s:%s", username, password);
    
    char encoded[256];
    base64_encode(credentials, encoded, sizeof(encoded));
    
    snprintf(authHeader, sizeof(authHeader), "Basic %s", encoded);
}

static char *http_get(const char *url, int *statusCode) {
    httpcContext context;
    Result ret;
    
    *statusCode = 0;
    
    // Debug: log outgoing request
    if (debugLevel >= API_DEBUG_REQUESTS) {
        printf("[DEBUG] GET %s\n", url);
    }
    
    ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);
    if (R_FAILED(ret)) {
        printf("httpcOpenContext failed: %08lX\n", ret);
        return NULL;
    }
    
    // Set headers
    ret = httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
    ret = httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
    ret = httpcAddRequestHeaderField(&context, "User-Agent", "Rommlet/1.0");
    ret = httpcAddRequestHeaderField(&context, "Accept", "application/json");
    
    if (authHeader[0] != '\0') {
        ret = httpcAddRequestHeaderField(&context, "Authorization", authHeader);
        if (debugLevel >= API_DEBUG_REQUESTS) {
            printf("[DEBUG] Auth: %s\n", authHeader);
        }
    }
    
    ret = httpcBeginRequest(&context);
    if (R_FAILED(ret)) {
        printf("httpcBeginRequest failed: %08lX\n", ret);
        httpcCloseContext(&context);
        return NULL;
    }
    
    u32 status;
    ret = httpcGetResponseStatusCode(&context, &status);
    if (R_FAILED(ret)) {
        printf("httpcGetResponseStatusCode failed: %08lX\n", ret);
        httpcCloseContext(&context);
        return NULL;
    }
    *statusCode = (int)status;
    
    // Debug: log response status
    if (debugLevel >= API_DEBUG_REQUESTS) {
        printf("[DEBUG] Status: %lu\n", status);
    }
    
    if (status != 200) {
        printf("HTTP error: %lu\n", status);
        httpcCloseContext(&context);
        return NULL;
    }
    
    // Get content length
    u32 contentSize = 0;
    ret = httpcGetDownloadSizeState(&context, NULL, &contentSize);
    if (contentSize == 0 || contentSize > MAX_RESPONSE_SIZE) {
        contentSize = MAX_RESPONSE_SIZE;
    }
    
    // Allocate buffer
    char *buffer = malloc(contentSize + 1);
    if (!buffer) {
        printf("Failed to allocate response buffer\n");
        httpcCloseContext(&context);
        return NULL;
    }
    
    // Download data
    u32 downloadedSize = 0;
    ret = httpcDownloadData(&context, (u8*)buffer, contentSize, &downloadedSize);
    if (R_FAILED(ret) && ret != HTTPC_RESULTCODE_DOWNLOADPENDING) {
        printf("httpcDownloadData failed: %08lX\n", ret);
        free(buffer);
        httpcCloseContext(&context);
        return NULL;
    }
    
    buffer[downloadedSize] = '\0';
    httpcCloseContext(&context);
    
    // Debug: log response size and body
    if (debugLevel >= API_DEBUG_REQUESTS) {
        printf("[DEBUG] Size: %lu bytes\n", downloadedSize);
    }
    if (debugLevel >= API_DEBUG_BODIES) {
        printf("[DEBUG] Body:\n");
        if (downloadedSize <= DEBUG_BODY_PREVIEW_LEN) {
            printf("%s\n", buffer);
        } else {
            // Print truncated body
            printf("%.*s...\n[truncated, %lu more bytes]\n", 
                   DEBUG_BODY_PREVIEW_LEN, buffer, 
                   downloadedSize - DEBUG_BODY_PREVIEW_LEN);
        }
    }
    
    return buffer;
}

Platform *api_get_platforms(int *count) {
    *count = 0;
    
    char url[MAX_URL_LEN];
    snprintf(url, sizeof(url), "%s/api/platforms", baseUrl);
    
    int statusCode;
    char *response = http_get(url, &statusCode);
    if (!response) {
        return NULL;
    }
    
    // Parse JSON
    cJSON *json = cJSON_Parse(response);
    free(response);
    
    if (!json) {
        printf("JSON parse error\n");
        return NULL;
    }
    
    if (!cJSON_IsArray(json)) {
        printf("Expected array response\n");
        cJSON_Delete(json);
        return NULL;
    }
    
    int arraySize = cJSON_GetArraySize(json);
    if (arraySize == 0) {
        cJSON_Delete(json);
        return NULL;
    }
    
    Platform *platforms = calloc(arraySize, sizeof(Platform));
    if (!platforms) {
        cJSON_Delete(json);
        return NULL;
    }
    
    int i = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, json) {
        cJSON *id = cJSON_GetObjectItem(item, "id");
        cJSON *slug = cJSON_GetObjectItem(item, "slug");
        cJSON *name = cJSON_GetObjectItem(item, "name");
        cJSON *displayName = cJSON_GetObjectItem(item, "display_name");
        cJSON *romCount = cJSON_GetObjectItem(item, "rom_count");
        
        if (cJSON_IsNumber(id)) platforms[i].id = id->valueint;
        if (cJSON_IsString(slug)) strncpy(platforms[i].slug, slug->valuestring, sizeof(platforms[i].slug) - 1);
        if (cJSON_IsString(name)) strncpy(platforms[i].name, name->valuestring, sizeof(platforms[i].name) - 1);
        if (cJSON_IsString(displayName)) strncpy(platforms[i].displayName, displayName->valuestring, sizeof(platforms[i].displayName) - 1);
        if (cJSON_IsNumber(romCount)) platforms[i].romCount = romCount->valueint;
        
        // Fallback to name if displayName is empty
        if (platforms[i].displayName[0] == '\0' && platforms[i].name[0] != '\0') {
            strncpy(platforms[i].displayName, platforms[i].name, sizeof(platforms[i].displayName) - 1);
        }
        
        i++;
    }
    
    *count = i;
    cJSON_Delete(json);
    return platforms;
}

void api_free_platforms(Platform *platforms, int count) {
    (void)count;
    if (platforms) free(platforms);
}

Rom *api_get_roms(int platformId, int offset, int limit, int *count, int *total) {
    *count = 0;
    *total = 0;
    
    char url[MAX_URL_LEN];
    snprintf(url, sizeof(url), "%s/api/roms?platform_ids=%d&offset=%d&limit=%d&order_by=name", 
             baseUrl, platformId, offset, limit);
    
    int statusCode;
    char *response = http_get(url, &statusCode);
    if (!response) {
        return NULL;
    }
    
    // Parse JSON
    cJSON *json = cJSON_Parse(response);
    free(response);
    
    if (!json) {
        printf("JSON parse error\n");
        return NULL;
    }
    
    // Get pagination info
    cJSON *totalJson = cJSON_GetObjectItem(json, "total");
    if (cJSON_IsNumber(totalJson)) {
        *total = totalJson->valueint;
    }
    
    // Get items array
    cJSON *items = cJSON_GetObjectItem(json, "items");
    if (!items || !cJSON_IsArray(items)) {
        printf("Expected items array\n");
        cJSON_Delete(json);
        return NULL;
    }
    
    int arraySize = cJSON_GetArraySize(items);
    if (arraySize == 0) {
        cJSON_Delete(json);
        return NULL;
    }
    
    Rom *roms = calloc(arraySize, sizeof(Rom));
    if (!roms) {
        cJSON_Delete(json);
        return NULL;
    }
    
    int i = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, items) {
        cJSON *id = cJSON_GetObjectItem(item, "id");
        cJSON *platformIdJson = cJSON_GetObjectItem(item, "platform_id");
        cJSON *name = cJSON_GetObjectItem(item, "name");
        cJSON *fsName = cJSON_GetObjectItem(item, "fs_name");
        cJSON *pathCoverSmall = cJSON_GetObjectItem(item, "path_cover_small");
        
        if (cJSON_IsNumber(id)) roms[i].id = id->valueint;
        if (cJSON_IsNumber(platformIdJson)) roms[i].platformId = platformIdJson->valueint;
        if (cJSON_IsString(name)) strncpy(roms[i].name, name->valuestring, sizeof(roms[i].name) - 1);
        if (cJSON_IsString(fsName)) strncpy(roms[i].fsName, fsName->valuestring, sizeof(roms[i].fsName) - 1);
        if (cJSON_IsString(pathCoverSmall)) strncpy(roms[i].pathCoverSmall, pathCoverSmall->valuestring, sizeof(roms[i].pathCoverSmall) - 1);
        
        i++;
    }
    
    *count = i;
    cJSON_Delete(json);
    return roms;
}

void api_free_roms(Rom *roms, int count) {
    (void)count;
    if (roms) free(roms);
}
