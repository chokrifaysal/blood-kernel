#!/bin/sh
# misra_check.sh - run PC-Lint / cppcheck MISRA
cppcheck --enable=all --std=c11 --library=posix \
         --addon=misra.py src/ 2>&1 | tee misra.log
