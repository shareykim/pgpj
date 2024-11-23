#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "parson.h" 

#define TRUE 1
#define FALSE 0
#define MAX_VERTICES	100	
#define INF	1000000  



typedef struct GraphType {
	int n;	
	int weight[MAX_VERTICES][MAX_VERTICES];
} GraphType;


void load_graph(GraphType *g);
void create_array(GraphType *g);
void find_path(GraphType *g);


int every_distance[MAX_VERTICES][MAX_VERTICES]; 
int every_found[MAX_VERTICES]; 
int distance[MAX_VERTICES];  
int found[MAX_VERTICES]; 


int choose(int distance[], int n, int found[])
{
	int i, min, minpos;

	min = INT_MAX;  
	minpos = -1; 
for (i = 0; i < n; i++)  
        if (distance[i] < min && !found[i]) {  
			min = distance[i];
			minpos = i;
		}
	return minpos; 
}


void shortest_path(GraphType * g, int start)
{
	int i, u, w;
	for (i = 0; i < g->n; i++) 
	{
		distance[i] = g->weight[start][i]; 
		found[i] = FALSE;  
	}
	found[start] = TRUE;  
distance[start] = 0; 
	for (i = 0; i < g->n - 1; i++) { 
		u = choose(distance, g->n, found); 
		found[u] = TRUE;  
		for (w = 0; w < g->n; w++)  
            if (!found[w])
				if (distance[u] + g->weight[u][w] < distance[w])
					distance[w] = distance[u] + g->weight[u][w];
	}
}


int main(void)
{
    GraphType g;
    load_graph(&g); 
/*
    printf("노드 수: %d\n", g.n);
    printf("가중치 인접 행렬:\n");
    for (int i = 0; i < g.n; i++) {
        for (int j = 0; j < g.n; j++) {
            if (g.weight[i][j] == INF)
                printf("INF ");
            else
                printf("%d ", g.weight[i][j]);
        }
        printf("\n");
    }
    printf("\n");
*/
    create_array(&g);  
    find_path(&g);  

	return 0;
}


void load_graph(GraphType *g) {
    JSON_Value *rootValue = json_parse_file("거리.json");
    if (rootValue == NULL) {
        printf("파일을 열 수 없습니다.\n");
        return;
    }

    JSON_Object *rootObject = json_value_get_object(rootValue);
    JSON_Array *weightArray = json_object_get_array(rootObject, "weight");

    g->n = json_array_get_count(weightArray);

    for (int i = 0; i < g->n; i++) {
        JSON_Array *rowArray = json_array_get_array(weightArray, i);
        for (int j = 0; j < g->n; j++) {
            JSON_Value *value = json_array_get_value(rowArray, j);
            if (json_value_get_type(value) == JSONString &&
                strcmp(json_value_get_string(value), "INF") == 0) {
                g->weight[i][j] = INF;
            } else {
                g->weight[i][j] = (int)json_value_get_number(value);
            }
        }
    }

    json_value_free(rootValue);
}


void create_array(GraphType *g) {
    for (int i = 0; i < g->n; i++) {
        shortest_path(g, i); 

        for (int j = 0; j < g->n; j++) {
            every_distance[i][j] = distance[j];  
        }
    }

    for (int a = 0; a < g->n; a++) {
        for (int b = 0; b < g->n; b++) {
            printf("%d ", every_distance[a][b]);
        }
        printf("\n");
    }
}


void find_path(GraphType *g){
	for (int i = 0; i < g->n; i++){
		every_found[i] = FALSE;
	}
    int current_row = 0;  
    every_found[current_row] = TRUE;  
/*
    printf("\n방문 순서: %d", current_row);
*/
    JSON_Value *rootValue = json_value_init_array();
    JSON_Array *pathArray = json_value_get_array(rootValue);

    json_array_append_number(pathArray, current_row);

    while(1){
        int minValue = INF;
        int minIndex = -1;
        for(int j=0;j<g->n;j++){
            if (!every_found[j] && every_distance[current_row][j] < minValue){
                minValue = every_distance[current_row][j]; 
                minIndex = j;
            }
        }
        if (minIndex == -1){
            break;
        }
        every_found[minIndex] = TRUE; 
        current_row = minIndex;
        json_array_append_number(pathArray, current_row);

    ///    printf("-> %d", current_row);

    }
    /// printf("\n");
    json_serialize_to_file_pretty(rootValue, "거리.json");
    json_value_free(rootValue);
}