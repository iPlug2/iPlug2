#ifndef __IPLUGGROUP__
#define __IPLUGGROUP__

#include "CEffectGroup.h"

class IPlugGroup : public CEffectGroup
{
public:
  IPlugGroup(void);
  virtual ~IPlugGroup(void);

protected:
  virtual void CreateEffectTypes(void);
  virtual void Initialize (void);
};

#endif  // __IPLUGGROUP__
