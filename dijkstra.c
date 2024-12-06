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
    double weight[MAX_VERTICES][MAX_VERTICES]; // 揶쏉옙餓λ쵐?뒄 獄쏄퀣肉? (筌ㅼ뮆占쏙옙 100 占쎈걗占쎈굡)
    char categories[MAX_VERTICES]; // 燁삳똾??믤?⑥쥓?봺 獄쏄퀣肉? ?빊遺쏙옙占?
} GraphType;

// 占쎈맙占쎈땾 占쎄퐨占쎈섧
void save_complete_path_to_json(GraphType* g, int start, int* complete_path, int path_length);
void load_graph(GraphType* g);
void load_categories(GraphType* g); // 燁삳똾??믤?⑥쥓?봺 占쎈쑓占쎌뵠占쎄숲 嚥≪뮆諭? 占쎈맙占쎈땾
int choose_start_node(GraphType* g);
void shortest_path(GraphType* g, int start);
int choose_min(double distance[], int n, int found[]);
int get_path(int end, int* path);
void build_complete_path(GraphType* g, int start, int* complete_path, int* path_length);
void updateWeightMatrix(GraphType* g);
void printMatrix(GraphType* graph);

double distance_arr[MAX_VERTICES];
int prev_arr[MAX_VERTICES]; 
int found_arr[MAX_VERTICES];
int path[MAX_VERTICES];

void load_graph(GraphType* g) {
    JSON_Value* rootValue = json_parse_file("거리.json");
    if (rootValue == NULL) {
        printf("no 거리.json file.\n");
        exit(1); 
    }

    JSON_Object* rootObject = json_value_get_object(rootValue);
    JSON_Array* weightArray = json_object_get_array(rootObject, "weight");

    g->n = json_array_get_count(weightArray);

    for (int i = 0; i < g->n; i++) {
        JSON_Array* rowArray = json_array_get_array(weightArray, i);
        for (int j = 0; j < g->n; j++) {
            JSON_Value* value = json_array_get_value(rowArray, j);
            if (json_value_get_type(value) == JSONString && strcmp(json_value_get_string(value), "INF") == 0) {
                g->weight[i][j] = INF;  
            }
            else {
                double num = json_value_get_number(value);
                if (i != j && num == 0.0000) {
                    g->weight[i][j] = INF;
                }
                else {
                    g->weight[i][j] = num;  
                }
            }
        }
    }

    json_value_free(rootValue);
}

void load_categories(GraphType* g) {
    JSON_Value* rootValue = json_parse_file("results.json");
    if (rootValue == NULL) {
        printf("no results.json file.\n");
        exit(1);
    }

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

    if (category_count != g->n) {
        printf("Category count does not match weight matrix size.\n");
        json_value_free(rootValue);
        exit(1);
    }

    for (int i = 0; i < g->n; i++) {
        JSON_Object* itemObject = json_array_get_object(itemArray, i);
        if (itemObject == NULL) {
            printf("Invalid item in JSON array at index %d.\n", i);
            g->categories[i] = '0';
            continue;
        }

        const char* categoryValue = json_object_get_string(itemObject, "category");
        if (categoryValue == NULL) {
            printf("Category not found for item %d.\n", i);
            g->categories[i] = '0';
        }
        else {
            g->categories[i] = categoryValue[0];
        }
    }
    json_value_free(rootValue);
}

void updateWeightMatrix(GraphType* g) {
    for (int i = 0; i < g->n; i++) {
        for (int j = 0; j < g->n; j++) {
            if (i != j && g->categories[i] == '1' && g->categories[j] == '1') {
                g->weight[i][j] = INF;
                printf("Set INF for category 1 nodes: %d to %d\n", i, j);
            }
        }
    }
}

/*
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
*/

int choose_start_node(GraphType* g) {
    for (int i = 0; i < g->n; i++) {
        if (g->categories[i] == '1') {
            return i; 
        }
    }
    return -1;
}

// 최소 거리 노드 선택 함수
int choose_min(double distance[], int n, int found[]) {
    int minIndex = -1;
    double minValue = INF;

    for (int i = 0; i < n; i++) {
        if (!found[i] && distance[i] < minValue) {
            minValue = distance[i];
            minIndex = i;
        }
    }

    return minIndex; // 최소 거리 노드의 인덱스 반환
}

