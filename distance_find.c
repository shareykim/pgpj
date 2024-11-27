#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "cJSON.h"

// 응답 데이터를 저장하기 위한 구조체
struct MemoryStruct {
    char *memory;
    size_t size;
};

// libcurl 데이터를 저장하는 콜백 함수
size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
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
int get_coordinates(const char *address, const char *client_id, const char *client_secret, char *latitude, char *longitude) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk = {0};

    char url[512];
    snprintf(url, sizeof(url), "https://naveropenapi.apigw.ntruss.com/map-geocode/v2/geocode?query=%s", address);

    struct curl_slist *headers = NULL;
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
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            return 0;
        }

        cJSON *json = cJSON_Parse(chunk.memory);
        if (!json) {
            printf("Error parsing JSON response.\n");
            return 0;
        }

        cJSON *addresses = cJSON_GetObjectItem(json, "addresses");
        if (cJSON_GetArraySize(addresses) > 0) {
            cJSON *first_address = cJSON_GetArrayItem(addresses, 0);
            strcpy(latitude, cJSON_GetObjectItem(first_address, "y")->valuestring);
            strcpy(longitude, cJSON_GetObjectItem(first_address, "x")->valuestring);
        } else {
            printf("No addresses found for '%s'\n", address);
            return 0;
        }

        cJSON_Delete(json);
        curl_easy_cleanup(curl);
        free(chunk.memory);
        return 1;
    }
    return 0;
}

// 두 좌표 간 거리를 계산하는 함수
double calculate_distance(const char *start_lat, const char *start_lon, const char *end_lat, const char *end_lon, const char *client_id, const char *client_secret) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk = {0};

    char url[512];
    snprintf(url, sizeof(url), "https://naveropenapi.apigw.ntruss.com/map-direction/v1/driving?start=%s,%s&goal=%s,%s&option=trafast",
             start_lon, start_lat, end_lon, end_lat);

    struct curl_slist *headers = NULL;
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
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            return -1;
        }

        cJSON *json = cJSON_Parse(chunk.memory);
        if (!json) {
            printf("Error parsing JSON response.\n");
            return -1;
        }

        cJSON *route = cJSON_GetObjectItem(json, "route");
        cJSON *trafast = cJSON_GetObjectItem(route, "trafast");
        if (cJSON_GetArraySize(trafast) > 0) {
            cJSON *summary = cJSON_GetObjectItem(cJSON_GetArrayItem(trafast, 0), "summary");
            double distance = cJSON_GetObjectItem(summary, "distance")->valuedouble / 1000.0; // km 단위로 변환
            cJSON_Delete(json);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            return distance;
        } else {
            printf("No routes found for (%s, %s) to (%s, %s)\n", start_lat, start_lon, end_lat, end_lon);
            return -1;
        }
    }
    return -1;
}

// JSON 파일에 거리 데이터를 저장하는 함수
void save_distances_to_json(const char *file_path, double **matrix, int size) {
    cJSON *root = cJSON_CreateObject();
    cJSON *weights = cJSON_CreateArray();

    for (int i = 0; i < size; i++) {
        cJSON *row = cJSON_CreateArray();
        for (int j = 0; j < size; j++) {
            cJSON_AddItemToArray(row, cJSON_CreateNumber(matrix[i][j]));
        }
        cJSON_AddItemToArray(weights, row);
    }

    cJSON_AddItemToObject(root, "weight", weights);

    FILE *file = fopen(file_path, "w");
    if (file) {
        char *json_str = cJSON_Print(root);
        fprintf(file, "%s\n", json_str);
        fclose(file);
        free(json_str);
    }
    cJSON_Delete(root);
    printf("Distance matrix saved to %s\n", file_path);
}

int main() {
    const char *client_id = "YOUR_CLIENT_ID";
    const char *client_secret = "YOUR_CLIENT_SECRET";

    // 예제 주소 리스트
    const char *addresses[] = {"강릉역", "경포대", "주문진"};
    int num_addresses = sizeof(addresses) / sizeof(addresses[0]);

    // 좌표 저장
    char latitudes[num_addresses][32];
    char longitudes[num_addresses][32];

    for (int i = 0; i < num_addresses; i++) {
        if (!get_coordinates(addresses[i], client_id, client_secret, latitudes[i], longitudes[i])) {
            printf("Failed to get coordinates for %s\n", addresses[i]);
            return 1;
        }
    }

    // 거리 행렬 생성
    double **distances = malloc(num_addresses * sizeof(double *));
    for (int i = 0; i < num_addresses; i++) {
        distances[i] = malloc(num_addresses * sizeof(double));
    }

    for (int i = 0; i < num_addresses; i++) {
        for (int j = 0; j < num_addresses; j++) {
            if (i == j) {
                distances[i][j] = 0;
            } else {
                distances[i][j] = calculate_distance(latitudes[i], longitudes[i], latitudes[j], longitudes[j], client_id, client_secret);
            }
        }
    }

    // JSON 파일로 저장
    save_distances_to_json("distances.json", distances, num_addresses);

    for (int i = 0; i < num_addresses; i++) {
        free(distances[i]);
    }
    free(distances);

    return 0;
}
