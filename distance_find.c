#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "parson.h"
#include <curl/curl.h>

#define BUFFER_SIZE 1024
#define API_URL_GEOCODE "https://naveropenapi.apigw.ntruss.com/map-geocode/v2/geocode"
#define API_URL_DIRECTION "https://naveropenapi.apigw.ntruss.com/map-direction/v1/driving"
#define CLIENT_ID "l10kq6x6md"
#define CLIENT_SECRET "B42VmUxX7qTtnmwcukOKBm9qKwu158D14VygAIUy"

typedef struct {
    char *data;
    size_t size;
} MemoryStruct;

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    MemoryStruct *mem = (MemoryStruct *)userp;

    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (ptr == NULL) {
        printf("Memory allocation failed\n");
        return 0;
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = '\0';

    return realsize;
}

char *send_request(const char *url, const char *headers[], int headers_count) {
    CURL *curl;
    CURLcode res;
    MemoryStruct chunk;

    chunk.data = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        struct curl_slist *chunk_headers = NULL;
        for (int i = 0; i < headers_count; i++) {
            chunk_headers = curl_slist_append(chunk_headers, headers[i]);
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk_headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "CURL request failed: %s\n", curl_easy_strerror(res));
            free(chunk.data);
            chunk.data = NULL;
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(chunk_headers);
    }

    curl_global_cleanup();
    return chunk.data;
}

void parse_addresses(const char *filename, char addresses[][BUFFER_SIZE], int *count) {
    JSON_Value *root_value = json_parse_file(filename);
    JSON_Array *array = json_value_get_array(root_value);
    size_t array_size = json_array_get_count(array);

    *count = 0;
    for (size_t i = 0; i < array_size; i++) {
        JSON_Object *object = json_array_get_object(array, i);
        const char *address = json_object_get_string(object, "address");
        if (address) {
            strncpy(addresses[*count], address, BUFFER_SIZE);
            (*count)++;
        }
    }

    json_value_free(root_value);
}

int get_coordinates(const char *address, char *lat, char *lon) {
    char url[BUFFER_SIZE];
    snprintf(url, BUFFER_SIZE, "%s?query=%s", API_URL_GEOCODE, address);

    const char *headers[] = {
        "X-NCP-APIGW-API-KEY-ID: " CLIENT_ID,
        "X-NCP-APIGW-API-KEY: " CLIENT_SECRET
    };

    char *response = send_request(url, headers, 2);
    if (response) {
        JSON_Value *root_value = json_parse_string(response);
        JSON_Object *root_object = json_value_get_object(root_value);
        JSON_Array *addresses = json_object_get_array(root_object, "addresses");
        if (json_array_get_count(addresses) > 0) {
            JSON_Object *address_object = json_array_get_object(addresses, 0);
            const char *latitude = json_object_get_string(address_object, "y");
            const char *longitude = json_object_get_string(address_object, "x");

            strcpy(lat, latitude);
            strcpy(lon, longitude);

            free(response);
            json_value_free(root_value);
            return 1;
        }

        free(response);
        json_value_free(root_value);
    }

    return 0;
}

double calculate_distance(const char *start_lat, const char *start_lon, const char *end_lat, const char *end_lon) {
    char url[BUFFER_SIZE];
    snprintf(url, BUFFER_SIZE, "%s?start=%s,%s&goal=%s,%s&option=trafast",
             API_URL_DIRECTION, start_lon, start_lat, end_lon, end_lat);

    const char *headers[] = {
        "X-NCP-APIGW-API-KEY-ID: " CLIENT_ID,
        "X-NCP-APIGW-API-KEY: " CLIENT_SECRET
    };

    char *response = send_request(url, headers, 2);
    if (response) {
        JSON_Value *root_value = json_parse_string(response);
        JSON_Object *root_object = json_value_get_object(root_value);
        JSON_Object *route = json_object_get_object(root_object, "route");
        JSON_Array *trafast = json_object_get_array(route, "trafast");

        if (json_array_get_count(trafast) > 0) {
            JSON_Object *summary = json_object_get_object(json_array_get_object(trafast, 0), "summary");
            double distance = json_object_get_number(summary, "distance") / 1000.0;

            free(response);
            json_value_free(root_value);
            return distance;
        }

        free(response);
        json_value_free(root_value);
    }

    return -1.0;
}

void save_distances_to_json(const char *filename, double distances[][BUFFER_SIZE], int coord_count) {
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);

    JSON_Value *array_value = json_value_init_array();
    JSON_Array *array = json_value_get_array(array_value);

    for (int i = 0; i < coord_count; i++) {
        JSON_Value *row_value = json_value_init_array();
        JSON_Array *row = json_value_get_array(row_value);
        for (int j = 0; j < coord_count; j++) {
            json_array_append_number(row, distances[i][j]);
        }
        json_array_append_value(array, row_value);
    }

    json_object_set_value(root_object, "weight", array_value);
    json_serialize_to_file_pretty(root_value, filename);
    json_value_free(root_value);
}

int main() {
    char addresses[100][BUFFER_SIZE];
    int address_count;

    parse_addresses("results.json", addresses, &address_count);

    char coordinates[100][2][BUFFER_SIZE];
    for (int i = 0; i < address_count; i++) {
        if (!get_coordinates(addresses[i], coordinates[i][0], coordinates[i][1])) {
            printf("Failed to get coordinates for address: %s\n", addresses[i]);
        }
    }

    double distances[100][100];
    for (int i = 0; i < address_count; i++) {
        for (int j = 0; j < address_count; j++) {
            if (i == j) {
                distances[i][j] = 0.0;
            } else {
                distances[i][j] = calculate_distance(coordinates[i][0], coordinates[i][1], coordinates[j][0], coordinates[j][1]);
                usleep(100000); // 0.1초 대기
            }
        }
    }

    save_distances_to_json("거리.json", distances, address_count);

    return 0;
}