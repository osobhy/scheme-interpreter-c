#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"
#include "linkedlist.h"
#include "talloc.h"

// a version of strdup but with talloc. takes in a string
// and returns a newly allocated copy of that string using talloc
char *talloc_strdup(const char *s) {
    if (!s) {
        return NULL;
    }
    size_t len = strlen(s) + 1;
    char *new_s = talloc(len);
    if (new_s) strcpy(new_s, s);
    return new_s;
}

// handles errors during evaluation. takes in an error
// message and prints it. does not return anything.
// it then safely exits the program using texit()
void evaluationError(const char *message) {
    printf("Evaluation error: %s\n", message);
    texit(1);
}

// create a new frame with a specified parent. takes
// in the parents and returns the frame
Frame *createFrame(Frame *parent) {
    Frame *frame = talloc(sizeof(Frame));
    frame->bindings = makeNull();
    frame->parent = parent;
    return frame;
}

// add a binding of a variable to a value in a frame. takes in a frame pointer, 
// a variable name, and a value pointer. does not return anything
void addBinding(Frame *frame, char *var, Item *value) {
    Item *symbol = talloc(sizeof(Item));
    symbol->type = SYMBOL_TYPE;
    symbol->s = talloc_strdup(var);
    Item *binding = cons(symbol, value);
    frame->bindings = cons(binding, frame->bindings);
}

// look up a binding in the current frame or its parents.
Item *lookupSymbol(char *symbol, Frame *frame) {
    while (frame != NULL) {
        Item *binding = frame->bindings;
        while (!isNull(binding)) {
            Item *currentBinding = car(binding);
            if (strcmp(car(currentBinding)->s, symbol) == 0) {
                return cdr(currentBinding);
            }
            binding = cdr(binding);
        }
        frame = frame->parent;
    }
    evaluationError("Unbound symbol");
    return NULL;
}

// Evaluate the body of a lambda function. Takes in a list of expressions
// and a frame pointer. Returns the result of evaluating the body.
Item *evalBody(Item *body, Frame *frame) {
    Item *result = makeNull();
    while (!isNull(body)) {
        result = eval(car(body), frame);
        body = cdr(body);
    }
    return result;
}

// evaluate an if expression. takes in a list of arguments and a frame pointer, 
// returns the result of evaluating the right branch
Item *evalIf(Item *args, Frame *frame) {
    if (length(args) != 3) {
        evaluationError("if expects exactly 3 arguments");
    }
    Item *test = eval(car(args), frame);
    if (test->type != BOOL_TYPE) {
        evaluationError("if expects a boolean as the first argument");
    }
    if (test->i) {
        return eval(car(cdr(args)), frame);
    } else {
        return eval(car(cdr(cdr(args))), frame);
    }
}

// evaluate a let expression. takes in a list of arguments and a frame pointer,
// returns the result of evaluating the body in a new frame
Item *evalLet(Item *args, Frame *frame) {
    if (length(args) < 2) {
        evaluationError("let expects at least 2 arguments");
    }

    Item *bindings = car(args);
    Item *body = cdr(args);
    if (bindings->type != CONS_TYPE && bindings->type != NULL_TYPE) {
        evaluationError("not a list");
    }

    Frame *letFrame = createFrame(frame);
    while (!isNull(bindings)) {
        Item *currentBinding = car(bindings);
        if (currentBinding->type != CONS_TYPE || length(currentBinding) != 2) {
            evaluationError("binding invalid");
        }
        Item *var = car(currentBinding);
        if (var->type != SYMBOL_TYPE) {
            evaluationError("variable doesn't exist");
        }

        Item *innerBindings = car(args);
        while (innerBindings != bindings) {
            if (strcmp(car(car(innerBindings))->s, var->s) == 0) {
                evaluationError("variable duplicate");
            }
            innerBindings = cdr(innerBindings);
        }

        Item *value = eval(car(cdr(currentBinding)), frame);
        addBinding(letFrame, var->s, value);
        bindings = cdr(bindings);
    }

    return evalBody(body, letFrame);
}

