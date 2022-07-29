/*
This file is part of NppExec
Copyright (C) 2016 DV <dvv81 (at) ukr (dot) net>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _npp_exec_helpers_h_
#define _npp_exec_helpers_h_
//--------------------------------------------------------------------

#include "base.h"
#include "cpp/CStrT.h"
#include "cpp/CListT.h"
#include <cstddef>
#include <iterator>
#include <algorithm>
#include <functional>

typedef CStrT<TCHAR> tstr;

class CCriticalSection // <-- inspired by CCriticalSection from Win32++ (C) David Nash
{
public:
    CCriticalSection()
    {
        ::InitializeCriticalSection(&m_cs);
        m_isValid = true;
    }

    CCriticalSection(CCriticalSection&& other)
    {
        other.m_isValid = false;
        m_cs = other.m_cs;
        m_isValid = true;
    }

    ~CCriticalSection()
    {
        if ( m_isValid )
            ::DeleteCriticalSection(&m_cs);
    }

    CCriticalSection& operator=(CCriticalSection&& other)
    {
        other.m_isValid = false;
        m_cs = other.m_cs;
        m_isValid = true;
        return *this;
    }

    CCriticalSection(const CCriticalSection&) = delete;
    CCriticalSection& operator=(const CCriticalSection&) = delete;

    void Lock()
    {
        if ( m_isValid )
            ::EnterCriticalSection(&m_cs);
    }

    void Unlock()
    {
        if ( m_isValid )
            ::LeaveCriticalSection(&m_cs);
    }

private:
    CRITICAL_SECTION m_cs;
    bool m_isValid;
};

class CCriticalSectionLockGuard // <-- inspired by std::lock_guard
{
public:
    CCriticalSectionLockGuard(CCriticalSection& lock) : m_lock(lock)
    {
        m_lock.Lock();
    }

    ~CCriticalSectionLockGuard()
    {
        m_lock.Unlock();
    }

    CCriticalSectionLockGuard(const CCriticalSectionLockGuard&) = delete;
    CCriticalSectionLockGuard& operator=(const CCriticalSectionLockGuard&) = delete;

private:
    CCriticalSection& m_lock;
};

class CEvent // <-- inspired by common sense :)
{
public:
    CEvent() : m_hEvent(NULL)
    {
    }

    ~CEvent()
    {
        Destroy();
    }

    CEvent(const CEvent&) = delete;
    CEvent& operator=(const CEvent&) = delete;

    bool IsNull() const
    {
        return (m_hEvent == NULL);
    }

    HANDLE GetHandle() const
    {
        return m_hEvent;
    }

    HANDLE Create(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName)
    {
        Destroy();
        m_hEvent = ::CreateEvent(lpEventAttributes, bManualReset, bInitialState, lpName);
        return m_hEvent;
    }

    BOOL Destroy()
    {
        return ( (m_hEvent != NULL) ? ::CloseHandle(m_hEvent) : FALSE );
    }

    BOOL Set()
    {
        return ( (m_hEvent != NULL) ? ::SetEvent(m_hEvent) : FALSE );
    }

    BOOL Reset()
    {
        return ( (m_hEvent != NULL) ? ::ResetEvent(m_hEvent) : FALSE );
    }

    DWORD Wait(DWORD dwMilliseconds) const
    {
        return ( (m_hEvent != NULL) ? ::WaitForSingleObject(m_hEvent, dwMilliseconds) : WAIT_FAILED );
    }

private:
    HANDLE m_hEvent;
};


// This is an interesting concept, so I better keep it here
// even though it is not really used :)
// For the ConcurrentProxy to be really useful ouside of the
// CConcurrentObjectT, it most likely need to use shared_ptr
// at least for the m_lock, but it looks like too much overhead
// comparing to a simple pair of (object + lock).
/*
template<class ObjectType> class CConcurrentObjectT
{
public:
    class ConcurrentProxy
    {
    public:
        ConcurrentProxy(ObjectType* obj, CCriticalSection* lock) : m_obj(obj), m_lock(lock)
        {
            m_lock->Lock();
        }

        ~ConcurrentProxy()
        {
            m_lock->Unlock();
        }

        ConcurrentProxy(const ConcurrentProxy&) = delete;
        ConcurrentProxy& operator=(const ConcurrentProxy&) = delete;

        ObjectType* operator->()
        {
            return m_obj;
        }

        const ObjectType* operator->() const
        {
            return m_obj;
        }

        ObjectType& Object()
        {
            return *m_obj;
        }

        const ObjectType& Object() const
        {
            return *m_obj;
        }

    private:
        ObjectType* m_obj;
        CCriticalSection* m_lock;
    };

    class ConstConcurrentProxy
    {
    public:
        explicit ConstConcurrentProxy(const ObjectType* obj, CCriticalSection* lock) : m_obj(obj), m_lock(lock)
        {
            m_lock->Lock();
        }

        ~ConstConcurrentProxy()
        {
            m_lock->Unlock();
        }

        ConstConcurrentProxy(const ConstConcurrentProxy&) = delete;
        ConstConcurrentProxy& operator=(const ConstConcurrentProxy&) = delete;

        const ObjectType* operator->() const
        {
            return m_obj;
        }

        const ObjectType& Object() const
        {
            return *m_obj;
        }
    
    private:
        const ObjectType* m_obj;
        CCriticalSection* m_lock;
    };

public:
    CConcurrentObjectT() : m_obj(ObjectType())
    {
    }

    CConcurrentObjectT(const ObjectType& obj) : m_obj(obj)
    {
    }

    ConcurrentProxy Concurrent()
    {
        return ConcurrentProxy(&m_obj, &m_lock); // GCC 4.8.1 _insists_ on copy-ctor here due to its RVO implementation
    }

    ConstConcurrentProxy Concurrent() const
    {
        return ConstConcurrentProxy(&m_obj, &m_lock); // GCC 4.8.1 _insists_ on copy-ctor here due to its RVO implementation
    }

    const ObjectType& Get() const
    {
        return m_obj;
    }

    ObjectType& Get()
    {
        return m_obj;
    }

protected:
    mutable CCriticalSection m_lock;
    ObjectType m_obj;
};
*/

