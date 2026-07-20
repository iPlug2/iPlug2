/*
 *
 * Copyright 2022 Mark Grimes. Most/all of the work is copied from Apple so copyright is theirs if they want it.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
// AppKit/NSGridView.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "AppKitPrivate.hpp"
#include "NSView.hpp"
#include <Foundation/NSArray.hpp>

namespace NS
{
	class GridView : public NS::Referencing< GridView, View >
	{
		public:
			static GridView* gridView( NS::Array* rows );
	};
}


_NS_INLINE NS::GridView* NS::GridView::gridView( NS::Array* rows )
{
	return Object::sendMessage< GridView* >( _APPKIT_PRIVATE_CLS( NSGridView ), _APPKIT_PRIVATE_SEL( gridViewWithViews_ ), rows );
}
