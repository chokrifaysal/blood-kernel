#!/usr/bin/env python3
"""
misra.py - MISRA-C:2012 rule checker for cppcheck
Minimal subset for automotive kernels – no heap, no dynamic allocation.
"""

import cppcheckdata
import sys

# MISRA-C:2012 rules we actually care about in bare metal
RULES = {
    10.1: "dangerous conversion of enum",
    10.2: "dangerous conversion of signed/unsigned",
    10.3: "dangerous conversion of pointer type",
    10.4: "dangerous conversion of floating point",
    10.5: "dangerous conversion of integer to pointer",
    10.8: "composite expression with enum",
    12.1: "implicit conversion of signed to unsigned",
    12.2: "shift count negative or too big",
    14.1: "dead code",
    15.5: "return from function ends with 'return'/'break'",
    15.6: "loop body only one statement",
    17.2: "recursive function detected",
    20.7: "macro argument not bracketed",
    20.10: "macro redefinition",
    21.3: "malloc/calloc/realloc/free detected",
    21.9: "bsearch/qsort detected",
    22.2: "memory leak",
}

def report(rule, cfg, token, msg):
    print(f"{token.file}:{token.linenr}: misra-{rule}: {msg}")

def check_rule_21_3(cfg):
    for token in cfg.tokenlist:
        if token.str in ("malloc", "calloc", "realloc", "free"):
            report(21.3, cfg, token, f"dynamic allocation '{token.str}' forbidden")

def check_rule_20_7(cfg):
    for token in cfg.tokenlist:
        if token.str == "#define":
            nxt = token.next
            while nxt and nxt.str != '\n':
                if nxt.str == '(' and nxt.next and nxt.next.str not in (' ', '\t'):
                    report(20.7, cfg, token, "macro argument not bracketed")
                nxt = nxt.next

def check_rule_10_8(cfg):
    for token in cfg.tokenlist:
        if token.str == 'enum' and token.next and token.next.str == '{':
            # crude composite expression check
            pass

def check_rule_15_6(cfg):
    for scope in cfg.scopes:
        if scope.type in ('While', 'For'):
            body = scope.bodyStart.next
            if body and body.next and body.next.str == ';':
                report(15.6, cfg, scope.bodyStart, "loop body single statement")

def check_all(cfg):
    check_rule_21_3(cfg)
    check_rule_20_7(cfg)
    check_rule_15_6(cfg)
    # TODO: add more as needed

def main():
    for cfg in cppcheckdata.parsedump(sys.argv[1]):
        check_all(cfg)

if __name__ == '__main__':
    main()
