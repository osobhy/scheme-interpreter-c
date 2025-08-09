#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "talloc.h"
#include "parser.h"
#include "linkedlist.h"
#include "item.h"

/* "pops" an item from a given stack, which means it returns
   the item at the very top of the stack (or more accurately
   the linkedlist acting as a stack) */
Item *pop(Item **stack) {
    if (isNull(*stack)) {
        return NULL;
    }
    Item *top =car(*stack);
    *stack = cdr(*stack);
    return top;
}

/* "pushes" a given item onto a given stack, which means it
   puts that item on top of the stack */
void push(Item **stack, Item *element) {
    *stack = cons(element, *stack);
}

/* prints a given syntax error and exits the program using
   "texit" to clear memory */
void syntaxError(const char *message) {
    printf("Syntax error: %s\n", message);
    texit(1);
}

/* a helper function to print a given parse tree's items
   to a buffer list at a certain position*/
void printToBuffer(Item *tree, char *buf, int *pos) {
    if (tree == NULL) {
        return;
    }

    if (tree->type == CONS_TYPE && car(tree)->type == BOOL_TYPE ) {
        Item *current = tree;
        while (current->type == CONS_TYPE) {
            printTree(car(current));
            current = cdr(current);
            if (current->type != NULL_TYPE) {
                printf(" ");
            }
        }
    } else {
        switch (tree->type) {
            case CONS_TYPE:
                {
                    buf[(*pos)++] = '(';
                    Item *current = tree;
                    while (current != NULL && current->type == CONS_TYPE) {
                        printToBuffer(car(current), buf, pos);
                        current = cdr(current);
                        if (current != NULL && current->type != NULL_TYPE) {
                            buf[(*pos)++] = ' ';
                        }
                    }
                    if (current != NULL && current->type != NULL_TYPE) {
                        buf[(*pos)++] = '.';
                        buf[(*pos)++] = ' ';
                        printToBuffer(current, buf, pos);
                    }
                    buf[(*pos)++] = ')';
                }
                break;
            case SYMBOL_TYPE:
                sprintf(buf + *pos, "%s", tree->s);
                *pos += strlen(tree->s);
                break;
            case STR_TYPE:
                sprintf(buf + *pos, "\"%s\"", tree->s);
                *pos += strlen(tree->s) + 2;
                break;
            case INT_TYPE:
                sprintf(buf + *pos, "%d", tree->i);
                *pos += snprintf(NULL, 0, "%d", tree->i);
                break;
            case DOUBLE_TYPE:
                sprintf(buf + *pos, "%f", tree->d);
                *pos += snprintf(NULL, 0, "%f", tree->d);
                break;
            case BOOL_TYPE:
                if (tree->i) {
                    sprintf(buf + *pos, "#t");
                    *pos += strlen("#t");
                } else {
                    sprintf(buf + *pos, "#f");
                    *pos += strlen("#f");
                }
                break;
            case NULL_TYPE:
                sprintf(buf + *pos, "()");
                *pos += 2;
                break;
            default:
                syntaxError("Item type unrecognized");
            }
    }
}

/* prints the items of a given parse tree in perfect Scheme code */
void printTree(Item *tree) {
    char buffer[1000] = {0};  
    int pos = 0;
    printToBuffer(tree, buffer, &pos);
    size_t len = strlen(buffer);

    if (buffer[0] == '(' && buffer[1] == '(' && buffer[2] != ')') {
        if (buffer[len - 1] == ')' && buffer[len - 2] == ')') {
            buffer[len - 1] = '\0';
            printf("%s", buffer + 1);
        } else {
            printf("%s", buffer);
        }
    } else {
        printf("%s", buffer);
    }
}

/* converts a list of tokenized input into a parsed abstract syntax tree,
   to handle Scheme syntax rules and structure */
Item *parse(Item *tokens) {
    Item *stack = makeNull();
    int openParentheses = 0;
    Item *previousToken = NULL;

    while (!isNull(tokens)) {
        Item *token = car(tokens);
        tokens = cdr(tokens);

        switch (token->type) {
            case OPEN_TYPE:
            case OPENBRACKET_TYPE:
                push(&stack, token);
                openParentheses++;
                break;
            case SYMBOL_TYPE:
                if (strcmp(token->s, "lambda") == 0) {
                    previousToken = token;
                    push(&stack, token);
                } else if (previousToken && (strcmp(previousToken->s, "lambda") == 0) && strcmp(token->s, "quote") == 0) {
                    syntaxError("lambda is not followed by arguments");
                } else {
                    push(&stack, token);
                }
                break;
            case CLOSE_TYPE:
            case CLOSEBRACKET_TYPE:
                if (openParentheses == 0) {
                    syntaxError("too many close parentheses");
                }
                openParentheses--;
                Item *sublist = makeNull();
                while (!isNull(stack) && (car(stack)->type != OPEN_TYPE && car(stack)->type != OPENBRACKET_TYPE)) {
                    sublist = cons(pop(&stack), sublist);
                }
                pop(&stack);
                if (!isNull(sublist) && isNull(cdr(sublist)) && car(sublist)->type == CONS_TYPE && !isNull(car(sublist)->c.cdr)) {
                    sublist = car(sublist);
                }
                push(&stack, sublist);
                break;
            default:
                push(&stack, token);
                break;
        }
    }

    if (openParentheses > 0) {
        syntaxError("not enough close parentheses");
    }

    Item *parseTree = makeNull();
    while (!isNull(stack)) {
        parseTree = cons(pop(&stack), parseTree);
    }

    if (isNull(cdr(parseTree))) {
        return car(parseTree);
    }

    return parseTree; 
}
