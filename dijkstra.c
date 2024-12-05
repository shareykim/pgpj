#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "parson.h" 
#include <locale.h>

#define TRUE 1
#define FALSE 0
#define MAX_VERTICES 100   
#define INF 1000000  

typedef struct GraphType {
    int n;
    double weight[MAX_VERTICES][MAX_VERTICES]; // 가중치 배열 (최대 100 노드)
    char categories[MAX_VERTICES]; // 카테고리 배열 추가
} GraphType;

// 함수 선언
void save_complete_path_to_json(GraphType* g, int start, int* complete_path, int path_length);
void load_graph(GraphType* g);
void load_categories(GraphType* g); // 카테고리 데이터 로드 함수
int choose_start_node(GraphType* g);
void shortest_path(GraphType* g, int start);
int choose_min(double distance[], int n, int found[]);
int get_path(int end, int* path);
void build_complete_path(GraphType* g, int start, int* complete_path, int* path_length);
void updateWeightMatrix(GraphType* g);
void printMatrix(GraphType* graph);

// 전역 변수 선언
double distance_arr[MAX_VERTICES];
int prev_arr[MAX_VERTICES];  // 이전 노드를 기록하는 배열
int found_arr[MAX_VERTICES];
int path[MAX_VERTICES];

// 그래프와 카테고리 데이터를 읽는 함수
void load_graph(GraphType* g) {
    JSON_Value* rootValue = json_parse_file("C:\\Users\\USER\\pgpj\\app_file\\distance.json");
    if (rootValue == NULL) {
        printf("no distance.json file.\n");
        exit(1); // 파일이 없으면 프로그램 종료
    }

    JSON_Object* rootObject = json_value_get_object(rootValue);
    JSON_Array* weightArray = json_object_get_array(rootObject, "weight");

    g->n = json_array_get_count(weightArray);

    // 가중치 배열을 g->weight에 실수 값으로 저장
    for (int i = 0; i < g->n; i++) {
        JSON_Array* rowArray = json_array_get_array(weightArray, i);
        for (int j = 0; j < g->n; j++) {
            JSON_Value* value = json_array_get_value(rowArray, j);
            if (json_value_get_type(value) == JSONString && strcmp(json_value_get_string(value), "INF") == 0) {
                g->weight[i][j] = INF;  // "INF" 문자열 값 처리
            }
            else {
                double num = json_value_get_number(value);
                // 0.0000을 INF로 처리, 단 자기 자신인 경우는 0 유지
                if (i != j && num == 0.0000) {
                    g->weight[i][j] = INF;
                }
                else {
                    g->weight[i][j] = num;  // 실수 값 처리
                }
            }
        }
    }

    // g->weight 배열 출력 (실수 값 소수점 4자리까지)
    printf("Weight Matrix:\n");
    for (int i = 0; i < g->n; i++) {
        for (int j = 0; j < g->n; j++) {
            if (g->weight[i][j] == INF) {
                printf("INF ");
            }
            else {
                printf("%.4f ", g->weight[i][j]);  // 실수값을 소수점 4자리까지 출력
            }
        }
        printf("\n");
    }

    json_value_free(rootValue);
}

// 카테고리 데이터를 로드하여 배열에 저장
void load_categories(GraphType* g) {
    // JSON 파일 파싱
    JSON_Value* rootValue = json_parse_file("C:\\Users\\USER\\pgpj\\results.json");
    if (rootValue == NULL) {
        printf("no results.json file.\n");
        exit(1); // 파일이 없으면 프로그램 종료
    }

    // JSON 배열 접근
    JSON_Array* itemArray = json_value_get_array(rootValue);
    if (itemArray == NULL) {
        printf("Invalid JSON structure.\n");
        json_value_free(rootValue);
        exit(1);
    }

    int category_count = json_array_get_count(itemArray);
    if (category_count > MAX_VERTICES) {
        printf("Category count exceeds MAX_VERTICES.\n");
        json_value_free(rootValue);
        exit(1);
    }

    // g->n을 유지 (이미 load_graph에서 설정됨)
    // 단, results.json의 노드 수가 distance.json과 일치해야 합니다.
    if (category_count != g->n) {
        printf("Category count does not match weight matrix size.\n");
        json_value_free(rootValue);
        exit(1);
    }

    // 각 항목에서 category 값 읽기
    for (int i = 0; i < g->n; i++) {
        JSON_Object* itemObject = json_array_get_object(itemArray, i);
        if (itemObject == NULL) {
            printf("Invalid item in JSON array at index %d.\n", i);
            g->categories[i] = '0'; // 기본값 설정
            continue;
        }

        const char* categoryValue = json_object_get_string(itemObject, "category");
        if (categoryValue == NULL) {
            printf("Category not found for item %d.\n", i);
            g->categories[i] = '0'; // 기본값 설정
        }
        else {
            g->categories[i] = categoryValue[0]; // '0' 또는 '1' 저장
        }
    }

    // 저장된 카테고리 출력
    printf("Categories: ");
    for (int i = 0; i < g->n; i++) {
        printf("%c ", g->categories[i]);
    }
    printf("\n");

    json_value_free(rootValue);
}

