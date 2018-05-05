
# generated from a Rosetta Code github repo with some help from
# find . -name "*.ss" | xargs grep -Eo '\([^\ ]+' | sort | uniq -c | sort -rn | sed -E 's/\ +([0-9]+)\ +\((.+)/(\1, "\2"),/g' | head -n 100
from collections import namedtuple
from itertools import groupby
import sys

Symbol = namedtuple('Symbol', ['count', 'chars', 'full_str'])
CharNode = namedtuple('CharNode', ['predicate', 'full_str'])
Branch = namedtuple('BranchNode', ['predicate', 'branches'])

# duplicate symbol string to simplify recursion
symbols = [Symbol(s[0], s[1], s[1]) for s in [
    (374, "+"),
    (319, "car"),
    (243, "-"),
    (228, "cdr"),
    (209, "*"),
    (170, "lambda"),
    (126, "cons"),
    (124, "display"),
    (110, "list"),
    (109, "/"),
    (94, "null?"),
    (90, "="),
    (67, "length"),
    (66, "map"),
    (66, "list-ref"),
    (58, "<"),
    (57, "vector-ref"),
    (56, ">"),
    (38, "zero?"),
    (37, "if"),
    (37, "apply"),
    (36, "scheme"),
    (36, "not"),
    (31, "string-length"),
    (31, "remainder"),
    (29, "cadr"),
    (28, "modulo"),
    (28, "expt"),
    (27, "iota"),
    (26, "quotient"),
    (25, "string-append"),
    (24, "reverse"),
]]

def make_identifier(str):
    return str.upper(
        ).replace('*', '__STAR__'
        ).replace('<', '__GREATER_THAN__'
        ).replace('>', '__LESS_THAN__'
        ).replace('?', '__QUESTION_MARK__'
        ).replace('-', '__DASH__'
        ).replace('/', '__FORWARD_SLASH__'
        ).replace('+', '__PLUS__'
        )


def split_list(keyfn, lst):
    """ 
    iterates through list, adding to `start` while `keyfn` is false, 
    `middle` while true, `end` with the remainder of list
    :return: (`start`, `middle`, `end`)
    """
    start = []
    middle = []
    end = []
    i = 0
    while i < len(lst) and not keyfn(lst[i]):
        start.append(lst[i])
        i += 1
    while i < len(lst) and keyfn(lst[i]):
        middle.append(lst[i])
        i += 1
    while i < len(lst):
        end.append(lst[i])
        i += 1
    return (start, middle, end)    


def tree_split(symbols, confirmed_length=1):
    res = []

    # strip out zero length symbols
    zero_len_symbols = [s for s in symbols if len(s.chars) == 0]       
    assert(len(zero_len_symbols) <= 1)
    res.extend([
        CharNode("len == %d" % len(s.full_str), s.full_str) 
        for s in zero_len_symbols])

    symbols = [s for s in symbols if len(s.chars) > 0]
    symbols = sorted(symbols, key=lambda x: x.chars)

    if len(symbols) == 0:
        return res

     # current char index in symbol comparison
    cur_index = len(symbols[0].full_str) - len(symbols[0].chars)

    # find an efficent grouping
    GroupedCharConditionCount = namedtuple(
        'GroupedCharConditionCount', ['char_count', 'condition_count'])
    least_conditions = GroupedCharConditionCount(1, sys.maxsize)
    for i in range(1, min([len(s.full_str)+1 for s in symbols])):
        # characters grouped by same first i-characters
        grps = [
            (cs, [s for s in l]) 
            for (cs, l) in groupby(symbols, lambda x: x[:i])
            ]
        condition_count = sum([len(s) for ss in grps for s in ss[1]])
        if condition_count < least_conditions.condition_count:
            least_conditions = GroupedCharConditionCount(i, condition_count)
    
    symbol_prefix_groupings = (
        groupby(symbols, lambda x: x[:least_conditions.char_count]))

    for common_prefix, prefixed_symbols in symbol_prefix_groupings:
        char_checks = [
            "str[%d] == '%c'" % (cur_index + i, c) 
            for i, c in enumerate(common_prefix)
            ]
        length_check = cur_index + len(common_prefix)
        if length_check > confirmed_length:
            length_check = 'len == %d && ' % (len(symbols[0].full_str))
        else:
            length_check = ''
        res.append(
            Branch(
                length_check + " && ".join(char_checks), 
                tree_split(
                    [Symbol(s.count, s.chars[len(common_prefix):], s.full_str) 
                    for s in prefixed_symbols])
            ))

    char_and_length_check =  length_check + ' && '.join(char_checks)

    res.extend([CharNode(char_and_length_check, s.full_str) for s in symbols])
    return res
    
    if len(symbols) == 1:
        midpoint = 0
    elif len(symbols) <= 3:
        # it's also >1 as this considered in the previous lines
        midpoint = 1
    else:
        # make the character comparison at midpoint of frequency of scheme fns
        total_count = sum([x.count for x in symbols])
        half_total_count = total_count / 2
        for i, symbol in enumerate(symbols):
            half_total_count -= symbol.count
            if half_total_count <= 0:
                midpoint = i
                break
                
    (start, middle, end) = (
        split_list(keyfn=lambda s: s.chars[0] == symbols[midpoint].chars[0], 
                   lst=symbols))
    
    # middle symbols are ones where char-equality is used.  once an equality
    #  predicate is used, the first char must be remove from the symbol
    #  - this is how the recursion terminates.  re-enable length checks
    middle = [Symbol(s.count, s.chars[1:], s.full_str) for s in middle]

    def predicate(operator):
        fmt = "str[%(index)d] %(oper)s '%(char)c'"
        return fmt % ({
            'index': cur_index,
            'oper': operator,
            'char': symbols[midpoint].chars[0],
        })

    # add a deep branch with a length check if length > 0
    if middle:
        res.append(Branch(predicate('=='), tree_split(middle, cur_index)))
    if end:
        res.append(Branch(predicate('>'), tree_split(end, cur_index)))
    if start:
        res.append(Branch(predicate('<'), tree_split(start, cur_index)))

    always_trues = [b for b in res if b.predicate == "TRUE"]   
    sometimes_trues = [b for b in res if b.predicate != "TRUE"]
    if sometimes_trues and cur_index > 0:
        # the sometimes trues need a length check
        return always_trues + [Branch("len > %d" % cur_index, sometimes_trues)]
    else:
        return always_trues + sometimes_trues

res = tree_split(symbols)

def print_tree(node_tree, prefix=''):
    #import pdb; pdb.set_trace()
    for i, node in enumerate(node_tree):
        branchtype = "if" if i == 0 else "else if"
        if node.predicate != "TRUE":
            print(prefix, branchtype, "(", node.predicate, ") {")
            prefix += "  "
            
        if isinstance(node, Branch):
            print_tree(node.branches, prefix)
        else:
            if node.full_str == "list":
                pass #import pdb; pdb.set_trace()
            print(prefix, 
                ("return " 
                + make_identifier(node.full_str) 
                + ";  // '" 
                + node.full_str + "'"))
        
        if node.predicate != "TRUE":
            prefix = prefix[:-2]
            print(prefix, "}")

print_tree(res)
