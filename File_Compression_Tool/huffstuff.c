#include "huffmancoding.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#define TABLE_SIZE 256

int compareNode(const void *N1, const void *N2) {
    /* this is our comparator for qsort, nulls at the end*/
    const HuffManNode *node1 = *(HuffManNode **) N1;
    const HuffManNode *node2 = *(HuffManNode **) N2;
    if (!node1 && !node2) {
        return 0;
    }
    if(!node1) {
        return 1;
    }
    if(!node2) {
        return -1;
    }
    if (((node1->freq) - (node2->freq)) == 0){
        /* sort based on char */
        return (node1->c) - (node2->c);
    }
    else {
        /* sort based on val */
        return (node1->freq) - (node2->freq);
    }
}

int compareBytes(const void *N1, const void *N2) {
    /*sort based on char with null's at the end */
    const HuffManNode *node1 = *(HuffManNode **) N1;
    const HuffManNode *node2 = *(HuffManNode **) N2;
    if (!node1 && !node2) {
        return 0;
    }
    if(!node1) {
        return 1;
    }
    if(!node2) {
        return -1;
    }
    /* sort based on characters */
    return (node1->c) - (node2->c);
}

void copyArray(HuffManNode* arr[], HuffManNode* cpy[], int size) {
    int i;
    for (i = 0; i < size; i++) {
        cpy[i] = arr[i];
    }
}

HuffManNode* make_HuffNode(int freq, unsigned int c, HuffManNode* next, 
HuffManNode* left, HuffManNode* right) {
    HuffManNode* hNode = (HuffManNode *) malloc(sizeof(HuffManNode));
    if (hNode == NULL) {
        fprintf(stderr, "Unable to allocate memory for the new node.");
        exit(1);
    }
    /* automatically malloc the max length of the code */
    hNode->code = (char *) malloc(MAX_CODE_SIZE);
    if (hNode->code == NULL) {
        fprintf(stderr, "error allocating memory for the code string");
        exit(1);
    }
    /*set all the values, except for code, that is set later*/
    hNode->freq = freq;
    hNode->c = c;
    hNode->next = next;
    hNode->right = right;
    hNode->left = left;
    return hNode;
}

uint8_t size_of_table(HuffManNode* t[]) {
    /* returns the number of uniqe characters */
    uint8_t count = 0;
    int i = 0;
    while (i < TABLE_SIZE) {
        if (t[i] != NULL) {
            count++;
        }
        i++;
    }
    return count;
}
