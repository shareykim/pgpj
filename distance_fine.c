#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "cJSON.h"

// API 요청 결과를 저장할 구조체
struct MemoryStruct {
    char *memory;
    size_t size;
};

// cURL 콜백 함수: 응답 데이터를 메모리에 저장
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        printf("Not enough memory!\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// 주소로 좌표 가져오기
int get_coordinates(const char *address, const char *client_id, const char *client_secret, char *lat, char *lon) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk = {malloc(1), 0};

    char url[512];
    snprintf(url, sizeof(url), "https://naveropenapi.apigw.ntruss.com/map-geocode/v2/geocode?query=%s", address);

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        char header_id[128], header_secret[128];
        snprintf(header_id, sizeof(header_id), "X-NCP-APIGW-API-KEY-ID: %s", client_id);
        snprintf(header_secret, sizeof(header_secret), "X-NCP-APIGW-API-KEY: %s", client_secret);
        headers = curl_slist_append(headers, header_id);
        headers = curl_slist_append(headers, header_secret);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            return 0;
        }

        // JSON 파싱
        cJSON *json = cJSON_Parse(chunk.memory);
        cJSON *addresses = cJSON_GetObjectItem(json, "addresses");
        if (cJSON_IsArray(addresses) && cJSON_GetArraySize(addresses) > 0) {
            cJSON *first_address = cJSON_GetArrayItem(addresses, 0);
            strcpy(lat, cJSON_GetObjectItem(first_address, "y")->valuestring);
            strcpy(lon, cJSON_GetObjectItem(first_address, "x")->valuestring);
            cJSON_Delete(json);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            return 1;
        } else {
            printf("Address not found: %s\n", address);
        }
        cJSON_Delete(json);
        curl_easy_cleanup(curl);
    }
    free(chunk.memory);
    return 0;
}

// 거리 계산
double calculate_distance(const char *start_lat, const char *start_lon, const char *end_lat, const char *end_lon, const char *client_id, const char *client_secret) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk = {malloc(1), 0};

    char url[512];
    snprintf(url, sizeof(url), "https://naveropenapi.apigw.ntruss.com/map-direction/v1/driving?start=%s,%s&goal=%s,%s&option=trafast", start_lon, start_lat, end_lon, end_lat);

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist *headers = NULL;
        char header_id[128], header_secret[128];
        snprintf(header_id, sizeof(header_id), "X-NCP-APIGW-API-KEY-ID: %s", client_id);
        snprintf(header_secret, sizeof(header_secret), "X-NCP-APIGW-API-KEY: %s", client_secret);
        headers = curl_slist_append(headers, header_id);
        headers = curl_slist_append(headers, header_secret);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            return -1;
        }

        // JSON 파싱
        cJSON *json = cJSON_Parse(chunk.memory);
        cJSON *route = cJSON_GetObjectItem(json, "route");
        cJSON *trafast = cJSON_GetObjectItem(route, "trafast");
        if (cJSON_IsArray(trafast) && cJSON_GetArraySize(trafast) > 0) {
            cJSON *summary = cJSON_GetObjectItem(cJSON_GetArrayItem(trafast, 0), "summary");
            double distance = cJSON_GetObjectItem(summary, "distance")->valuedouble / 1000.0;
            cJSON_Delete(json);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            return distance;
        }
        cJSON_Delete(json);
        curl_easy_cleanup(curl);
    }
    free(chunk.memory);
    return -1;
}

// 거리 행렬 계산
void calculate_distances_matrix(char coordinates[][2][32], int num_coordinates, double matrix[100][100], const char *client_id, const char *client_secret) {
    for (int i = 0; i < num_coordinates; i++) {
        for (int j = 0; j < num_coordinates; j++) {
            if (i == j) {
                matrix[i][j] = 0;
            } else {
                double distance = calculate_distance(coordinates[i][0], coordinates[i][1], coordinates[j][0], coordinates[j][1], client_id, client_secret);
                matrix[i][j] = distance;
            }
        }
    }
}

// 거리 데이터 JSON으로 저장
void save_distances_to_json(const char *file_path, double matrix[100][100], int num_coordinates) {
    FILE *file = fopen(file_path, "w");
    fprintf(file, "{\n\t\"weight\": [\n");
    for (int i = 0; i < num_coordinates; i++) {
        fprintf(file, "\t\t[");
        for (int j = 0; j < num_coordinates; j++) {
            fprintf(file, "%.2f", matrix[i][j]);
            if (j < num_coordinates - 1) fprintf(file, ", ");
        }
        fprintf(file, "]");
        if (i < num_coordinates - 1) fprintf(file, ",\n");
    }
    fprintf(file, "\n\t]\n}\n");
    fclose(file);
    printf("Distances saved to %s\n", file_path);
}

int main() {
    const char *client_id = "479rqju7wq";
    const char *client_secret = "Bf0dUPBBzbK55YwEb5f0zKFkjhPgu5Ugag7tHf6m";

    char coordinates[100][2][32]; // 최대 100개의 좌표
    int num_coordinates = 0;

    // 좌표 예시 추가 (수동으로 입력)
    strcpy(coordinates[num_coordinates][0], "37.5665"); // 위도
    strcpy(coordinates[num_coordinates++][1], "126.9780"); // 경도

    strcpy(coordinates[num_coordinates][0], "37.4563");
    strcpy(coordinates[num_coordinates++][1], "126.7052");

    double distances[100][100] = {0};
    calculate_distances_matrix(coordinates, num_coordinates, distances, client_id, client_secret);
    save_distances_to_json("distances.json", distances, num_coordinates);

    return 0;
}
