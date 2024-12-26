#ifndef _REAPER_VIDEO_FRAME_H_
#define _REAPER_VIDEO_FRAME_H_

class IVideoFrame 
{
  public:

  double time_start WDL_FIXALIGN;
  double time_end;

    virtual void AddRef()=0;
    virtual void Release()=0;
    virtual char *get_bits()=0;
    virtual int get_w()=0;
    virtual int get_h()=0;
    virtual int get_fmt()=0; // 'YV12', 'YUY2', 'RGBA' are valid
    virtual int get_rowspan()=0;
    virtual void resize_img(int wantw, int wanth, int wantfmt)=0;

    virtual INT_PTR Extended(int code, INT_PTR parm1, INT_PTR parm2, void* parm3) { return 0; }
  protected:
    virtual ~IVideoFrame() { }
};


#endif

