#include <netinet/in.h>
#define MAX_CODE_SIZE 40
#define TABLE_SIZE 256
#define READ_BUF_SIZE 50

typedef struct HuffManNode HuffManNode;
struct HuffManNode
{
    int freq; /* store the freq */
    unsigned int c; /* store the char */
    char* code; /* This is the code for the given character*/
    struct HuffManNode* next; /* for linked list */
    /* for tree */
    struct HuffManNode* left; /* this allows us to represent a tree: */
    struct HuffManNode* right;
};

HuffManNode* make_HuffNode(int freq, unsigned int c, HuffManNode* next, 
HuffManNode* left, HuffManNode* right);
uint8_t size_of_table(HuffManNode* t[]);
void copyArray(HuffManNode* arr[], HuffManNode* cpy[], int size);
int compareNode(const void *N1, const void *N2);
int compareBytes(const void *N1, const void *N2);
