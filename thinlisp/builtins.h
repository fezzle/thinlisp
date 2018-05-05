#include "thinlisp.h"

builtin_fn ADD;
builtin_fn CAR;
builtin_fn SUBTRACT;
builtin_fn CDR;
builtin_fn MULTIPLY;
builtin_fn LAMBDA;
builtin_fn CONS;
builtin_fn DISPLAY;
builtin_fn LIST;
builtin_fn DIVIDE;
builtin_fn NULL__QUESTION_MARK__;
builtin_fn EQUALS;
builtin_fn LENGTH;
builtin_fn MAP;
builtin_fn LIST__REF;
builtin_fn __GREATER_THAN__;
builtin_fn VECTOR__REF;
builtin_fn __LESS_THAN__;
builtin_fn ZERO__QUESTION_MARK__;
builtin_fn IF;
builtin_fn APPLY;
builtin_fn SCHEME;
builtin_fn NOT;
builtin_fn STRING__LENGTH;
builtin_fn REMAINDER;
builtin_fn CADR;
builtin_fn MODULO;
builtin_fn EXPT;
builtin_fn IOTA;
builtin_fn QUOTIENT;
builtin_fn STRING__APPEND;
builtin_fn REVERSE;

builtin_fn resolve_builtin(char *str, uint8_t len) {
    if ( str[0] == 'c' ) {
    if ( len > 1 ) {
        if ( str[1] == 'd' ) {
        if ( len == 3 && str[2] == 'r' ) {
            return CDR;  // 'cdr'
        }
        }
        else if ( str[1] > 'd' ) {
        if ( len > 3 && str[1] == 'o' && str[2] == 'n' && str[3] == 's' ) {
            if ( len == 4 ) {
            return CONS;  // 'cons'
            }
        }
        }
        else if ( str[1] < 'd' ) {
        if ( len > 1 ) {
            if ( str[1] == 'a' ) {
            if ( len > 2 ) {
                if ( str[2] == 'r' ) {
                if ( len == 3 ) {
                    return CAR;  // 'car'
                }
                }
                else if ( str[2] < 'r' ) {
                if ( len > 3 && str[2] == 'd' && str[3] == 'r' ) {
                    if ( len == 4 ) {
                    return CADR;  // 'cadr'
                    }
                }
                }
            }
            }
        }
        }
    }
    }
    else if ( str[0] > 'c' ) {
    if ( str[0] == 'l' ) {
        if ( len > 1 ) {
        if ( str[1] == 'e' ) {
            if ( len > 5 && str[2] == 'n' && str[3] == 'g' && str[4] == 't' && str[5] == 'h' ) {
            if ( len == 6 ) {
                return LENGTH;  // 'length'
            }
            }
        }
        else if ( str[1] > 'e' ) {
            if ( len > 3 && str[1] == 'i' && str[2] == 's' && str[3] == 't' ) {
            if ( len == 4 ) {
                return LIST;  // 'list'
            }
            else if ( len > 7 && str[4] == '-' && str[5] == 'r' && str[6] == 'e' && str[7] == 'f' ) {
                if ( len == 8 ) {
                return LIST__REF;  // 'list-ref'
                }
            }
            }
        }
        else if ( str[1] < 'e' ) {
            if ( len > 5 && str[1] == 'a' && str[2] == 'm' && str[3] == 'b' && str[4] == 'd' && str[5] == 'a' ) {
            if ( len == 6 ) {
                return LAMBDA;  // 'lambda'
            }
            }
        }
        }
    }
    else if ( str[0] > 'l' ) {
        if ( str[0] == 'q' ) {
        if ( len > 7 && str[1] == 'u' && str[2] == 'o' && str[3] == 't' && str[4] == 'i' && str[5] == 'e' && str[6] == 'n' && str[7] == 't' ) {
            if ( len == 8 ) {
            return QUOTIENT;  // 'quotient'
            }
        }
        }
        else if ( str[0] > 'q' ) {
        if ( str[0] == 's' ) {
            if ( len > 1 ) {
            if ( str[1] == 't' ) {
                if ( len > 6 && str[2] == 'r' && str[3] == 'i' && str[4] == 'n' && str[5] == 'g' && str[6] == '-' ) {
                if ( len > 7 ) {
                    if ( str[7] == 'l' ) {
                    if ( len > 12 && str[8] == 'e' && str[9] == 'n' && str[10] == 'g' && str[11] == 't' && str[12] == 'h' ) {
                        if ( len == 13 ) {
                        return STRING__LENGTH;  // 'string-length'
                        }
                    }
                    }
                    else if ( str[7] < 'l' ) {
                    if ( len > 12 && str[7] == 'a' && str[8] == 'p' && str[9] == 'p' && str[10] == 'e' && str[11] == 'n' && str[12] == 'd' ) {
                        if ( len == 13 ) {
                        return STRING__APPEND;  // 'string-append'
                        }
                    }
                    }
                }
                }
            }
            else if ( str[1] < 't' ) {
                if ( len > 5 && str[1] == 'c' && str[2] == 'h' && str[3] == 'e' && str[4] == 'm' && str[5] == 'e' ) {
                if ( len == 6 ) {
                    return SCHEME;  // 'scheme'
                }
                }
            }
            }
        }
        else if ( str[0] > 's' ) {
            if ( str[0] == 'z' ) {
            if ( len > 4 && str[1] == 'e' && str[2] == 'r' && str[3] == 'o' && str[4] == '?' ) {
                if ( len == 5 ) {
                return ZERO__QUESTION_MARK__;  // 'zero?'
                }
            }
            }
            else if ( str[0] < 'z' ) {
            if ( len > 9 && str[0] == 'v' && str[1] == 'e' && str[2] == 'c' && str[3] == 't' && str[4] == 'o' && str[5] == 'r' && str[6] == '-' && str[7] == 'r' && str[8] == 'e' && str[9] == 'f' ) {
                if ( len == 10 ) {
                return VECTOR__REF;  // 'vector-ref'
                }
            }
            }
        }
        else if ( str[0] < 's' ) {
            if ( len > 1 && str[0] == 'r' && str[1] == 'e' ) {
            if ( len > 2 ) {
                if ( str[2] == 'v' ) {
                if ( len > 6 && str[3] == 'e' && str[4] == 'r' && str[5] == 's' && str[6] == 'e' ) {
                    if ( len == 7 ) {
                    return REVERSE;  // 'reverse'
                    }
                }
                }
                else if ( str[2] < 'v' ) {
                if ( len > 8 && str[2] == 'm' && str[3] == 'a' && str[4] == 'i' && str[5] == 'n' && str[6] == 'd' && str[7] == 'e' && str[8] == 'r' ) {
                    if ( len == 9 ) {
                    return REMAINDER;  // 'remainder'
                    }
                }
                }
            }
            }
        }
        }
        else if ( str[0] < 'q' ) {
        if ( str[0] == 'n' ) {
            if ( len > 1 ) {
            if ( str[1] == 'u' ) {
                if ( len > 4 && str[2] == 'l' && str[3] == 'l' && str[4] == '?' ) {
                if ( len == 5 ) {
                    return NULL__QUESTION_MARK__;  // 'null?'
                }
                }
            }
            else if ( str[1] < 'u' ) {
                if ( len > 2 && str[1] == 'o' && str[2] == 't' ) {
                if ( len == 3 ) {
                    return NOT;  // 'not'
                }
                }
            }
            }
        }
        else if ( str[0] < 'n' ) {
            if ( str[0] == 'm' ) {
            if ( len > 1 ) {
                if ( str[1] == 'o' ) {
                if ( len > 5 && str[2] == 'd' && str[3] == 'u' && str[4] == 'l' && str[5] == 'o' ) {
                    if ( len == 6 ) {
                    return MODULO;  // 'modulo'
                    }
                }
                }
                else if ( str[1] < 'o' ) {
                if ( len > 2 && str[1] == 'a' && str[2] == 'p' ) {
                    if ( len == 3 ) {
                    return MAP;  // 'map'
                    }
                }
                }
            }
            }
        }
        }
    }
    else if ( str[0] < 'l' ) {
        if ( str[0] == 'd' ) {
        if ( len > 6 && str[1] == 'i' && str[2] == 's' && str[3] == 'p' && str[4] == 'l' && str[5] == 'a' && str[6] == 'y' ) {
            if ( len == 7 ) {
            return DISPLAY;  // 'display'
            }
        }
        }
        else if ( str[0] > 'd' ) {
        if ( str[0] == 'i' ) {
            if ( len > 1 ) {
            if ( str[1] == 'o' ) {
                if ( len > 3 && str[2] == 't' && str[3] == 'a' ) {
                if ( len == 4 ) {
                    return IOTA;  // 'iota'
                }
                }
            }
            else if ( str[1] < 'o' ) {
                if ( len == 2 && str[1] == 'f' ) {
                return IF;  // 'if'
                }
            }
            }
        }
        else if ( str[0] < 'i' ) {
            if ( len > 3 && str[0] == 'e' && str[1] == 'x' && str[2] == 'p' && str[3] == 't' ) {
            if ( len == 4 ) {
                return EXPT;  // 'expt'
            }
            }
        }
        }
    }
    }
    else if ( str[0] < 'c' ) {
    if ( str[0] == '-' ) {
        if ( len == 1 ) {
        return SUBTRACT;  // '-'
        }
    }
    else if ( str[0] > '-' ) {
        if ( str[0] == '=' ) {
        if ( len == 1 ) {
            return EQUALS;  // '='
        }
        }
        else if ( str[0] > '=' ) {
        if ( str[0] == 'a' ) {
            if ( len > 4 && str[1] == 'p' && str[2] == 'p' && str[3] == 'l' && str[4] == 'y' ) {
            if ( len == 5 ) {
                return APPLY;  // 'apply'
            }
            }
        }
        else if ( str[0] < 'a' ) {
            if ( len == 1 && str[0] == '>' ) {
            return __LESS_THAN__;  // '>'
            }
        }
        }
        else if ( str[0] < '=' ) {
        if ( str[0] == '<' ) {
            if ( len == 1 ) {
            return __GREATER_THAN__;  // '<'
            }
        }
        else if ( str[0] < '<' ) {
            if ( len == 1 && str[0] == '/' ) {
            return DIVIDE;  // '/'
            }
        }
        }
    }
    else if ( str[0] < '-' ) {
        if ( str[0] == '+' ) {
        if ( len == 1 ) {
            return ADD;  // '+'
        }
        }
        else if ( str[0] < '+' ) {
        if ( len == 1 && str[0] == '*' ) {
            return MULTIPLY;  // '*'
        }
        }
    }
    }
}