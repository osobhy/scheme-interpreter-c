#include "linkedlist.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// creates a node of type NULL_TYPE and allocates memory for it.
Item *makeNull() {
    Item *newItem = (Item *)malloc(sizeof(Item));
    assert(newItem != NULL);
    newItem->type = NULL_TYPE;
    return newItem;
}

// create a cons_cell type node by taking in a car and a cdr and allocates memory for it.
Item *cons(Item *newCar, Item *newCdr) {
    Item *newItem = (Item *)malloc(sizeof(Item));
    assert(newItem != NULL);
    newItem->type = CONS_TYPE;
    newItem->c.car = newCar;
    newItem->c.cdr = newCdr;
    return newItem;
}

// displays the contents inside a linked list in traditional
// Scheme format.
void display(Item *list) {
    assert(list != NULL && "Error (display): input list is NULL");
    printf("(");
    while (!isNull(list)) {
        Item *element = car(list);
        if (element == NULL) {
            break;
        } else {
            switch (element->type) {
                case INT_TYPE:
                    printf("%d ", element->i);
                    break;
                case DOUBLE_TYPE:
                    printf("%f ", element->d);
                    break;
                case STR_TYPE:
                    printf("%s ", element->s);
                    break;
                case CONS_TYPE:
                    break;
                default:
                    printf("Unknown type ");
            }
        }
        list = cdr(list);
        if (!isNull(list)) {
            printf(", ");
        }
    }
    printf(")");
}

// a helper function we made to clone items. it takes in a source item,
// checks if it is a valid item, and returns a new item that is identical.
Item *cloneItem(Item *source) {
    if (source == NULL) {
        exit(EXIT_FAILURE);
    }

    Item *newItem = malloc(sizeof(Item));
    if (newItem == NULL) {
        exit(EXIT_FAILURE);
    }

    newItem->type = source->type;
    switch (source->type) {
        case INT_TYPE:
            newItem->i = source->i;
            break;
        case DOUBLE_TYPE:
            newItem->d = source->d;
            break;
        case STR_TYPE:
            newItem->s = strdup(source->s);
            if (newItem->s == NULL) {
                free(newItem);
                exit(EXIT_FAILURE);
            }
            break;
        default:
            free(newItem);
            exit(EXIT_FAILURE);
    }
    return newItem;
}

// takes in a list and returns a new reversed list by using
// the helper function defined above.
Item *reverse(Item *list) {
    Item *reversed = makeNull();
    while (!isNull(list)) {
        Item *clonedItem = cloneItem(car(list));
        reversed = cons(clonedItem, reversed); 
        list = cdr(list);
    }
    return reversed;
}

//  takes in a list and frees up ALL memory associated with it.
void cleanup(Item *list) {
    if (list == NULL) {
        return;
    }

    if (list->type == CONS_TYPE) {
        cleanup(list->c.car);
        cleanup(list->c.cdr);
    } else if (list->type == STR_TYPE) {
        free(list->s);
    }

    free(list);
}

// returns the car of a cons cell that is input into the function.
Item *car(Item *list) {
    assert(list != NULL);
    return list->c.car;
}

// returns the cdr of a cons cell that is input into the function.
Item *cdr(Item *list) {
    assert(list != NULL);
    return list->c.cdr;
}

// checks if the given item is a null type or not part of a cons cell,
// returning true if the conditions are met.
bool isNull(Item *item) {
    assert(item != NULL);
    if (item == NULL || item->type != CONS_TYPE) {
        return 1;
    } else {
        return 0;
    }
}

// takes in a linked list and returns the number of elements
// inside it.
int length(Item *list) {
    assert(list != NULL);
    int count = 0;
    Item *current = list;
    while (!isNull(current)) {
        count++;
        current = cdr(current);
    }
    return count;
}


