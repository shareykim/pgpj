#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "parson.h"

// 메모리 구조체
struct MemoryStruct {
    char* memory;
    size_t size;
};

// 콜백 함수: HTTP 응답 데이터를 메모리에 저장
static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct* mem = (struct MemoryStruct*)userp;

    char* ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        printf("메모리 할당 실패!\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// API 요청 함수
int fetch_data_from_api(const char* url, const char* client_id, const char* client_secret, struct MemoryStruct* chunk) {
    CURL* curl = curl_easy_init();
    if (!curl) return 0;

    struct curl_slist* headers = NULL;
    char id_header[128], secret_header[128];
    snprintf(id_header, sizeof(id_header), "X-NCP-APIGW-API-KEY-ID: %s", client_id);
    snprintf(secret_header, sizeof(secret_header), "X-NCP-APIGW-API-KEY: %s", client_secret);

    headers = curl_slist_append(headers, id_header);
    headers = curl_slist_append(headers, secret_header);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, chunk);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return res == CURLE_OK;
}

// 좌표 얻기 함수
int get_coordinates(const char* address, const char* client_id, const char* client_secret, char* latitude, char* longitude) {
    char url[512];
    snprintf(url, sizeof(url), "https://naveropenapi.apigw.ntruss.com/map-geocode/v2/geocode?query=%s", address);

    struct MemoryStruct chunk = { .memory = malloc(1), .size = 0 };
    if (!fetch_data_from_api(url, client_id, client_secret, &chunk)) {
        free(chunk.memory);
        printf("API 요청 실패!\n");
        return 0;
    }

    JSON_Value* root_value = json_parse_string(chunk.memory);
    if (!root_value) {
        printf("JSON 파싱 실패!\n");
        free(chunk.memory);
        return 0;
    }

    JSON_Object* root_object = json_value_get_object(root_value);
    JSON_Array* addresses = json_object_get_array(root_object, "addresses");
    if (addresses && json_array_get_count(addresses) > 0) {
        JSON_Object* first_address = json_array_get_object(addresses, 0);
        strcpy(latitude, json_object_get_string(first_address, "y"));
        strcpy(longitude, json_object_get_string(first_address, "x"));
        json_value_free(root_value);
        free(chunk.memory);
        return 1;
    }

    printf("'%s' 주소를 찾을 수 없습니다.\n", address);
    json_value_free(root_value);
    free(chunk.memory);
    return 0;
}

// 거리 계산 함수
double calculate_distance(const char* start_lat, const char* start_lon, const char* end_lat, const char* end_lon, const char* client_id, const char* client_secret) {
    char url[512];
    snprintf(url, sizeof(url),
             "https://naveropenapi.apigw.ntruss.com/map-direction/v1/driving?start=%s,%s&goal=%s,%s&option=trafast",
             start_lon, start_lat, end_lon, end_lat);

    struct MemoryStruct chunk = { .memory = malloc(1), .size = 0 };
    if (!fetch_data_from_api(url, client_id, client_secret, &chunk)) {
        free(chunk.memory);
        printf("API 요청 실패!\n");
        return -1.0;
    }

    JSON_Value* root_value = json_parse_string(chunk.memory);
    if (!root_value) {
        printf("JSON 파싱 실패!\n");
        free(chunk.memory);
        return -1.0;
    }

    JSON_Object* root_object = json_value_get_object(root_value);
    JSON_Object* route = json_object_get_object(root_object, "route");
    JSON_Array* trafast = json_object_get_array(route, "trafast");
    if (trafast && json_array_get_count(trafast) > 0) {
        JSON_Object* summary = json_object_get_object(json_array_get_object(trafast, 0), "summary");
        double distance = json_object_get_number(summary, "distance") / 1000.0;
        json_value_free(root_value);
        free(chunk.memory);
        return distance;
    }

    printf("'%s,%s'에서 '%s,%s'로의 경로를 찾을 수 없습니다.\n", start_lat, start_lon, end_lat, end_lon);
    json_value_free(root_value);
    free(chunk.memory);
    return -1.0;
}

// 거리 행렬 JSON 저장
void save_distances_to_json(const char* filename, double** distances, int size) {
    JSON_Value* root_value = json_value_init_object();
    JSON_Object* root_object = json_value_get_object(root_value);
    JSON_Value* weight_array_value = json_value_init_array();
    JSON_Array* weight_array = json_value_get_array(weight_array_value);

    for (int i = 0; i < size; i++) {
        JSON_Value* row_value = json_value_init_array();
        JSON_Array* row = json_value_get_array(row_value);
        for (int j = 0; j < size; j++) {
            json_array_append_number(row, distances[i][j]);
        }
        json_array_append_value(weight_array, row_value);
    }

    json_object_set_value(root_object, "weight", weight_array_value);

    FILE* file = fopen(filename, "w");
    if (file) {
        char* serialized_string = json_serialize_to_string_pretty(root_value);
        fprintf(file, "%s\n", serialized_string);
        fclose(file);
        json_free_serialized_string(serialized_string);
    }
    json_value_free(root_value);
    printf("거리가 %s에 저장되었습니다.\n", filename);
}

int main() {
    const char* client_id = "l10kq6x6md";
    const char* client_secret = "B42VmUxX7qTtnmwcukOKBm9qKwu158D14VygAIUy";

    // 주소 리스트
    JSON_Value* root_value = json_parse_file("results.json");
    JSON_Array* results = json_value_get_array(root_value);
    int num_addresses = json_array_get_count(results);

    // 좌표 리스트
    char latitudes[num_addresses][32];
    char longitudes[num_addresses][32];

    for (int i = 0; i < num_addresses; i++) {
        JSON_Object* item = json_array_get_object(results, i);
        const char* address = json_object_get_string(item, "address");
        if (!get_coordinates(address, client_id, client_secret, latitudes[i], longitudes[i])) {
            printf("좌표를 가져오는 데 실패했습니다: %s\n", address);
            json_value_free(root_value);
            return 1;
        }
    }

    // 거리 행렬 계산
    double** distances = (double**)malloc(num_addresses * sizeof(double*));
    for (int i = 0; i < num_addresses; i++) {
        distances[i] = (double*)malloc(num_addresses * sizeof(double));
        for (int j = 0; j < num_addresses; j++) {
            if (i == j) {
                distances[i][j] = 0.0;
            } else {
                distances[i][j] = calculate_distance(latitudes[i], longitudes[i], latitudes[j], longitudes[j], client_id, client_secret);
                if (distances[i][j] < 0) {
                    printf("거리 계산 실패\n");
                    return 1;
                }
            }
        }
    }

    // 거리 데이터를 JSON 파일로 저장
    save_distances_to_json("거리.json", distances, num_addresses);

    // 메모리 해제
    for (int i = 0; i < num_addresses; i++) {
        free(distances[i]);
    }
    free(distances);

    return 0;
}
