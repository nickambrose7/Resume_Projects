#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "huffmancoding.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>

HuffManNode* table[TABLE_SIZE] = {NULL}; /*create the array for our freq count
this table will get sorted.*/
HuffManNode* lookup_table[TABLE_SIZE] = {NULL}; /*this table we can
index using the char*/

HuffManNode* head = NULL; /*points to the huffnode @ head of the linked list*/
HuffManNode* root = NULL; /* this is the root of our huffman tree*/

typedef struct BitWriter BitWriter;
struct BitWriter {
    uint8_t byte; /* the byte we will write the the file*/
    int counter; /* counts the position we are in the byte */
    int fd; /* file descriptor we want to write out to */
};

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

BitWriter* create_bitwriter(int file_descriptor) {
    BitWriter* bw;
    bw = (BitWriter*) malloc(sizeof(BitWriter));
    if (!bw) {
        fprintf(stderr, "Unable to allocate mem for bitwriter");
        exit(1);
    }
    bw->byte = 0; /* start with all 0 bits */
    bw->counter = 8; /* start at index 0 of bit */
    bw->fd = file_descriptor; /* the file we want to write the byte to */
    return bw;
}

void clear_bitwriter(BitWriter* bw) {
    /* clears the bw, used every time we write the byte */
    bw->byte = 0;
    bw->counter = 8;
}


int make_table(const char* infile, const char* outfile) {
    /* read the given file, make the table of chacters:
    on sucess -> return 0 ; if given file is empty -> exit with an error.
    if our file has only one character -> return 1    */
    int fd; /* file describtor */
    int n; /* number of bytes read in by our read function */
    int i; /* index in the for loop */
    unsigned int c; /* current character */
    int count = 0; /* the amount of characters in the file */
    unsigned char buf[READ_BUF_SIZE]; /* this is the buffer we will read into */
    if ((fd = open(infile, O_RDONLY)) == -1) {
        perror("Error");
        exit(1);
    }
    while ((n = read(fd, buf, READ_BUF_SIZE)) > 0) {
        for (i = 0; i < n; i++) {
            /* make new node if it is an empty spot */
            c = buf[i]; 
            count++;
            if (table[c] == NULL) {
                table[c] = make_HuffNode(1, c, NULL, NULL, NULL);
            }
            else {
            /* otherwise just update freq */
            table[c]->freq += 1;
            }
        }
    }
    close(fd); /* must close what we open */
    if (count == 0) { /* if gvn an empty file, just open
    an empty output file and exit the program */
        if ((open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 
        S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP)) == -1) {
            perror("Error");
            exit(1);
        } 
        exit(1); /* do not need to do anything to empty file*/
    }
    if (count == 1) {
    /* file only has one char*/
        return 1;
    }
    return 0;
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

void write_byte(BitWriter* bw, char* code, int fd_out) {
    u_int8_t byte; /* use this to write the byte */
    int i = 0; /* index into our code string */
    while (code[i] != '\0') { /* while not at the end of code */
        if (bw->counter == 0) {
        /* we have no more space in the byte to write the code */
            /* write out the byte to the outfile*/
            byte = bw->byte;
            if ((write(fd_out, &byte, sizeof(u_int8_t))) == -1) {
                perror("Error");
                exit(1);
            } 
            clear_bitwriter(bw);/* reset bitwriter*/
        }
        else if (code[i] == '1' && (bw->counter >= 0)) { 
            bw->counter--;
            /* we must set the bit*/
            (bw->byte) = (bw->byte) | (1 << (bw->counter));
            i++;
        }
        else { /* leave as a zero bit */
            bw->counter--;
            i++;
        }
    }
}



void write_file(HuffManNode* l_table[], const char* outfile, 
const char* infile) {
    /* this function takes the lookup table and the outfile the user provides
    otherwise will just write to stdout */
    uint8_t num_unique; /* need to write as a byte*/ 
    uint8_t byte;
    int fd_out; /* file describtor for the output file */
    int fd_in; /* file describtor for the input file */
    unsigned char c; /* the character */
    int n; /* number of bytes read in by our read function */
    int freq; /* freq of the character */
    char buf[READ_BUF_SIZE]; /* this is the buffer we will read into */
    int i = 0; /* index */
    char* code = NULL; /* holds the code */
    HuffManNode* arrcpy[TABLE_SIZE] = {NULL};
    BitWriter* bw; 
    copyArray(l_table, arrcpy, TABLE_SIZE); /* dont want to modify orginal*/
    qsort(arrcpy, TABLE_SIZE, sizeof(HuffManNode*), compareBytes);
    num_unique = size_of_table(arrcpy); /* first thing in our header*/
    num_unique--; /* must be num - 1, see spec for why */
    if (strcmp(outfile, "stdout") == 0) {
        fd_out = STDOUT_FILENO; /* write to stdout */
    }
    else { /*set fd to gvn output file */
        if ((fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 
        S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP)) == -1) {
            perror("Error");
            exit(1);
        } 
    }
    if (write(fd_out, &num_unique, sizeof(uint8_t)) == -1) {
        /* write num unique chars*/
        perror("Error");
        exit(1);
    } 
    while ((arrcpy[i] != NULL) && (i <= 256)) { /* write the header*/
        c = arrcpy[i]->c;
        write(fd_out, &c, sizeof(char)); /*write the char */
        freq = htonl(arrcpy[i]->freq); 
        write(fd_out, &freq, sizeof(int)); /*write the freq */
        i++;
    }
    if ((fd_in = open(infile, O_RDONLY)) == -1) {
        /* open the input file */
        perror("Error");
        exit(1);
    }
    bw = create_bitwriter(fd_out);
    while((n = read(fd_in, buf, READ_BUF_SIZE)) > 0) {
        for (i = 0; i < n; i++) { /* for each char in the buffer */
        /* get look in the table at the index based on the char 
        in the buffer and get the code. */
            c = buf[i];
            code = lookup_table[c]->code;
            write_byte(bw, code, fd_out);
        }
    }
    /* need to write out the last byte */
    if (bw->byte != 0) { /* it should be padded b/c 
    we start with most sig bit.*/
        byte = bw->byte;
        if ((write(fd_out, &byte, sizeof(u_int8_t))) == -1) {
            perror("Error");
            exit(1);
        } 
    }
    free(bw);
    close(fd_out);
    close(fd_in);
}

int main(int argc, char* argv[]) {
    const char* infile_name;
    const char* outfile_name;
    char* s; /* holds the code in a string*/
    HuffManNode* super = NULL; /* our super node */
    if (argc == 1) { /* only arg is the program name, need at least one arg */
        fprintf(stderr, "no file given to encode");
        exit(1);
    }
    if (argc == 2) {/* only gvn an input file, so we will use std out*/
        infile_name = argv[1];
        outfile_name = "stdout"; /* use stdout */
    }
    else if (argc == 3) { /* gvn all args */
        infile_name = argv[1];
        outfile_name = argv[2];
    }
    else { /* most args this program can have is 3 */
        fprintf(stderr, "too many arguments");
        exit(1);
    }
    if (make_table(infile_name, outfile_name) == 1) {
        /* handle the edge case of one char. */
    }

     /*need to keep the orginal array to look up characters, 
    need to do this before I sort*/
    copyArray(table, lookup_table, TABLE_SIZE);
    qsort(table, TABLE_SIZE, sizeof(HuffManNode*), compareNode);
    arr_to_linkedlist(table); /* head of linked list is stored in 
    global variable head */
    
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
    write_file(lookup_table, outfile_name, infile_name);
    return 0;
}
