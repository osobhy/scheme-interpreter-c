#include <stdio.h>
#include "tokenizer.h"
#include "item.h"
#include "linkedlist.h"
#include "parser.h"
#include "talloc.h"
#include "interpreter.h"

int main() {

    Item *list = tokenize();
    Item *tree = parse(list);
    interpret(tree);

    tfree();
    return 0;
}
