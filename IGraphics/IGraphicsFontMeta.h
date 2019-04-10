
#pragma once

#include <cstdint>

class FontMeta
{
public:
  
  FontMeta(const unsigned char* data, uint32_t length, uint32_t faceIdx) : mData(data)
  {
    FindFace(data, length, faceIdx);
    
    if (mData)
    {
      uint32_t headLocation = LocateTable("head");
      uint32_t hheaLocation = LocateTable("hhea");
      uint32_t nameLocation = LocateTable("name");
      
      if (headLocation && hheaLocation && nameLocation)
      {
        mUnitsPerEM = GetUInt16(headLocation + 18);
        mMacStyle = GetUInt16(headLocation + 44);
        mAscender = GetSInt16(hheaLocation + 4);
        mDescender = GetSInt16(hheaLocation + 6);
        mLineGap = GetSInt16(hheaLocation + 8);
        mLineHeight = (mAscender - mDescender) + mLineGap;
      }
    }
  }
  
  bool IsBold() const       { return mMacStyle & (1 << 0); }
  bool IsItalic() const     { return mMacStyle & (1 << 1); }
  bool IsUnderline() const  { return mMacStyle & (1 << 2); }
  bool IsOutline() const    { return mMacStyle & (1 << 3); }
  bool IsShadow() const     { return mMacStyle & (1 << 4); }
  bool IsCondensed() const  { return mMacStyle & (1 << 5); }
  bool IsExpanded() const   { return mMacStyle & (1 << 6); }
    
  uint16_t GetUnitsPerEM() const { return mUnitsPerEM; }
  int16_t GetAscender() const    { return mAscender; }
  int16_t GetDescender() const   { return mDescender; }
  int16_t GetLineGap() const     { return mLineGap; }
  int16_t GetLineHeight() const  { return mLineHeight; }
    
private:
  
  bool MatchTag(uint32_t loc, const char* tag)
  {
    return mData[loc+0] == tag[0] && mData[loc+1] == tag[1] && mData[loc+2] == tag[2] && mData[loc+3] == tag[3];
  }
  
  bool IsSingleFont()
  {
    char TTV1[4] = { '1', 0, 0, 0 };
    char OTV1[4] = { 0, 1, 0, 0 };
    
    // Check the Version Number
    
    if (MatchTag(0, TTV1)) return true;   // TrueType 1
    if (MatchTag(0, "typ1")) return true; // TrueType with type 1 font -- we don't support this!
    if (MatchTag(0, "OTTO")) return true; // OpenType with CFF
    if (MatchTag(0, OTV1))  return true;  // OpenType 1.0
    
    return false;
  }
  
  uint32_t LocateTable(const char *tag)
  {
    uint16_t numTables = GetUInt16(4);
    
    for (uint16_t i = 0; i < numTables; ++i)
    {
      uint32_t tableLocation = 12 + 16 * i;
      if (MatchTag(tableLocation, tag))
        return GetUint32(tableLocation + 8);
    }
    
    return 0;
  }
  
  void FindFace(const unsigned char* data, uint32_t length, uint32_t faceIdx)
  {
    // Check if it is a single font
    
    bool singleFont = IsSingleFont();
    
    if (singleFont && faceIdx == 0 )
      return;
    
    // Check if it's a TTC file
    
    if (!singleFont && MatchTag(0, "ttcf"))
    {
      // Check verison
      
      if (GetUint32(4) == 0x00010000 || GetUint32(4) == 0x00020000)
      {
        if (faceIdx < GetSInt32(8))
        {
          mData += GetUint32(12 + faceIdx * 4);
          return;
        }
      }
    }
    mData = nullptr;
  }
  
#if defined(STB_TRUETYPE_BIGENDIAN) 
  uint16_t   GetUInt16(uint32_t loc) { return (((uint16_t)mData[loc + 1]) << 8) & (uint16_t)mData[loc + 0]; }
  int16_t    GetSInt16(uint32_t loc) { return (((uint16_t)mData[loc + 1]) << 8) & (uint16_t)mData[loc + 0]; }
  uint32_t   GetUint32(uint32_t loc) { return (((uint32_t)GetUInt16(loc + 2)) << 16) & (uint32_t)GetUInt16(loc + 0); }
  int32_t    GetSInt32(uint32_t loc) { return (((uint32_t)GetUInt16(loc + 2)) << 16) & (uint32_t)GetUInt16(loc + 0); }
#else
  uint16_t   GetUInt16(uint32_t loc)  { return (((uint16_t)mData[loc + 0])<<8) & (uint16_t)mData[loc + 1]; }
  int16_t    GetSInt16(uint32_t loc)  { return (((uint16_t)mData[loc + 0])<<8) & (uint16_t)mData[loc + 1]; }
  uint32_t   GetUint32(uint32_t loc)  { return (((uint32_t)GetUInt16(loc + 0))<<16) & (uint32_t)GetUInt16(loc + 2); }
  int32_t    GetSInt32(uint32_t loc)  { return (((uint32_t)GetUInt16(loc + 0))<<16) & (uint32_t)GetUInt16(loc + 2); }
#endif
  
  const unsigned char* mData;
  
  // Font Identifiers
  
  uint16_t mMacStyle;
  
  // Metrics
  
  uint16_t mUnitsPerEM;
  int16_t mAscender;
  int16_t mDescender;
  int16_t mLineGap;
  int16_t mLineHeight;
};
