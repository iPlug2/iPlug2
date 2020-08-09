
#include "IPlugPlatform.h"

namespace iplug::Generic
{
	class GenericExampleClass
	{
	  protected:
		GenericExampleClass()  = default;
		~GenericExampleClass() = default;
	};
}  // namespace iplug::Generic

#include PLATFORM_HEADER(ExampleClass.h)
