#pragma once
class IGraphics;
class IParam;

class IGraphicsDelegate
{
public:
  IGraphicsDelegate() {}
  virtual ~IGraphicsDelegate() {}
  virtual IGraphics* GetUI() = 0;

  virtual IParam* GetParamFromUI(int paramIdx) = 0;
  virtual void BeginInformHostOfParamChangeFromUI(int paramIdx) = 0;
  virtual void SetParameterValueFromUI(int paramIdx, double value) = 0;
  virtual void EndInformHostOfParamChangeFromUI(int paramIdx) = 0;
};
