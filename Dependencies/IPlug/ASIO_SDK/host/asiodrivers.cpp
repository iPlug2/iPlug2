//------------------------------------------------------------------------
// Project     : ASIO SDK
//
// Category    : Helpers
// Filename    : host/asiodrivers.cpp
// Created by  : Steinberg, 05/1996
// Description : Steinberg Audio Stream I/O Helpers
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2025, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include <string.h>
#include "asiodrivers.h"

AsioDrivers* asioDrivers = 0;

bool loadAsioDriver(char *name);

bool loadAsioDriver(char *name)
{
	if(!asioDrivers)
		asioDrivers = new AsioDrivers();
	if(asioDrivers)
		return asioDrivers->loadDriver(name);
	return false;
}

//------------------------------------------------------------------------------------

#if MAC

bool resolveASIO(unsigned long aconnID);

AsioDrivers::AsioDrivers() : CodeFragments("ASIO Drivers", 'AsDr', 'Asio')
{
	connID = -1;
	curIndex = -1;
}

AsioDrivers::~AsioDrivers()
{
	removeCurrentDriver();
}

bool AsioDrivers::getCurrentDriverName(char *name)
{
	if(curIndex >= 0)
		return getName(curIndex, name);
	return false;
}

long AsioDrivers::getDriverNames(char **names, long maxDrivers)
{
	for(long i = 0; i < getNumFragments() && i < maxDrivers; i++)
		getName(i, names[i]);
	return getNumFragments() < maxDrivers ? getNumFragments() : maxDrivers;
}

bool AsioDrivers::loadDriver(char *name)
{
	char dname[64];
	unsigned long newID;

	for(long i = 0; i < getNumFragments(); i++)
	{
		if(getName(i, dname) && !strcmp(name, dname))
		{
			if(newInstance(i, &newID))
			{
				if(resolveASIO(newID))
				{
					if(connID != -1)
						removeInstance(curIndex, connID);
					curIndex = i;
					connID = newID;
					return true;
				}
			}
			break;
		}
	}
	return false;
}

void AsioDrivers::removeCurrentDriver()
{
	if(connID != -1)
		removeInstance(curIndex, connID);
	connID = -1;
	curIndex = -1;
}

//------------------------------------------------------------------------------------

#elif WINDOWS

#include "iasiodrv.h"

extern IASIO* theAsioDriver;

AsioDrivers::AsioDrivers() : AsioDriverList()
{
	curIndex = -1;
}

AsioDrivers::~AsioDrivers()
{
}

bool AsioDrivers::getCurrentDriverName(char *name)
{
	if(curIndex >= 0)
		return asioGetDriverName(curIndex, name, 32) == 0 ? true : false;
	name[0] = 0;
	return false;
}

long AsioDrivers::getDriverNames(char **names, long maxDrivers)
{
	for(long i = 0; i < asioGetNumDev() && i < maxDrivers; i++)
		asioGetDriverName(i, names[i], 32);
	return asioGetNumDev() < maxDrivers ? asioGetNumDev() : maxDrivers;
}

bool AsioDrivers::loadDriver(char *name)
{
	char dname[64];
	char curName[64];

	for(long i = 0; i < asioGetNumDev(); i++)
	{
		if(!asioGetDriverName(i, dname, 32) && !strcmp(name, dname))
		{
			curName[0] = 0;
			getCurrentDriverName(curName);	// in case we fail...
			removeCurrentDriver();

			if(!asioOpenDriver(i, (void **)&theAsioDriver))
			{
				curIndex = i;
				return true;
			}
			else
			{
				theAsioDriver = 0;
				if(curName[0] && strcmp(dname, curName))
					loadDriver(curName);	// try restore
			}
			break;
		}
	}
	return false;
}

void AsioDrivers::removeCurrentDriver()
{
	if(curIndex != -1)
		asioCloseDriver(curIndex);
	curIndex = -1;
}

#elif SGI || BEOS

#include "asiolist.h"

AsioDrivers::AsioDrivers() 
	: AsioDriverList()
{
	curIndex = -1;
}

AsioDrivers::~AsioDrivers()
{
}

bool AsioDrivers::getCurrentDriverName(char *name)
{
	return false;
}

long AsioDrivers::getDriverNames(char **names, long maxDrivers)
{
	return 0;
}

bool AsioDrivers::loadDriver(char *name)
{
	return false;
}

void AsioDrivers::removeCurrentDriver()
{
}

#else
#error implement me
#endif