// 카테고리가 1인 노드끼리 INF로 설정하는 함수
void updateWeightMatrix(GraphType* g) {
    for (int i = 0; i < g->n; i++) {
        for (int j = 0; j < g->n; j++) {
            if (i != j && g->categories[i] == '1' && g->categories[j] == '1') {
                g->weight[i][j] = INF; // 카테고리가 1인 인덱스끼리는 INF
                printf("Set INF for category 1 nodes: %d to %d\n", i, j);
            }
        }
    }
}

// 가중치 행렬 출력 함수
void printMatrix(GraphType* graph) {
    printf("Weight Matrix:\n");
    for (int i = 0; i < graph->n; i++) {
        for (int j = 0; j < graph->n; j++) {
            if (graph->weight[i][j] == INF) {
                printf("INF\t");
            }
            else {
                printf("%.4f\t", graph->weight[i][j]);
            }
        }
        printf("\n");
    }
}

// 카테고리가 1인 노드 중 하나를 선택하는 함수
int choose_start_node(GraphType* g) {
    for (int i = 0; i < g->n; i++) {
        if (g->categories[i] == '1') {
            return i;  // 카테고리 1인 노드 중 첫 번째 노드를 선택
        }
    }
    return -1;
}

// 아직 방문하지 않은 노드 중 가장 작은 거리를 가진 노드를 선택하는 함수
int choose_min(double distance[], int n, int found[]) {
    int minIndex = -1;
    double minValue = INF;

    for (int i = 0; i < n; i++) {
        if (!found[i] && distance[i] < minValue) {
            minValue = distance[i];
            minIndex = i;
        }
    }

    return minIndex; // 가장 작은 거리의 노드를 반환
}

// 다익스트라 알고리즘 실행
void shortest_path(GraphType* g, int start) {
    int i, u, w;

    // prev[] 배열 초기화 (다익스트라 알고리즘 시작 전에)
    for (int i = 0; i < g->n; i++) {
        distance_arr[i] = g->weight[start][i];  // start 노드와 다른 노드들 사이의 초기 거리
        found_arr[i] = FALSE;                   // 방문 여부 배열
        prev_arr[i] = (g->weight[start][i] < INF && i != start) ? start : -1;  // 경로 추적을 위한 prev[] 초기화
    }

    found_arr[start] = TRUE;  // 시작 노드는 이미 방문했다고 설정
    distance_arr[start] = 0;  // 시작 노드의 거리는 0

    for (int i = 0; i < g->n - 1; i++) {
        // 최소 거리를 가진 노드 선택
        u = choose_min(distance_arr, g->n, found_arr);
        if (u == -1) break;  // 더 이상 갱신할 노드가 없으면 종료

        printf("Selected node: %d\n", u);
        found_arr[u] = TRUE;

        // 경로 갱신
        for (w = 0; w < g->n; w++) {
            if (g->weight[u][w] != INF && distance_arr[u] + g->weight[u][w] < distance_arr[w]) {
                distance_arr[w] = distance_arr[u] + g->weight[u][w];
                prev_arr[w] = u;  // 경로 추적을 위한 prev 배열 갱신
            }
        }
    }

    // 최단 경로와 prev 배열 출력 (디버깅 용도)
    printf("prev 배열: \n");
    for (int i = 0; i < g->n; i++) {
        printf("prev[%d] = %d\n", i, prev_arr[i]);
    }
}

// Helper function to get the path from start to end using the prev array
int get_path(int end, int* path) {
    int count = 0;
    int current = end;
    while (current != -1) {
        path[count++] = current;
        current = prev_arr[current];
    }
    // Reverse the path
    for (int i = 0; i < count / 2; i++) {
        int temp = path[i];
        path[i] = path[count - i - 1];
        path[count - i - 1] = temp;
    }
    return count;
}

