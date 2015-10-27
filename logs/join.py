#!/usr/bin/env python

import sys

glue = ""

for line in sys.stdin.readlines():
        line = line.rstrip()
        if len(line) == 2:
                glue = glue + line
        else:
                if glue != '':
                        print glue
                        glue = ""
                print line
