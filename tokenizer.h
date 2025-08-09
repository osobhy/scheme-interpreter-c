#include "item.h"

#ifndef TOKENIZER_H
#define TOKENIZER_H

// Read all of the input from stdin, and return a linked list consisting of the
// tokens.
Item *tokenize();

// Displays the contents of the linked list as tokens, with type information
void displayTokens(Item *list);

#endif
