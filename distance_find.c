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

// JSON 응답에서 필요한 값을 추출하는 유틸리티 함수
cJSON* parse_json_response(const char* json_data, const char* key) {
    cJSON* json = cJSON_Parse(json_data);
    if (!json) {
        printf("Error parsing JSON response.\n");
        return NULL;
    }
    cJSON* value = cJSON_GetObjectItem(json, key);
    cJSON_Delete(json);  // json 메모리 해제
    return value;
}

// 네이버 지도 API에서 좌표를 가져오는 함수
// get_coordinates와 calculate_distance를 통합
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

    cJSON_AddItemToObject(root, "weight", weight_array);

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
        const char* address = cJSON_GetObjectItem(item, "address")->valuestring;

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


int main() {
    // 네이버 API 키 정보 (네이버 개발자 센터에서 발급받은 값)
    const char* client_id = 'l10kq6x6md';
    const char* client_secret = 'B42VmUxX7qTtnmwcukOKBm9qKwu158D14VygAIUy';

    // result.json 파일에서 주소 데이터를 가져와 처리 및 거리 데이터 저장
    if (!process_addresses_and_save("results.json", client_id, client_secret, "거리.json")) {
        printf("Failed to process addresses and save distances.\n");
        return 1;
    }

    printf("Successfully processed addresses and saved distances to 거리.json.\n");
    return 0;
}