// evaluate a define expression. takes in arguments pointer and a frame 
// pointer, returns a void type pointer
Item *evalDefine(Item *args, Frame *frame) {
    if (length(args) != 2) {
        evaluationError("there must be 2 arguments for define");
    }
    Item *varName = car(args);
    if (varName->type != SYMBOL_TYPE) {
        evaluationError("the first argument must be symbol");
    }
    Item *expression = car(cdr(args));
    Item *result = eval(expression, frame);
    addBinding(frame, varName->s, result);
    Item *voidReturn = talloc(sizeof(Item));
    voidReturn->type = VOID_TYPE;
    return voidReturn;
}

// evaluate a lambda expression. takes in arguments pointer and a frame 
// pointer, returns a closure type pointer
Item *evalLambda(Item *args, Frame *frame) {
    if (length(args) < 2) {
        evaluationError("there must be at least 2 arguments");
    }

    Item *params = car(args);
    Item *body = cdr(args);

    if (params->type != CONS_TYPE && params->type != NULL_TYPE && params->type != SYMBOL_TYPE) {
        evaluationError("must be list of parameters");
    }

    if (params->type == CONS_TYPE) {
        Item *paramList = params;
        while (paramList->type == CONS_TYPE) {
            Item *param = car(paramList);
            if (param->type != SYMBOL_TYPE) {
                evaluationError("parameters must be symbols");
            }
            Item *innerList = params;
            while (innerList != paramList) {
                if (strcmp(car(innerList)->s, param->s) == 0) {
                    evaluationError("repeated symbol");
                }
                innerList = cdr(innerList);
            }
            paramList = cdr(paramList);
        }
        if (paramList->type != NULL_TYPE) {
            evaluationError("must be a list");
        }
    } else if (params->type != SYMBOL_TYPE && params->type != NULL_TYPE) {
        evaluationError("must be list of symbols or single symbol");
    }

    Item *closure = talloc(sizeof(Item));
    closure->type = CLOSURE_TYPE;
    closure->cl.paramNames = params;
    closure->cl.functionCode = body;
    closure->cl.frame = frame;
    return closure;
}

// evaluate a quote expression. takes in arguments and returns
// the unevaluated argument
Item *evalQuote(Item *args) {
    if (length(args) != 1) {
        evaluationError("quote expects one argument");
    }
    return car(args);
}

// apply a function to arguments. takes in a function pointer and an 
// arguments pointer. returns the result of applying the function 
Item *apply(Item *function, Item *args) {
    if (function->type == PRIMITIVE_TYPE) {
        return function->pf(args);
    } else if (function->type != CLOSURE_TYPE) {
        evaluationError("not a function");
    }
    
    Frame *newFrame = createFrame(function->cl.frame);
    Item *paramNames = function->cl.paramNames;

    while (!isNull(paramNames)) {
        if (paramNames->type == SYMBOL_TYPE) {
            addBinding(newFrame, paramNames->s, args);
            return evalBody(function->cl.functionCode, newFrame);
        }
        if (isNull(args)) {
            evaluationError("too few arguments");
        }
        addBinding(newFrame, car(paramNames)->s, car(args));
        paramNames = cdr(paramNames);
        args = cdr(args);
    }

    if (!isNull(args)) {
        evaluationError("too many arguments");
    }

    return evalBody(function->cl.functionCode, newFrame);
}

// recursively evaluate each element in a given list
// in a frame. takes in a list and a frame and evaluates the list's
// arguments
Item *evalList(Item *list, Frame *frame) {
    if (isNull(list)) {
        return makeNull();
    } else {
        Item *evaluatedCar = eval(car(list), frame);
        Item *evaluatedCdr = evalList(cdr(list), frame);
        return cons(evaluatedCar, evaluatedCdr);
    }
}

// implement let* special form. takes in at least 2 arguments
// and a frame and returns their letstar evaluation
Item *evalLetStar(Item *args, Frame *frame) {
    if (length(args) < 2) {
        evaluationError("not 2 arguments");
    }
    
    Item *bindings = car(args);
    Item *body = cdr(args);
    Frame *letStarFrame = frame;

    while (!isNull(bindings)) {
        Item *currentBinding = car(bindings);
        if (currentBinding->type != CONS_TYPE || length(currentBinding) != 2) {
            evaluationError("binding invalid");
        }
        Item *var = car(currentBinding);
        if (var->type != SYMBOL_TYPE) {
            evaluationError("variable doesn't exist");
        }
        Item *value = eval(car(cdr(currentBinding)), letStarFrame);
        letStarFrame = createFrame(letStarFrame);
        addBinding(letStarFrame, var->s, value);
        bindings = cdr(bindings);
    }
    
    return evalBody(body, letStarFrame);
}

