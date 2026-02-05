/*
 * API module - RomM API wrapper with HTTP and JSON parsing
 */

#include "api.h"
#include "log.h"
#include "cJSON/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <3ds.h>

#define MAX_URL_LEN 512
#define MAX_RESPONSE_SIZE (512 * 1024)  // 512KB max response
#define TRACE_BODY_PREVIEW_LEN 500      // Max chars to show for response body

static char baseUrl[256] = "";
static char authHeader[512] = "";

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
    // Nothing to initialize
}

void api_exit(void) {
    // Nothing to cleanup
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
    
    log_debug("GET %s", url);
    
    ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);
    if (R_FAILED(ret)) {
        log_error("httpcOpenContext failed: %08lX", ret);
        return NULL;
    }
    
    // Set headers
    ret = httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
    ret = httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
    ret = httpcAddRequestHeaderField(&context, "User-Agent", "Rommlet/1.0");
    ret = httpcAddRequestHeaderField(&context, "Accept", "application/json");
    
    if (authHeader[0] != '\0') {
        ret = httpcAddRequestHeaderField(&context, "Authorization", authHeader);
        log_trace("Auth: %s", authHeader);
    }
    
    ret = httpcBeginRequest(&context);
    if (R_FAILED(ret)) {
        log_error("httpcBeginRequest failed: %08lX", ret);
        httpcCloseContext(&context);
        return NULL;
    }
    
    u32 status;
    ret = httpcGetResponseStatusCode(&context, &status);
    if (R_FAILED(ret)) {
        log_error("httpcGetResponseStatusCode failed: %08lX", ret);
        httpcCloseContext(&context);
        return NULL;
    }
    *statusCode = (int)status;
    
    log_debug("Status: %lu", status);
    
    if (status != 200) {
        log_error("HTTP error: %lu", status);
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
        log_error("Failed to allocate response buffer");
        httpcCloseContext(&context);
        return NULL;
    }
    
    // Download data
    u32 downloadedSize = 0;
    ret = httpcDownloadData(&context, (u8*)buffer, contentSize, &downloadedSize);
    if (R_FAILED(ret) && ret != HTTPC_RESULTCODE_DOWNLOADPENDING) {
        log_error("httpcDownloadData failed: %08lX", ret);
        free(buffer);
        httpcCloseContext(&context);
        return NULL;
    }
    
    buffer[downloadedSize] = '\0';
    httpcCloseContext(&context);
    
    log_debug("Size: %lu bytes", downloadedSize);
    if (downloadedSize <= TRACE_BODY_PREVIEW_LEN) {
        log_trace("Body:\n%s", buffer);
    } else {
        log_trace("Body (truncated):\n%.*s...\n[%lu more bytes]", 
               TRACE_BODY_PREVIEW_LEN, buffer, 
               downloadedSize - TRACE_BODY_PREVIEW_LEN);
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
        log_error("JSON parse error");
        return NULL;
    }
    
    if (!cJSON_IsArray(json)) {
        log_error("Expected array response");
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
        if (cJSON_IsString(slug)) snprintf(platforms[i].slug, sizeof(platforms[i].slug), "%s", slug->valuestring);
        if (cJSON_IsString(name)) snprintf(platforms[i].name, sizeof(platforms[i].name), "%s", name->valuestring);
        if (cJSON_IsString(displayName)) snprintf(platforms[i].displayName, sizeof(platforms[i].displayName), "%s", displayName->valuestring);
        if (cJSON_IsNumber(romCount)) platforms[i].romCount = romCount->valueint;
        
        // Fallback to name if displayName is empty
        if (platforms[i].displayName[0] == '\0' && platforms[i].name[0] != '\0') {
            snprintf(platforms[i].displayName, sizeof(platforms[i].displayName), "%s", platforms[i].name);
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
        log_error("JSON parse error");
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
        log_error("Expected items array");
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
        if (cJSON_IsString(name)) snprintf(roms[i].name, sizeof(roms[i].name), "%s", name->valuestring);
        if (cJSON_IsString(fsName)) snprintf(roms[i].fsName, sizeof(roms[i].fsName), "%s", fsName->valuestring);
        if (cJSON_IsString(pathCoverSmall)) snprintf(roms[i].pathCoverSmall, sizeof(roms[i].pathCoverSmall), "%s", pathCoverSmall->valuestring);
        
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

RomDetail *api_get_rom_detail(int romId) {
    char url[MAX_URL_LEN];
    snprintf(url, sizeof(url), "%s/api/roms/%d", baseUrl, romId);
    
    int statusCode;
    char *response = http_get(url, &statusCode);
    if (!response) {
        return NULL;
    }
    
    cJSON *json = cJSON_Parse(response);
    free(response);
    
    if (!json) {
        log_error("JSON parse error");
        return NULL;
    }
    
    RomDetail *detail = calloc(1, sizeof(RomDetail));
    if (!detail) {
        cJSON_Delete(json);
        return NULL;
    }
    
    // Basic fields
    cJSON *id = cJSON_GetObjectItem(json, "id");
    cJSON *platformId = cJSON_GetObjectItem(json, "platform_id");
    cJSON *name = cJSON_GetObjectItem(json, "name");
    cJSON *fsName = cJSON_GetObjectItem(json, "fs_name");
    cJSON *summary = cJSON_GetObjectItem(json, "summary");
    cJSON *md5Hash = cJSON_GetObjectItem(json, "md5_hash");
    cJSON *pathCoverSmall = cJSON_GetObjectItem(json, "path_cover_small");
    cJSON *firstReleaseDate = cJSON_GetObjectItem(json, "first_release_date");
    
    // Platform name from nested platform object
    cJSON *platform = cJSON_GetObjectItem(json, "platform");
    cJSON *platformName = platform ? cJSON_GetObjectItem(platform, "display_name") : NULL;
    if (!platformName && platform) {
        platformName = cJSON_GetObjectItem(platform, "name");
    }
    
    if (cJSON_IsNumber(id)) detail->id = id->valueint;
    if (cJSON_IsNumber(platformId)) detail->platformId = platformId->valueint;
    if (cJSON_IsString(name)) snprintf(detail->name, sizeof(detail->name), "%s", name->valuestring);
    if (cJSON_IsString(fsName)) snprintf(detail->fileName, sizeof(detail->fileName), "%s", fsName->valuestring);
    if (cJSON_IsString(summary)) snprintf(detail->summary, sizeof(detail->summary), "%s", summary->valuestring);
    if (cJSON_IsString(md5Hash)) snprintf(detail->md5Hash, sizeof(detail->md5Hash), "%s", md5Hash->valuestring);
    if (cJSON_IsString(pathCoverSmall)) snprintf(detail->pathCoverSmall, sizeof(detail->pathCoverSmall), "%s", pathCoverSmall->valuestring);
    if (cJSON_IsString(platformName)) snprintf(detail->platformName, sizeof(detail->platformName), "%s", platformName->valuestring);
    if (cJSON_IsString(firstReleaseDate)) snprintf(detail->firstReleaseDate, sizeof(detail->firstReleaseDate), "%s", firstReleaseDate->valuestring);
    
    cJSON_Delete(json);
    return detail;
}

void api_free_rom_detail(RomDetail *detail) {
    if (detail) free(detail);
}

bool api_download_rom(int romId, const char *fileName, const char *destPath) {
    char url[MAX_URL_LEN];
    snprintf(url, sizeof(url), "%s/api/roms/%d/content/%s", baseUrl, romId, fileName);
    
    httpcContext context;
    Result ret;
    
    log_debug("GET %s", url);
    log_debug("Saving to: %s", destPath);
    
    ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);
    if (R_FAILED(ret)) {
        log_error("httpcOpenContext failed: %08lX", ret);
        return false;
    }
    
    // Set headers
    httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
    httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
    httpcAddRequestHeaderField(&context, "User-Agent", "Rommlet/1.0");
    httpcAddRequestHeaderField(&context, "Accept", "*/*");
    
    if (authHeader[0] != '\0') {
        httpcAddRequestHeaderField(&context, "Authorization", authHeader);
    }
    
    ret = httpcBeginRequest(&context);
    if (R_FAILED(ret)) {
        log_error("httpcBeginRequest failed: %08lX", ret);
        httpcCloseContext(&context);
        return false;
    }
    
    // Handle redirects
    u32 status;
    ret = httpcGetResponseStatusCode(&context, &status);
    if (R_FAILED(ret)) {
        log_error("httpcGetResponseStatusCode failed: %08lX", ret);
        httpcCloseContext(&context);
        return false;
    }
    
    while (status >= 300 && status < 400) {
        char newUrl[MAX_URL_LEN];
        ret = httpcGetResponseHeader(&context, "Location", newUrl, sizeof(newUrl));
        if (R_FAILED(ret)) {
            log_error("Failed to get redirect location");
            httpcCloseContext(&context);
            return false;
        }
        
        log_debug("Redirect %lu -> %s", status, newUrl);
        
        httpcCloseContext(&context);
        
        ret = httpcOpenContext(&context, HTTPC_METHOD_GET, newUrl, 1);
        if (R_FAILED(ret)) {
            log_error("httpcOpenContext failed on redirect: %08lX", ret);
            return false;
        }
        
        httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
        httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
        httpcAddRequestHeaderField(&context, "User-Agent", "Rommlet/1.0");
        
        ret = httpcBeginRequest(&context);
        if (R_FAILED(ret)) {
            log_error("httpcBeginRequest failed on redirect: %08lX", ret);
            httpcCloseContext(&context);
            return false;
        }
        
        ret = httpcGetResponseStatusCode(&context, &status);
        if (R_FAILED(ret)) {
            log_error("httpcGetResponseStatusCode failed: %08lX", ret);
            httpcCloseContext(&context);
            return false;
        }
    }
    
    log_debug("Status: %lu", status);
    
    if (status != 200) {
        log_error("HTTP error: %lu", status);
        httpcCloseContext(&context);
        return false;
    }
    
    // Open destination file
    FILE *file = fopen(destPath, "wb");
    if (!file) {
        log_error("Failed to open file: %s (errno: %d)", destPath, errno);
        httpcCloseContext(&context);
        return false;
    }
    
    // Download in chunks and write to file
    #define DOWNLOAD_CHUNK_SIZE (64 * 1024)
    u8 *buffer = malloc(DOWNLOAD_CHUNK_SIZE);
    if (!buffer) {
        log_error("Failed to allocate download buffer");
        fclose(file);
        httpcCloseContext(&context);
        return false;
    }
    
    u32 totalDownloaded = 0;
    bool success = true;
    
    while (true) {
        u32 bytesRead = 0;
        ret = httpcDownloadData(&context, buffer, DOWNLOAD_CHUNK_SIZE, &bytesRead);
        
        if (bytesRead > 0) {
            size_t written = fwrite(buffer, 1, bytesRead, file);
            if (written != bytesRead) {
                log_error("Failed to write to file");
                success = false;
                break;
            }
            totalDownloaded += bytesRead;
        }
        
        if (ret == HTTPC_RESULTCODE_DOWNLOADPENDING) {
            continue;
        } else if (R_FAILED(ret)) {
            log_error("httpcDownloadData failed: %08lX", ret);
            success = false;
            break;
        } else {
            // Download complete
            break;
        }
    }
    
    free(buffer);
    fclose(file);
    httpcCloseContext(&context);
    
    log_debug("Downloaded %lu bytes", totalDownloaded);
    
    // If download failed, remove partial file
    if (!success) {
        remove(destPath);
    }
    
    return success;
}
