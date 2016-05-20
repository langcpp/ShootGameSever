#pragma once

#include <wtypes.h>
#include <atltypes.h>
#include <atlstr.h>
#include <string>

inline INT32 GetRandomNum(INT32 iValue)
{
	if (iValue > 0x7FFF)
	{
		return ((rand()<<15) | rand()) % iValue;
	}
	else
	{
		return rand() % iValue;
	}
}

inline INT64 GetRandomNum(INT64 lValue)
{
	return ((((((rand()<<15) | rand())<<15) | rand())<<15) | rand()) % lValue;
}

inline FLOAT GetRandomNum(FLOAT fValue)
{
	return (fValue * FLOAT(rand() % (0x7FFF + 1)) / 0x7FFF);
}

inline INT32 GetRandomNum(INT32 iMin, INT32 iMax)
{
	if (iMin > iMax)
	{
		std::swap(iMin, iMax);
	}
	return iMin + GetRandomNum(iMax - iMin + 1);
}

inline FLOAT GetRandomNum(FLOAT fMin, FLOAT fMax)
{
	if (fMin > fMax)
	{
		std::swap(fMin, fMax);
	}
	return fMin + GetRandomNum(fMax - fMin);
}

inline INT64 GetRandomNum(INT64 lMin, INT64 lMax)
{
	if (lMin > lMax)
	{
		std::swap(lMin, lMax);
	}
	return lMin + GetRandomNum(lMax - lMin + 1);
}

inline BOOLEAN CheckProbability(FLOAT fProbability)
{
	return GetRandomNum(1.f) <= fProbability;
}

#ifndef _countof
#define _countof(x) (sizeof(x) / sizeof(x[0]))
#endif

#if _MSC_VER >= 1500
#define SPRINTF sprintf_s
#else
#define SPRINTF sprintf
#endif

class String_cl : public CStringA
{
public:
    String_cl()
    {
    }
    template<INT32 iMaxCount> String_cl(IN const CHAR (&acString)[iMaxCount])
        : CStringA(acString)
    {
    }
    String_cl(IN const CHAR* pcString)
        : CStringA(pcString)
    {
    }
    String_cl(INT32 iData)
    {
        CHAR acInfo[1000] = {0};
        SPRINTF(acInfo, "%d", iData);
        CStringA::operator=(CStringA(acInfo));
	}
	String_cl(FLOAT fData)
	{
		CHAR acInfo[1000] = {0};
		SPRINTF(acInfo, "%.4f", fData);
		CStringA::operator=(CStringA(acInfo));
	}
	String_cl(DWORD dwData)
	{
		CHAR acInfo[1000] = {0};
		SPRINTF(acInfo, "%u", dwData);
		CStringA::operator=(CStringA(acInfo));
	}
	String_cl(INT64 lData)
	{
		CHAR acInfo[1000] = {0};
		SPRINTF(acInfo, "%I64d", lData);
		CStringA::operator=(CStringA(acInfo));
	}
    String_cl(const std::string& clString)
    {
        CStringA::operator=(clString.c_str());
    }
    String_cl(const CStringA& clString)
    {
        CStringA::operator=(clString);
    }
    String_cl& operator<<(const String_cl& clString)
    {
        Append(clString);
        return *this;
    }
    String_cl& operator<<(INT32 iData)
    {
        CHAR acInfo[1000] = {0};
        SPRINTF(acInfo, "%d", iData);
        Append(CStringA(acInfo));
        return *this;
	}
	String_cl& operator<<(FLOAT fData)
	{
		CHAR acInfo[1000] = {0};
		SPRINTF(acInfo, "%.4f", fData);
		Append(CStringA(acInfo));
		return *this;
	}
	String_cl& operator<<(UINT32 uiData)
	{
		CHAR acInfo[1000] = {0};
		SPRINTF(acInfo, "%u", uiData);
		Append(CStringA(acInfo));
		return *this;
	}
	String_cl& operator<<(DWORD dwData)
	{
		CHAR acInfo[1000] = {0};
		SPRINTF(acInfo, "%u", dwData);
		Append(CStringA(acInfo));
		return *this;
	}
	String_cl& operator<<(INT64 lData)
	{
		CHAR acInfo[1000] = {0};
		SPRINTF(acInfo, "%I64d", lData);
		Append(CStringA(acInfo));
		return *this;
	}
    template<INT32 iMaxCount> inline String_cl& operator<<(IN const CHAR (&acString)[iMaxCount])
    {
        Append(CStringA(acString));
        return *this;
    }
    BOOLEAN IsNull() const
    {
        return GetLength() == 0;
    }
    const CHAR* ToChar() const
    {
        return (LPCTSTR)(*this);
    }
    INT32 GetSize() const
    {
        return GetLength();
    }
    INT32 GetCount() const
    {
        return GetLength();
    }
    void Copy(const CHAR* pData, INT32 iCount)
    {
        CStringA::SetString(pData, iCount);
    }
    BOOLEAN GetInt(INT32& iValue) const
    {
        iValue = atoi(ToChar());
        return TRUE;
    }
};

namespace Transform_ns //数据转换
{
/*----------------------------------------------------------------------------*/
/*                              Data Structures                               */
/*----------------------------------------------------------------------------*/
    inline String_cl UTF8_To_GBK(const String_cl& clString)
    {
        return String_cl(CW2A(CA2W(clString.ToChar(), CP_UTF8), CP_ACP));
    }
    inline String_cl GBK_To_UTF8(const String_cl& clString)
    {
        return String_cl(CW2A(CA2W(clString.ToChar(), CP_ACP), CP_UTF8));
    }
    template<INT32 iCount> inline void CopyCharArray(CHAR (&acArray)[iCount], const String_cl& clString)
    {
        memset(acArray, 0, iCount);
        memcpy(acArray, clString.ToChar(), min(iCount - 1, clString.GetSize()));
    }
}

namespace SystemInfo_ns //系统信息
{
    inline String_cl GetCurrentDirectory()
    {
        CHAR acDirectory[1024] = {0};
		::GetCurrentDirectory(sizeof(acDirectory), acDirectory);
        return String_cl(acDirectory);
	}
	inline String_cl GetApplicationDirectory()
	{
		CHAR acDirectory[1024] = {0};
		GetModuleFileName(NULL, acDirectory, sizeof(acDirectory));
		String_cl clInfo = String_cl(acDirectory);
		return clInfo.Mid(max(clInfo.ReverseFind('\\'), clInfo.ReverseFind('/')));
	}
}
