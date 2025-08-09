#include "talloc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

Item *activeList = NULL;

// dynamically allocates memory of an input size in bytes
// and returns a pointer to the allocated memory in a linked
// list
void *talloc(size_t size) {
    Item *memory = malloc(size);
    Item *newNode = malloc(sizeof(Item));
    newNode->type = PTR_TYPE;
    newNode->p = memory;
    newNode->c.cdr = activeList;
    activeList = newNode;
    return memory;
}

// frees all pointers and memory in lists that have been 
// allocated with talloc. no input or output.
void tfree() {
    Item *current = activeList;
    while (current != NULL) {
        Item *next = current->c.cdr;
        free(current->p); 
        free(current); 
        current = next;
    }
    activeList = NULL; 
}

// exits the program and ensures all allocated memory is freed
// by takes in a status command to exit
void texit(int status) {
    tfree();
    exit(status);
}
