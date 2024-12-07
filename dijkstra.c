// ������ - ���ͽ�Ʈ�� �ڵ� ����, ������ - ������ ���ͽ�Ʈ�� �ڵ忡 �Ĵ� ���
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "parson.h" // JSON �Ľ��� ���� ���̺귯��
#include <locale.h> // ���� ������ ���� ��� ����

#define TRUE 1
#define FALSE 0
#define MAX_VERTICES 100  // �ִ� ��� ���� ����
#define INF 1000000  // ���Ѵ븦 ��Ÿ���� ��� (����ġ�� ���� ��� ���)

// �׷��� ������ �����ϴ� ����ü
typedef struct GraphType {
    int n; // ���(����)�� ����
    double weight[MAX_VERTICES][MAX_VERTICES]; // ���� ��� ������ ����ġ ����
    char categories[MAX_VERTICES]; // �� ����� ī�װ� ����
} GraphType;

// �Լ� ����
// ������ ��θ� JSON ���Ϸ� �����ϴ� �Լ�
void save_complete_path_to_json(GraphType* g, int start, int* complete_path, int path_length); 
// �׷��� �����͸� JSON ���Ͽ��� �ε��ϴ� �Լ�
void load_graph(GraphType* g);
// ī�װ� �����͸� JSON ���Ͽ��� �ε��ϴ� �Լ�
void load_categories(GraphType* g); 
// ���� ��带 �����ϴ� �Լ�(Ư�� ī�װ��� �ش��ϴ� ��� �� ù ��° ���)
int choose_start_node(GraphType* g);  
// ���ͽ�Ʈ�� �˰����� ����Ͽ� �ִ� ��θ� ã�� �Լ�
void shortest_path(GraphType* g, int start);
// ���� �湮���� ���� ��� �� �ּ� �Ÿ��� ���� ��带 �����ϴ� �Լ�
int choose_min(double distance[], int n, int found[]);
// �ִ� ��θ� �������Ͽ� ��θ� �����ϴ� �Լ�
int get_path(int end, int* path);
// ��� ��带 �湮�ϴ� ������ ��θ� �����ϴ� �Լ�
void build_complete_path(GraphType* g, int start, int* complete_path, int* path_length);
// Ư�� ���ǿ� ���� ����ġ ����� ������Ʈ�ϴ� �Լ�(ī�װ� 1 ó��)
void updateWeightMatrix(GraphType* g);
// ������� ���� ����ġ ����� ����ϴ� �Լ� (���� �ּ� ó����)
//void printMatrix(GraphType* graph);

// ���� ���� ����
double distance_arr[MAX_VERTICES]; // �� �������� �ִ� �Ÿ��� �����ϴ� �迭
int prev_arr[MAX_VERTICES];  // �� ��忡 �����ϱ� ���� ���� ��带 �����ϴ� �迭
int found_arr[MAX_VERTICES];  // ��� �湮 ���θ� �����ϴ� �迭
int path[MAX_VERTICES];  // ���� ��θ� �����ϴ� �迭

