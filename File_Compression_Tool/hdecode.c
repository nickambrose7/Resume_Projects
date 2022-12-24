#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include "huffmancoding.h"

#define FIVE_BYTES 5
#define ONE_BYTE 1
#define LAST_BIT -1

HuffManNode* table[TABLE_SIZE] = {NULL};
HuffManNode* lookup_table[TABLE_SIZE] = {NULL}; /*this table we can
index using the char*/
HuffManNode* head = NULL; /*points to the huffnode @ head of the linked list*/
HuffManNode* root = NULL; /* this is the root of our huffman tree*/


HuffManNode* pop() {
    /*takes in a linked list and pops the head off and returns it.*/
    HuffManNode* prev_head;
    prev_head = head; 
    if (prev_head != NULL) {
        head = head->next; /* move head to the next spot */
        prev_head->next = NULL;
    }
    return prev_head;
}

HuffManNode* insert(HuffManNode* new) {
    /*insert a new node in our linked list*/
    HuffManNode* cur;
    HuffManNode* prev_node;
    cur = head;
    if (head == NULL) {
        head = new;
        return head;
    }
    if (new->freq <= head->freq) {
    /* We want to insert at the head.*/
        new->next = head;
        head = new; /* new head of the linked list */
    }
    else {
        while (cur != NULL) {
            if (cur->freq >= new->freq) {
            /* we need to insert */
            new->next = cur;
            prev_node->next = new; 
            return head;
            }
            if (cur->next == NULL){
                cur->next = new;
                return head;
            }
        prev_node = cur;
        cur = cur->next;
        }
    }
    return head;
}

void insert_at_end(HuffManNode* new) {
/* inserts the new node at the end of the linked list
 and returns a the head of the list, this is how we go arr -> ll*/
    HuffManNode* temp = head;
    if (head == NULL) {
    /* if head is null, the thing we want to insert becomes head*/
        head = new;
    }
    else {
        while (temp->next != NULL) {
        /*move to end of the list*/
            temp = temp->next;
        }
        temp->next = new;
    }   
}

HuffManNode* extract_nodes() {
    /* extracts the two lowest freq nodes and returns a new 
    super node with the sum of their frequencies. */
    HuffManNode* n1;
    HuffManNode* n2;
    HuffManNode* new;
    n1 = pop();
    if (head != NULL) {
        n2 = pop();
    }
    else {
        return n1;
    }
    new = make_HuffNode((n1->freq + n2->freq), -1, NULL, n1, n2);
    return new;
}

int make_table_from_header(int in_fd, const char* outfile) {
    /* this function will take in a input file and read the header,
    making the table necessary to generate our linked list in the process,
    it returns the number of characters in the input file */
    int num_unique; /* use to compare out counter */
    unsigned int freq; /* gets our freq from the header file*/
    int n; /* num of bytes read by our read func */
    uint8_t buf[READ_BUF_SIZE]; /* buff we read into*/
    u_int8_t c; /* char from out header*/
    int total_chars = 0;
    int counter = 0; /* makes sure we don't read past header */    
    if ((n = read(in_fd, buf, sizeof(uint8_t))) <= 0) {
        if (n == -1) {
            perror("Error");
            exit(1);
        }
        /* empty file just open the outfile and end program*/
        if ((open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 
        S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP)) == -1) {
            perror("Error");
            exit(1);
        } 
        exit(1);
    }
    num_unique = buf[0]; /* first byte of header is num unique*/
    while (counter <= num_unique) {
        if ((n = read(in_fd, buf, FIVE_BYTES)) > 0) {
            c = buf[0]; /* only reading one thing into buf, so always 0*/
            /* b/c we can only read in bytes: */
            freq = (unsigned int)(buf[1] << 24 | 
                        buf[2] << 16 | buf[3] << 8 | buf[4]);
            total_chars += freq; /* need to keep track for when we read*/
            table[c] = make_HuffNode(freq, c, NULL, NULL, NULL);
        }
        counter++; /* this is how we know we reached end of header*/
    }
    return total_chars;
}

void arr_to_linkedlist(HuffManNode* arr[]) {
    /*itterate throught array and make linked list */
    int i;
    head = arr[0];
    for (i = 1; i < TABLE_SIZE; i++) {
        if (arr[i] != NULL) {
            insert_at_end(arr[i]);
        }
    }
}


