// 김현경 - 다익스트라 코드 구현, 최은지 - 구현된 다익스트라 코드에 식당 노드
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "parson.h" // JSON 파싱을 위한 라이브러리
#include <locale.h> // 지역 설정을 위한 헤더 파일

#define TRUE 1
#define FALSE 0
#define MAX_VERTICES 100  // 최대 노드 개수 정의
#define INF 1000000  // 무한대를 나타내는 상수 (가중치가 없는 경우 사용)

// 그래프 정보를 저장하는 구조체
typedef struct GraphType {
    int n; // 노드(정점)의 개수
    double weight[MAX_VERTICES][MAX_VERTICES]; // 인접 행렬 형태의 가중치 정보
    char categories[MAX_VERTICES]; // 각 노드의 카테고리 정보
} GraphType;

// 함수 선언
// 완전한 경로를 JSON 파일로 저장하는 함수
void save_complete_path_to_json(GraphType* g, int start, int* complete_path, int path_length); 
// 그래프 데이터를 JSON 파일에서 로드하는 함수
void load_graph(GraphType* g);
// 카테고리 데이터를 JSON 파일에서 로드하는 함수
void load_categories(GraphType* g); 
// 시작 노드를 선택하는 함수(특정 카테고리에 해당하는 노드 중 첫 번째 노드)
int choose_start_node(GraphType* g);  
// 다익스트라 알고리즘을 사용하여 최단 경로를 찾는 함수
void shortest_path(GraphType* g, int start);
// 아직 방문하지 않은 노드 중 최소 거리를 가진 노드를 선택하는 함수
int choose_min(double distance[], int n, int found[]);
// 최단 경로를 역추적하여 경로를 구성하는 함수
int get_path(int end, int* path);
// 모든 노드를 방문하는 완전한 경로를 구축하는 함수
void build_complete_path(GraphType* g, int start, int* complete_path, int* path_length);
// 특정 조건에 따라 가중치 행렬을 업데이트하는 함수(카테고리 1 처리)
void updateWeightMatrix(GraphType* g);
// 디버깅을 위해 가중치 행렬을 출력하는 함수 (현재 주석 처리됨)
//void printMatrix(GraphType* graph);

// 전역 변수 선언
double distance_arr[MAX_VERTICES]; // 각 노드까지의 최단 거리를 저장하는 배열
int prev_arr[MAX_VERTICES];  // 각 노드에 도달하기 위한 이전 노드를 저장하는 배열
int found_arr[MAX_VERTICES];  // 노드 방문 여부를 저장하는 배열
int path[MAX_VERTICES];  // 최종 경로를 저장하는 배열

// 그래프와 카테고리 데이터를 읽는 함수
void load_graph(GraphType* g) {
    // "거리.json" 파일을 파싱하여 루트 값 얻기
    JSON_Value* rootValue = json_parse_file("거리.json");
    // 파일이 존재하지 않으면 프로그램 종료
    if (rootValue == NULL) {
        printf("no 거리.json file.\n");
        exit(1); 
    }

    // JSON 루트 객체 가져오기
    JSON_Object* rootObject = json_value_get_object(rootValue);
    JSON_Array* weightArray = json_object_get_array(rootObject, "weight");

    // 노드 개수 설정
    g->n = json_array_get_count(weightArray);

    // 가중치 배열 초기화
    for (int i = 0; i < g->n; i++) {
        JSON_Array* rowArray = json_array_get_array(weightArray, i);
        for (int j = 0; j < g->n; j++) {
            JSON_Value* value = json_array_get_value(rowArray, j);
            // "INF" 문자열을 만나면 해당 가중치를 INF로 설정
            if (json_value_get_type(value) == JSONString && strcmp(json_value_get_string(value), "INF") == 0) {
                g->weight[i][j] = INF;  
            }
            else {
                double num = json_value_get_number(value);
                // 대각 원소가 아닌데 0.0000이면 INF 처리 (경로 없음)
                if (i != j && num == 0.0000) {
                    g->weight[i][j] = INF;
                } else {
                    g->weight[i][j] = num;  
                }
            }
        }
    }

    json_value_free(rootValue);
}

void load_categories(GraphType* g) {
    // "results.json" 파일 파싱
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
    // 카테고리 수가 MAX_VERTICES를 초과하면 에러
    if (category_count > MAX_VERTICES) {
        printf("Category count exceeds MAX_VERTICES.\n");
        json_value_free(rootValue);
        exit(1);
    }

    // 노드 수와 카테고리 수 불일치 시 에러
    if (category_count != g->n) {
        printf("Category count does not match weight matrix size.\n");
        json_value_free(rootValue);
        exit(1);
    }

    // 각 노드에 해당하는 카테고리 정보를 가져와서 g->categories에 저장
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
            g->categories[i] = '0';
        } else {
            // categoryValue의 첫 글자를 저장
            g->categories[i] = categoryValue[0];
        }
    }
    json_value_free(rootValue);
}

// 카테고리가 '1'인 노드들끼리는 경로를 INF로 설정 (예: 서로 이동 불가능 처리)
void updateWeightMatrix(GraphType* g) {
    for (int i = 0; i < g->n; i++) {
        for (int j = 0; j < g->n; j++) {
            // i와 j 모두 카테고리가 '1'이면 이동 불가 처리
            if (i != j && g->categories[i] == '1' && g->categories[j] == '1') {
                g->weight[i][j] = INF;
                printf("Set INF for category 1 nodes: %d to %d\n", i, j);
            }
        }
    }
}

