#!/usr/bin/env python3

# Copyright 2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: ISC

import shutil
import sys

shutil.copy(sys.argv[1], sys.argv[2])
