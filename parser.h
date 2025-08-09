#include "item.h"

#ifndef PARSER_H
#define PARSER_H

// Takes a list of tokens from a Scheme program, and returns a pointer to a
// parse tree representing that program.
Item *parse(Item *tokens);


// Prints the tree to the screen in a readable fashion. It should look just like
// Scheme code; use parentheses to indicate subtrees.
void printTree(Item *tree);


#endif