/*
// 디버깅용 가중치 행렬 출력 함수
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

// 시작 노드 선택 함수
// 그래프 내에서 카테고리가 '1'인 노드를 찾아 그 중 첫 번째 노드의 인덱스를 반환
// 없으면 -1 반환
int choose_start_node(GraphType* g) {
    for (int i = 0; i < g->n; i++) {
        if (g->categories[i] == '1') {
            return i; 
        }
    }
    return -1;
}

// 최소 거리 노드를 선택하는 함수 (다익스트라 알고리즘에서 사용)
int choose_min(double distance[], int n, int found[]) {
    int minIndex = -1;
    double minValue = INF;

    // 방문하지 않은 노드 중 가장 거리가 짧은 노드 선택
    for (int i = 0; i < n; i++) {
        if (!found[i] && distance[i] < minValue) {
            minValue = distance[i];
            minIndex = i;
        }
    }

    return minIndex; // 최소 거리 노드의 인덱스 반환
}

// 다익스트라 알고리즘을 통한 최단 경로 계산 함수
void shortest_path(GraphType* g, int start) {
    int i, u, w;

    // 초기화: 시작 노드로부터의 거리, 방문 여부, 이전 노드 설정
    for (int i = 0; i < g->n; i++) {
        distance_arr[i] = g->weight[start][i];  
        found_arr[i] = FALSE;                 
        prev_arr[i] = (g->weight[start][i] < INF && i != start) ? start : -1;  
    }

    found_arr[start] = TRUE;  // 시작 노드 방문 표시
    distance_arr[start] = 0;  // 시작 노드 거리 0

    // 노드 수 - 1번 반복
    for (int i = 0; i < g->n - 1; i++) {
        // 방문하지 않은 노드 중 최소 거리 노드 u 선택
        u = choose_min(distance_arr, g->n, found_arr);
        if (u == -1) break;  // 방문할 노드 없으면 종료

        printf("Selected node: %d\n", u);
        found_arr[u] = TRUE;

        // u에 인접한 노드 w에 대해 거리 갱신
        for (w = 0; w < g->n; w++) {
            if (g->weight[u][w] != INF && distance_arr[u] + g->weight[u][w] < distance_arr[w]) {
                distance_arr[w] = distance_arr[u] + g->weight[u][w];
                prev_arr[w] = u;  // 이전 노드 갱신
            }
        }
    }
}

// 최단 경로 역추적 함수
// end 노드로부터 prev_arr를 이용해 시작 지점까지 거슬러 올라가며 경로 구성
// count 반환 (경로 길이)
int get_path(int end, int* path) {
    int count = 0;
    int current = end;
    while (current != -1) {
        path[count++] = current;
        current = prev_arr[current];
    }
    // 역순으로 들어갔으므로 뒤집어 정렬
    for (int i = 0; i < count / 2; i++) {
        int temp = path[i];
        path[i] = path[count - i - 1];
        path[count - i - 1] = temp;
    }
    return count;
}

// 모든 노드를 방문하는 완전한 경로 구축 함수
// 현재 노드에서 다익스트라 실행 -> 최소 거리의 미방문 노드 선택 -> 경로 연결 반복
void build_complete_path(GraphType* g, int start, int* complete_path, int* path_length) {
    int visited[MAX_VERTICES] = { FALSE };
    int current = start;
    int count = 0;

    complete_path[count++] = current;
    visited[current] = TRUE;

    while (count < g->n) {
        // 이미 방문한 노드(현재 노드 제외)의 가중치를 임시로 INF 처리하여 다시 선택되지 않게 함
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

        // 다음 방문할 미방문 노드 중 최소 거리 노드 선택
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

        // 현재 노드 -> next_node 경로 추출
        int temp_path[MAX_VERTICES];
        int temp_path_len = get_path(next_node, temp_path);

        // 경로가 정상적으로 current에서 시작하는지 확인
        if (temp_path_len == 0 || temp_path[0] != current) {
            printf("Path does not start with current node. Skipping.\n");
            break;
        }

        // 추출한 경로를 complete_path에 이어붙이되, 시작 노드 중복 방지를 위해 i=1부터 추가
        for (int i = 1; i < temp_path_len; i++) {
            complete_path[count++] = temp_path[i];
            visited[temp_path[i]] = TRUE;
            if (count >= g->n) break;
        }

        // 현재 노드를 next_node로 업데이트
        current = next_node;
    }

    *path_length = count;
}

// 최종적으로 완전한 경로를 "최적의_경로.json" 파일로 저장하는 함수
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

// 메인 함수
int main(void) {
    setlocale(LC_ALL, "ko_KR.UTF-8");

    GraphType g;

    // 그래프 및 카테고리 정보 로드
    load_graph(&g);
    load_categories(&g);
    updateWeightMatrix(&g);
    //printMatrix(&g);  // 필요 시 주석 해제

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

    // 전체 경로 빌드 (모든 노드를 방문하는 경로)
    int complete_path[MAX_VERTICES];
    int path_length = 0;
    build_complete_path(&g, start, complete_path, &path_length);

    // 결과를 JSON 파일로 저장
    save_complete_path_to_json(&g, start, complete_path, path_length);

    return 0;
}