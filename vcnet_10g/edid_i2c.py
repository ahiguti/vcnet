#!/usr/bin/env python3

import sys

i = 0
for line in sys.stdin:
  toks = line.split()
  if len(toks) != 18 or toks[1] != '|':
    continue
  for j in range(2, 18):
    print('6C', '%02X' % i, toks[j])
    i += 1
  # print(toks)