// implement letrec special form. takes in at least 2 arguments
// and a frame and returns an evaluation of these 2 arguments'
// body recursively
Item *evalLetRec(Item *args, Frame *frame) {
    if (length(args) < 2) {
        evaluationError("not 2 arguments");
    }

    Item *bindings = car(args);
    Item *body = cdr(args);
    Frame *letRecFrame = createFrame(frame);

    Item *tempBindings = bindings;
    while (!isNull(tempBindings)) {
        Item *currentBinding = car(tempBindings);
        if (currentBinding->type != CONS_TYPE || length(currentBinding) != 2) {
            evaluationError("binding invalid");
        }
        Item *var = car(currentBinding);
        if (var->type != SYMBOL_TYPE) {
            evaluationError("variable doesn't exist");
        }
        Item *nullItem = talloc(sizeof(Item));
        nullItem->type = NULL_TYPE;
        addBinding(letRecFrame, var->s, nullItem);
        tempBindings = cdr(tempBindings);
    }

    tempBindings = bindings;
    while (!isNull(tempBindings)) {
        Item *currentBinding = car(tempBindings);
        Item *var = car(currentBinding);
        Item *value = eval(car(cdr(currentBinding)), letRecFrame);

        if (value->type == NULL_TYPE) {
            evaluationError("variable cannot be NULL");
        }

        Item *binding = lookupSymbol(var->s, letRecFrame);
        binding->type = value->type;
        switch (value->type) {
            case INT_TYPE:
                binding->i = value->i;
                break;
            case DOUBLE_TYPE:
                binding->d = value->d;
                break;
            case STR_TYPE:
            case SYMBOL_TYPE:
                binding->s = talloc_strdup(value->s);
                break;
            case CONS_TYPE:
                binding->c.car = value->c.car;
                binding->c.cdr = value->c.cdr;
                break;
            case CLOSURE_TYPE:
                binding->cl.paramNames = value->cl.paramNames;
                binding->cl.functionCode = value->cl.functionCode;
                binding->cl.frame = value->cl.frame;
                break;
            case PRIMITIVE_TYPE:
                binding->pf = value->pf;
                break;
            default:
                evaluationError("type unknown");
        }
        tempBindings = cdr(tempBindings);
    }

    return evalBody(body, letRecFrame);
}

// implement set! special form. takes in two arguments and a frame
// and returns a void_type
Item *evalSet(Item *args, Frame *frame) {
    if (length(args) != 2) {
        evaluationError("not 2 arguments");
    }
    Item *var = car(args);
    if (var->type != SYMBOL_TYPE) {
        evaluationError("not a symbol");
    }
    Item *value = eval(car(cdr(args)), frame);
    Item *binding = lookupSymbol(var->s, frame);
    binding->type = value->type;
    binding->s = value->s;
    Item *voidReturn = talloc(sizeof(Item));
    voidReturn->type = VOID_TYPE;
    return voidReturn;
}

// implement set-car! special form. takes in two arguments and a frame
// and returns void type
Item *evalSetCar(Item *args, Frame *frame) {
    if (length(args) != 2) {
        evaluationError("not 2 arguments");
    }
    Item *pair = eval(car(args), frame);
    if (pair->type != CONS_TYPE) {
        evaluationError("not a pair");
    }
    Item *value = eval(car(cdr(args)), frame);
    pair->c.car = value;
    Item *voidReturn = talloc(sizeof(Item));
    voidReturn->type = VOID_TYPE;
    return voidReturn;
}

