#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "cJSON.h"

// 응답 데이터를 저장하기 위한 구조체
struct MemoryStruct {
    char* memory;
    size_t size;
};

// libcurl 데이터를 저장하는 콜백 함수
size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct* mem = (struct MemoryStruct*)userp;

    char* ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// 네이버 지도 API에서 좌표를 가져오는 함수
int get_coordinates(const char* address, const char* client_id, const char* client_secret, double* latitude, double* longitude) {
    CURL* curl;
    CURLcode res;
    struct MemoryStruct chunk = { 0 };

    char url[512];
    snprintf(url, sizeof(url), "https://naveropenapi.apigw.ntruss.com/map-geocode/v2/geocode?query=%s", address);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    char id_header[128];
    snprintf(id_header, sizeof(id_header), "X-NCP-APIGW-API-KEY-ID: %s", client_id);
    headers = curl_slist_append(headers, id_header);
    char secret_header[128];
    snprintf(secret_header, sizeof(secret_header), "X-NCP-APIGW-API-KEY: %s", client_secret);
    headers = curl_slist_append(headers, secret_header);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            return 0;
        }

        cJSON* json = cJSON_Parse(chunk.memory);
        if (!json) {
            printf("Error parsing JSON response.\n");
            return 0;
        }

        cJSON* addresses = cJSON_GetObjectItem(json, "addresses");
        if (cJSON_GetArraySize(addresses) > 0) {
            cJSON* address_data = cJSON_GetArrayItem(addresses, 0);
            *latitude = cJSON_GetObjectItem(address_data, "y")->valuedouble; // 위도
            *longitude = cJSON_GetObjectItem(address_data, "x")->valuedouble; // 경도
            cJSON_Delete(json);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            return 1;
        }
        cJSON_Delete(json);
        curl_easy_cleanup(curl);
        free(chunk.memory);
    }

    return 0;
}

// 거리 배열을 JSON 파일로 저장하는 함수
void save_distances_to_json(const char* filename, double** distances, int size) {
    cJSON* root = cJSON_CreateObject();
    cJSON* weight_array = cJSON_CreateArray();

    for (int i = 0; i < size; i++) {
        cJSON* row = cJSON_CreateArray();
        for (int j = 0; j < size; j++) {
            cJSON_AddItemToArray(row, cJSON_CreateNumber(distances[i][j]));
        }
        cJSON_AddItemToArray(weight_array, row);
    }

    cJSON_AddItemToObject(root, "distances", weight_array);

    FILE* file = fopen(filename, "w");
    if (file) {
        char* json_data = cJSON_Print(root);
        fprintf(file, "%s\n", json_data);
        fclose(file);
        free(json_data);
    }
    cJSON_Delete(root);
    printf("Distances saved to %s\n", filename);
}

int process_addresses_and_save(const char* filename, const char* client_id, const char* client_secret, const char* output_file) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Unable to open file: %s\n", filename);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* data = (char*)malloc(file_size + 1);
    fread(data, 1, file_size, file);
    fclose(file);
    data[file_size] = '\0';

    cJSON* json = cJSON_Parse(data);
    if (!json) {
        printf("Error parsing JSON file.\n");
        free(data);
        return 0;
    }

    int num_addresses = cJSON_GetArraySize(json);
    if (num_addresses < 2) {
        printf("Not enough addresses to calculate distances.\n");
        cJSON_Delete(json);
        free(data);
        return 0;
    }

    double* latitudes = malloc(num_addresses * sizeof(double));
    double* longitudes = malloc(num_addresses * sizeof(double));

    for (int i = 0; i < num_addresses; i++) {
        cJSON* item = cJSON_GetArrayItem(json, i);
        const char* address = cJSON_GetObjectItem(item, "roadAddress")->valuestring;

        if (!get_coordinates(address, client_id, client_secret, &latitudes[i], &longitudes[i])) {
            printf("Unable to get coordinates for address: %s\n", address);
            free(latitudes);
            free(longitudes);
            cJSON_Delete(json);
            free(data);
            return 0;
        }
    }

    // 거리 계산 배열 생성
    double** distances = malloc(num_addresses * sizeof(double*));
    for (int i = 0; i < num_addresses; i++) {
        distances[i] = malloc(num_addresses * sizeof(double));
        for (int j = 0; j < num_addresses; j++) {
            if (i == j) {
                distances[i][j] = 0.0; // 같은 위치는 거리 0
            } else {
                distances[i][j] = calculate_distance(latitudes[i], longitudes[i], latitudes[j], longitudes[j], client_id, client_secret);
            }
        }
    }

    // JSON 파일로 저장
    save_distances_to_json(output_file, distances, num_addresses);

    // 메모리 해제
    for (int i = 0; i < num_addresses; i++) {
        free(distances[i]);
    }
    free(distances);
    free(latitudes);
    free(longitudes);
    cJSON_Delete(json);
    free(data);

    return 1;
}

