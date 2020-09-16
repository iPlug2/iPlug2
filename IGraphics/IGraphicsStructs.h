/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file Public structs and small classes used throughout IGraphics code
 * \addtogroup IGraphicsStructs
 * @{
 */

#include "IGraphicsPrivate.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class IGraphics;
class IControl;
class ILambdaControl;
class IPopupMenu;
struct IRECT;
struct IVec2;
struct IMouseInfo;
struct IColor;
struct IGestureInfo;

using IActionFunction             = std::function<void(IControl*)>;
using IAnimationFunction          = std::function<void(IControl*)>;
using ILambdaDrawFunction         = std::function<void(ILambdaControl*, IGraphics&, IRECT&)>;
using IKeyHandlerFunc             = std::function<bool(const IKeyPress& key, bool isUp)>;
using IMsgBoxCompletionHanderFunc = std::function<void(EMsgBoxResult result)>;
using IColorPickerHandlerFunc     = std::function<void(const IColor& result)>;
using IGestureFunc                = std::function<void(IControl*, const IGestureInfo&)>;
using IPopupFunction              = std::function<void(IPopupMenu* pMenu)>;
using IDisplayTickFunc            = std::function<void()>;
using ITouchID                    = uintptr_t;

/** A click action function that does nothing */
void EmptyClickActionFunc(IControl* pCaller);

/** A click action function that triggers the default animation function for DEFAULT_ANIMATION_DURATION */
void DefaultClickActionFunc(IControl* pCaller);

/** An animation function that just calls the caller control's OnEndAnimation() method at the end of the animation  */
void DefaultAnimationFunc(IControl* pCaller);

/** The splash click action function is used by IVControls to start SplashAnimationFunc */
void SplashClickActionFunc(IControl* pCaller);

/** The splash animation function is used by IVControls to animate the splash */
void SplashAnimationFunc(IControl* pCaller);

/** Use with a param-linked control to popup the bubble control horizontally */
void ShowBubbleHorizontalActionFunc(IControl* pCaller);

/** Use with a param-linked control to popup the bubble control vertically */
void ShowBubbleVerticalActionFunc(IControl* pCaller);

using MTLTexturePtr = void*;

using Milliseconds = std::chrono::duration<double, std::chrono::milliseconds::period>;
using TimePoint    = std::chrono::time_point<std::chrono::high_resolution_clock, Milliseconds>;

/** User-facing bitmap abstraction that you use to manage bitmap data, independant of draw class/platform.
 * IBitmap doesn't actually own the image data \see APIBitmap
 * An IBitmap's width and height are always in relation to a 1:1 (low dpi) screen. Any scaling happens at the drawing
 * stage. */
class IBitmap
{
 public:
	/** IBitmap Constructor
	 @param pAPIBitmap Pointer to a drawing API bitmap
	 @param n Number of frames (for multi frame film-strip bitmaps)
	 @param framesAreHorizontal framesAreHorizontal \c true if the frames are positioned horizontally
	 @param name Resource name for the bitmap */
	IBitmap(APIBitmap* pAPIBitmap, int n, bool framesAreHorizontal, const char* name = "")
		: mAPIBitmap(pAPIBitmap)
		, mW(pAPIBitmap->GetWidth() / pAPIBitmap->GetScale())
		, mH(pAPIBitmap->GetHeight() / pAPIBitmap->GetScale())
		, mN(n)
		, mFramesAreHorizontal(framesAreHorizontal)
		, mResourceName(name, static_cast<int>(strlen(name)))
	{
	}

	IBitmap() : mAPIBitmap(nullptr), mW(0), mH(0), mN(0), mFramesAreHorizontal(false) {}

	/** @return overall bitmap width in pixels */
	int W() const
	{
		return mW;
	}

	/** @return overall bitmap height in pixels */
	int H() const
	{
		return mH;
	}

	/** @return Width of a single frame in pixels */
	inline constexpr int FW() const
	{
		return (mFramesAreHorizontal ? mW / mN : mW);
	}

	/** @return Height of a single frame in pixels */
	inline constexpr int FH() const
	{
		return (mFramesAreHorizontal ? mH : mH / mN);
	}

	/** @return number of frames */
	int N() const
	{
		return mN;
	}

	/** @return the scale of the bitmap */
	int GetScale() const
	{
		return mAPIBitmap->GetScale();
	}

	/** @return the draw scale of the bitmap */
	float GetDrawScale() const
	{
		return mAPIBitmap->GetDrawScale();
	}

	/** @return a pointer to the referenced APIBitmap */
	APIBitmap* GetAPIBitmap() const
	{
		return mAPIBitmap;
	}

	/** @return whether or not frames are stored horizontally */
	bool GetFramesAreHorizontal() const
	{
		return mFramesAreHorizontal;
	}

	/** @return the resource name */
	const WDL_String& GetResourceName() const
	{
		return mResourceName;
	}

	/** @return \true if the bitmap has valid data */
	inline bool IsValid() const
	{
		return mAPIBitmap != nullptr;
	}

 private:
	/** Pointer to the API specific bitmap */
	APIBitmap* mAPIBitmap;
	/** Bitmap width (in pixels) */
	int mW;
	/** Bitmap height (in pixels) */
	int mH;
	/** Number of frames (for stacked bitmaps) */
	int mN;
	/** \c true if the frames are positioned horizontally */
	bool mFramesAreHorizontal;
	/** Resource path/name for the bitmap */
	WDL_String mResourceName;
};

/** User-facing SVG abstraction that you use to manage SVG data
 * ISVG doesn't actually own the image data */

#ifdef IGRAPHICS_SKIA
struct ISVG
{
	ISVG(sk_sp<SkSVGDOM> svgDom) : mSVGDom(svgDom) {}

	/** /todo */
	float W() const
	{
		if (mSVGDom)
			return mSVGDom->containerSize().width();
		else
			return 0;
	}

	/** /todo */
	float H() const
	{
		if (mSVGDom)
			return mSVGDom->containerSize().height();
		else
			return 0;
	}

	/** @return \true if the SVG has valid data */
	inline bool IsValid() const
	{
		return mSVGDom != nullptr;
	}

	sk_sp<SkSVGDOM> mSVGDom;
};
#else
struct ISVG
{
	ISVG(NSVGimage* pImage)
	{
		mImage = pImage;
	}

	/** /todo */
	float W() const
	{
		if (mImage)
			return mImage->width;
		else
			return -1;  // returning 0 generates divide by 0 warning when compiling
	}

	/** /todo */
	float H() const
	{
		if (mImage)
			return mImage->height;
		else
			return -1;  // returning 0 generates divide by 0 warning when compiling
	}

	/** @return \true if the SVG has valid data */
	inline bool IsValid() const
	{
		return mImage != nullptr;
	}

	NSVGimage* mImage = nullptr;
};
#endif

/** Used to manage color data, independent of draw class/platform. */
struct IColor
{
	int A, R, G, B;

	IColor(int a = 255, int r = 0, int g = 0, int b = 0) : A(a), R(r), G(g), B(b) {}

	bool operator==(const IColor& rhs)
	{
		return (rhs.A == A && rhs.R == R && rhs.G == G && rhs.B == B);
	}

	bool operator!=(const IColor& rhs)
	{
		return !operator==(rhs);
	}

	void Set(int a = 255, int r = 0, int g = 0, int b = 0)
	{
		A = a;
		R = r;
		G = g;
		B = b;
	}

	/** /todo */
	bool Empty() const
	{
		return A == 0 && R == 0 && G == 0 && B == 0;
	}

	/** /todo */
	void Clamp()
	{
		A = math::Clamp(A, 0, 255);
		R = math::Clamp(R, 0, 255);
		G = math::Clamp(G, 0, 255);
		B = math::Clamp(B, 0, 255);
	}

	/** /todo
	 * @param alpha */
	void Randomise(int alpha = 255)
	{
		A = alpha;
		R = std::rand() % 255;
		G = std::rand() % 255;
		B = std::rand() % 255;
	}

	/** Set the color's opacity/alpha component with a float
	 * @param alpha float in the range 0. to 1. */
	void SetOpacity(float alpha)
	{
		A = static_cast<int>(math::Clamp(alpha, 0.f, 1.f) * 255.f);
	}

	/** Returns a new IColor with a different opacity
	 * @param alpha float in the range 0. to 1.
	 * @return IColor new Color */
	IColor WithOpacity(float alpha) const
	{
		IColor n = *this;
		n.SetOpacity(alpha);
		return n;
	}

	/** Contrast the color
	 * @param c Contrast value in the range -1.f to 1.f */
	void Contrast(float c)
	{
		const int mod = static_cast<int>(c * 255.f);
		R             = math::Clamp(R += mod, 0, 255);
		G             = math::Clamp(G += mod, 0, 255);
		B             = math::Clamp(B += mod, 0, 255);
	}

	/** Returns a new contrasted IColor based on this one
	 * @param c Contrast value in the range -1. to 1.
	 * @return IColor new Color */
	IColor WithContrast(float c) const
	{
		IColor n = *this;
		n.Contrast(c);
		return n;
	}

	/** Get the color as a 3 float array
	 * @param rgbf ptr to array of 3 floats */
	void GetRGBf(float* rgbf) const
	{
		rgbf[0] = R / 255.f;
		rgbf[1] = G / 255.f;
		rgbf[2] = B / 255.f;
	}

	/** Get the color as a 4 float array
	 * @param rgbaf ptr to array of 4 floats */
	void GetRGBAf(float* rgbaf) const
	{
		rgbaf[0] = R / 255.f;
		rgbaf[1] = G / 255.f;
		rgbaf[2] = B / 255.f;
		rgbaf[3] = A / 255.f;
	}

	/** Get the Hue, Saturation and Luminance of the color
	 * @param h hue value to set, output in the range 0. to 1.
	 * @param s saturation value to set, output in the range 0. to 1.
	 * @param l luminance value to set, output in the range 0. to 1.
	 * @param a alpha value to set, output in the range 0. to 1. */
	void GetHSLA(float& h, float& s, float& l, float& a) const
	{
		const float fR = R / 255.f;
		const float fG = G / 255.f;
		const float fB = B / 255.f;
		a              = A / 255.f;

		const float fMin  = std::min(fR, std::min(fG, fB));
		const float fMax  = std::max(fR, std::max(fG, fB));
		const float fDiff = fMax - fMin;
		const float fSum  = fMax + fMin;

		l = 50.f * fSum;

		if (fMin == fMax)
		{
			s = 0.f;
			h = 0.f;
			l /= 100.f;
			return;
		}
		else if (l < 50.f)
		{
			s = 100.f * fDiff / fSum;
		}
		else
		{
			s = 100.f * fDiff / (2.f - fDiff);
		}

		if (fMax == fR)
		{
			h = 60.f * (fG - fB) / fDiff;
		}
		if (fMax == fG)
		{
			h = 60.f * (fB - fR) / fDiff + 120.f;
		}
		if (fMax == fB)
		{
			h = 60.f * (fR - fG) / fDiff + 240.f;
		}

		if (h < 0.f)
		{
			h = h + 360.f;
		}

		h /= 360.f;
		s /= 100.f;
		l /= 100.f;
	}

	/** /todo
	 * @return int /todo */
	int GetLuminosity() const
	{
		int min = R < G ? (R < B ? R : B) : (G < B ? G : B);
		int max = R > G ? (R > B ? R : B) : (G > B ? G : B);
		return (min + max) / 2;
	};

	/** /todo
	 * @param randomAlpha /todo
	 * @return IColor /todo */
	static IColor GetRandomColor(bool randomAlpha = false)
	{
		int A = randomAlpha ? std::rand() & 0xFF : 255;
		int R = std::rand() & 0xFF;
		int G = std::rand() & 0xFF;
		int B = std::rand() & 0xFF;

		return IColor(A, R, G, B);
	}

	/** Create an IColor from a 3 float RGB array
	 * @param rgbf ptr to array of 3 floats
	 * @return IColor A new IColor based on the input array */
	static IColor FromRGBf(float* rgbf)
	{
		int A = 255;
		int R = static_cast<int>(rgbf[0] * 255.f);
		int G = static_cast<int>(rgbf[1] * 255.f);
		int B = static_cast<int>(rgbf[2] * 255.f);

		return IColor(A, R, G, B);
	}

	/** Create an IColor from a 4 float RGBA array
	 * @param rgbaf ptr to array of 3 floats
	 * @return IColor A new IColor based on the input array */
	static IColor FromRGBAf(float* rgbaf)
	{
		int R = static_cast<int>(rgbaf[0] * 255.f);
		int G = static_cast<int>(rgbaf[1] * 255.f);
		int B = static_cast<int>(rgbaf[2] * 255.f);
		int A = static_cast<int>(rgbaf[3] * 255.f);

		return IColor(A, R, G, B);
	}

	/** Create an IColor from a color code. Can be used to convert a hex code into an IColor object.
	 * @code
	 *   IColor color = IColor::FromColorCode(0x55a6ff);
	 *   IColor colorWithAlpha = IColor::FromColorCode(0x55a6ff, 0x88); // alpha is 0x88
	 * @endcode
	 *
	 * @param colorCode Integer representation of the color. Use with hexadecimal numbers, e.g. 0xff38a2
	 * @param A Integer representation of the alpha channel
	 * @return IColor A new IColor based on the color code provided */
	static IColor FromColorCode(int colorCode, int A = 0xFF)
	{
		int R = (colorCode >> 16) & 0xFF;
		int G = (colorCode >> 8) & 0xFF;
		int B = colorCode & 0xFF;

		return IColor(A, R, G, B);
	}

	/** Create an IColor from a color code in a CString. Can be used to convert a hex code into an IColor object.
	 * @param hexStr CString representation of the color code (no alpha). Use with hex numbers, e.g. "#ff38a2". WARNING:
	 * This does very little error checking
	 * @return IColor A new IColor based on the color code provided */
	static IColor FromColorCodeStr(const char* hexStr)
	{
		WDL_String str(hexStr);

		if (str.GetLength() == 7 && str.Get()[0] == '#')
		{
			str.DeleteSub(0, 1);

			return FromColorCode(static_cast<int>(std::stoul(str.Get(), nullptr, 16)));
		}
		else
		{
			assert(0 && "Invalid color code str, returning black");
			return IColor();
		}
	}

	int ToColorCode() const
	{
		return (R << 16) | (G << 8) | B;
	}

	void ToColorCodeStr(WDL_String& str) const
	{
		str.SetFormatted(32, "#%02x%02x%02x%02x", R, G, B, A);
	}

	/** Create an IColor from Hue Saturation and Luminance values
	 * @param h hue value in the range 0.f-1.f
	 * @param s saturation value in the range 0.f-1.f
	 * @param l luminance value in the range 0.f-1.f
	 * @param a alpha value in the range 0.f-1.f
	 * @return The new IColor */
	static IColor FromHSLA(float h, float s, float l, float a = 1.f)
	{
		auto hue = [](float h, float m1, float m2) {
			if (h < 0)
				h += 1;
			if (h > 1)
				h -= 1;
			if (h < 1.0f / 6.0f)
				return m1 + (m2 - m1) * h * 6.0f;
			else if (h < 3.0f / 6.0f)
				return m2;
			else if (h < 4.0f / 6.0f)
				return m1 + (m2 - m1) * (2.0f / 3.0f - h) * 6.0f;
			return m1;
		};

		IColor col;
		h = std::fmodf(h, 1.0f);
		if (h < 0.0f)
			h += 1.0f;

		s = math::Clamp(s, 0.0f, 1.0f);
		l = math::Clamp(l, 0.0f, 1.0f);

		float m2 = l <= 0.5f ? (l * (1 + s)) : (l + s - l * s);
		float m1 = 2 * l - m2;
		col.R    = static_cast<int>(math::Clamp(hue(h + 1.0f / 3.0f, m1, m2), 0.0f, 1.0f) * 255.f);
		col.G    = static_cast<int>(math::Clamp(hue(h, m1, m2), 0.0f, 1.0f) * 255.f);
		col.B    = static_cast<int>(math::Clamp(hue(h - 1.0f / 3.0f, m1, m2), 0.0f, 1.0f) * 255.f);
		col.A    = static_cast<int>(a * 255.f);
		return col;
	}

	/** /todo
	 * @param start /todo
	 * @param dest /todo
	 * @param progress /todo */
	static IColor LinearInterpolateBetween(const IColor& start, const IColor& dest, float progress)
	{
		IColor result;
		result.A = start.A + static_cast<int>(progress * static_cast<float>(dest.A - start.A));
		result.R = start.R + static_cast<int>(progress * static_cast<float>(dest.R - start.R));
		result.G = start.G + static_cast<int>(progress * static_cast<float>(dest.G - start.G));
		result.B = start.B + static_cast<int>(progress * static_cast<float>(dest.B - start.B));
		return result;
	}
};

const IColor COLOR_TRANSPARENT(0, 0, 0, 0);
const IColor COLOR_TRANSLUCENT(10, 0, 0, 0);
const IColor COLOR_BLACK(255, 0, 0, 0);
const IColor COLOR_BLACK_DROP_SHADOW(128, 0, 0, 0);
const IColor COLOR_GRAY(255, 127, 127, 127);
const IColor COLOR_LIGHT_GRAY(255, 240, 240, 240);
const IColor COLOR_MID_GRAY(255, 200, 200, 200);
const IColor COLOR_DARK_GRAY(255, 70, 70, 70);
const IColor COLOR_WHITE(255, 255, 255, 255);
const IColor COLOR_RED(255, 255, 0, 0);
const IColor COLOR_GREEN(255, 0, 255, 0);
const IColor COLOR_BLUE(255, 0, 0, 255);
const IColor COLOR_YELLOW(255, 255, 255, 0);
const IColor COLOR_ORANGE(255, 255, 127, 0);
const IColor COLOR_INDIGO(255, 75, 0, 130);
const IColor COLOR_VIOLET(255, 148, 0, 211);

static IColor GetRainbow(int colorIdx)
{
	switch (colorIdx)
	{
		case 0:
			return COLOR_RED;
		case 1:
			return COLOR_ORANGE;
		case 2:
			return COLOR_YELLOW;
		case 3:
			return COLOR_GREEN;
		case 4:
			return COLOR_BLUE;
		case 5:
			return COLOR_INDIGO;
		case 6:
			return COLOR_VIOLET;
		default:
			assert(0);
			return COLOR_WHITE;
	}
}

const IColor DEFAULT_GRAPHICS_BGCOLOR = COLOR_GRAY;
const IColor DEFAULT_BGCOLOR          = COLOR_TRANSPARENT;
const IColor DEFAULT_FGCOLOR          = COLOR_MID_GRAY;
const IColor DEFAULT_PRCOLOR          = COLOR_LIGHT_GRAY;

const IColor DEFAULT_FRCOLOR = COLOR_DARK_GRAY;
const IColor DEFAULT_HLCOLOR = COLOR_TRANSLUCENT;
const IColor DEFAULT_SHCOLOR = IColor(60, 0, 0, 0);
const IColor DEFAULT_X1COLOR = COLOR_BLACK;
const IColor DEFAULT_X2COLOR = COLOR_GREEN;
const IColor DEFAULT_X3COLOR = COLOR_BLUE;

const IColor DEFAULT_TEXT_FGCOLOR      = COLOR_BLACK;
const IColor DEFAULT_TEXTENTRY_BGCOLOR = COLOR_WHITE;
const IColor DEFAULT_TEXTENTRY_FGCOLOR = COLOR_BLACK;

/** Used to manage composite/blend operations, independent of draw class/platform */
struct IBlend
{
	EBlend mMethod;
	float mWeight;

	/** Creates a new IBlend
	 * @param type Blend type (defaults to none)
	 * @param weight normalised alpha blending amount */
	IBlend(EBlend type = EBlend::Default, float weight = 1.0f) : mMethod(type), mWeight(math::Clamp(weight, 0.f, 1.f))
	{
	}
};

/** /todo
 * @param pBlend /todo
 * @return float /todo */
inline float BlendWeight(const IBlend* pBlend)
{
	return (pBlend ? pBlend->mWeight : 1.0f);
}

const IBlend BLEND_75       = IBlend(EBlend::Default, 0.75f);
const IBlend BLEND_50       = IBlend(EBlend::Default, 0.5f);
const IBlend BLEND_25       = IBlend(EBlend::Default, 0.25f);
const IBlend BLEND_10       = IBlend(EBlend::Default, 0.1f);
const IBlend BLEND_05       = IBlend(EBlend::Default, 0.05f);
const IBlend BLEND_01       = IBlend(EBlend::Default, 0.01f);
const IBlend BLEND_DST_IN   = IBlend(EBlend::DstIn, 1.f);
const IBlend BLEND_DST_OVER = IBlend(EBlend::DstOver, 1.f);

/** Used to manage fill behaviour for path based drawing back ends */
struct IFillOptions
{
	EFillRule mFillRule {EFillRule::Winding};
	bool mPreserve {false};

	IFillOptions(bool preserve = false, EFillRule fillRule = EFillRule::Winding)
		: mFillRule(fillRule)
		, mPreserve(preserve)
	{
	}
};

/** Used to manage stroke behaviour for path based drawing back ends */
struct IStrokeOptions
{
	/** Used to manage dashes for stroke */
	class DashOptions
	{
	 public:
		DashOptions() : mOffset(0), mCount(0) {}

		DashOptions(float* array, float offset, int count)
		{
			SetDash(array, offset, count);
		}

		/** @return int /todo */
		int GetCount() const
		{
			return mCount;
		}

		/** @return float  /todo */
		float GetOffset() const
		{
			return mOffset;
		}

		/** @return float* /todo */
		const float* GetArray() const
		{
			return mArray;
		}

		/** /todo
		 * @param array /todo
		 * @param offset /todo
		 * @param count /todo */
		void SetDash(float* array, float offset, int count)
		{
			assert(count >= 0 && count <= 8);

			mCount  = count;
			mOffset = offset;

			for (int i = 0; i < count; i++)
				mArray[i] = array[i];
		}

	 private:
		float mArray[8];
		float mOffset;
		int mCount;
	};

	float mMiterLimit     = 10.f;
	bool mPreserve        = false;
	ELineCap mCapOption   = ELineCap::Butt;
	ELineJoin mJoinOption = ELineJoin::Miter;
	DashOptions mDash;
};

static const char* TextStyleString(ETextStyle style)
{
	switch (style)
	{
		case ETextStyle::Bold:
			return "Bold";
		case ETextStyle::Italic:
			return "Italic";
		case ETextStyle::Normal:
		default:
			return "Regular";
	}
}

/** Used to manage font and text/text entry style for a piece of text on the UI, independent of draw class/platform.*/
struct IText
{
	/** /todo
	 * @param size /todo
	 * @param color /todo
	 * @param font /todo
	 * @param align /todo
	 * @param valign /todo
	 * @param angle /todo
	 * @param TEBGColor /todo
	 * @param TEFGColor /todo */
	IText(float size              = DEFAULT_TEXT_SIZE,
		  const IColor& color     = DEFAULT_TEXT_FGCOLOR,
		  const char* font        = nullptr,
		  EAlign align            = EAlign::Center,
		  EVAlign valign          = EVAlign::Middle,
		  float angle             = 0,
		  const IColor& TEBGColor = DEFAULT_TEXTENTRY_BGCOLOR,
		  const IColor& TEFGColor = DEFAULT_TEXTENTRY_FGCOLOR)
		: mSize(size)
		, mFGColor(color)
		, mTextEntryBGColor(TEBGColor)
		, mTextEntryFGColor(TEFGColor)
		, mAngle(angle)
		, mAlign(align)
		, mVAlign(valign)
	{
		strcpy(mFont, (font ? font : DEFAULT_FONT));
	}

	/** /todo
	 * @param size /todo
	 * @param valign /todo */
	IText(float size, EVAlign valign, const IColor& color = DEFAULT_TEXT_FGCOLOR) : IText()
	{
		mSize    = size;
		mVAlign  = valign;
		mFGColor = color;
	}

	/** /todo
	 * @param size /todo
	 * @param align /todo */
	IText(float size, EAlign align, const IColor& color = DEFAULT_TEXT_FGCOLOR) : IText()
	{
		mSize    = size;
		mAlign   = align;
		mFGColor = color;
	}

	IText(float size, const char* font) : IText()
	{
		mSize = size;
		strcpy(mFont, (font ? font : DEFAULT_FONT));
	}

	IText WithFGColor(const IColor& fgColor) const
	{
		IText newText    = *this;
		newText.mFGColor = fgColor;
		return newText;
	}
	IText WithTEColors(const IColor& teBgColor, const IColor& teFgColor) const
	{
		IText newText             = *this;
		newText.mTextEntryBGColor = teBgColor;
		newText.mTextEntryFGColor = teFgColor;
		return newText;
	}
	IText WithAlign(EAlign align) const
	{
		IText newText  = *this;
		newText.mAlign = align;
		return newText;
	}
	IText WithVAlign(EVAlign valign) const
	{
		IText newText   = *this;
		newText.mVAlign = valign;
		return newText;
	}
	IText WithSize(float size) const
	{
		IText newText = *this;
		newText.mSize = size;
		return newText;
	}
	IText WithAngle(float v) const
	{
		IText newText  = *this;
		newText.mAngle = v;
		return newText;
	}
	IText WithFont(const char* font) const
	{
		IText newText = *this;
		strcpy(newText.mFont, (font ? font : DEFAULT_FONT));
		;
		return newText;
	}

	char mFont[FONT_LEN];
	float mSize;
	IColor mFGColor;
	IColor mTextEntryBGColor;
	IColor mTextEntryFGColor;
	float mAngle    = 0.f;  // Degrees ccwise from normal.
	EAlign mAlign   = EAlign::Near;
	EVAlign mVAlign = EVAlign::Middle;
};

const IText DEFAULT_TEXT = IText();

/** Used to manage a rectangular area, independent of draw class/platform.
 * An IRECT is always specified in 1:1 pixels, any scaling for high DPI happens in the drawing class.
 * In IGraphics 0,0 is top left. */
struct IRECT
{
	/** Left side of the rectangle (X) */
	float L = 0.f;
	/** Top of the rectangle (Y) */
	float T = 0.f;
	/** Right side of the rectangle (X + W) */
	float R = 0.f;
	/** Bottom of the rectangle (Y + H) */
	float B = 0.f;

	/** Construct an empty IRECT  */
	inline constexpr IRECT() = default;

	/** Construct a new IRECT with dimensions
	 * @param l Left
	 * @param t Top
	 * @param r Right
	 * @param b Bottom */
	inline constexpr IRECT(float l, float t, float r, float b)
	{
		// floating point precision can cause r or b to be greater than l or t
		L = l;
		T = t;
		R = std::max(r, l);
		B = std::max(b, t);
		DEBUG_ASSERT(L <= R);
		DEBUG_ASSERT(T <= B);
	}

	/** Construct a new IRECT at the given position and with the same size as the bitmap
	 * @param x Top
	 * @param y Left
	 * @param bitmap Bitmap for the size */
	inline constexpr IRECT(float x, float y, const IBitmap& bitmap)
	{
		L = x;
		T = y;
		R = L + (float) bitmap.FW();
		B = T + (float) bitmap.FH();
		DEBUG_ASSERT(L <= R);
		DEBUG_ASSERT(T <= B);
	}

	/** Create a new IRECT with the given position and size
	 * @param l Left/X of new IRECT
	 * @param t Top/Y of new IRECT
	 * @param w Width of new IRECT
	 * @param h Height of new IRECT
	 * @return the new IRECT */
	static inline constexpr IRECT MakeXYWH(float l, float t, float w, float h)
	{
		DEBUG_ASSERT(w >= 0);
		DEBUG_ASSERT(h >= 0);
		return IRECT(l, t, l + w, t + h);
	}

	/** @return bool true if all the fields of this IRECT are 0 */
	inline constexpr bool Empty() const
	{
		return math::IsNearlyZeroDelta(L, T, R, B);
	}

	/** Set all fields of this IRECT to 0 */
	inline constexpr void Clear()
	{
		L = T = R = B = 0.f;
	}

	inline constexpr bool operator==(const IRECT& rhs) const
	{
		return (L == rhs.L && T == rhs.T && R == rhs.R && B == rhs.B);
	}

	inline constexpr bool operator!=(const IRECT& rhs) const
	{
		return !(*this == rhs);
	}

	/** @return float the width of this IRECT  */
	inline constexpr float W() const
	{
		return R - L;
	}

	/** @return float the height of this IRECT  */
	inline constexpr float H() const
	{
		return B - T;
	}

	/** @return float the midpoint of this IRECT on the x-axis (middle-width) */
	inline constexpr float MW() const
	{
		return 0.5f * (L + R);
	}

	/** @return float the midpoint of this IRECT on the y-axis (middle-height) */
	inline constexpr float MH() const
	{
		return 0.5f * (T + B);
	}

	/** @return float the area of this IRECT  */
	inline constexpr float Area() const
	{
		return W() * H();
	}

	/** Create a new IRECT that is a union of this IRECT and `rhs`.
	 * The resulting IRECT will have the minimim L and T values and maximum R and B values of the inputs.
	 * @param rhs another IRECT
	 * @return IRECT the new IRECT */
	inline constexpr IRECT Union(const IRECT& rhs) const
	{
		if (Empty())
			return rhs;

		if (rhs.Empty())
			return *this;

		return IRECT(std::min(L, rhs.L), std::min(T, rhs.T), std::max(R, rhs.R), std::max(B, rhs.B));
	}

	/** Create a new IRECT that is the intersection of this IRECT and `rhs`.
	 * The resulting IRECT will have the maximum L and T values and minimum R and B values of the inputs.
	 * @param rhs another IRECT
	 * @return IRECT the new IRECT  */
	inline constexpr IRECT Intersect(const IRECT& rhs) const
	{
		if (Intersects(rhs))
			return IRECT(std::max(L, rhs.L), std::max(T, rhs.T), std::min(R, rhs.R), std::min(B, rhs.B));

		return IRECT();
	}

	/** Returns true if this IRECT shares any common pixels with `rhs`, false otherwise.
	 * @param rhs another IRECT
	 * @return true this IRECT shares any common space with `rhs`
	 * @return false this IRECT and `rhs` are completely separate  */
	inline constexpr bool Intersects(const IRECT& rhs) const
	{
		return (!Empty() && !rhs.Empty() && R >= rhs.L && L < rhs.R && B >= rhs.T && T < rhs.B);
	}

	/** Returns true if this IRECT completely contains `rhs`.
	 * @param rhs another IRECT
	 * @return true if this IRECT completely contains `rhs`
	 * @return false if any part of `rhs` is outside this IRECT */
	inline constexpr bool Contains(const IRECT& rhs) const
	{
		return (!Empty() && !rhs.Empty() && rhs.L >= L && rhs.R <= R && rhs.T >= T && rhs.B <= B);
	}

	/** Returns true if this IRECT completely contains the point (x,y).
	 * @param x point X
	 * @param y point Y
	 * @return true the point (x,y) is inside this IRECT
	 * @return false the point (x,y) is outside this IRECT */
	inline constexpr bool Contains(float x, float y) const
	{
		return (!Empty() && x >= L && x < R && y >= T && y < B);
	}

	/** Returns true if the point (x,y) is either contained in this IRECT or on an edge.
	 * Unlike Contains(x,y) this method includes right-most and bottom-most pixels.
	 * @param x point X
	 * @param y point Y
	 * @return true the point (x,y) is inside this IRECT
	 * @return false the point (x,y) is outside this IRECT */
	inline constexpr bool ContainsEdge(float x, float y) const
	{
		return (!Empty() && x >= L && x <= R && y >= T && y <= B);
	}

	/** Ensure the point (x,y) is inside this IRECT.
	 * @param x point X, will be modified if it's outside this IRECT
	 * @param y point Y, will be modified if it's outside this IRECT */
	inline constexpr void Constrain(float& x, float& y) const
	{
		x = math::Clamp(x, L, R);
		y = math::Clamp(y, T, B);
	}

	/** Offsets the input IRECT based on the parent
	 * @param rhs IRECT to offset */
	inline constexpr IRECT Inset(const IRECT& rhs) const
	{
		return IRECT(L + rhs.L, T + rhs.T, L + rhs.R, T + rhs.B);
	}

	/** Return if this IRECT and `rhs` may be merged.
	 * The two rects cover exactly the area returned by Union()
	 * @param rhs another IRECT
	 * @return true this IRECT wholly contains `rhs` or `rhs` wholly contains this IRECT
	 * @return false any part of these IRECTs does not overlap */
	inline constexpr bool Mergeable(const IRECT& rhs) const
	{
		if (Empty() || rhs.Empty())
			return true;
		if (L == rhs.L && R == rhs.R && ((T >= rhs.T && T <= rhs.B) || (rhs.T >= T && rhs.T <= B)))
			return true;
		return T == rhs.T && B == rhs.B && ((L >= rhs.L && L <= rhs.R) || (rhs.L >= L && rhs.L <= R));
	}

	/** Get a new rectangle which is a fraction of this rectangle
	 * @param layoutDir EDirection::Vertical or EDirection::Horizontal
	 * @param frac Fractional multiplier
	 * @param fromTopOrRight If true the new rectangle will expand from the top (Vertical) or right (Horizontal)
		   otherwise it will expand from the bottom (Vertical) or left (Horizontal)
	 * @return IRECT the new rectangle */
	inline constexpr IRECT FracRect(EDirection layoutDir, float frac, bool fromTopOrRight = false) const
	{
		if (layoutDir == EDirection::Vertical)
			return FracRectVertical(frac, fromTopOrRight);
		else
			return FracRectHorizontal(frac, fromTopOrRight);
	}

	/** Returns a new IRECT with a width that is multiplied by `frac`.
	 * @param frac width multiplier
	 * @param rhs if true, the new IRECT will expand/contract from the right, otherwise it will come from the left
	 * @return IRECT the new IRECT */
	inline constexpr IRECT FracRectHorizontal(float frac, bool rhs = false) const
	{
		float widthOfSubRect = W() * frac;

		if (rhs)
			return IRECT(R - widthOfSubRect, T, R, B);
		else
			return IRECT(L, T, L + widthOfSubRect, B);
	}

	/** Returns a new IRECT with a height that is multiplied by `frac`.
	 * @param frac height multiplier
	 * @param fromTop if true, the new IRECT will expand/contract from the top, otherwise it will come from the bottom
	 * @return IRECT the new IRECT */
	inline constexpr IRECT FracRectVertical(float frac, bool fromTop = false) const
	{
		float heightOfSubRect = H() * frac;

		if (fromTop)
			return IRECT(L, T, R, T + heightOfSubRect);
		else
			return IRECT(L, B - heightOfSubRect, R, B);
	}

	/** Returns a new IRECT which is a horizontal "slice" of this IRECT.
	 * First divide the current height into `numSlices` equal parts, then return the n'th "slice"
	 * where "n" is `sliceIdx`. The returned IRECT will have the same width as this IRECT.
	 *
	 * @param numSlices number of equal-sized parts to divide this IRECT into
	 * @param sliceIdx which "slice" to select
	 * @return IRECT the new IRECT */
	inline constexpr IRECT SubRectVertical(int numSlices, int sliceIdx) const
	{
		float heightOfSubRect = H() / (float) numSlices;
		float t               = heightOfSubRect * (float) sliceIdx;

		return IRECT(L, T + t, R, T + t + heightOfSubRect);
	}

	/** Returns a new IRECT which is a vertical "slice" of this IRECT.
	 * First divide the current width into `numSlices` equal parts, then return the n'th "slice"
	 * where "n" is `sliceIdx`. The returned IRECT will have the same height as this IRECT.
	 *
	 * @param numSlices number of equal-sized parts to divide this IRECT into
	 * @param sliceIdx which "slice" to select
	 * @return IRECT the new IRECT */
	inline constexpr IRECT SubRectHorizontal(int numSlices, int sliceIdx) const
	{
		float widthOfSubRect = W() / (float) numSlices;
		float l              = widthOfSubRect * (float) sliceIdx;

		return IRECT(L + l, T, L + l + widthOfSubRect, B);
	}

	/** Get a new rectangle which is a "slice" of this rectangle.
	 * @param layoutDir EDirection::Vertical or EDirection::Horizontal
	 * @param numSlices Number of equal-sized parts to divide this IRECT into
	 * @param sliceIdx Which "slice" to return
	 * @return IRECT the new rectangle */
	inline constexpr IRECT SubRect(EDirection layoutDir, int numSlices, int sliceIdx) const
	{
		if (layoutDir == EDirection::Vertical)
			return SubRectVertical(numSlices, sliceIdx);
		else
			return SubRectHorizontal(numSlices, sliceIdx);
	}

	/** Get a subrect of this IRECT expanding from the top-left corner
	 * @param w Width of the desired IRECT
	 * @param h Weight of the desired IRECT
	 * @return IRECT The resulting subrect */
	inline constexpr IRECT GetFromTLHC(float w, float h) const
	{
		return IRECT(L, T, L + w, T + h);
	}

	/** Get a subrect of this IRECT expanding from the bottom-left corner
	 * @param w Width of the desired IRECT
	 * @param h Height of the desired IRECT
	 * @return IRECT The resulting subrect */
	inline constexpr IRECT GetFromBLHC(float w, float h) const
	{
		return IRECT(L, B - h, L + w, B);
	}

	/** Get a subrect of this IRECT expanding from the top-right corner
	 * @param w Width of the desired IRECT
	 * @param h Height of the desired IRECT
	 * @return IRECT The resulting subrect */
	inline constexpr IRECT GetFromTRHC(float w, float h) const
	{
		return IRECT(R - w, T, R, T + h);
	}

	/** Get a subrect of this IRECT expanding from the bottom-right corner
	 * @param w Width of the desired IRECT
	 * @param h Height of the desired IRECT
	 * @return IRECT The resulting subrect */
	inline constexpr IRECT GetFromBRHC(float w, float h) const
	{
		return IRECT(R - w, B - h, R, B);
	}

	/** Get a subrect of this IRECT bounded in Y by the top edge and 'amount'
	 * @param amount Size in Y of the desired IRECT
	 * @return IRECT The resulting subrect */
	inline constexpr IRECT GetFromTop(float amount) const
	{
		return IRECT(L, T, R, T + amount);
	}

	/** Get a subrect of this IRECT bounded in Y by 'amount' and the bottom edge
	 * @param amount Size in Y of the desired IRECT
	 * @return IRECT The resulting subrect */
	inline constexpr IRECT GetFromBottom(float amount) const
	{
		return IRECT(L, B - amount, R, B);
	}

	/** Get a subrect of this IRECT bounded in X by the left edge and 'amount'
	 * @param amount Size in X of the desired IRECT
	 * @return IRECT The resulting subrect */
	inline constexpr IRECT GetFromLeft(float amount) const
	{
		return IRECT(L, T, L + amount, B);
	}

	/** Get a subrect of this IRECT bounded in X by 'amount' and the right edge
	 * @param amount Size in X of the desired IRECT
	 * @return IRECT The resulting subrect */
	inline constexpr IRECT GetFromRight(float amount) const
	{
		return IRECT(R - amount, T, R, B);
	}

	/** Get a subrect of this IRECT reduced in height from the top edge by 'amount'
	 * @param amount Size in Y to reduce by
	 * @return IRECT The resulting subrect */
	inline constexpr IRECT GetReducedFromTop(float amount) const
	{
		return IRECT(L, T + amount, R, B);
	}

	/** Get a subrect of this IRECT reduced in height from the bottom edge by 'amount'
	 * @param amount Size in Y to reduce by
	 * @return IRECT The resulting subrect */
	inline constexpr IRECT GetReducedFromBottom(float amount) const
	{
		return IRECT(L, T, R, B - amount);
	}

	/** Get a subrect of this IRECT reduced in width from the left edge by 'amount'
	 * @param amount Size in X to reduce by
	 * @return IRECT The resulting subrect */
	inline constexpr IRECT GetReducedFromLeft(float amount) const
	{
		return IRECT(L + amount, T, R, B);
	}

	/** Get a subrect of this IRECT reduced in width from the right edge by 'amount'
	 * @param amount Size in X to reduce by
	 * @return IRECT The resulting subrect */
	inline constexpr IRECT GetReducedFromRight(float amount) const
	{
		return IRECT(L, T, R - amount, B);
	}

	/** Reduce in height from the top edge by 'amount' and return the removed region
	 * @param amount Size in Y to reduce by
	 * @return IRECT The removed subrect */
	inline constexpr IRECT ReduceFromTop(float amount)
	{
		IRECT r = GetFromTop(amount);
		T += amount;
		return r;
	}

	/** Reduce in height from the bottom edge by 'amount' and return the removed region
	 * @param amount Size in Y to reduce by
	 * @return IRECT The removed subrect */
	inline constexpr IRECT ReduceFromBottom(float amount)
	{
		IRECT r = GetFromBottom(amount);
		B -= amount;
		return r;
	}

	/** Reduce in width from the left edge by 'amount' and return the removed region
	 * @param amount Size in X to reduce by
	 * @return IRECT The removed subrect */
	inline constexpr IRECT ReduceFromLeft(float amount)
	{
		IRECT r = GetFromLeft(amount);
		L += amount;
		return r;
	}

	/** Reduce in width from the right edge by 'amount' and return the removed region
	 * @param amount Size in X to reduce by
	 * @return IRECT The removed subrect */
	inline constexpr IRECT ReduceFromRight(const float amount)
	{
		IRECT r = GetFromRight(amount);
		R -= amount;
		return r;
	}

	/** Get a subrect (by row, column) of this IRECT which is a cell in a grid of size (nRows * nColumns)
	 * @param row Row index of the desired subrect
	 * @param col Column index of the desired subrect
	 * @param nRows Number of rows in the cell grid
	 * @param nColumns Number of columns in the cell grid
	 * @return IRECT The resulting subrect */
	inline constexpr IRECT GetGridCell(int row,
									   int col,
									   int nRows,
									   int nColumns /*, EDirection = EDirection::Horizontal*/) const
	{
		DEBUG_ASSERT(row * col <= nRows * nColumns);  // not enough cells !

		const IRECT vrect = SubRectVertical(nRows, row);
		return vrect.SubRectHorizontal(nColumns, col);
	}

	/** Get a subrect (by index) of this IRECT which is a cell (or union of nCells sequential cells on same row/column)
	 * in a grid of size (nRows * nColumns)
	 * @param cellIndex Index of the desired cell in the cell grid
	 * @param nRows Number of rows in the cell grid
	 * @param nColumns Number of columns in the cell grid
	 * @param dir Desired direction of indexing, by row (EDirection::Horizontal) or by column (EDirection::Vertical)
	 * @param nCells Number of desired sequential cells to join (on same row/column)
	 * @return IRECT The resulting subrect */
	IRECT GetGridCell(
		int cellIndex, int nRows, int nColumns, EDirection dir = EDirection::Horizontal, int nCells = 1) const
	{
		DEBUG_ASSERT(cellIndex <= nRows * nColumns);  // not enough cells !

		int cell = 0;

		if (dir == EDirection::Horizontal)
		{
			for (int row = 0; row < nRows; row++)
			{
				for (int col = 0; col < nColumns; col++)
				{
					if (cell == cellIndex)
					{
						const IRECT vrect = SubRectVertical(nRows, row);
						IRECT rect        = vrect.SubRectHorizontal(nColumns, col);

						for (int n = 1; n < nCells && (col + n) < nColumns; n++)
						{
							rect = rect.Union(vrect.SubRectHorizontal(nColumns, col + n));
						}
						return rect;
					}

					cell++;
				}
			}
		}
		else
		{
			for (int col = 0; col < nColumns; col++)
			{
				for (int row = 0; row < nRows; row++)
				{
					if (cell == cellIndex)
					{
						const IRECT hrect = SubRectHorizontal(nColumns, col);
						IRECT rect        = hrect.SubRectVertical(nRows, row);
						;

						for (int n = 1; n < nCells && (row + n) < nRows; n++)
						{
							rect = rect.Union(hrect.SubRectVertical(nRows, row + n));
						}
						return rect;
					}

					cell++;
				}
			}
		}

		return *this;
	}

	/** @return true If all the values of this IRECT are within 1/1000th of being an integer */
	inline constexpr bool IsPixelAligned() const
	{
		// If all values are within 1/1000th of a pixel of an integer the IRECT is considered pixel aligned
		return math::IsNearlyInteger(L, T, R, B, 1e-3f);
	}

	/** Return true if, when scaled by `scale`, this IRECT is pixel aligned
	 * When scaling this mutliples each value of the IRECT, it does not scale from the center.
	 * @param scale Scale value for the test
	 * @return true The scaled IRECT is pixel-aligned */
	inline const bool IsPixelAligned(float scale) const
	{
		IRECT r = *this;
		r.Scale(scale);
		return r.IsPixelAligned();
	}

	/** Pixel aligns the rect in an inclusive manner (moves all points outwards) */
	inline constexpr void PixelAlign()
	{
		L = math::Floor(L);
		T = math::Floor(T);
		R = math::Ceil(R);
		B = math::Ceil(B);
	}

	/** Pixel-align this IRECT at the given scale factor then scale it back down
	 * When scaling this mutliples each value of the IRECT, it does not scale from the center.
	 * @param scale Scale value for the alignment */
	inline constexpr void PixelAlign(float scale)
	{
		// N.B. - double precision is *required* for accuracy of the reciprocal
		// ? Are you sure? I can't see any difference.

		Scale(scale);
		PixelAlign();
		// Scale(static_cast<float>(1.0 / static_cast<double>(scale)));
		Scale((1.0f / scale));
	}

	/** Get a copy of this IRECT with PixelAlign() called
	 * @return IRECT the new rectangle */
	inline constexpr IRECT GetPixelAligned() const
	{
		IRECT r = *this;
		r.PixelAlign();
		return r;
	}

	/** Get a copy of this IRECT with PixelAlign(scale) called
	 * @param scale Scaling factor for the alignment
	 * @return IRECT the new rectangle */
	inline constexpr IRECT GetPixelAligned(float scale) const
	{
		IRECT r = *this;
		r.PixelAlign(scale);
		return r;
	}

	/** Pixel aligns to nearest pixels
	 * This may make the IRECT smaller, unlike PixelAlign(). */
	inline constexpr void PixelSnap()
	{
		L = math::Round(L);
		T = math::Round(T);
		R = math::Round(R);
		B = math::Round(B);
	}

	/** Pixel align a scaled version of this IRECT
	 * @param scale Scaling factor for the alignment */
	inline constexpr void PixelSnap(float scale)
	{
		// N.B. - double precision is *required* for accuracy of the reciprocal
		Scale(scale);
		PixelSnap();
		// Scale(static_cast<float>(1.0 / static_cast<double>(scale)));
		Scale(1.0f / scale);
	}

	/** @return IRECT A copy of this IRECT with PixelSnap() called */
	inline constexpr IRECT GetPixelSnapped() const
	{
		IRECT r = *this;
		r.PixelSnap();
		return r;
	}

	/** Get a copy of this IRECT with PixelSnap(scale) called
	 * @param Scaling factor for the alignment
	 * @return IRECT the new rectangle */
	inline constexpr IRECT GetPixelSnapped(float scale) const
	{
		IRECT r = *this;
		r.PixelSnap(scale);
		return r;
	}

	/** Pad this IRECT
	 * N.B. Using a positive padding value will expand the IRECT, a negative value will contract it
	 * @param padding Padding amount */
	inline constexpr void Pad(float padding)
	{
		L -= padding;
		T -= padding;
		R += padding;
		B += padding;
	}

	/** Pad this IRECT
	 * N.B. Using a positive padding value will expand the IRECT, a negative value will contract it
	 * @param padL Left-padding
	 * @param padT Top-padding
	 * @param padR Right-padding
	 * @param padB Bottom-padding */
	inline constexpr void Pad(float padL, float padT, float padR, float padB)
	{
		L -= padL;
		T -= padT;
		R += padR;
		B += padB;
	}

	/** Pad this IRECT in the X-axis
	 * N.B. Using a positive padding value will expand the IRECT, a negative value will contract it
	 * @param padding Left and right padding */
	inline constexpr void HPad(float padding)
	{
		L -= padding;
		R += padding;
	}

	/** Pad this IRECT in the Y-axis
	 * N.B. Using a positive padding value will expand the IRECT, a negative value will contract it
	 * @param padding Top and bottom padding */
	inline constexpr void VPad(float padding)
	{
		T -= padding;
		B += padding;
	}

	/** Set the width of this IRECT to 2*padding without changing it's center point on the X-axis
	 * @param padding Left and right padding (1/2 the new width) */
	inline constexpr void MidHPad(float padding)
	{
		const float mw = MW();
		L              = mw - padding;
		R              = mw + padding;
	}

	/** Set the height of this IRECT to 2*padding without changing it's center point on the Y-axis
	 * @param padding Top and bottom padding (1/2 the new height) */
	inline constexpr void MidVPad(float padding)
	{
		const float mh = MH();
		T              = mh - padding;
		B              = mh + padding;
	}

	/** Get a copy of this IRECT with each value padded by `padding`
	 * N.B. Using a positive padding value will expand the IRECT, a negative value will contract it
	 * @param padding Padding amount
	 * @return IRECT the new rectangle */
	inline constexpr IRECT GetPadded(const float padding) const
	{
		return IRECT(L - padding, T - padding, R + padding, B + padding);
	}

	/** Get a copy of this IRECT with the values padded
	 * N.B. Using a positive padding value will expand the IRECT, a negative value will contract it
	 * @param padL Left-padding
	 * @param padT Top-padding
	 * @param padR Right-padding
	 * @param padB Bottom-padding
	 * @return IRECT the new rectangle */
	inline constexpr IRECT GetPadded(float padL, float padT, float padR, float padB) const
	{
		return IRECT(L - padL, T - padT, R + padR, B + padB);
	}

	/** Get a copy of this IRECT padded in the X-axis
	 * N.B. Using a positive padding value will expand the IRECT, a negative value will contract it
	 * @param padding Left and right padding
	 * @return IRECT the new rectangle */
	inline constexpr IRECT GetHPadded(float padding) const
	{
		return IRECT(L - padding, T, R + padding, B);
	}

	/** Get a copy of this IRECT padded in the Y-axis
	 * N.B. Using a positive padding value will expand the IRECT, a negative value will contract it
	 * @param padding Top and bottom padding
	 * @return IRECT the new rectangle */
	inline constexpr IRECT GetVPadded(float padding) const
	{
		return IRECT(L, T - padding, R, B + padding);
	}

	/** Get a copy of this IRECT where its width = 2 * padding but the center point on the X-axis has not changed
	 * @param padding Left and right padding (1/2 the new width)
	 * @return IRECT the new rectangle */
	inline constexpr IRECT GetMidHPadded(float padding) const
	{
		return IRECT(MW() - padding, T, MW() + padding, B);
	}

	/** Get a copy of this IRECT where its height = 2 * padding but the center point on the Y-axis has not changed
	 * @param padding Top and bottom padding (1/2 the new height)
	 * @return IRECT the new rectangle */
	inline constexpr IRECT GetMidVPadded(float padding) const
	{
		return IRECT(L, MH() - padding, R, MH() + padding);
	}

	/** Get a copy of this IRECT with a new width
	 * @param w Width of the new rectangle
	 * @param rhs If true the new rectangle will expand from the right side, otherwise it will expand from the left
	 * @return IRECT the new rectangle */
	inline constexpr IRECT GetHSliced(float w, bool rhs = false) const
	{
		if (rhs)
			return IRECT(R - w, T, R, B);
		else
			return IRECT(L, T, L + w, B);
	}

	/** Get a copy of this IRECT with a new height
	 * @param h Height of the new rectangle
	 * @param bot If true the new rectangle will expand from the bottom, otherwise it will expand from the top
	 * @return IRECT the new rectangle */
	inline constexpr IRECT GetVSliced(float h, bool bot = false) const
	{
		if (bot)
			return IRECT(L, B - h, R, B);
		else
			return IRECT(L, T + h, R, B);
	}

	/** /todo
	 * @param rhs /todo */
	void Clank(const IRECT& rhs)
	{
		if (L < rhs.L)
		{
			R = std::min(rhs.R - 1, R + rhs.L - L);
			L = rhs.L;
		}
		if (T < rhs.T)
		{
			B = std::min(rhs.B - 1, B + rhs.T - T);
			T = rhs.T;
		}
		if (R >= rhs.R)
		{
			L = std::max(rhs.L, L - (R - rhs.R + 1));
			R = rhs.R - 1;
		}
		if (B >= rhs.B)
		{
			T = std::max(rhs.T, T - (B - rhs.B + 1));
			B = rhs.B - 1;
		}
	}

	/** Multiply each field of this IRECT by `scale`.
	 * @param scale The amount to multiply each field by */
	inline constexpr void Scale(float scale)
	{
		L *= scale;
		T *= scale;
		R *= scale;
		B *= scale;
	}

	/** Scale the width and height of this IRECT by `scale` without changing the center point
	 * @param scale The scaling factor */
	inline constexpr void ScaleAboutCentre(float scale)
	{
		float x  = MW();
		float y  = MH();
		float hw = W() * 0.5f;
		float hh = H() * 0.5f;
		L        = x - (hw * scale);
		T        = y - (hh * scale);
		R        = x + (hw * scale);
		B        = y + (hh * scale);
	}

	/** Get a copy of this IRECT with all values multiplied by `scale`.
	 * @param scale The amount to multiply each value by
	 * @return IRECT the resulting rectangle */
	inline constexpr IRECT GetScaled(float scale) const
	{
		IRECT r = *this;
		r.Scale(scale);
		return r;
	}

	/** Get a copy of this IRECT where the width and height are multiplied by `scale` without changing the center point
	 * @param scale Scaling factor
	 * @return IRECT the resulting rectangle */
	inline constexpr IRECT GetScaledAboutCentre(float scale) const
	{
		IRECT r = *this;
		r.ScaleAboutCentre(scale);
		return r;
	}

	/** Get a rectangle that is a linear interpolation between `start` and `dest`
	 * @param start Starting rectangle
	 * @param dest Ending rectangle
	 * @param progress Interpolation point
	 * @return IRECT the new rectangle */
	static inline constexpr IRECT LinearInterpolateBetween(const IRECT& start, const IRECT& dest, float progress)
	{
		IRECT result;
		result.L = start.L + progress * (dest.L - start.L);
		result.T = start.T + progress * (dest.T - start.T);
		result.R = start.R + progress * (dest.R - start.R);
		result.B = start.B + progress * (dest.B - start.B);
		return result;
	}

	/** Get a random point within this rectangle
	 * @param x OUT output X value of point
	 * @param y OUT output Y value of point */
	inline const void GetRandomPoint(float& x, float& y) const
	{
		const float r1 = static_cast<float>(std::rand() / (static_cast<float>(RAND_MAX) + 1.f));
		const float r2 = static_cast<float>(std::rand() / (static_cast<float>(RAND_MAX) + 1.f));

		x = L + r1 * W();
		y = T + r2 * H();
	}

	/** @return IRECT A random rectangle inside this IRECT */
	inline const IRECT GetRandomSubRect() const
	{
		float l, t, r, b;
		GetRandomPoint(l, t);
		IRECT tmp = IRECT(l, t, R, B);
		tmp.GetRandomPoint(r, b);

		return IRECT(l, t, r, b);
	}

	/** Offset each field of the rectangle
	 * @param l Left offset
	 * @param t Top offset
	 * @param r Right offset
	 * @param b Bottom offset */
	inline constexpr void Alter(float l, float t, float r, float b)
	{
		L += l;
		T += t;
		R += r;
		B += b;
	}

	/** Get a copy of this rectangle where each field is offset
	 * @param l Left offset
	 * @param t Top offset
	 * @param r Right offset
	 * @param b Bottom offset
	 * @return IRECT the new rectangle */
	inline constexpr IRECT GetAltered(float l, float t, float r, float b) const
	{
		return IRECT(L + l, T + t, R + r, B + b);
	}

	/** Translate this rectangle
	 * @param x Offset in the X axis
	 * @param y Offset in the Y axis */
	inline constexpr void Translate(float x, float y)
	{
		L += x;
		T += y;
		R += x;
		B += y;
	}

	/** Get a translated copy of this rectangle
	 * @param x Offset in the X axis
	 * @param y Offset in the Y axis
	 * @return IRECT the new rectangle */
	inline constexpr IRECT GetTranslated(float x, float y) const
	{
		return IRECT(L + x, T + y, R + x, B + y);
	}

	/** Get a copy of this rectangle translated on the X axis
	 * @param x Offset
	 * @return IRECT the new rectangle */
	inline constexpr IRECT GetHShifted(float x) const
	{
		return GetTranslated(x, 0.f);
	}

	/** Get a copy of this rectangle translated on the Y axis
	 * @param y Offset
	 * @return IRECT the new rectangle */
	inline constexpr IRECT GetVShifted(float y) const
	{
		return GetTranslated(0.f, y);
	}

	/** Get a rectangle the size of `sr` but with the same center point as this rectangle
	 * @param sr Size rectangle
	 * @return IRECT the new rectangle */
	inline constexpr IRECT GetCentredInside(const IRECT& sr) const
	{
		IRECT r;
		r.L = MW() - sr.W() * 0.5f;
		r.T = MH() - sr.H() * 0.5f;
		r.R = r.L + sr.W();
		r.B = r.T + sr.H();

		return r;
	}

	/** Get a rectangle with the same center point as this rectangle and the given size
	 * @param w Width of the new rectangle (minimum 1.0)
	 * @param h Height of the new rectangle (a value of 0 will make it the same as w, thus a square)
	 * @return IRECT the new rectangle */
	inline constexpr IRECT GetCentredInside(float w, float h = 0.f) const
	{
		w = std::max(w, 1.f);

		if (h <= 0.f)
			h = w;

		IRECT r;
		r.L = MW() - w / 2.f;
		r.T = MH() - h / 2.f;
		r.R = r.L + w;
		r.B = r.T + h;

		return r;
	}

	/** Get a rectangle with the same center point as this rectangle and the size of the bitmap
	 * @param bitmap Bitmap used to size the new rectangle
	 * @return IRECT the new rectangle */
	inline constexpr IRECT GetCentredInside(const IBitmap& bitmap) const
	{
		IRECT r;
		r.L = MW() - bitmap.FW() * 0.5f;
		r.T = MH() - bitmap.FH() * 0.5f;
		r.R = r.L + (float) bitmap.FW();
		r.B = r.T + (float) bitmap.FH();

		return r;
	}

	/** Vertically align this rect to the reference IRECT
	 * @param sr the source IRECT to use as reference
	 * @param align the vertical alignment */
	inline constexpr void VAlignTo(const IRECT& sr, const EVAlign align)
	{
		const float height = H();
		switch (align)
		{
			case EVAlign::Top:
				T = sr.T;
				B = sr.T + height;
				break;
			case EVAlign::Bottom:
				T = sr.B - height;
				B = sr.B;
				break;
			case EVAlign::Middle:
				T = sr.T + (sr.H() * 0.5f) - (height * 0.5f);
				B = sr.T + (sr.H() * 0.5f) - (height * 0.5f) + height;
				break;
		}
	}

	/** Horizontally align this rect to the reference IRECT
	 * @param sr the IRECT to use as reference
	 * @param align the horizontal alignment */
	inline constexpr void HAlignTo(const IRECT& sr, const EAlign align)
	{
		const float width = W();
		switch (align)
		{
			case EAlign::Near:
				L = sr.L;
				R = sr.L + width;
				break;
			case EAlign::Far:
				L = sr.R - width;
				R = sr.R;
				break;
			case EAlign::Center:
				L = sr.L + (sr.W() * 0.5f) - (width * 0.5f);
				R = sr.L + (sr.W() * 0.5f) - (width * 0.5f) + width;
				break;
		}
	}

	/** Get a rectangle the same dimensions as this one, vertically aligned to the reference IRECT
	 * @param sr the source IRECT to use as reference
	 * @param align the vertical alignment
	 * @return the new rectangle */
	inline constexpr IRECT GetVAlignedTo(const IRECT& sr, EVAlign align) const
	{
		IRECT result = *this;
		result.VAlignTo(sr, align);
		return result;
	}

	/** @return float Either the width or the height of the rectangle, whichever is less */
	inline constexpr float GetLengthOfShortestSide() const
	{
		return W() < H() ? W() : H();
	}

	/** Get a rectangle the same dimensions as this one, horizontally aligned to the reference IRECT
	 * @param sr the IRECT to use as reference
	 * @param align the horizontal alignment
	 * @return the new rectangle */
	inline constexpr IRECT GetHAlignedTo(const IRECT& sr, const EAlign align) const
	{
		IRECT result = *this;
		result.HAlignTo(sr, align);
		return result;
	}

	/** Print the IRECT's detailes to the console in Debug builds */
	inline const void DBGPrint() const
	{
		DBGMSG("L: %f, T: %f, R: %f, B: %f,: W: %f, H: %f\n", L, T, R, B, W(), H());
	}
};

/** Used to manage mouse modifiers i.e. right click and shift/control/alt keys. Also used for multiple touches, to keep
 * track of touch radius */
struct IMouseMod
{
	bool L, R, S, C, A;
	ITouchID touchID  = 0;
	float touchRadius = 0.f;

	/** Create an IMouseMod
	 * @param l left mouse button pressed
	 * @param r right mouse button pressed
	 * @param s shift pressed
	 * @param c ctrl pressed
	 * @param a alt pressed
	 * @param touchID touch identifier, for multi-touch */
	IMouseMod(bool l = false, bool r = false, bool s = false, bool c = false, bool a = false, ITouchID touchID = 0)
		: L(l)
		, R(r)
		, S(s)
		, C(c)
		, A(a)
		, touchID(touchID)
	{
	}

	/** \c true if this IMouseMod is linked to a touch event */
	bool IsTouch() const
	{
		return touchID > 0;
	}

	/** Print the mouse modifier values to the console in Debug builds */
	void DBGPrint()
	{
		DBGMSG("L: %i, R: %i, S: %i, C: %i,: A: %i\n", L, R, S, C, A);
	}
};

/** Used to group mouse coordinates with mouse modifier information */
struct IMouseInfo
{
	float x, y;
	float dX, dY;
	IMouseMod ms;
};

/** Used to describe a particular gesture */
struct IGestureInfo
{
	float x             = 0.f;
	float y             = 0.f;
	float scale         = 0.f;  // pinch,
	float velocity      = 0.f;  // pinch, rotate
	float angle         = 0.f;  // rotate,
	EGestureState state = EGestureState::Unknown;
	EGestureType type   = EGestureType::Unknown;
};

/** Used to manage a list of rectangular areas and optimize them for drawing to the screen. */
class IRECTList
{
 public:
	IRECTList() {}

	IRECTList(const IRECTList&) = delete;
	IRECTList& operator=(const IRECTList&) = delete;

	/** /todo
	 * @return int /todo */
	int Size() const
	{
		return mRects.GetSize();
	}

	/** /todo
	 * @param rect /todo */
	void Add(const IRECT rect)
	{
		mRects.Add(rect);
	}

	/** /todo
	 * @param idx /todo
	 * @param rect /todo */
	void Set(int idx, const IRECT rect)
	{
		*(mRects.GetFast() + idx) = rect;
	}

	/** /todo
	 * @param idx /todo
	 * @return const IRECT& /todo */
	const IRECT& Get(int idx) const
	{
		return *(mRects.GetFast() + idx);
	}

	/** /todo */
	void Clear()
	{
		mRects.Resize(0);
	}

	/** /todo * @return IRECT /todo */
	IRECT Bounds()
	{
		IRECT r = Get(0);
		for (auto i = 1; i < mRects.GetSize(); i++)
			r = r.Union(Get(i));
		return r;
	}

	/** /todo */
	void PixelAlign()
	{
		for (auto i = 0; i < Size(); i++)
		{
			IRECT r = Get(i);
			r.PixelAlign();
			Set(i, r);
		}
	}

	/** /todo
	 * @param scale /todo */
	void PixelAlign(float scale)
	{
		for (auto i = 0; i < Size(); i++)
		{
			IRECT r = Get(i);
			r.PixelAlign(scale);
			Set(i, r);
		}
	}

	/** Find the first index of the rect that contains point x, y, if it exists
	 * @param x Horizontal position to check
	 * @param y Vertical position to check
	 * @return integer index of rect that contains point x,y or -1 if not found */
	int Find(float x, float y) const
	{
		for (auto i = 0; i < Size(); i++)
		{
			if (Get(i).Contains(x, y))
				return i;
		}

		return -1;
	}

	/** /todo
	 * @param input /todo
	 * @param rects /todo
	 * @param rowFractions /todo
	 * @param colFractions /todo
	 * @return true /todo
	 * @return false /todo */
	static bool GetFracGrid(const IRECT& input,
							IRECTList& rects,
							const std::initializer_list<float>& rowFractions,
							const std::initializer_list<float>& colFractions)
	{
		IRECT rowsLeft = input;
		float y        = 0.;
		float x        = 0.;

		if (std::accumulate(rowFractions.begin(), rowFractions.end(), 0.f) != 1.)
			return false;

		if (std::accumulate(colFractions.begin(), colFractions.end(), 0.f) != 1.)
			return false;

		for (auto& rowFrac : rowFractions)
		{
			IRECT thisRow = input.FracRectVertical(rowFrac, true).GetTranslated(0, y);

			x = 0.;

			for (auto& colFrac : colFractions)
			{
				IRECT thisCell = thisRow.FracRectHorizontal(colFrac).GetTranslated(x, 0);

				rects.Add(thisCell);

				x += thisCell.W();
			}

			rowsLeft.Intersect(thisRow);

			y = rects.Bounds().H();
		}

		return true;
	}

	/** /todo  */
	void Optimize()
	{
		// Remove rects that are contained by other rects and intersections
		for (int i = 0; i < Size(); i++)
		{
			for (int j = i + 1; j < Size(); j++)
			{
				if (Get(i).Contains(Get(j)))
				{
					mRects.Delete(j);
					j--;
				}
				else if (Get(j).Contains(Get(i)))
				{
					mRects.Delete(i);
					i--;
					break;
				}
				else if (Get(i).Intersects(Get(j)))
				{
					IRECT intersection = Get(i).Intersect(Get(j));

					if (Get(i).Mergeable(intersection))
						Set(i, Shrink(Get(i), intersection));
					else if (Get(j).Mergeable(intersection))
						Set(j, Shrink(Get(j), intersection));
					else if (Get(i).Area() < Get(j).Area())
						Set(i, Split(Get(i), intersection));
					else
						Set(j, Split(Get(j), intersection));
				}
			}
		}

		// Merge any rects that can be merged
		for (int i = 0; i < Size(); i++)
		{
			for (int j = i + 1; j < Size(); j++)
			{
				if (Get(i).Mergeable(Get(j)))
				{
					Set(j, Get(i).Union(Get(j)));
					mRects.Delete(i);
					i = -1;
					break;
				}
			}
		}
	}

 private:
	/** /todo
	 * @param r /todo
	 * @param i /todo
	 * @return IRECT /todo */
	IRECT Shrink(const IRECT& r, const IRECT& i)
	{
		if (i.L != r.L)
			return IRECT(r.L, r.T, i.L, r.B);
		if (i.T != r.T)
			return IRECT(r.L, r.T, r.R, i.T);
		if (i.R != r.R)
			return IRECT(i.R, r.T, r.R, r.B);
		return IRECT(r.L, i.B, r.R, r.B);
	}

	/** /todo
	 * @param r /todo
	 * @param i /todo
	 * @return IRECT /todo */
	IRECT Split(const IRECT r, const IRECT& i)
	{
		if (r.L == i.L)
		{
			if (r.T == i.T)
			{
				Add(IRECT(i.R, r.T, r.R, i.B));
				return IRECT(r.L, i.B, r.R, r.B);
			}
			else
			{
				Add(IRECT(r.L, r.T, r.R, i.T));
				return IRECT(i.R, i.T, r.R, r.B);
			}
		}

		if (r.T == i.T)
		{
			Add(IRECT(r.L, r.T, i.L, i.B));
			return IRECT(r.L, i.B, r.R, r.B);
		}
		else
		{
			Add(IRECT(r.L, r.T, r.R, i.T));
			return IRECT(r.L, i.T, i.L, r.B);
		}
	}

	WDL_TypedBuf<IRECT> mRects;
};

/** Used to store transformation matrices **/
struct IMatrix
{
	/** /todo
	 * @param xx /todo
	 * @param yx /todo
	 * @param xy /todo
	 * @param yy /todo
	 * @param tx /todo
	 * @param ty /todo */
	IMatrix(double xx, double yx, double xy, double yy, double tx, double ty)
		: mXX(xx)
		, mYX(yx)
		, mXY(xy)
		, mYY(yy)
		, mTX(tx)
		, mTY(ty)
	{
	}

	/** /todo */
	IMatrix() : IMatrix(1.0, 0.0, 0.0, 1.0, 0.0, 0.0) {}

	/** /todo
	 * @param x /todo
	 * @param y /todo
	 * @return IMatrix& /todo */
	IMatrix& Translate(float x, float y)
	{
		return Transform(IMatrix(1.0, 0.0, 0.0, 1.0, x, y));
	}

	/** /todo
	 * @param x /todo
	 * @param y /todo
	 * @return IMatrix& /todo */
	IMatrix& Scale(float x, float y)
	{
		return Transform(IMatrix(x, 0.0, 0.0, y, 0.0, 0.0));
	}

	/** /todo
	 * @param a /todo
	 * @return IMatrix& /todo */
	IMatrix& Rotate(float a)
	{
		const double rad = math::DegToRad(a);
		const double c   = std::cos(rad);
		const double s   = std::sin(rad);

		return Transform(IMatrix(c, s, -s, c, 0.0, 0.0));
	}

	/** /todo
	 * @param xa /todo
	 * @param ya /todo
	 * @return IMatrix& /todo */
	IMatrix& Skew(float xa, float ya)
	{
		return Transform(IMatrix(1.0, math::Tan(math::DegToRad(ya)), math::Tan(math::DegToRad(xa)), 1.0, 0.0, 0.0));
	}

	/** /todo
	 * @param x /todo
	 * @param y /todo
	 * @param x0 /todo
	 * @param y0 /todo */
	void TransformPoint(float& x, float& y, float x0, float y0) const
	{
		x = x0 * mXX + y0 * mXY + mTX;
		y = x0 * mYX + y0 * mYY + mTY;
	};

	void TransformPoint(double& x, double& y, double x0, double y0) const
	{
		x = x0 * mXX + y0 * mXY + mTX;
		y = x0 * mYX + y0 * mYY + mTY;
	};

	/** /todo
	 * @param x /todo
	 * @param y /todo */
	void TransformPoint(double& x, double& y) const
	{
		TransformPoint(x, y, x, y);
	};

	/** /todo
	 * @param before /todo
	 * @param after /todo
	 * @return IMatrix& /todo */
	IMatrix& Transform(const IRECT& before, const IRECT& after)
	{
		const double sx = after.W() / before.W();
		const double sy = after.H() / before.H();
		const double tx = after.L - before.L * sx;
		const double ty = after.T - before.T * sy;

		return *this = IMatrix(sx, 0.0, 0.0, sy, tx, ty);
	}

	/** /todo
	 * @param m /todo
	 * @return IMatrix& /todo */
	IMatrix& Transform(const IMatrix& m)
	{
		IMatrix p = *this;

		mXX = m.mXX * p.mXX + m.mYX * p.mXY;
		mYX = m.mXX * p.mYX + m.mYX * p.mYY;
		mXY = m.mXY * p.mXX + m.mYY * p.mXY;
		mYY = m.mXY * p.mYX + m.mYY * p.mYY;
		mTX = m.mTX * p.mXX + m.mTY * p.mXY + p.mTX;
		mTY = m.mTX * p.mYX + m.mTY * p.mYY + p.mTY;

		return *this;
	}

	/** /todo
	 * @return IMatrix& /todo */
	IMatrix& Invert()
	{
		IMatrix m = *this;

		double d = 1.0 / (m.mXX * m.mYY - m.mYX * m.mXY);

		mXX = m.mYY * d;
		mYX = -m.mYX * d;
		mXY = -m.mXY * d;
		mYY = m.mXX * d;
		mTX = (-(m.mTX * mXX) - (m.mTY * mXY));
		mTY = (-(m.mTX * mYX) - (m.mTY * mYY));

		return *this;
	}

	double mXX, mYX, mXY, mYY, mTX, mTY;
};

/** Used to represent a point/stop in a gradient **/
struct IColorStop
{
	IColorStop() : mOffset(0.f) {}

	/** /todo
	 * @param color /todo
	 * @param offset /todo */
	IColorStop(IColor color, float offset) : mColor(color), mOffset(offset)
	{
		assert(offset >= 0.0 && offset <= 1.0);
	}

	IColor mColor;
	float mOffset;
};

/** Used to store pattern information for gradients **/
struct IPattern
{
	EPatternType mType;
	EPatternExtend mExtend;
	IColorStop mStops[16];
	int mNStops;
	IMatrix mTransform;

	/** /todo
	 * @param type /todo */
	IPattern(EPatternType type) : mType(type), mExtend(EPatternExtend::Pad), mNStops(0) {}

	/** /todo
	 * @param color /todo */
	IPattern(const IColor& color) : mType(EPatternType::Solid), mExtend(EPatternExtend::Pad), mNStops(1)
	{
		mStops[0] = IColorStop(color, 0.0);
	}

	/** /todo
	 * @param x1 /todo
	 * @param y1 /todo
	 * @param x2 /todo
	 * @param y2 /todo
	 * @param stops /todo
	 * @return IPattern /todo */
	static IPattern CreateLinearGradient(
		float x1, float y1, float x2, float y2, const std::initializer_list<IColorStop>& stops = {})
	{
		IPattern pattern(EPatternType::Linear);

		// Calculate the affine transform from one line segment to another!
		const double xd = x2 - x1;
		const double yd = y2 - y1;
		const double d  = sqrt(xd * xd + yd * yd);
		const double a  = atan2(xd, yd);
		const double s  = std::sin(a) / d;
		const double c  = std::cos(a) / d;

		const double x0 = -(x1 * c - y1 * s);
		const double y0 = -(x1 * s + y1 * c);

		pattern.SetTransform(static_cast<float>(c),
							 static_cast<float>(s),
							 static_cast<float>(-s),
							 static_cast<float>(c),
							 static_cast<float>(x0),
							 static_cast<float>(y0));

		for (auto& stop : stops)
			pattern.AddStop(stop.mColor, stop.mOffset);

		return pattern;
	}

	/** /todo
	 * @param bounds /todo
	 * @param direction /todo
	 * @param stops /todo
	 * @return IPattern /todo */
	static IPattern CreateLinearGradient(const IRECT& bounds,
										 EDirection direction,
										 const std::initializer_list<IColorStop>& stops = {})
	{
		float x1, y1, x2, y2;

		if (direction == EDirection::Horizontal)
		{
			y1 = bounds.MH();
			y2 = y1;
			x1 = bounds.L;
			x2 = bounds.R;
		}
		else  //(direction == EDirection::Vertical)
		{
			x1 = bounds.MW();
			x2 = x1;
			y1 = bounds.T;
			y2 = bounds.B;
		}

		return CreateLinearGradient(x1, y1, x2, y2, stops);
	}

	/** /todo
	 * @param x1 /todo
	 * @param y1 /todo
	 * @param r /todo
	 * @param stops /todo
	 * @return IPattern /todo */
	static IPattern CreateRadialGradient(float x1,
										 float y1,
										 float r,
										 const std::initializer_list<IColorStop>& stops = {})
	{
		IPattern pattern(EPatternType::Radial);

		const float s = 1.f / r;

		pattern.SetTransform(s, 0, 0, s, -(x1 * s), -(y1 * s));

		for (auto& stop : stops)
			pattern.AddStop(stop.mColor, stop.mOffset);

		return pattern;
	}

	static IPattern CreateSweepGradient(float x1,
										float y1,
										const std::initializer_list<IColorStop>& stops = {},
										float angleStart                               = 0.f,
										float angleEnd                                 = 360.f)
	{
		IPattern pattern(EPatternType::Sweep);

#ifdef IGRAPHICS_SKIA
		angleStart -= 90;
		angleEnd -= 90;
#endif

		float rad = math::DegToRad(angleStart);
		float c   = std::cos(rad);
		float s   = std::sin(rad);

		pattern.SetTransform(c, s, -s, c, -x1, -y1);

		for (auto& stop : stops)
		{
			pattern.AddStop(stop.mColor, stop.mOffset * (angleEnd - angleStart) / 360.f);
		}
		return pattern;
	}

	/** /todo
	 * @return int /todo */
	int NStops() const
	{
		return mNStops;
	}

	/** /todo
	 * @param idx /todo
	 * @return const IColorStop& /todo */
	const IColorStop& GetStop(int idx) const
	{
		return mStops[idx];
	}

	/** /todo
	 * @param color /todo
	 * @param offset /todo */
	void AddStop(IColor color, float offset)
	{
		assert(mType != EPatternType::Solid && mNStops < 16);
		assert(!mNStops || GetStop(mNStops - 1).mOffset < offset);
		if (mNStops < 16)
			mStops[mNStops++] = IColorStop(color, offset);
	}

	/** /todo
	 * @param xx /todo
	 * @param yx /todo
	 * @param xy /todo
	 * @param yy /todo
	 * @param x0 /todo
	 * @param y0 /todo */
	void SetTransform(float xx, float yx, float xy, float yy, float x0, float y0)
	{
		mTransform = IMatrix(xx, yx, xy, yy, x0, y0);
	}

	/** /todo
	 * @param transform /todo */
	void SetTransform(const IMatrix& transform)
	{
		mTransform = transform;
	}
};

/** An abstraction that is used to store a temporary raster image/framebuffer.
 * The layer is drawn with a specific offset to the graphics context.
 * ILayers take ownership of the underlying bitmaps
 * In GPU-based backends (NanoVG), this is a texture. */
class ILayer
{
	friend IGraphics;

 public:
	/** /todo
	 * @param pBitmap /todo
	 * @param r /todo
	 * @param pControl /todo
	 * @param cr /todo */
	ILayer(APIBitmap* pBitmap, const IRECT& r, IControl* pControl, const IRECT& cr)
		: mBitmap(pBitmap)
		, mControl(pControl)
		, mControlRECT(cr)
		, mRECT(r)
		, mInvalid(false)
	{
	}

	ILayer(const ILayer&) = delete;
	ILayer operator=(const ILayer&) = delete;

	/** /todo */
	void Invalidate()
	{
		mInvalid = true;
	}

	/**  @return const APIBitmap* /todo */
	const APIBitmap* GetAPIBitmap() const
	{
		return mBitmap.get();
	}

	/** @return IBitmap /todo */
	IBitmap GetBitmap() const
	{
		return IBitmap(mBitmap.get(), 1, false);
	}

	/** @return const IRECT& /todo*/
	const IRECT& Bounds() const
	{
		return mRECT;
	}

 private:
	std::unique_ptr<APIBitmap> mBitmap;
	IControl* mControl;
	IRECT mControlRECT;
	IRECT mRECT;
	bool mInvalid;
};

/** ILayerPtr is a managed pointer for transferring the ownership of layers */
using ILayerPtr = std::unique_ptr<ILayer>;

/** Used to specify a gaussian drop-shadow. */
struct IShadow
{
	IShadow() {}

	/** /todo
	 * @param pattern /todo
	 * @param blurSize /todo
	 * @param xOffset /todo
	 * @param yOffset /todo
	 * @param opacity /todo
	 * @param drawForeground /todo */
	IShadow(const IPattern& pattern,
			float blurSize,
			float xOffset,
			float yOffset,
			float opacity,
			bool drawForeground = true)
		: mPattern(pattern)
		, mBlurSize(blurSize)
		, mXOffset(xOffset)
		, mYOffset(yOffset)
		, mOpacity(opacity)
		, mDrawForeground(drawForeground)
	{
	}

	IPattern mPattern    = COLOR_BLACK;
	float mBlurSize      = 0.f;
	float mXOffset       = 0.f;
	float mYOffset       = 0.f;
	float mOpacity       = 1.f;
	bool mDrawForeground = true;
};

/** Contains a set of 9 colors used to theme IVControls */
struct IVColorSpec
{
	IColor mColors[kNumVColors];

	const IColor& GetColor(EVColor color) const
	{
		return mColors[(int) color];
	}

	static const IColor& GetDefaultColor(EVColor idx)
	{
		switch (idx)
		{
			case kBG:
				return DEFAULT_BGCOLOR;  // Background
			case kFG:
				return DEFAULT_FGCOLOR;  // OFF/Foreground
			case kPR:
				return DEFAULT_PRCOLOR;  // ON/Pressed
			case kFR:
				return DEFAULT_FRCOLOR;  // Frame
			case kHL:
				return DEFAULT_HLCOLOR;  // Highlight
			case kSH:
				return DEFAULT_SHCOLOR;  // Shadow
			case kX1:
				return DEFAULT_X1COLOR;  // Extra 1
			case kX2:
				return DEFAULT_X2COLOR;  // Extra 2
			case kX3:
				return DEFAULT_X3COLOR;  // Extra 3
			default:
				return COLOR_TRANSPARENT;
		};
	}

	IVColorSpec()
	{
		ResetColors();
	}

	IVColorSpec(const std::initializer_list<IColor>& colors)
	{
		assert(colors.size() <= kNumVColors);

		int i = 0;

		for (auto& c : colors)
		{
			mColors[i++] = c;
		}

		for (; i < kNumVColors; i++)
		{
			mColors[i] = GetDefaultColor((EVColor) i);
		}
	}

	/** Reset the colors to the defaults  */
	void ResetColors()
	{
		for (int i = 0; i < kNumVColors; i++)
		{
			mColors[i] = GetDefaultColor((EVColor) i);
		}
	}
};

const IVColorSpec DEFAULT_COLOR_SPEC = IVColorSpec();

static constexpr bool DEFAULT_HIDE_CURSOR      = true;
static constexpr bool DEFAULT_SHOW_VALUE       = true;
static constexpr bool DEFAULT_SHOW_LABEL       = true;
static constexpr bool DEFAULT_DRAW_FRAME       = true;
static constexpr bool DEFAULT_DRAW_SHADOWS     = true;
static constexpr bool DEFAULT_EMBOSS           = false;
static constexpr float DEFAULT_ROUNDNESS       = 0.f;
static constexpr float DEFAULT_FRAME_THICKNESS = 1.f;
static constexpr float DEFAULT_SHADOW_OFFSET   = 3.f;
static constexpr float DEFAULT_WIDGET_FRAC     = 1.f;
static constexpr float DEFAULT_WIDGET_ANGLE    = 0.f;
const IText DEFAULT_LABEL_TEXT {DEFAULT_TEXT_SIZE + 5.f, EVAlign::Top};
const IText DEFAULT_VALUE_TEXT {DEFAULT_TEXT_SIZE, EVAlign::Bottom};

/** A struct encapsulating a set of properties used to configure IVControls */
struct IVStyle
{
	bool hideCursor       = DEFAULT_HIDE_CURSOR;
	bool showLabel        = DEFAULT_SHOW_LABEL;
	bool showValue        = DEFAULT_SHOW_VALUE;
	bool drawFrame        = DEFAULT_DRAW_FRAME;
	bool drawShadows      = DEFAULT_DRAW_SHADOWS;
	bool emboss           = DEFAULT_EMBOSS;
	float roundness       = DEFAULT_ROUNDNESS;
	float frameThickness  = DEFAULT_FRAME_THICKNESS;
	float shadowOffset    = DEFAULT_SHADOW_OFFSET;
	float widgetFrac      = DEFAULT_WIDGET_FRAC;
	float angle           = DEFAULT_WIDGET_ANGLE;
	IVColorSpec colorSpec = DEFAULT_COLOR_SPEC;
	IText labelText       = DEFAULT_LABEL_TEXT;
	IText valueText       = DEFAULT_VALUE_TEXT;

	IVStyle(bool showLabel            = DEFAULT_SHOW_LABEL,
			bool showValue            = DEFAULT_SHOW_VALUE,
			const IVColorSpec& colors = {DEFAULT_BGCOLOR,
										 DEFAULT_FGCOLOR,
										 DEFAULT_PRCOLOR,
										 DEFAULT_FRCOLOR,
										 DEFAULT_HLCOLOR,
										 DEFAULT_SHCOLOR,
										 DEFAULT_X1COLOR,
										 DEFAULT_X2COLOR,
										 DEFAULT_X3COLOR},
			const IText& labelText    = DEFAULT_LABEL_TEXT,
			const IText& valueText    = DEFAULT_VALUE_TEXT,
			bool hideCursor           = DEFAULT_HIDE_CURSOR,
			bool drawFrame            = DEFAULT_DRAW_FRAME,
			bool drawShadows          = DEFAULT_DRAW_SHADOWS,
			bool emboss               = DEFAULT_EMBOSS,
			float roundness           = DEFAULT_ROUNDNESS,
			float frameThickness      = DEFAULT_FRAME_THICKNESS,
			float shadowOffset        = DEFAULT_SHADOW_OFFSET,
			float widgetFrac          = DEFAULT_WIDGET_FRAC,
			float angle               = DEFAULT_WIDGET_ANGLE)
		: showLabel(showLabel)
		, showValue(showValue)
		, colorSpec(colors)
		, labelText(labelText)
		, valueText(valueText)
		, hideCursor(hideCursor)
		, drawFrame(drawFrame)
		, drawShadows(drawShadows)
		, emboss(emboss)
		, roundness(roundness)
		, frameThickness(frameThickness)
		, shadowOffset(shadowOffset)
		, widgetFrac(widgetFrac)
		, angle(angle)
	{
	}

	IVStyle(const std::initializer_list<IColor>& colors) : colorSpec(colors) {}

	IVStyle WithShowLabel(bool show = true) const
	{
		IVStyle newStyle   = *this;
		newStyle.showLabel = show;
		return newStyle;
	}
	IVStyle WithShowValue(bool show = true) const
	{
		IVStyle newStyle   = *this;
		newStyle.showValue = show;
		return newStyle;
	}
	IVStyle WithLabelText(const IText& text) const
	{
		IVStyle newStyle   = *this;
		newStyle.labelText = text;
		return newStyle;
	}
	IVStyle WithValueText(const IText& text) const
	{
		IVStyle newStyle   = *this;
		newStyle.valueText = text;
		return newStyle;
	}
	IVStyle WithHideCursor(bool hide = true) const
	{
		IVStyle newStyle    = *this;
		newStyle.hideCursor = hide;
		return newStyle;
	}
	IVStyle WithColor(EVColor idx, IColor color) const
	{
		IVStyle newStyle                = *this;
		newStyle.colorSpec.mColors[idx] = color;
		return newStyle;
	}
	IVStyle WithColors(IVColorSpec spec) const
	{
		IVStyle newStyle   = *this;
		newStyle.colorSpec = spec;
		return newStyle;
	}
	IVStyle WithRoundness(float v) const
	{
		IVStyle newStyle   = *this;
		newStyle.roundness = math::Clamp(v, 0.f, 1.f);
		return newStyle;
	}
	IVStyle WithFrameThickness(float v) const
	{
		IVStyle newStyle        = *this;
		newStyle.frameThickness = v;
		return newStyle;
	}
	IVStyle WithShadowOffset(float v) const
	{
		IVStyle newStyle      = *this;
		newStyle.shadowOffset = v;
		return newStyle;
	}
	IVStyle WithDrawShadows(bool v = true) const
	{
		IVStyle newStyle     = *this;
		newStyle.drawShadows = v;
		return newStyle;
	}
	IVStyle WithDrawFrame(bool v = true) const
	{
		IVStyle newStyle   = *this;
		newStyle.drawFrame = v;
		return newStyle;
	}
	IVStyle WithWidgetFrac(float v) const
	{
		IVStyle newStyle    = *this;
		newStyle.widgetFrac = math::Clamp(v, 0.f, 1.f);
		return newStyle;
	}
	IVStyle WithAngle(float v) const
	{
		IVStyle newStyle = *this;
		newStyle.angle   = math::Clamp(v, 0.f, 360.f);
		return newStyle;
	}
	IVStyle WithEmboss(bool v = true) const
	{
		IVStyle newStyle = *this;
		newStyle.emboss  = v;
		return newStyle;
	}
};

const IVStyle DEFAULT_STYLE = IVStyle();

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

/**@}*/
