#include "tokenizer.h"
#include "talloc.h"
#include "linkedlist.h"
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// create an item of a specific type (input) and allocates memory to it using talloc.
// returns the new item.
Item *createItem(itemType type) {
    Item *item = talloc(sizeof(Item));
    item->type = type;
    return item;
}

// check if a character is a proper initial for an identifier (e.g. an 
// alphabetic letter or one of the indicated symbols). returns true 
// or false accordingly
bool isInitial(char c) {
    if (isalpha(c)|| c == '!' || c == '$' || c == '%' || c == '&' || c == '*' ||
        c == '/' || c == ':' || c == '<' || c == '=' || c == '>' ||
        c == '?' || c == '~' || c == '_' || c == '^') {
        return true;
    }
    return false;
}

// check if a subsequent character can be valid part of an identifer
// e.g. same as above, but also could be a a number, ., +, or -
bool isSubsequent(char c) {
    if (isInitial(c) || isdigit(c) || c == '.' || c == '+' || c == '-') {
        return true;
    }
    return false;
}

// adds an item to a list, simply returning a cons cell that includes
// the item in the car spot and the rest of the list in the cdr spot
Item *addItem(Item *list, Item *newItem) {
    return cons(newItem, list);
}

// creates an item from a string and allocates memory to it, returns
// the item
Item *createStringItem(const char *str) {
    Item *item = talloc(sizeof(Item));
    item->type = SYMBOL_TYPE;
    item->s = talloc(strlen(str) + 1);
    strcpy(item->s, str);
    return item; }

// looks (or "peeks") at the next character without actually
// traversing through the next character. returns the next
// character.
int peekChar() {
    int c = fgetc(stdin);
    if (c != EOF) {
        ungetc(c, stdin);
    }
    return c;
}

// defines a simple linked list of nodes for boolean types
// (will prove useful when detecting #t or #f)
typedef struct BoolNode {
    bool value;
    struct BoolNode* next;
} BoolNode;

// appends a boolean value to the linked list of nodes.
// allocates memory for them and returns the head
BoolNode* addBoolState(BoolNode* head, bool value) {
    BoolNode* newNode = talloc(sizeof(BoolNode));
    newNode->value = value;
    newNode->next = NULL;
    if (head == NULL) {
        return newNode;
    }
    BoolNode* current = head;
    while (current->next != NULL) {
        current = current->next;  
    }
    current->next = newNode;
    return head; 
}

BoolNode* boolStates = NULL;

// tokenize takes no input and returns a list of tokens from a Scheme file
// to be displayed
Item *tokenize() {
    int charRead;
    Item *list = makeNull();
    char storedTokens[310];

    while ((charRead = fgetc(stdin)) != EOF) {
        if (isspace(charRead)) {
            continue;
        }

        if (charRead == ';') {
            while ((charRead = fgetc(stdin)) != '\n' && charRead != EOF);
            continue;
        }

        if (charRead == '"' || charRead == '(' || charRead == ')' || charRead == '[' || charRead == ']' || charRead == '#') {
            if (charRead == '"') {
                int i = 0;
                while ((charRead = fgetc(stdin)) != '"' && charRead != EOF && i < 300) {
                    storedTokens[i] = charRead;
                    i++;
                }
                storedTokens[i] = '\0';
                Item *item = createItem(STR_TYPE);
                item->s = createStringItem(storedTokens)->s;
                list = addItem(list, item);
            } else if (charRead == '(') {
                list = addItem(list, createItem(OPEN_TYPE));
            } else if (charRead == ')') {
                list = addItem(list, createItem(CLOSE_TYPE));
            } else if (charRead == '[') {
                list = addItem(list, createItem(OPENBRACKET_TYPE));
            } else if (charRead == ']') {
                list = addItem(list, createItem(CLOSEBRACKET_TYPE));
            } else if (charRead == '#') {
                charRead = fgetc(stdin);
                Item *item = createItem(BOOL_TYPE);
                if (charRead == 't') {
                    boolStates = addBoolState(boolStates, true);
                } else if (charRead == 'f') {
                    boolStates = addBoolState(boolStates, false);
                } else {
                    printf("Syntax error\n");
                    texit(1);
                }
                list = addItem(list, item);
                continue;
            } else {
                printf("Syntax error\n");
                texit(1);
            }
            continue;
        }

        if (isdigit(charRead) || charRead == '+' || charRead == '-') {
            storedTokens[0] = charRead;
            int i = 1;
            while ((isdigit(peekChar()) || peekChar() == '.')) {
                storedTokens[i] = fgetc(stdin);
                i++;
            }
            storedTokens[i] = '\0';
            if (isdigit(storedTokens[0]) || storedTokens[0] == '-' && i > 1) {
                Item *item;
                if (strchr(storedTokens, '.') != NULL) {
                    item = createItem(DOUBLE_TYPE);
                    item->d = strtod(storedTokens, NULL); 
                } else {
                    item = createItem(INT_TYPE);
                    item->i = atoi(storedTokens); 
                }
                list = addItem(list, item); 
            } else {
                list = addItem(list, createStringItem(storedTokens));
            }
            continue;
        }

        if (isInitial(charRead)) {
            int i = 0;
            storedTokens[i] = charRead;
            i++;
            while (isSubsequent(peekChar())) {
                storedTokens[i] = fgetc(stdin);
                i++;
            }
            storedTokens[i] = '\0';
            list = addItem(list, createStringItem(storedTokens));
            continue;
        }
        printf("Syntax error\n");
        texit(1);
    }
    return reverse(list);
}

// displayTokens takes in a list of tokens generated by tokenize
// and prints them out based on their types
void displayTokens(Item *list) {
    BoolNode* boolStatePtr = boolStates; 
    while (!isNull(list)) {
        Item *token = car(list);
        switch (token->type) {
            case INT_TYPE:
                printf("%d:integer ", token->i);
                break;
            case DOUBLE_TYPE:
                printf("%.2f:double ", token->d);
                break;
            case STR_TYPE:
                printf("\"%s\":string ", token->s);
                break;
            case SYMBOL_TYPE:
                printf("%s:symbol ", token->s);
                break;
            case OPEN_TYPE:
                printf("(:open ");
                break;
            case CLOSE_TYPE:
                printf("):close ");
                break;
            case OPENBRACKET_TYPE:
                printf("[:openbracket ");
                break;
            case CLOSEBRACKET_TYPE:
                printf("]:closebracket ");
                break;
            case BOOL_TYPE:
                if (boolStatePtr != NULL) {
                    if (boolStatePtr->value) {
                        printf("#t:boolean ");
                    } else {
                        printf("#f:boolean ");
                    }
                    boolStatePtr = boolStatePtr->next;
                }
                break;
            default:
                printf("Unknown type ");
                break;
        }
        list = cdr(list);
    }
    printf("\n");
}