// �׷����� ī�װ� �����͸� �д� �Լ�
void load_graph(GraphType* g) {
    // "�Ÿ�.json" ������ �Ľ��Ͽ� ��Ʈ �� ���
    JSON_Value* rootValue = json_parse_file("�Ÿ�.json");
    // ������ �������� ������ ���α׷� ����
    if (rootValue == NULL) {
        printf("no �Ÿ�.json file.\n");
        exit(1); 
    }

    // JSON ��Ʈ ��ü ��������
    JSON_Object* rootObject = json_value_get_object(rootValue);
    JSON_Array* weightArray = json_object_get_array(rootObject, "weight");

    // ��� ���� ����
    g->n = json_array_get_count(weightArray);

    // ����ġ �迭 �ʱ�ȭ
    for (int i = 0; i < g->n; i++) {
        JSON_Array* rowArray = json_array_get_array(weightArray, i);
        for (int j = 0; j < g->n; j++) {
            JSON_Value* value = json_array_get_value(rowArray, j);
            // "INF" ���ڿ��� ������ �ش� ����ġ�� INF�� ����
            if (json_value_get_type(value) == JSONString && strcmp(json_value_get_string(value), "INF") == 0) {
                g->weight[i][j] = INF;  
            }
            else {
                double num = json_value_get_number(value);
                // �밢 ���Ұ� �ƴѵ� 0.0000�̸� INF ó�� (��� ����)
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
    // "results.json" ���� �Ľ�
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
    // ī�װ� ���� MAX_VERTICES�� �ʰ��ϸ� ����
    if (category_count > MAX_VERTICES) {
        printf("Category count exceeds MAX_VERTICES.\n");
        json_value_free(rootValue);
        exit(1);
    }

    // ��� ���� ī�װ� �� ����ġ �� ����
    if (category_count != g->n) {
        printf("Category count does not match weight matrix size.\n");
        json_value_free(rootValue);
        exit(1);
    }

    // �� ��忡 �ش��ϴ� ī�װ� ������ �����ͼ� g->categories�� ����
    for (int i = 0; i < g->n; i++) {
        JSON_Object* itemObject = json_array_get_object(itemArray, i);
        if (itemObject == NULL) {
            printf("Invalid item in JSON array at index %d.\n", i);
            g->categories[i] = '0'; // �⺻�� ����
            continue;
        }

        const char* categoryValue = json_object_get_string(itemObject, "category");
        if (categoryValue == NULL) {
            printf("Category not found for item %d.\n", i);
            g->categories[i] = '0';
        } else {
            // categoryValue�� ù ���ڸ� ����
            g->categories[i] = categoryValue[0];
        }
    }
    json_value_free(rootValue);
}

// ī�װ��� '1'�� ���鳢���� ��θ� INF�� ���� (��: ���� �̵� �Ұ��� ó��)
void updateWeightMatrix(GraphType* g) {
    for (int i = 0; i < g->n; i++) {
        for (int j = 0; j < g->n; j++) {
            // i�� j ��� ī�װ��� '1'�̸� �̵� �Ұ� ó��
            if (i != j && g->categories[i] == '1' && g->categories[j] == '1') {
                g->weight[i][j] = INF;
                printf("Set INF for category 1 nodes: %d to %d\n", i, j);
            }
        }
    }
}

/*
// ������ ����ġ ��� ��� �Լ�
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

// ���� ��� ���� �Լ�
// �׷��� ������ ī�װ��� '1'�� ��带 ã�� �� �� ù ��° ����� �ε����� ��ȯ
// ������ -1 ��ȯ
int choose_start_node(GraphType* g) {
    for (int i = 0; i < g->n; i++) {
        if (g->categories[i] == '1') {
            return i; 
        }
    }
    return -1;
}

// �ּ� �Ÿ� ��带 �����ϴ� �Լ� (���ͽ�Ʈ�� �˰��򿡼� ���)
int choose_min(double distance[], int n, int found[]) {
    int minIndex = -1;
    double minValue = INF;

    // �湮���� ���� ��� �� ���� �Ÿ��� ª�� ��� ����
    for (int i = 0; i < n; i++) {
        if (!found[i] && distance[i] < minValue) {
            minValue = distance[i];
            minIndex = i;
        }
    }

    return minIndex; // �ּ� �Ÿ� ����� �ε��� ��ȯ
}

// ���ͽ�Ʈ�� �˰����� ���� �ִ� ��� ��� �Լ�
void shortest_path(GraphType* g, int start) {
    int i, u, w;

    // �ʱ�ȭ: ���� ���κ����� �Ÿ�, �湮 ����, ���� ��� ����
    for (int i = 0; i < g->n; i++) {
        distance_arr[i] = g->weight[start][i];  
        found_arr[i] = FALSE;                 
        prev_arr[i] = (g->weight[start][i] < INF && i != start) ? start : -1;  
    }

    found_arr[start] = TRUE;  // ���� ��� �湮 ǥ��
    distance_arr[start] = 0;  // ���� ��� �Ÿ� 0

    // ��� �� - 1�� �ݺ�
    for (int i = 0; i < g->n - 1; i++) {
        // �湮���� ���� ��� �� �ּ� �Ÿ� ��� u ����
        u = choose_min(distance_arr, g->n, found_arr);
        if (u == -1) break;  // �湮�� ��� ������ ����

        printf("Selected node: %d\n", u);
        found_arr[u] = TRUE;

        // u�� ������ ��� w�� ���� �Ÿ� ����
        for (w = 0; w < g->n; w++) {
            if (g->weight[u][w] != INF && distance_arr[u] + g->weight[u][w] < distance_arr[w]) {
                distance_arr[w] = distance_arr[u] + g->weight[u][w];
                prev_arr[w] = u;  // ���� ��� ����
            }
        }
    }
}

// �ִ� ��� ������ �Լ�
// end ���κ��� prev_arr�� �̿��� ���� �������� �Ž��� �ö󰡸� ��� ����
// count ��ȯ (��� ����)
int get_path(int end, int* path) {
    int count = 0;
    int current = end;
    while (current != -1) {
        path[count++] = current;
        current = prev_arr[current];
    }
    // �������� �����Ƿ� ������ ����
    for (int i = 0; i < count / 2; i++) {
        int temp = path[i];
        path[i] = path[count - i - 1];
        path[count - i - 1] = temp;
    }
    return count;
}

// ��� ��带 �湮�ϴ� ������ ��� ���� �Լ�
// ���� ��忡�� ���ͽ�Ʈ�� ���� -> �ּ� �Ÿ��� �̹湮 ��� ���� -> ��� ���� �ݺ�
void build_complete_path(GraphType* g, int start, int* complete_path, int* path_length) {
    int visited[MAX_VERTICES] = { FALSE };
    int current = start;
    int count = 0;

    complete_path[count++] = current;
    visited[current] = TRUE;

    while (count < g->n) {
        // �̹� �湮�� ���(���� ��� ����)�� ����ġ�� �ӽ÷� INF ó���Ͽ� �ٽ� ���õ��� �ʰ� ��
        double original_weights[MAX_VERTICES][MAX_VERTICES];
        for (int i = 0; i < g->n; i++) {
            for (int j = 0; j < g->n; j++) {
                original_weights[i][j] = g->weight[i][j];
                if (visited[i] && i != current) {
                    g->weight[i][j] = INF;
                }
            }
        }

        // ���� ��忡�� ���ͽ�Ʈ�� �˰��� ����
        shortest_path(g, current); // distance_arr�� prev_arr ������Ʈ

        // ���� ����ġ ����
        for (int i = 0; i < g->n; i++) {
            for (int j = 0; j < g->n; j++) {
                g->weight[i][j] = original_weights[i][j];
            }
        }

        // ���� �湮�� �̹湮 ��� �� �ּ� �Ÿ� ��� ����
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

        // ���� ��� -> next_node ��� ����
        int temp_path[MAX_VERTICES];
        int temp_path_len = get_path(next_node, temp_path);

        // ��ΰ� ���������� current���� �����ϴ��� Ȯ��
        if (temp_path_len == 0 || temp_path[0] != current) {
            printf("Path does not start with current node. Skipping.\n");
            break;
        }

        // ������ ��θ� complete_path�� �̾���̵�, ���� ��� �ߺ� ������ ���� i=1���� �߰�
        for (int i = 1; i < temp_path_len; i++) {
            complete_path[count++] = temp_path[i];
            visited[temp_path[i]] = TRUE;
            if (count >= g->n) break;
        }

        // ���� ��带 next_node�� ������Ʈ
        current = next_node;
    }

    *path_length = count;
}

// ���������� ������ ��θ� "������_���.json" ���Ϸ� �����ϴ� �Լ�
void save_complete_path_to_json(GraphType* g, int start, int* complete_path, int path_length) {
    FILE* fp = fopen("������_���.json", "w");
    if (fp == NULL) {
        printf("���� ���� ����\n");
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

// ���� �Լ�
int main(void) {
    setlocale(LC_ALL, "ko_KR.UTF-8");

    GraphType g;

    // �׷��� �� ī�װ� ���� �ε�
    load_graph(&g);
    load_categories(&g);
    updateWeightMatrix(&g);
    //printMatrix(&g);  // �ʿ� �� �ּ� ����

    // ���� ��� ���� (ī�װ� 1�� ��� �� ù ��° ���)
    int start = choose_start_node(&g);  

    if (start == -1) {
        printf("No category 1 node found, selecting category 0 node.\n");
        // ī�װ� 1�� ��尡 ���� ��� ī�װ� 0�� ��� �� ù ��° ��� ����
        for (int i = 0; i < g.n; i++) {
            if (g.categories[i] == '0') {
                start = i;
                break;
            }
        }
    }

    // ��ü ��� ���� (��� ��带 �湮�ϴ� ���)
    int complete_path[MAX_VERTICES];
    int path_length = 0;
    build_complete_path(&g, start, complete_path, &path_length);

    // ����� JSON ���Ϸ� ����
    save_complete_path_to_json(&g, start, complete_path, path_length);

    return 0;
}