int get_codes(HuffManNode* root, char* s, int len) {
    /* traverses our tree and gets the codes for each char and 
    puts them in the lookup table */
    if (root->left != NULL) {
        s[len] = '0';
        get_codes(root->left, s, len + 1);
    }
    if (root->right != NULL) {
        s[len] = '1';
        get_codes(root->right, s, len + 1);
    }
    if (!root->left && !root->right) {
        /* we are at a leaf NEED TO USE
        STR COPY OR ELSE ALL OUR STR POINTER WILL
        BE THE SAME */
        s[len] = '\0';
        strcpy((lookup_table[(root->c)])->code, s); /*copy current string 
        to the correct spot in our table*/
        s[len-1] = '\0';    
        
    }
    return 0;
}

void decode_file(int total_chars, int fd_in, int fd_out) {
    int n; /* number of bytes read by read */
    uint8_t byte;
    int chars_written = 0; /* keeps track of the total 
    amount of characters we write*/
    int bit_count = LAST_BIT; /* keeps track of position in the byte */
    HuffManNode* temp = root; /* acts as our temperary root 
    so that we always keep root ref*/
    uint8_t buf[ONE_BYTE]; /* buff we read into*/
    while (chars_written < total_chars) { /* while we still 
    have chars to write */
        if (bit_count == LAST_BIT) { /* need to read in a new byte */
            if ((n = read(fd_in, buf, ONE_BYTE)) != -1) {
                bit_count = 7;
            }
            else {
                perror("Error"); /* error with reading the infile*/
                exit(1);
            }
        }
        while (temp->left != NULL && temp->right != NULL) { /* not at a leaf */
            if (bit_count == LAST_BIT) {
                break; /* need to read in a new byte */
            }
            if ((*buf & (1 << bit_count)) == 0) { /* and with one to check bit*/
                temp = temp->left;
            }
            else { /* bit is a one */
                temp = temp->right;
            }
            bit_count--; 
        }
        if (temp->left == NULL && temp->right == NULL) { /* we want to 
        write the leaf */
            byte = temp->c; /* write out the leaf */
            if (write(fd_out, &byte, ONE_BYTE) == -1) { /* write the byte 
            to the outfile */
                perror("Error");
                exit(1);
            } 
            chars_written++; /* we just wrote a char, so need to increment*/
            temp = root; /* reset temp to the root */
        }
          
    }
}

int main(int argc, char* argv[]) {
    const char* infile_name;
    const char* outfile_name;
    int fd_in, fd_out; 
    HuffManNode* super = NULL; /* our super node*/
    char* s; /* holds the code string*/
    int total_chars;
    /* get our command line arguements */
    if (argc == 1) { /* no args, use stdin and out */
        infile_name = "stdin";
        outfile_name = "stdout";
    }
    else if (argc == 2) { /* one arg, determine in or out */
        if ((strcmp(argv[1], "-")) == 0) {
            infile_name = "stdin";
            outfile_name = "stdout";
        }
        else {
            infile_name = argv[1];
            outfile_name = "stdout";
        }
    }   
    else if (argc == 3) { /* both args, check for - */
        if ((strcmp(argv[1], "-")) == 0) {
            infile_name = "stdin";
            outfile_name = argv[2];
        }
        else {
            infile_name = argv[1];
            outfile_name = argv[2];
        }
    }
    else {
        fprintf(stderr, "Too many arguments");
        exit(1);
    }

    /* set file describtors */
    if (strcmp(infile_name, "stdin") == 0) {
        fd_in = STDIN_FILENO; /* take from std in */
    }
     /* open the input file */
    else {
        if ((fd_in = open(infile_name, O_RDONLY)) == -1) {
            perror("Error");
            exit(1); 
        } 
    }
    if (strcmp(outfile_name, "stdout") == 0) {
        fd_out = STDOUT_FILENO; /* write to stdout */
    }
    else {
        if ((fd_out = open(outfile_name,  O_WRONLY | O_CREAT | O_TRUNC, 
        S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP)) == -1) {
            perror("Error");
            exit(1); 
        }
    }
    total_chars = make_table_from_header(fd_in, outfile_name);
    copyArray(table, lookup_table, TABLE_SIZE);
    qsort(table, TABLE_SIZE, sizeof(HuffManNode*), compareNode);
    arr_to_linkedlist(table);
    while(head->next != NULL) {
        super = extract_nodes();
        insert(super);
    }
    /*since we have reorganized the linked list, 
    the head is now the root of our huffman tree*/
    root = head;
    s = (char*) malloc(MAX_CODE_SIZE); /* malloc 100 bytes */
    if (s == NULL) {
        fprintf(stderr, "Exceded the max huffcode size");
    }
    get_codes(root, s, 0);  /*after my call to this function, all my codes
    are available @ their char index in the lookup table */ 
    /* we have now recreated the tree!! */
    decode_file(total_chars, fd_in, fd_out); /* write decoded file*/
    close(fd_out);
    close(fd_in); 
    return 0;
}


