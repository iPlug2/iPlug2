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
// AppKit/NSStackView.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "AppKitPrivate.hpp"
#include "NSView.hpp"
#include "NSUserInterfaceLayout.hpp"
#include <Foundation/NSArray.hpp>

namespace NS
{
	_NS_ENUM(NS::Integer, StackViewDistribution) {
		StackViewDistributionGravityAreas = -1,
		StackViewDistributionFill = 0,
		StackViewDistributionFillEqually,
		StackViewDistributionFillProportionally,
		StackViewDistributionEqualSpacing,
		StackViewDistributionEqualCentering,
	};

	class StackView : public NS::Referencing< StackView, View >
	{
		public:
			static StackView*                  stackView( NS::Array* views );
			void                               setOrientation( NS::UserInterfaceLayoutOrientation orientation );
			NS::UserInterfaceLayoutOrientation orientation() const;
			void                               setSpacing( double spacing );
			double                             spacing() const;
			void                               setDistribution( NS::StackViewDistribution distribution );
			NS::StackViewDistribution          distribution() const;
	};
}


_NS_INLINE NS::StackView* NS::StackView::stackView( NS::Array* views )
{
	return Object::sendMessage< StackView* >( _APPKIT_PRIVATE_CLS( NSStackView ), _APPKIT_PRIVATE_SEL( stackViewWithViews_ ), views );
}

_NS_INLINE void NS::StackView::setOrientation( NS::UserInterfaceLayoutOrientation orientation )
{
    NS::Object::sendMessage< void >( this, _APPKIT_PRIVATE_SEL( setOrientation_ ), orientation );
}

_NS_INLINE NS::UserInterfaceLayoutOrientation NS::StackView::orientation() const
{
    return NS::Object::sendMessage< NS::UserInterfaceLayoutOrientation >( this, _APPKIT_PRIVATE_SEL( orientation ) );
}

_NS_INLINE void NS::StackView::setSpacing( double spacing )
{
	NS::Object::sendMessage< void >( this, _APPKIT_PRIVATE_SEL( setSpacing_ ), spacing );
}

_NS_INLINE double NS::StackView::spacing() const
{
	return NS::Object::sendMessage< double >( this, _APPKIT_PRIVATE_SEL( spacing ) );
}

_NS_INLINE void NS::StackView::setDistribution( NS::StackViewDistribution orientation )
{
    NS::Object::sendMessage< void >( this, _APPKIT_PRIVATE_SEL( setDistribution_ ), orientation );
}

_NS_INLINE NS::StackViewDistribution NS::StackView::distribution() const
{
    return NS::Object::sendMessage< NS::StackViewDistribution >( this, _APPKIT_PRIVATE_SEL( distribution ) );
}
