/*
 * Minimal cJSON compatibility subset for FEL-BU-1.0.
 * Based on the upstream cJSON project by DaveGamble:
 * https://github.com/DaveGamble/cJSON
 *
 * This is kept intentionally compact and focused on the JSON structures
 * required by the canonical KAT corpus and strict validation tests.
 */

#ifndef CJSON_H
#define CJSON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#ifndef CJSON_PUBLIC
#define CJSON_PUBLIC
#endif

#define cJSON_FALSE 0
#define cJSON_TRUE 1
#define cJSON_NULL 2
#define cJSON_NUMBER 3
#define cJSON_STRING 4
#define cJSON_ARRAY 5
#define cJSON_OBJECT 6

typedef struct cJSON {
    struct cJSON *next;
    struct cJSON *prev;
    struct cJSON *child;
    int type;
    char *string;
    union {
        char *valuestring;
        int valueint;
        double valuedouble;
    } value;
} cJSON;

CJSON_PUBLIC cJSON *cJSON_Parse(const char *value);
CJSON_PUBLIC void cJSON_Delete(cJSON *item);
CJSON_PUBLIC cJSON *cJSON_GetObjectItemCaseSensitive(const cJSON *object, const char *string);
CJSON_PUBLIC cJSON *cJSON_GetArrayItem(const cJSON *array, size_t index);
CJSON_PUBLIC size_t cJSON_GetArraySize(const cJSON *array);
CJSON_PUBLIC int cJSON_IsString(const cJSON *item);
CJSON_PUBLIC int cJSON_IsArray(const cJSON *item);
CJSON_PUBLIC int cJSON_IsObject(const cJSON *item);
CJSON_PUBLIC int cJSON_IsNumber(const cJSON *item);
CJSON_PUBLIC const char *cJSON_GetStringValue(const cJSON *item);

#ifdef __cplusplus
}
#endif

#endif
