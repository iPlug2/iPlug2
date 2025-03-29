#pragma once

#include "IGraphics.h"

#include <faust/gui/JSONUI.h>


BEGIN_IPLUG_NAMESPACE

class IGraphicsFaustUI :
{
public:
    IGraphicsFaustUI(IGraphics& pGraphics);

    void Draw(IGraphics& pGraphics);

private:
    IGraphics& mGraphics;
};

END_IPLUG_NAMESPACE