// implements cond special form. takes in arguments that are clauses
// and a frame and returns a void type
Item *evalCond(Item *args, Frame *frame) {
    while (!isNull(args)) {
        Item *clause = car(args);
        if (clause->type != CONS_TYPE || length(clause) < 1) {
            evaluationError("clauses can't be empty lists");
        }
        Item *test = car(clause);
        if (test->type == SYMBOL_TYPE && strcmp(test->s, "else") == 0) {
            return evalBody(cdr(clause), frame);
        }
        Item *result = eval(test, frame);
        if (result->type == BOOL_TYPE && result->i) {
            return evalBody(cdr(clause), frame);
        }
        args = cdr(args);
    }
    Item *voidReturn = talloc(sizeof(Item));
    voidReturn->type = VOID_TYPE;
    return voidReturn;
}


// implement set-cdr! special form. takes in 2 arguments
// and a frame and returns a void_type
Item *evalSetCdr(Item *args, Frame *frame) {
    if (length(args) != 2) {
        evaluationError("set-cdr! expects exactly 2 arguments");
    }
    Item *pair = eval(car(args), frame);
    if (pair->type != CONS_TYPE) {
        evaluationError("set-cdr! expects a pair as the first argument");
    }
    Item *value = eval(car(cdr(args)), frame);
    pair->c.cdr = value;
    Item *voidReturn = talloc(sizeof(Item));
    voidReturn->type = VOID_TYPE;
    return voidReturn;
}

// implement and special form. takes in boolean arguments
// and returns true if all are true.
Item *evalAnd(Item *args, Frame *frame) {
    while (!isNull(args)) {
        Item *result = eval(car(args), frame);
        if (result->type != BOOL_TYPE) {
            evaluationError("boolean arguments expected");
        }
        if (!result->i) {
            return result;
        }
        args = cdr(args);
    }
    Item *trueItem = talloc(sizeof(Item));
    trueItem->type = BOOL_TYPE;
    trueItem->i = 1;
    return trueItem;
}

// implements or. takes in boolean arguments and a frame
// and returns true if one of them is true
Item *evalOr(Item *args, Frame *frame) {
    while (!isNull(args)) {
        Item *result = eval(car(args), frame);
        if (result->type != BOOL_TYPE) {
            evaluationError("boolean arguments expected");
        }
        if (result->i) {
            return result;
        }
        args = cdr(args);
    }
    Item *falseItem = talloc(sizeof(Item));
    falseItem->type = BOOL_TYPE;
    falseItem->i = 0;
    return falseItem;
}

// evaluate an expression in a given frame. takes in a parsed tree
// and a frame and returns what it evaluates to
Item *eval(Item *tree, Frame *frame) {
    if (tree == NULL) return NULL;

    switch (tree->type) {
        case INT_TYPE:
        case DOUBLE_TYPE:
        case STR_TYPE:
        case BOOL_TYPE:
            return tree;
        case SYMBOL_TYPE:
            return lookupSymbol(tree->s, frame);
        case CONS_TYPE: {
            Item *first = car(tree);
            Item *args = cdr(tree);
            if (first->type != SYMBOL_TYPE) {
                Item *function = eval(first, frame);
                Item *evaluatedArgs = evalList(args, frame);
                return apply(function, evaluatedArgs);
            }
            if (strcmp(first->s, "define") == 0) {
                return evalDefine(args, frame);
            } else if (strcmp(first->s, "let") == 0) {
                return evalLet(args, frame);
            } else if (strcmp(first->s, "let*") == 0) {
                return evalLetStar(args, frame);
            } else if (strcmp(first->s, "letrec") == 0) {
                return evalLetRec(args, frame);
            } else if (strcmp(first->s, "set!") == 0) {
                return evalSet(args, frame);
            } else if (strcmp(first->s, "set-car!") == 0) {
                return evalSetCar(args, frame);
            } else if (strcmp(first->s, "set-cdr!") == 0) {
                return evalSetCdr(args, frame);
            } else if (strcmp(first->s, "lambda") == 0) {
                return evalLambda(args, frame);
            } else if (strcmp(first->s, "cond") == 0) {
                return evalCond(args, frame);
            } else if (strcmp(first->s, "if") == 0) {
                return evalIf(args, frame);
            } else if (strcmp(first->s, "quote") == 0) {
                return evalQuote(args);
            } else if (strcmp(first->s, "and") == 0) {
                return evalAnd(args, frame);
            } else if (strcmp(first->s, "or") == 0) {
                return evalOr(args, frame);
            } else {
                Item *function = eval(first, frame);
                Item *evaluatedArgs = evalList(args, frame);
                return apply(function, evaluatedArgs);
            }
        }
        default:
            evaluationError("unknown type");
            return NULL;
    }
}