// another interesting concept, this time inspired by boost::variant
// but we don't need it since std::variant and std::any
/*
class CValue
{
public:
    enum eValueType {
        vtNone = 0,
        vtInt,
        vtUint,
        vtLong,
        vtUlong,
        vtLongLong,
        vtUlongLong,
        vtBool,
        vtFloat,
        vtDouble,
        vtPtr,
        vtStr
    };

public:
    CValue();
    explicit CValue(int i);
    explicit CValue(unsigned int u);
    explicit CValue(long l);
    explicit CValue(unsigned long ul);
    explicit CValue(long long ll);
    explicit CValue(unsigned long long ull);
    explicit CValue(bool b);
    explicit CValue(float f);
    explicit CValue(double d);
    explicit CValue(std::nullptr_t);
    explicit CValue(const void* p); // just a pointer
    explicit CValue(const TCHAR* s); // copies a string
    CValue(const TCHAR* s, size_t len); // copies a string
    CValue(const CValue& v);
    CValue(CValue&& v);
    ~CValue();

    CValue& operator=(const CValue& v);
    CValue& operator=(CValue&& v);

    eValueType GetType() const;

    int GetInt() const;
    unsigned int GetUint() const;
    long GetLong() const;
    unsigned long GetUlong() const;
    long long GetLongLong() const;
    unsigned long long GetUlongLong() const;
    bool GetBool() const;
    float GetFloat() const;
    double GetDouble() const;
    const void* GetPtr() const;
    const TCHAR* GetStr(size_t* len = nullptr) const;

    template<typename T> T GetValue() const;

    void Swap(CValue& v);

protected:
    struct sStr {
        TCHAR* str;
        size_t len;
    };

protected:
    template<typename N> N getNumber() const;

    void setStrData(const TCHAR* s, size_t len);
    void setData(const CValue& v);
    void clearData();

protected:
    eValueType m_type;
    union uData {
        int i;
        unsigned int u;
        long l;
        unsigned long ul;
        long long ll;
        unsigned long long ull;
        float f;
        double d;
        const void* p;
        sStr s;
    } m_data;
};
*/

namespace NppExecHelpers
{
    bool CreateNewThread(LPTHREAD_START_ROUTINE lpFunc, LPVOID lpParam, HANDLE* lphThread = NULL);

    HWND GetFocusedWnd();

    tstr GetClipboardText(HWND hWndOwner = NULL);
    bool SetClipboardText(const tstr& text, HWND hWndOwner = NULL);

    tstr GetEnvironmentVar(const TCHAR* szVarName);
    tstr GetEnvironmentVar(const tstr& sVarName);

    tstr GetCurrentDir();

    bool IsFullPath(const tstr& path);
    bool IsFullPath(const TCHAR* path);

    tstr NormalizePath(const tstr& path);
    tstr NormalizePath(const TCHAR* path);

    enum eFileNamePart {
        fnpDrive,   // "C" in "C:\User\Docs\name.ext"
        fnpDirPath, // "C:\User\Docs\" in "C:\User\Docs\name.ext"
        fnpNameExt, // "name.ext" in "C:\User\Docs\name.ext"
        fnpName,    // "name" in "C:\User\Docs\name.ext"
        fnpExt      // ".ext" in "C:\User\Docs\name.ext"
    };
    tstr GetFileNamePart(const tstr& path, eFileNamePart whichPart);
    tstr GetFileNamePart(const TCHAR* path, eFileNamePart whichPart);

    bool CreateDirectoryTree(const tstr& dir);
    bool CreateDirectoryTree(const TCHAR* dir);

    bool CheckDirectoryExists(const tstr& dir);
    bool CheckDirectoryExists(const TCHAR* dir);

    bool CheckFileExists(const tstr& filename);
    bool CheckFileExists(const TCHAR* filename);

    bool IsValidTextFile(const tstr& filename);
    bool IsValidTextFile(const TCHAR* filename);

