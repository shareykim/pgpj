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
    double weight[MAX_VERTICES][MAX_VERTICES]; // åª›ï¿½ä»¥ë¬’?Š‚ è«›ê³—ë¿? (ï§¤ì’•ï¿½ï¿½ 100 ï¿½ë‚ï¿½ë±¶)
    char categories[MAX_VERTICES]; // ç§»ëŒ„??’æ?¨ì¢Š?” è«›ê³—ë¿? ?•°ë¶½ï¿½ï¿?
} GraphType;

// ï¿½ë¸¿ï¿½ë‹” ï¿½ê½‘ï¿½ë¼µ
void save_complete_path_to_json(GraphType* g, int start, int* complete_path, int path_length);
void load_graph(GraphType* g);
void load_categories(GraphType* g); // ç§»ëŒ„??’æ?¨ì¢Š?” ï¿½ëœ²ï¿½ì” ï¿½ê½£ æ¿¡ì’•ë±? ï¿½ë¸¿ï¿½ë‹”
int choose_start_node(GraphType* g);
void shortest_path(GraphType* g, int start);
int choose_min(double distance[], int n, int found[]);
int get_path(int end, int* path);
void build_complete_path(GraphType* g, int start, int* complete_path, int* path_length);
void updateWeightMatrix(GraphType* g);
void printMatrix(GraphType* graph);

// ï¿½ìŸ¾ï¿½ë¿­ è¹‚ï¿½ï¿½ë‹” ï¿½ê½‘ï¿½ë¼µ
double distance_arr[MAX_VERTICES];
int prev_arr[MAX_VERTICES];  // ï¿½ì” ï¿½ìŸ¾ ï¿½ë‚ï¿½ë±¶?‘œï¿? æ¹²ê³•ì¤‰ï¿½ë¸?ï¿½ë’— è«›ê³—ë¿?
int found_arr[MAX_VERTICES];
int path[MAX_VERTICES];

// æ´¹ëªƒ?˜’ï¿½ë´½ï¿½ï¿½ï¿? ç§»ëŒ„??’æ?¨ì¢Š?” ï¿½ëœ²ï¿½ì” ï¿½ê½£?‘œï¿? ï¿½ì”«ï¿½ë’— ï¿½ë¸¿ï¿½ë‹”
void load_graph(GraphType* g) {
    JSON_Value* rootValue = json_parse_file("°Å¸®.json");
    if (rootValue == NULL) {
        printf("no °Å¸®.json file.\n");
        exit(1); // ï¿½ë™†ï¿½ì”ªï¿½ì”  ï¿½ë¾¾ï¿½ì‘ï§ï¿½ ï¿½ë´½æ¿¡ì’“? ‡ï¿½ì˜© ?†«?‚…ì¦?
    }

    JSON_Object* rootObject = json_value_get_object(rootValue);
    JSON_Array* weightArray = json_object_get_array(rootObject, "weight");

    g->n = json_array_get_count(weightArray);

    // åª›ï¿½ä»¥ë¬’?Š‚ è«›ê³—ë¿´ï¿½?“£ g->weightï¿½ë¿‰ ï¿½ë–ï¿½ë‹” åª›ë?ªì‘æ¿¡ï¿½ ï¿½ï¿½ï¿½ï¿½?˜£
    for (int i = 0; i < g->n; i++) {
        JSON_Array* rowArray = json_array_get_array(weightArray, i);
        for (int j = 0; j < g->n; j++) {
            JSON_Value* value = json_array_get_value(rowArray, j);
            if (json_value_get_type(value) == JSONString && strcmp(json_value_get_string(value), "INF") == 0) {
                g->weight[i][j] = INF;  // "INF" ?‡¾ëª„ì˜„ï¿½ë¿´ åª›ï¿½ ï§£ì„?”
            }
            else {
                double num = json_value_get_number(value);
                // 0.0000ï¿½ì“£ INFæ¿¡ï¿½ ï§£ì„?”, ï¿½ë–’ ï¿½ì˜„æ¹²ï¿½ ï¿½ì˜„ï¿½ë–Šï¿½ì”¤ å¯ƒìŒ?Š¦ï¿½ë’— 0 ï¿½ì??ï§ï¿½
                if (i != j && num == 0.0000) {
                    g->weight[i][j] = INF;
                }
                else {
                    g->weight[i][j] = num;  // ï¿½ë–ï¿½ë‹” åª›ï¿½ ï§£ì„?”
                }
            }
        }
    }

    // g->weight è«›ê³—ë¿? ?•°?’•? ° (ï¿½ë–ï¿½ë‹” åª›ï¿½ ï¿½ëƒ¼ï¿½ë‹”ï¿½ì  4ï¿½ì˜„?”±?ˆí‰´ï§ï¿½)
    printf("Weight Matrix:\n");
    for (int i = 0; i < g->n; i++) {
        for (int j = 0; j < g->n; j++) {
            if (g->weight[i][j] == INF) {
                printf("INF ");
            }
            else {
                printf("%.4f ", g->weight[i][j]);  // ï¿½ë–ï¿½ë‹”åª›ë?ªì“£ ï¿½ëƒ¼ï¿½ë‹”ï¿½ì  4ï¿½ì˜„?”±?ˆí‰´ï§ï¿½ ?•°?’•? °
            }
        }
        printf("\n");
    }

    json_value_free(rootValue);
}