// 최단 경로 계산 함수 (다익스트라 알고리즘)
void shortest_path(GraphType* g, int start) {
    int i, u, w;

    // 초기화
    for (int i = 0; i < g->n; i++) {
        distance_arr[i] = g->weight[start][i];  // 시작 노드로부터의 거리 초기화
        found_arr[i] = FALSE;                 // 방문 여부 초기화   
        prev_arr[i] = (g->weight[start][i] < INF && i != start) ? start : -1;  
    }

    found_arr[start] = TRUE;  // 시작 노드 방문 표시
    distance_arr[start] = 0;  // 시작 노드 거리 0으로 설정

    for (int i = 0; i < g->n - 1; i++) {
        // 가장 짧은 거리를 가진 노드 선택
        u = choose_min(distance_arr, g->n, found_arr);
        if (u == -1) break;  // 더 이상 방문할 노드가 없으면 종료


        printf("Selected node: %d\n", u);
        found_arr[u] = TRUE;

        // 인접 노드 업데이트
        for (w = 0; w < g->n; w++) {
            if (g->weight[u][w] != INF && distance_arr[u] + g->weight[u][w] < distance_arr[w]) {
                distance_arr[w] = distance_arr[u] + g->weight[u][w];
                prev_arr[w] = u;  // 이전 노드 업데이트
            }
        }
    }
}

// 이전 노드 업데이트
int get_path(int end, int* path) {
    int count = 0;
    int current = end;
    while (current != -1) {
        path[count++] = current;
        current = prev_arr[current];
    }
    // 경로 역순으로 뒤집기
    for (int i = 0; i < count / 2; i++) {
        int temp = path[i];
        path[i] = path[count - i - 1];
        path[count - i - 1] = temp;
    }
    return count;
}

// 전체 경로 빌드 함수 (모든 노드를 방문)
void build_complete_path(GraphType* g, int start, int* complete_path, int* path_length) {
    int visited[MAX_VERTICES] = { FALSE };
    int current = start;
    int count = 0;

    complete_path[count++] = current;
    visited[current] = TRUE;

    while (count < g->n) {
          // 방문한 노드(현재 노드 제외)의 가중치를 INF로 설정하여 경로 재탐색 방지
        double original_weights[MAX_VERTICES][MAX_VERTICES];
        for (int i = 0; i < g->n; i++) {
            for (int j = 0; j < g->n; j++) {
                original_weights[i][j] = g->weight[i][j];
                if (visited[i] && i != current) {
                    g->weight[i][j] = INF;
                }
            }
        }

        // 현재 노드에서 다익스트라 알고리즘 실행
        shortest_path(g, current); // distance_arr와 prev_arr 업데이트

        // 원래 가중치 복원
        for (int i = 0; i < g->n; i++) {
            for (int j = 0; j < g->n; j++) {
                g->weight[i][j] = original_weights[i][j];
            }
        }

        // 가장 가까운 미방문 노드 찾기
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

        // 현재 노드에서 다음 노드로 가는 경로 추출
        int temp_path[MAX_VERTICES];
        int temp_path_len = get_path(next_node, temp_path);

        // 경로의 시작이 현재 노드인지 확인
        if (temp_path_len == 0 || temp_path[0] != current) {
            printf("Path does not start with current node. Skipping.\n");
            break;
        }

        // 경로를 전체 경로에 추가 (중복 방지)
        for (int i = 1; i < temp_path_len; i++) {
            complete_path[count++] = temp_path[i];
            visited[temp_path[i]] = TRUE;
            if (count >= g->n) break;
        }

        // 현재 노드 업데이트
        current = next_node;
    }

    *path_length = count;
}

// 전체 경로를 JSON 파일로 저장하는 함수
void save_complete_path_to_json(GraphType* g, int start, int* complete_path, int path_length) {
    FILE* fp = fopen("최적의_경로.json", "w");
    if (fp == NULL) {
        printf("파일 존재 안함\n");
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
}

// 프로그램의 메인 함수
int main(void) {
    setlocale(LC_ALL, "ko_KR.UTF-8");

    GraphType g;

    // 그래프 및 카테고리 정보 로드
    load_graph(&g);
    load_categories(&g);
    updateWeightMatrix(&g);
    //printMatrix(&g);  -> 필요 시 주석 해제하여 디버깅

    // 시작 노드 선택 (카테고리 1인 노드 중 첫 번째 노드)
    int start = choose_start_node(&g);  

    if (start == -1) {
        printf("No category 1 node found, selecting category 0 node.\n");
         // 카테고리 1인 노드가 없을 경우 카테고리 0인 노드 중 첫 번째 노드 선택
        for (int i = 0; i < g.n; i++) {
            if (g.categories[i] == '0') {
                start = i;
                break;
            }
        }
    }

    // 전체 경로 빌드 (모든 노드를 방문)
    int complete_path[MAX_VERTICES];
    int path_length = 0;
    build_complete_path(&g, start, complete_path, &path_length);

    // 전체 경로를 JSON 파일로 저장
    save_complete_path_to_json(&g, start, complete_path, path_length);

    return 0;
}