    bool GetFileWriteTime(const TCHAR* filename, FILETIME* pLastWriteTime);
    bool SetFileWriteTime(const TCHAR* filename, const FILETIME* pLastWriteTime);

    tstr GetInstanceAsString(const void* pInstance);

    void StrLower(tstr& S); // converts to lower case
    void StrLower(TCHAR* S); // converts to lower case
    void StrUpper(tstr& S); // converts to upper case
    void StrUpper(TCHAR* S); // converts to upper case

    TCHAR LatinCharUpper(TCHAR ch); // converts [a-z] to [A-Z]
    TCHAR LatinCharLower(TCHAR ch); // converts [A-Z] to [a-z]

    void StrQuote(tstr& S); // adds the starting and trailing ""
    void StrUnquote(tstr& S); // removes the starting and trailing ""
    void StrUnquoteEx(tstr& S); // removes the starting and trailing "" or '' or ``

    bool IsStrQuoted(const tstr& S); // starts & ends with ""
    bool IsStrQuotedEx(const tstr& S); // starts & ends with "" or '' or ``
    bool IsStrNotQuoted(const tstr& S); // no " at start & at end
    bool IsStrNotQuotedEx(const tstr& S); // no " or ' or ` at start & at end

    void StrEscape(tstr& S); // '\' -> '\\', '<TAB>' -> '\t', '<CR>' -> '\r', '<LF>' -> '\n', '"' -> '\"'
    void StrUnescape(tstr& S); // '\\' -> '\', '\t' -> '<TAB>', '\r' -> '<CR>', '\n' -> '<LF>', '\?' -> '?'

    int StrCmpNoCase(const tstr& S1, const tstr& S2); // comparing case-insensitively
    int StrCmpNoCase(const tstr& S1, const TCHAR* S2); // comparing case-insensitively
    int StrCmpNoCase(const TCHAR* S1, const tstr& S2); // comparing case-insensitively
    int StrCmpNoCase(const TCHAR* S1, const TCHAR* S2); // comparing case-insensitively

    inline bool IsTabSpaceChar(char ch)
    {
        return ((ch == ' ') || (ch == '\t'));
    }
    inline bool IsTabSpaceChar(wchar_t ch)
    {
        return ((ch == L' ') || (ch == L'\t'));
    }
    inline bool IsAnySpaceChar(char ch)
    {
        switch ( ch )
        {
            case ' ':   // 0x20, space
            case '\t':  // 0x09, tabulation
            //case '\n':  // 0x0A, line feed
            case '\v':  // 0x0B, line tabulation
            case '\f':  // 0x0C, form feed
            //case '\r':  // 0x0D, carriage return
                return true;
        }
        return false;
    }
    inline bool IsAnySpaceChar(wchar_t ch)
    {
        switch ( ch )
        {
            case L' ':   // 0x20, space
            case L'\t':  // 0x09, tabulation
            //case L'\n':  // 0x0A, line feed
            case L'\v':  // 0x0B, line tabulation
            case L'\f':  // 0x0C, form feed
            //case L'\r':  // 0x0D, carriage return
                return true;
        }
        return false;
    }

    void StrDelLeadingTabSpaces(CStrT<char>& S);
    void StrDelLeadingTabSpaces(CStrT<wchar_t>& S);
    void StrDelTrailingTabSpaces(CStrT<char>& S);
    void StrDelTrailingTabSpaces(CStrT<wchar_t>& S);
    void StrDelLeadingAnySpaces(CStrT<char>& S);
    void StrDelLeadingAnySpaces(CStrT<wchar_t>& S);
    void StrDelTrailingAnySpaces(CStrT<char>& S);
    void StrDelTrailingAnySpaces(CStrT<wchar_t>& S);

    CWStr CStrToWStr(const CStr& S, UINT aCodePage = CP_ACP);
    CStr  WStrToCStr(const CWStr& S, UINT aCodePage = CP_ACP);
    tstr  CStrToTStr(const CStr& S, UINT aCodePage = CP_ACP);
    CStr  TStrToCStr(const tstr& S, UINT aCodePage = CP_ACP);
}

namespace std_helpers
{
    template<class _RevIt> inline typename _RevIt::iterator_type forward_iterator(_RevIt reverse_it)
    {
        // "a reverse_iterator has always an offset of -1 with respect to its base iterator"...
        // why on earth would someone find it logical or intuitive?
        return std::prev(reverse_it.base());
    }

    template<class _Seq, class _Pr> inline void remove_if(_Seq& Sequence, _Pr Pred)
    {
        // it's kind of strange to have a standard algorithm with its name not corresponding to its implementation...
        auto new_end = std::remove_if(Sequence.begin(), Sequence.end(), Pred);
        Sequence.erase(new_end, Sequence.end());
    }

    template<class T, size_t N> inline /*constexpr*/ size_t size(T (&)[N])
    {
        // length of array: e.g. int a[] = {1,2,3}; std_helpers::size(a) returns 3
        return N;
    }
}

//--------------------------------------------------------------------
#endif