// ç§»ëŒ„??’æ?¨ì¢Š?” ï¿½ëœ²ï¿½ì” ï¿½ê½£?‘œï¿? æ¿¡ì’•ë±¶ï¿½ë¸?ï¿½ë¿¬ è«›ê³—ë¿´ï¿½ë¿? ï¿½ï¿½ï¿½ï¿½?˜£
void load_categories(GraphType* g) {
    // JSON ï¿½ë™†ï¿½ì”ª ï¿½ë™†ï¿½ë–›
    JSON_Value* rootValue = json_parse_file("results.json");
    if (rootValue == NULL) {
        printf("no results.json file.\n");
        exit(1); // ï¿½ë™†ï¿½ì”ªï¿½ì”  ï¿½ë¾¾ï¿½ì‘ï§ï¿½ ï¿½ë´½æ¿¡ì’“? ‡ï¿½ì˜© ?†«?‚…ì¦?
    }

    // JSON è«›ê³—ë¿? ï¿½ì ’æ´¹ï¿½
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

    // g->nï¿½ì“£ ï¿½ì??ï§ï¿½ (ï¿½ì” èª˜ï¿½ load_graphï¿½ë¿‰ï¿½ê½Œ ï¿½ê½•ï¿½ì ™ï¿½ë§–)
    // ï¿½ë–’, results.jsonï¿½ì“½ ï¿½ë‚ï¿½ë±¶ ï¿½ë‹”åª›ï¿½ distance.json??¨ï¿½ ï¿½ì”ªç§»ì„‘ë¹ï¿½ë¹? ï¿½ë??ï¿½ë•²ï¿½ë–.
    if (category_count != g->n) {
        printf("Category count does not match weight matrix size.\n");
        json_value_free(rootValue);
        exit(1);
    }

    // åª›ï¿½ ï¿½ë¹†ï§â‘¹ë¿‰ï¿½ê½? category åª›ï¿½ ï¿½ì”«æ¹²ï¿½
    for (int i = 0; i < g->n; i++) {
        JSON_Object* itemObject = json_array_get_object(itemArray, i);
        if (itemObject == NULL) {
            printf("Invalid item in JSON array at index %d.\n", i);
            g->categories[i] = '0'; // æ¹²ê³•?‚¯åª›ï¿½ ï¿½ê½•ï¿½ì ™
            continue;
        }

        const char* categoryValue = json_object_get_string(itemObject, "category");
        if (categoryValue == NULL) {
            printf("Category not found for item %d.\n", i);
            g->categories[i] = '0'; // æ¹²ê³•?‚¯åª›ï¿½ ï¿½ê½•ï¿½ì ™
        }
        else {
            g->categories[i] = categoryValue[0]; // '0' ï¿½ì‚‰ï¿½ë’— '1' ï¿½ï¿½ï¿½ï¿½?˜£
        }
    }

    // ï¿½ï¿½ï¿½ï¿½?˜£ï¿½ë§‚ ç§»ëŒ„??’æ?¨ì¢Š?” ?•°?’•? °
    printf("Categories: ");
    for (int i = 0; i < g->n; i++) {
        printf("%c ", g->categories[i]);
    }
    printf("\n");

    json_value_free(rootValue);
}

// ç§»ëŒ„??’æ?¨ì¢Š?”åª›ï¿½ 1ï¿½ì”¤ ï¿½ë‚ï¿½ë±¶ï¿½ê²®?”±ï¿? INFæ¿¡ï¿½ ï¿½ê½•ï¿½ì ™ï¿½ë¸¯ï¿½ë’— ï¿½ë¸¿ï¿½ë‹”
void updateWeightMatrix(GraphType* g) {
    for (int i = 0; i < g->n; i++) {
        for (int j = 0; j < g->n; j++) {
            if (i != j && g->categories[i] == '1' && g->categories[j] == '1') {
                g->weight[i][j] = INF; // ç§»ëŒ„??’æ?¨ì¢Š?”åª›ï¿½ 1ï¿½ì”¤ ï¿½ì”¤ï¿½ëœ³ï¿½ë’ªï¿½ê²®?”±?‰ë’— INF
                printf("Set INF for category 1 nodes: %d to %d\n", i, j);
            }
        }
    }
}