// 두 좌표 간 거리를 계산하는 함수 (단위: km)
double calculate_distance(double start_lat, double start_lon, double end_lat, double end_lon, const char* client_id, const char* client_secret) {
    CURL* curl;
    CURLcode res;
    struct MemoryStruct chunk = { 0 };

    char url[512];
    snprintf(url, sizeof(url), "https://naveropenapi.apigw.ntruss.com/map-direction/v1/driving?start=%f,%f&goal=%f,%f&option=trafast",
        start_lon, start_lat, end_lon, end_lat);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    char id_header[128];
    snprintf(id_header, sizeof(id_header), "X-NCP-APIGW-API-KEY-ID: %s", client_id);
    headers = curl_slist_append(headers, id_header);
    char secret_header[128];
    snprintf(secret_header, sizeof(secret_header), "X-NCP-APIGW-API-KEY: %s", client_secret);
    headers = curl_slist_append(headers, secret_header);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            return -1;
        }

        cJSON* json = cJSON_Parse(chunk.memory);
        if (!json) {
            printf("Error parsing JSON response.\n");
            return -1;
        }

        cJSON* route = cJSON_GetObjectItem(json, "route");
        cJSON* trafast = cJSON_GetObjectItem(route, "trafast");
        if (cJSON_GetArraySize(trafast) > 0) {
            cJSON* summary = cJSON_GetObjectItem(cJSON_GetArrayItem(trafast, 0), "summary");
            double distance = cJSON_GetObjectItem(summary, "distance")->valuedouble / 1000.0; // km 단위로 변환
            cJSON_Delete(json);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            return distance;
        }
        cJSON_Delete(json);
        curl_easy_cleanup(curl);
        free(chunk.memory);
    }

    return -1;
}

// result.json 파일을 읽어서 주소 데이터를 처리하는 함수
int process_addresses_from_json(const char* filename, const char* client_id, const char* client_secret) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Unable to open file: %s\n", filename);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* data = (char*)malloc(file_size + 1);
    fread(data, 1, file_size, file);
    fclose(file);
    data[file_size] = '\0';

    cJSON* json = cJSON_Parse(data);
    if (!json) {
        printf("Error parsing JSON file.\n");
        free(data);
        return 0;
    }

    cJSON* items = cJSON_GetArrayItem(json, 0);
    int item_count = cJSON_GetArraySize(items);
    if (item_count < 2) {
        printf("Not enough addresses to calculate distance.\n");
        free(data);
        return 0;
    }

    double start_lat, start_lon, end_lat, end_lon;
    if (!get_coordinates(cJSON_GetObjectItem(items, 0)->address, client_id, client_secret, &start_lat, &start_lon)) {
        printf("Unable to get coordinates for starting address.\n");
        free(data);
        return 0;
    }
    if (!get_coordinates(cJSON_GetObjectItem(items, 1)->address, client_id, client_secret, &end_lat, &end_lon)) {
        printf("Unable to get coordinates for ending address.\n");
        free(data);
        return 0;
    }

    double distance = calculate_distance(start_lat, start_lon, end_lat, end_lon, client_id, client_secret);
    if (distance >= 0) {
        printf("Distance between the two locations: %.2f km\n", distance);
    }
    else {
        printf("Failed to calculate distance.\n");
    }

    free(data);
    cJSON_Delete(json);
    return 1;
}

int main() {
    // 네이버 API 키 정보 (네이버 개발자 센터에서 발급받은 값)
    const char* client_id = "l10kq6x6md"; // 콜론(:)이 아니라 등호(=)를 사용해야 합니다.
    const char* client_secret = "B42VmUxX7qTtnmwcukOKBm9qKwu158D14VygAIUy";

    // result.json 파일에서 주소 데이터를 가져와 처리 및 거리 데이터 저장
    if (!process_addresses_and_save("result.json", client_id, client_secret, "거리.json")) {
        printf("Failed to process addresses and save distances.\n");
        return 1;
    }

    printf("Successfully processed addresses and saved distances to 거리.json.\n");
    return 0;
}