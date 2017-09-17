#ifndef THINLISP_H
#define THINLISP_H

#include "defines.h"
#include "bistack.h"


typedef struct symbol_binding {
    CELL *symbol_cell;
    CELL *bound_cell;
    struct symbol_binding *next;
} SYMBOL_BINDING;

typedef struct symbol_binding_frame {
    SYMBOL_BINDING *symbol_binding;
    struct symbol_binding_frame *next;  
} SYMBOL_BINDING_FRAME;


typedef struct environment {
    SYMBOL_BINDING_FRAME *binding_frame;
    BISTACK *bs;
} ENVIRONMENT;

void env_push_frame(ENVIRONMENT *env) {
    
}

void env_push_symbol(ENVIRONMENT *env, CELL *symbol_cell, CELL *bound_cell) {
    SYMBOL_BINDING *binding = bistack_heapalloc(
        env->bs, sizeof(SYMBOL_BINDING));   
    binding->next = env->binding_frame->symbol_binding;
    binding->symbol_cell = symbol_cell;
    binding->bound_cell = bound_cell;
    env->binding_frame->symbol_binding = binding;
}



#endif