// åª›ï¿½ä»¥ë¬’?Š‚ ï¿½ë»¾ï¿½ì ¹ ?•°?’•? ° ï¿½ë¸¿ï¿½ë‹”
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

// ç§»ëŒ„??’æ?¨ì¢Š?”åª›ï¿½ 1ï¿½ì”¤ ï¿½ë‚ï¿½ë±¶ ä»¥ï¿½ ï¿½ë¸¯ï¿½êµ¹?‘œï¿? ï¿½ê½‘ï¿½ê¹®ï¿½ë¸¯ï¿½ë’— ï¿½ë¸¿ï¿½ë‹”
int choose_start_node(GraphType* g) {
    for (int i = 0; i < g->n; i++) {
        if (g->categories[i] == '1') {
            return i;  // ç§»ëŒ„??’æ?¨ì¢Š?” 1ï¿½ì”¤ ï¿½ë‚ï¿½ë±¶ ä»¥ï¿½ ï§£ï¿½ è¸°ë‰? ï¿½ë‚ï¿½ë±¶?‘œï¿? ï¿½ê½‘ï¿½ê¹®
        }
    }
    return -1;
}

// ï¿½ë¸˜ï§ï¿½ è«›â‘¸Ğ¦ï¿½ë¸¯ï§ï¿½ ï¿½ë¸¡ï¿½ï¿½ï¿? ï¿½ë‚ï¿½ë±¶ ä»¥ï¿½ åª›ï¿½ï¿½ì˜£ ï¿½ì˜‰ï¿½ï¿½ï¿? å«„ê³•?”?‘œï¿? åª›ï¿½ï§ï¿½ ï¿½ë‚ï¿½ë±¶?‘œï¿? ï¿½ê½‘ï¿½ê¹®ï¿½ë¸¯ï¿½ë’— ï¿½ë¸¿ï¿½ë‹”
int choose_min(double distance[], int n, int found[]) {
    int minIndex = -1;
    double minValue = INF;

    for (int i = 0; i < n; i++) {
        if (!found[i] && distance[i] < minValue) {
            minValue = distance[i];
            minIndex = i;
        }
    }

    return minIndex; // åª›ï¿½ï¿½ì˜£ ï¿½ì˜‰ï¿½ï¿½ï¿? å«„ê³•?”ï¿½ì“½ ï¿½ë‚ï¿½ë±¶?‘œï¿? è«›ì„‘?†š
}

