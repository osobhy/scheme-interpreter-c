#include "item.h"

#ifndef INTERPRETER_H
#define INTERPRETER_H

void interpret(Item *tree);
Item *eval(Item *tree, Frame *frame);

#endif

