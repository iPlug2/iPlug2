#ifndef _EDITOR_INTERFACE_H_
#define _EDITOR_INTERFACE_H_

class EditorInterface
{
public:
  EditorInterface(void *processPtr) : mProcess(processPtr) {}
  virtual ~EditorInterface(void) {}

  virtual bool Open(void *winPtr) = 0;
  virtual bool Close(void) = 0;
  //virtual long UpdateGraphicControl(long index, long value) = 0;
  //virtual void Idle() {};
  virtual void Draw(long left, long top, long right, long bottom) {};
  virtual void GetRect(short *left, short *top, short *right, short *bottom) = 0;
//  virtual void SetRect(short left, short top, short right, short bottom) = 0;
  virtual void SetRect(short left, short top, short right, short bottom) {};

  virtual void SetControlHighlight(long controlIndex, short isHighlighted, short color) = 0;
  virtual void GetControlIndexFromPoint(long x, long y, long *aControlIndex) = 0;
  
protected:
  void* mProcess;

};

#endif //_EDITOR_INTERFACE_H_