// ï¿½ë–ï¿½ì”¡ï¿½ë’ªï¿½ë“ƒï¿½ì”ª ï¿½ë¸£??¨ì¢Š?”ï§ï¿½ ï¿½ë–ï¿½ë»¾
void shortest_path(GraphType* g, int start) {
    int i, u, w;

    // prev[] è«›ê³—ë¿? ?¥?‡ë¦°ï¿½?†• (ï¿½ë–ï¿½ì”¡ï¿½ë’ªï¿½ë“ƒï¿½ì”ª ï¿½ë¸£??¨ì¢Š?”ï§ï¿½ ï¿½ë–†ï¿½ì˜‰ ï¿½ìŸ¾ï¿½ë¿‰)
    for (int i = 0; i < g->n; i++) {
        distance_arr[i] = g->weight[start][i];  // start ï¿½ë‚ï¿½ë±¶ï¿½ï¿½ï¿? ï¿½ë–?‘œï¿? ï¿½ë‚ï¿½ë±¶ï¿½ë±¾ ï¿½ê¶—ï¿½ì” ï¿½ì“½ ?¥?‡ë¦? å«„ê³•?”
        found_arr[i] = FALSE;                   // è«›â‘¸Ğ¦ ï¿½ë¿¬?ºï¿? è«›ê³—ë¿?
        prev_arr[i] = (g->weight[start][i] < INF && i != start) ? start : -1;  // å¯ƒìˆì¤? ?•°ë¶¿ìŸ»ï¿½ì“£ ï¿½ìï¿½ë¸³ prev[] ?¥?‡ë¦°ï¿½?†•
    }

    found_arr[start] = TRUE;  // ï¿½ë–†ï¿½ì˜‰ ï¿½ë‚ï¿½ë±¶ï¿½ë’— ï¿½ì” èª˜ï¿½ è«›â‘¸Ğ¦ï¿½ë»½ï¿½ë–??¨ï¿½ ï¿½ê½•ï¿½ì ™
    distance_arr[start] = 0;  // ï¿½ë–†ï¿½ì˜‰ ï¿½ë‚ï¿½ë±¶ï¿½ì“½ å«„ê³•?”ï¿½ë’— 0

    for (int i = 0; i < g->n - 1; i++) {
        // ï§¤ì’–?ƒ¼ å«„ê³•?”?‘œï¿? åª›ï¿½ï§ï¿½ ï¿½ë‚ï¿½ë±¶ ï¿½ê½‘ï¿½ê¹®
        u = choose_min(distance_arr, g->n, found_arr);
        if (u == -1) break;  // ï¿½ëœ‘ ï¿½ì” ï¿½ê¸½ åª›ê¹†?–Šï¿½ë¸· ï¿½ë‚ï¿½ë±¶åª›ï¿½ ï¿½ë¾¾ï¿½ì‘ï§ï¿½ ?†«?‚…ì¦?

        printf("Selected node: %d\n", u);
        found_arr[u] = TRUE;

        // å¯ƒìˆì¤? åª›ê¹†?–Š
        for (w = 0; w < g->n; w++) {
            if (g->weight[u][w] != INF && distance_arr[u] + g->weight[u][w] < distance_arr[w]) {
                distance_arr[w] = distance_arr[u] + g->weight[u][w];
                prev_arr[w] = u;  // å¯ƒìˆì¤? ?•°ë¶¿ìŸ»ï¿½ì“£ ï¿½ìï¿½ë¸³ prev è«›ê³—ë¿? åª›ê¹†?–Š
            }
        }
    }

    // ï§¤ì’•?–’ å¯ƒìˆì¤ˆï¿½ï¿½ï¿½ prev è«›ê³—ë¿? ?•°?’•? ° (ï¿½ëµ’è¸°ê¾§?‰­ ï¿½ìŠœï¿½ë£„)
    printf("prev è«›ê³—ë¿?: \n");
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
    FILE* fp = fopen("ÃÖÀûÀÇ_°æ·Î.json", "w");
    if (fp == NULL) {
        printf("ï¿½ë™†ï¿½ì”ªï¿½ì“£ ï¿½ë¿´ ï¿½ë‹” ï¿½ë¾¾ï¿½ë’¿ï¿½ë•²ï¿½ë–!\n");
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
    printf("ï¿½ì…¿ï¿½ìŸ¾ï¿½ë¸³ å¯ƒìˆì¤ˆåª›ï¿? route.json ï¿½ë™†ï¿½ì”ªï¿½ë¿‰ ï¿½ï¿½ï¿½ï¿½?˜£ï¿½ë¦ºï¿½ë??ï¿½ë’¿ï¿½ë•²ï¿½ë–.\n");
}

// ï§ë¶¿?”¤ ï¿½ë¸¿ï¿½ë‹”
int main(void) {
    setlocale(LC_ALL, "ko_KR.UTF-8");

    GraphType g;

    // ï¿½ëœ²ï¿½ì” ï¿½ê½£ æ¿¡ì’•ë±?
    load_graph(&g);
    load_categories(&g);
    updateWeightMatrix(&g);
    //printMatrix(&g);

    // ï¿½ë–†ï¿½ì˜‰ ï¿½ë‚ï¿½ë±¶ ï¿½ê½‘ï¿½ê¹® (ç§»ëŒ„??’æ?¨ì¢Š?” 1ï¿½ì”¤ ï¿½ë‚ï¿½ë±¶ ä»¥ï¿½ ï¿½ë¸¯ï¿½êµ¹)
    int start = choose_start_node(&g);  // ç§»ëŒ„??’æ?¨ì¢Š?” 1ï¿½ì”¤ ï¿½ë‚ï¿½ë±¶ ä»¥ï¿½ ï¿½ë¸¯ï¿½êµ¹?‘œï¿? ï¿½ë–†ï¿½ì˜‰ï¿½ì ï¿½ì‘æ¿¡ï¿½ ï¿½ê½‘ï¿½ê¹®

    if (start == -1) {
        printf("No category 1 node found, selecting category 0 node.\n");
        // ç§»ëŒ„??’æ?¨ì¢Š?” 1ï¿½ì”  ï¿½ë¾¾ï¿½ì‘ï§ï¿½ ç§»ëŒ„??’æ?¨ì¢Š?” 0ï¿½ì”¤ ï¿½ë‚ï¿½ë±¶ ä»¥ï¿½ ï¿½ë¸¯ï¿½êµ¹?‘œï¿? ï¿½ê½‘ï¿½ê¹®
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

    // ï¿½ê¹®ï¿½ê½¦ï¿½ë§‚ å¯ƒìˆì¤? ?•°?’•? ° (ï¿½ëµ’è¸°ê¾§?‰­ ï¿½ìŠœï¿½ë£„)
    printf("Complete Path: ");
    for (int i = 0; i < path_length; i++) {
        printf("%d ", complete_path[i]);
    }
    printf("\n");

    // å¯ƒìˆì¤ˆç‘œï¿? JSON ï¿½ë™†ï¿½ì”ªï¿½ë¿‰ ï¿½ï¿½ï¿½ï¿½?˜£
    save_complete_path_to_json(&g, start, complete_path, path_length);

    return 0;
}