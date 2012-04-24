//---------------------------------------------------------------------------
#pragma once

#include "stdafx.h"
#include "Classes.h"

//---------------------------------------------------------------------------
bool __fastcall ExceptionMessage(const std::exception *E, std::wstring &Message);
std::wstring __fastcall LastSysErrorMessage();
System::TStrings * __fastcall ExceptionToMoreMessages(const std::exception *E);
//---------------------------------------------------------------------------
enum TOnceDoneOperation { odoIdle, odoDisconnect, odoShutDown };
//---------------------------------------------------------------------------
class Exception : public std::exception
{
    typedef std::exception parent;
public:
    explicit Exception(const std::wstring Msg);
    explicit Exception(const Exception &E);
    // explicit Exception(const std::exception *E);
};
//---------------------------------------------------------------------------
class ExtException : public Exception
{
    typedef Exception parent;
public:
    explicit ExtException(const std::wstring Msg);
    explicit ExtException(const std::exception *E);
    // "copy the std::exception", just append message to the end
    explicit ExtException(const std::wstring Msg, const std::exception *E);
    // explicit ExtException(const std::wstring Msg, const std::wstring MoreMessages, const std::wstring HelpKeyword = L"");
    explicit ExtException(const std::wstring Msg, System::TStrings *MoreMessages, bool Own);
    explicit ExtException(const ExtException &) throw();
    ExtException &operator =(const ExtException &) throw();
    virtual ~ExtException(void) throw();

    System::TStrings *GetMoreMessages() const { return FMoreMessages; }
    std::wstring GetHelpKeyword() const { return FHelpKeyword; }
    const std::wstring GetMessage() const { return FMessage; }
    void SetMessage(const std::wstring value) { FMessage = value; }
protected:
    void __fastcall AddMoreMessages(const std::exception *E);

private:
    System::TStrings *FMoreMessages;
    std::wstring FHelpKeyword;
    std::wstring FMessage;
};
//---------------------------------------------------------------------------
#define DERIVE_EXT_EXCEPTION(NAME, BASE) \
  class NAME : public BASE \
  { \
    typedef BASE parent; \
  public: \
    explicit NAME(const std::wstring Msg, const std::exception *E) : parent(Msg, E) {} \
    virtual ~NAME(void) throw() {} \
  };

//---------------------------------------------------------------------------
DERIVE_EXT_EXCEPTION(ESsh, ExtException);
DERIVE_EXT_EXCEPTION(ETerminal, ExtException);
DERIVE_EXT_EXCEPTION(ECommand, ExtException);
DERIVE_EXT_EXCEPTION(EScp, ExtException); // SCP protocol fatal error (non-fatal in application context)
DERIVE_EXT_EXCEPTION(EScpSkipFile, ExtException);
DERIVE_EXT_EXCEPTION(EScpFileSkipped, EScpSkipFile);
//---------------------------------------------------------------------------
/*
class EOSExtException : public ExtException
{
  typedef ExtException parent;
public:
  EOSExtException();
  EOSExtException(const std::wstring Msg);
};
*/
//---------------------------------------------------------------------------
class EFatal : public ExtException
{
    typedef ExtException parent;
public:
    // fatal errors are always copied, new message is only appended
    explicit EFatal(const std::wstring Msg, const std::exception *E);
    bool GetReopenQueried() { return FReopenQueried; }
    void SetReopenQueried(bool value) { FReopenQueried = value; }

private:
    bool FReopenQueried;
};
//---------------------------------------------------------------------------
#define DERIVE_FATAL_EXCEPTION(NAME, BASE) \
  class NAME : public BASE \
  { \
    typedef BASE parent; \
  public: \
    explicit NAME(const std::wstring Msg, const std::exception *E) : parent(Msg, E) {} \
  };
//---------------------------------------------------------------------------
DERIVE_FATAL_EXCEPTION(ESshFatal, EFatal);
//---------------------------------------------------------------------------
// std::exception that closes application, but displayes info message (not error message)
// = close on completion
class ESshTerminate : public EFatal
{
    typedef EFatal parent;
public:
    explicit ESshTerminate(const std::wstring Msg, const std::exception *E, TOnceDoneOperation AOperation) :
        parent(Msg, E),
        Operation(AOperation)
    {}

    TOnceDoneOperation Operation;
};
//---------------------------------------------------------------------------