void printItem(Item *item);

// helper function to print list elements. takes in a list
// and simply prints elements.
void printList(Item *list) {
    Item *current = list;
    while (current != NULL && current->type == CONS_TYPE) {
        printItem(current->c.car);
        current = current->c.cdr;
        if (current != NULL && current->type == CONS_TYPE) {
            printf(" ");
        }
    }
    
    if (current != NULL && current->type != NULL_TYPE) {
        printf(" . ");
        printItem(current);
    }
}

// function to print an item. takes in an item and
// prints it out. doesn't return anything
void printItem(Item *item) {
    if (item == NULL) {
        printf("()");
        return;
    }

    switch (item->type) {
        case INT_TYPE:
            printf("%d", item->i);
            break;
        case DOUBLE_TYPE:
            printf("%f", item->d);
            break;
        case STR_TYPE:
            printf("\"%s\"", item->s);
            break;
        case BOOL_TYPE:
            if (item->i) {
                printf("#t");
            } else {
                printf("#f");
            }
            break;
        case SYMBOL_TYPE:
            printf("%s", item->s);
            break;
        case CONS_TYPE:
            printf("(");
            printList(item);
            printf(")");
            break;
        case NULL_TYPE:
            printf("()");
            break;
        case VOID_TYPE:
            break;
        case CLOSURE_TYPE:
            printf("#<procedure>");
            break;
        default:
            printf("Unknown type");
            break;
    }
}

// implements minus. takes in two argument sand returns their minus.
Item *primitiveMinus(Item *args) {
    if (length(args) != 2) {
        evaluationError("not 2 arguments");
    }
    
    Item *a = car(args);
    Item *b = car(cdr(args));
    Item *result = talloc(sizeof(Item));

    double aVal, bVal;
    
    if (a->type == DOUBLE_TYPE) {
        aVal = a->d;
    } else if (a->type == INT_TYPE) {
        aVal = a->i;
    } else {
        evaluationError("first argument must be a number");
    }
    
    if (b->type == DOUBLE_TYPE) {
        bVal = b->d;
    } else if (b->type == INT_TYPE) {
        bVal = b->i;
    } else {
        evaluationError("second argument must be a number");
    }
    
    if (a->type == DOUBLE_TYPE || b->type == DOUBLE_TYPE) {
        result->type = DOUBLE_TYPE;
        result->d = aVal - bVal;
    } else {
        result->type = INT_TYPE;
        result->i = (int)(aVal - bVal);
    }
    
    return result;
}
// implements less. takes in two arguments and returns 
// a boolean whether one is less than the other
Item *primitiveLess(Item *args) {
    if (length(args) != 2) {
        evaluationError("not 2 arguments");
    }
    Item *a = car(args);
    Item *b = car(cdr(args));
    Item *result = talloc(sizeof(Item));
    result->type = BOOL_TYPE;
    if (a->type == DOUBLE_TYPE || b->type == DOUBLE_TYPE) {
        double aVal, bVal;

        if (a->type == DOUBLE_TYPE) {
            aVal = a->d;
        } else {
            aVal = a->i;
        }

        if (b->type == DOUBLE_TYPE) {
            bVal = b->d;
        } else {
            bVal = b->i;
        }

        result->i = aVal < bVal;
    } else if (a->type == INT_TYPE && b->type == INT_TYPE) {
        result->i = a->i < b->i;
    } else {
        evaluationError("not a number");
    }
    return result;
}

// implements less. takes in two arguments and returns 
// a boolean whether one is greater than the other
Item *primitiveGreater(Item *args) {
    if (length(args) != 2) {
        evaluationError("not 2 arguments");
    }

    Item *a = car(args);
    Item *b = car(cdr(args));
    Item *result = talloc(sizeof(Item));
    result->type = BOOL_TYPE;

    double aVal, bVal;

    if (a->type == DOUBLE_TYPE) {
        aVal = a->d;
    } else if (a->type == INT_TYPE) {
        aVal = a->i;
    } else {
        evaluationError("first argument must be a number");
    }

    if (b->type == DOUBLE_TYPE) {
        bVal = b->d;
    } else if (b->type == INT_TYPE) {
        bVal = b->i;
    } else {
        evaluationError("second argument must be a number");
    }

    if (a->type == DOUBLE_TYPE || b->type == DOUBLE_TYPE) {
        result->i = aVal > bVal;
    } else {
        result->i = a->i > b->i;
    }

    return result;
}

