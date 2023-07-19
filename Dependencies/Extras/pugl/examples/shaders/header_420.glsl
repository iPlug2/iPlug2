// Copyright 2020-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#version 420 core

#define NOPERSPECTIVE noperspective
#define INTER(qualifiers) layout(qualifiers)
#define UBO(qualifiers) layout(std140, qualifiers)
