#!/bin/sh
IFS='
'
for x in `gcc -MM "$@"`; do
    echo $x | grep -q "^ " -;
    if [ $? -eq 1 ]; then
        echo $x | sed 's/^/\$(BINDIR)\//g';
    else
        echo $x;
    fi
    echo $x | grep -q "\\\\\$" -;
    if [ $? -eq 1 ]; then
        echo "	\$(CC) \$(CFLAGS) \$(INCLUDES) -c -o \$@ \$<";
    fi
done
