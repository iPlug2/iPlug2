//------------------------------------------------------------------------
// Project     : ASIO SDK
//
// Category    : Helpers
// Filename    : host/pc/asiolist.h
// Created by  : Steinberg, 05/1996
// Description : Steinberg Audio Stream I/O Helpers
// a simple Windows ASIO host example
//	**** Windows specific ****
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

#ifndef __asiolist__
#define __asiolist__

#define DRVERR			-5000
#define DRVERR_INVALID_PARAM		DRVERR-1
#define DRVERR_DEVICE_ALREADY_OPEN	DRVERR-2
#define DRVERR_DEVICE_NOT_FOUND		DRVERR-3

#define MAXPATHLEN			512
#define MAXDRVNAMELEN		128

struct asiodrvstruct
{
	int						drvID;
	CLSID					clsid;
	char					dllpath[MAXPATHLEN];
	char					drvname[MAXDRVNAMELEN];
	LPVOID					asiodrv;
	struct asiodrvstruct	*next;
};

typedef struct asiodrvstruct ASIODRVSTRUCT;
typedef ASIODRVSTRUCT	*LPASIODRVSTRUCT;

class AsioDriverList {
public:
	AsioDriverList();
	~AsioDriverList();
	
	LONG asioOpenDriver (int,VOID **);
	LONG asioCloseDriver (int);

	// nice to have
	LONG asioGetNumDev (VOID);
	LONG asioGetDriverName (int,char *,int);		
	LONG asioGetDriverPath (int,char *,int);
	LONG asioGetDriverCLSID (int,CLSID *);

	// or use directly access
	LPASIODRVSTRUCT	lpdrvlist;
	int				numdrv;
};

typedef class AsioDriverList *LPASIODRIVERLIST;

#endif
