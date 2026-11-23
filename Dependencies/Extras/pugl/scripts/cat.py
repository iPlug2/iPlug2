#!/usr/bin/env python3

# Copyright 2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: ISC

import sys

for filename in sys.argv[1:]:
    with open(filename, 'r') as f:
        sys.stdout.write(f.read())