// primitive function for equal. takes in 2 args and returns
// a boolean whether they are equal or not
Item *primitiveEqual(Item *args) {
    if (length(args) != 2) {
        evaluationError("not 2 arguments");
    }
    Item *a = car(args);
    Item *b = car(cdr(args));
    Item *result = talloc(sizeof(Item));
    result->type = BOOL_TYPE;

    double aVal, bVal;

    if (a->type == DOUBLE_TYPE) {
        aVal = a->d;
    } else if (a->type == INT_TYPE) {
        aVal = a->i;
    } else {
        evaluationError("first argument must be a number");
    }

    if (b->type == DOUBLE_TYPE) {
        bVal = b->d;
    } else if (b->type == INT_TYPE) {
        bVal = b->i;
    } else {
        evaluationError("second argument must be a number");
    }

    if (a->type == DOUBLE_TYPE || b->type == DOUBLE_TYPE) {
        result->i = aVal == bVal;
    } else {
        result->i = a->i == b->i;
    }

    return result;
}

// primitive function for +. takes in arguments and
// returns their summation
Item *primitivePlus(Item *args) {
    Item *currentArg;
    double total = 0.0;
    int hasDouble = 0;

    while (!isNull(args)) {
        currentArg = car(args);
        switch (currentArg->type) {
            case DOUBLE_TYPE:
                total += currentArg->d;
                hasDouble = 1;
                break;
            case INT_TYPE:
                total += currentArg->i;
                break;
            default:
                evaluationError("not numbers");
                break;
        }
        args = cdr(args);
    }

    Item *finalResult = talloc(sizeof(Item));
    if (hasDouble) {
        finalResult->type = DOUBLE_TYPE;
        finalResult->d = total;
    } else {
        finalResult->type = INT_TYPE;
        finalResult->i = (int)total;
    }

    return finalResult;
}

// primitive function for 'null?'. takes in one argument
// and returns a boolean of whether or not it evaluated 
// to null
Item *primitiveNull(Item *args) {
    if (length(args) != 1) {
        evaluationError("null? expects one argument");
    }
    Item *arg = car(args);
    Item *result = talloc(sizeof(Item));
    result->type = BOOL_TYPE;
    result->i = isNull(arg);
    return result;
}

// primitive function for 'car'. takes in one argument
// and finds the car of it, then returns it
Item *primitiveCar(Item *args) {
    if (length(args) != 1) {
        evaluationError("car expects one argument");
    }
    Item *arg = car(args);
    if (arg->type != CONS_TYPE) {
        evaluationError("car expects a list");
    }
    return car(arg);
}

// primitive function for 'cdr'. takes in one argument
// and finds the cdr of it, then returns it
Item *primitiveCdr(Item *args) {
    if (length(args) != 1) {
        evaluationError("cdr expects one argument");
    }
    Item *arg = car(args);
    if (arg->type != CONS_TYPE) {
        evaluationError("cdr expects a list");
    }
    return cdr(arg);
}

// primitive function for 'cons'. takes in 2 arguments
// and returns a cons cell of both.
Item *primitiveCons(Item *args) {
    if (length(args) != 2) {
        evaluationError("cons expects two arguments");
    }
    Item *first = car(args);
    Item *second = car(cdr(args));
    return cons(first, second);
}

// primitive function for 'append'. takes in 2 arguments
// and returns an appended version of both.
Item *primitiveAppend(Item *args) {
    if (length(args) != 2) {
        evaluationError("append expects two arguments");
    }
    Item *first = car(args);
    Item *second = car(cdr(args));
    if (first->type != CONS_TYPE && first->type != NULL_TYPE) {
        evaluationError("first argument of append must be a list");
    }
    if (isNull(first)) {
        return second;
    } else {
        return cons(car(first), primitiveAppend(cons(cdr(first), cons(second, makeNull()))));
    }
}

