#include <stdlib.h>
#include <string.h>

#include "huffman-tree.h"

// PRIVATE
hfm_Node* hfm_create_node(
        float prob,
        char symbol,
        hfm_Node *left,
        hfm_Node *right
) {
        hfm_Node *hn = (hfm_Node*) malloc(sizeof(hfm_Node));
        hn->prob = prob;
        hn->symbol = symbol;
        hn->left = left;
        hn->right = right;

    return hn;
}

hfm_Node* hfm_join_node(hfm_Node *hn1, hfm_Node *hn2) {
    hfm_Node *parent = NULL;

    if (hn1->prob < hn2->prob) {
        parent = hfm_create_node(hn1->prob + hn2->prob, '-', hn2, hn1);
    } else {
        parent = hfm_create_node(hn1->prob + hn2->prob, '-', hn1, hn2);
    }

    return parent;
}

void hfm_destroy_tree(hfm_Node *hn) {
    if (hn != NULL) {
        hfm_destroy_tree(hn->left);
        hfm_destroy_tree(hn->right);
        free(hn);
    }
}

void hfm_create_code_table_aux(hfm_Node *hn, char **table, char *path) {
    if (!hn) return;
    if (hn->symbol != '-') {
        table[(hn->symbol - 'a')] = path;
        return;
    }

    char *path_left = (char*) malloc(strlen(path) + 2);
    char *path_right = (char*) malloc(strlen(path) + 2);

    strcpy(path_left, path);
    strcpy(path_right, path);
    strcat(path_left, "0");
    strcat(path_right, "1");
    hfm_create_code_table_aux(hn->left, table, path_left);
    hfm_create_code_table_aux(hn->right, table, path_right);

    if (hn->symbol == '-') {
        if (hn->left->symbol == '-')
            free(path_left);
        if (hn->right->symbol == '-')
            free(path_right);
    }
}

char** hfm_create_code_table(hfm_Node *hn, int table_size) {
    char **table = (char**) malloc(sizeof(char*) * table_size);
    hfm_create_code_table_aux(hn, table, "");
    return table;
}

void hfm_destroy_table(hfm_Tree *ht) {
    for (int i = 0; i < ht->pool_size; i++){
        free(ht->table[i]);
    }
    free(ht->table);
}

// PUBLIC
hfm_Tree* hfm_Create() {
    hfm_Tree *ht = (hfm_Tree *) malloc(sizeof(hfm_Tree));

    ht->head = NULL;
    ht->pool = hfm_Create_Minheap(HFM_POOL_SIZE);
    ht->table = NULL;
    ht->pool_size = 0;

    return ht;
}

void hfm_Insert_Pool(hfm_Tree* ht, float prob, char symbol) {
    if (hfm_Is_Full_Minheap(ht->pool))
        ht->pool = hfm_Increase_Size_Minheap(ht->pool, HFM_POOL_SIZE_INC);

    hfm_Node *hn = hfm_create_node(prob, symbol, NULL, NULL);
    hfm_Insert_Minheap(ht->pool, hn);
    ht->pool_size += 1;
}

void hfm_Insert_Pool_From_File(hfm_Tree* ht, char *file){
    FILE *f = fopen(file, "r");
    if (!f) exit(1);
    char symbol[2], probs[10];
    float prob;
    while (!feof(f)){
        fscanf(f, "%s %s", symbol, probs);
        prob = strtof(probs, NULL);
        hfm_Insert_Pool(ht, prob, symbol[0]);
    }
    fclose(f);
}

void hfm_Gen_Tree(hfm_Tree* ht) {
    if (hfm_Is_Empty_Minheap(ht->pool)) return;
    hfm_destroy_tree(ht->head);

    hfm_Minheap *bck = hfm_Clone_Minheap(ht->pool);

    hfm_Node *hn1, *hn2, *head;

    while (ht->pool->tail > 0) {
        hn1 = hfm_Pop_Minheap(ht->pool);
        hn2 = hfm_Pop_Minheap(ht->pool);

        head = hfm_join_node(hn1, hn2);

        hfm_Insert_Minheap(ht->pool, head);
    }

    ht->head = hfm_Pop_Minheap(ht->pool);
    ht->table = hfm_create_code_table(ht->head, ht->pool_size);

    hfm_Destroy_Minheap(ht->pool);
    ht->pool = bck;
}

char* hfm_Encode_String(hfm_Tree *ht, char *msg) {
    if (ht->table == NULL) return NULL;

    int size = 0;
    for (int i = 0; i < strlen(msg); i++)
        size += strlen(ht->table[msg[i] - 'a']);

    char *encoded = (char *) malloc(size + 1);
    strcpy(encoded, "");
    for (int i = 0; i < strlen(msg); i++)
        strcat(encoded, ht->table[msg[i] - 'a']);

    return encoded;
}

char* hfm_Decode_String(hfm_Tree *ht, char *encoded) {
    if (ht->table == NULL) return NULL;

    int size = 0;
    hfm_Node *aux = ht->head;
    for (int i = 0; i <= strlen(encoded); i++) {
        if (aux->symbol != '-') {
            size++;
            aux = ht->head;
        }

        if (encoded[i] == '0')
            aux = aux->left;
        else
            aux = aux->right;
    }

    aux = ht->head;
    char *msg = (char*) malloc(sizeof(char) * size + 1);
    int count = 0;
    for (int i = 0; i <= strlen(encoded); i++) {
        if (aux->symbol != '-') {
            msg[count] = aux->symbol;
            count++;
            aux = ht->head;
        }

        if (encoded[i] == '0')
            aux = aux->left;
        else
            aux = aux->right;
    }

    return msg;
}

char* hfm_Encode_Msg(hfm_Tree *ht, char *msg){
    int str_count = 1, i = 0;
    char *aux = NULL;

    while (i <= strlen(msg)) {
        if (msg[i] == ' ')
            str_count++;
        i++;
    }

    char **str_array = (char **) malloc(sizeof(char *) * str_count);

    char *string = strtok(msg, " ");
    for (i = 0; i < str_count; i++) {
        str_array[i] = string;
        string = strtok(NULL, " ");
    }

    int size_msg = 0;
    for (i = 0; i < str_count; i++) {
        aux = hfm_Encode_String(ht, str_array[i]);
        size_msg += strlen(aux);
        free(aux);
    }

    char *answer = (char *) malloc(sizeof(char) * size_msg + str_count);
    strcpy(answer, "");
    aux = hfm_Encode_String(ht, str_array[0]);
    strcat(answer, aux);
    free(aux);
    for (i = 1; i < str_count; i++){
        strcat(answer, " ");
        aux = hfm_Encode_String(ht, str_array[0]);
        strcat(answer, aux);
        free(aux);
    }

    free(str_array);

    return answer;
}

void hfm_Destroy(hfm_Tree *ht) {
    hfm_destroy_table(ht);
    hfm_destroy_tree(ht->head);
    hfm_Destroy_Minheap(ht->pool);
    free(ht);
}
