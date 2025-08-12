#!/bin/sh
# misra_check.sh – run MISRA-C:2012 via cppcheck
cppcheck --enable=all --std=c11 --library=posix \
         --addon=tools/misra.py src/ 2>&1 | tee misra.log