// implements multiply. takes in >2 arguments and returns
// their multiplication
Item *primitiveMultiply(Item *args) {
    if (length(args) < 2) {
        evaluationError("requires at least 2 arguments");
    }

    double result = 1;
    int containsDouble = 0;

    while (!isNull(args)) {
        Item *currentArg = car(args);
        if (currentArg->type == DOUBLE_TYPE) {
            result *= currentArg->d;
            containsDouble = 1;
        } else if (currentArg->type == INT_TYPE) {
            result *= currentArg->i;
        } else {
            evaluationError("all arguments must be numbers");
        }
        args = cdr(args);
    }

    Item *finalResult = talloc(sizeof(Item));
    finalResult->type = containsDouble ? DOUBLE_TYPE : INT_TYPE;
    if (containsDouble) {
        finalResult->d = result;
    } else {
        finalResult->i = (int)result;
    }
    return finalResult;
}

// implements division. takes in 2 arguments and returns
// their divison.
Item *primitiveDivide(Item *args) {
    if (length(args) != 2) {
        evaluationError("2 arguments needed");
    }
    Item *a = car(args);
    Item *b = car(cdr(args));
    Item *result = talloc(sizeof(Item));
    if ((a->type == INT_TYPE || a->type == DOUBLE_TYPE) &&
        (b->type == INT_TYPE || b->type == DOUBLE_TYPE)) {
        double numerator = (a->type == DOUBLE_TYPE) ? a->d : a->i;
        double denominator = (b->type == DOUBLE_TYPE) ? b->d : b->i;
        if (denominator == 0) {
            evaluationError("can't divide by zero");
        }
        result->type = DOUBLE_TYPE;
        result->d = numerator / denominator;
    } else {
        evaluationError("not a number");
    }
    return result;
}

// implements modulo. takes in 2 arguments and returns
// the modulo result
Item *primitiveModulo(Item *args) {
    if (length(args) != 2) {
        evaluationError("2 arguments needed");
    }
    Item *result = talloc(sizeof(Item));
    Item *a = car(args);
    Item *b = car(cdr(args));
    if (a->type != INT_TYPE || b->type != INT_TYPE) {
        evaluationError("invalid arguments");
    }
    result->type = INT_TYPE;
    result->i = a->i % b->i;
    return result;
}


// binds a primitive function to a symbol in a frame,
// takes in the name of the symbol to which the primitive
// function will be bound,  pointer to the primitive function,
// and a pointer to the frame in which this binding should be
// made
void bind(char *name, Item *(*function)(Item *), Frame *frame) {
    Item *symbol = talloc(sizeof(Item));
    symbol->type = SYMBOL_TYPE;
    symbol->s = talloc_strdup(name);

    Item *prim = talloc(sizeof(Item));
    prim->type = PRIMITIVE_TYPE;
    prim->pf = function;

    Item *binding = cons(symbol, prim);
    frame->bindings = cons(binding, frame->bindings);
}

// main function to interpret the Scheme program. takes in
// a parse tree, evaluates it in a global frame, and prints
// what it evaluates to
void interpret(Item *tree) {
    Frame *globalFrame = createFrame(NULL);
    bind("+", primitivePlus, globalFrame);
    bind("-", primitiveMinus, globalFrame);
    bind("*", primitiveMultiply, globalFrame);
    bind("/", primitiveDivide, globalFrame);
    bind("modulo", primitiveModulo, globalFrame);
    bind("<", primitiveLess, globalFrame);
    bind(">", primitiveGreater, globalFrame);
    bind("=", primitiveEqual, globalFrame);
    bind("null?", primitiveNull, globalFrame);
    bind("car", primitiveCar, globalFrame);
    bind("cdr", primitiveCdr, globalFrame);
    bind("cons", primitiveCons, globalFrame);
    bind("append", primitiveAppend, globalFrame);

    while (tree != NULL && tree->type == CONS_TYPE) {
        Item *result = eval(car(tree), globalFrame);
        if (result->type != VOID_TYPE) {
            printItem(result);
            printf("\n"); 
        }
        tree = cdr(tree);
    }
}