// Function to build a complete path visiting all nodes using Dijkstra's algorithm
void build_complete_path(GraphType* g, int start, int* complete_path, int* path_length) {
    int visited[MAX_VERTICES] = { FALSE };
    int current = start;
    int count = 0;

    complete_path[count++] = current;
    visited[current] = TRUE;

    while (count < g->n) {
        // Temporarily set weights of visited nodes (except current) to INF to avoid passing through them
        double original_weights[MAX_VERTICES][MAX_VERTICES];
        for (int i = 0; i < g->n; i++) {
            for (int j = 0; j < g->n; j++) {
                original_weights[i][j] = g->weight[i][j];
                if (visited[i] && i != current) {
                    g->weight[i][j] = INF;
                }
            }
        }

        // Run Dijkstra's from current node
        shortest_path(g, current); // This populates distance_arr and prev_arr

        // Restore original weights
        for (int i = 0; i < g->n; i++) {
            for (int j = 0; j < g->n; j++) {
                g->weight[i][j] = original_weights[i][j];
            }
        }

        // Find the nearest unvisited node
        double min_distance = INF;
        int next_node = -1;
        for (int i = 0; i < g->n; i++) {
            if (!visited[i] && distance_arr[i] < min_distance) {
                min_distance = distance_arr[i];
                next_node = i;
            }
        }

        if (next_node == -1) {
            printf("Cannot reach remaining unvisited nodes from node %d.\n", current);
            break;
        }

        // Extract the path from current to next_node
        int temp_path[MAX_VERTICES];
        int temp_path_len = get_path(next_node, temp_path);

        // Check if the path starts with current node
        if (temp_path_len == 0 || temp_path[0] != current) {
            printf("Path does not start with current node. Skipping.\n");
            break;
        }

        // Append the path to complete_path, skipping the first node if it's already included
        for (int i = 1; i < temp_path_len; i++) {
            complete_path[count++] = temp_path[i];
            visited[temp_path[i]] = TRUE;
            if (count >= g->n) break;
        }

        // Update current node
        current = next_node;
    }

    *path_length = count;
}

// Function to save the complete path to JSON file
void save_complete_path_to_json(GraphType* g, int start, int* complete_path, int path_length) {
    FILE* fp = fopen("C:\\Users\\USER\\pgpj\\route.json", "w");
    if (fp == NULL) {
        printf("파일을 열 수 없습니다!\n");
        return;
    }

    fprintf(fp, "{\n");
    fprintf(fp, "  \"start\": %d,\n", start);
    fprintf(fp, "  \"path\": [");

    for (int i = 0; i < path_length; i++) {
        fprintf(fp, "%d", complete_path[i]);
        if (i != path_length - 1) {
            fprintf(fp, ", ");
        }
    }

    fprintf(fp, "]\n");
    fprintf(fp, "}\n");

    fclose(fp);
    printf("완전한 경로가 route.json 파일에 저장되었습니다.\n");
}

// 메인 함수
int main(void) {
    setlocale(LC_ALL, "ko_KR.UTF-8");

    GraphType g;

    // 데이터 로드
    load_graph(&g);
    load_categories(&g);
    updateWeightMatrix(&g);
    //printMatrix(&g);

    // 시작 노드 선택 (카테고리 1인 노드 중 하나)
    int start = choose_start_node(&g);  // 카테고리 1인 노드 중 하나를 시작점으로 선택

    if (start == -1) {
        printf("No category 1 node found, selecting category 0 node.\n");
        // 카테고리 1이 없으면 카테고리 0인 노드 중 하나를 선택
        for (int i = 0; i < g.n; i++) {
            if (g.categories[i] == '0') {
                start = i;
                break;
            }
        }
    }

    printf("start = %d\n", start);

    // Build complete path using Dijkstra's algorithm
    int complete_path[MAX_VERTICES];
    int path_length = 0;
    build_complete_path(&g, start, complete_path, &path_length);

    // 생성된 경로 출력 (디버깅 용도)
    printf("Complete Path: ");
    for (int i = 0; i < path_length; i++) {
        printf("%d ", complete_path[i]);
    }
    printf("\n");

    // 경로를 JSON 파일에 저장
    save_complete_path_to_json(&g, start, complete_path, path_length);

    return 0;
}
