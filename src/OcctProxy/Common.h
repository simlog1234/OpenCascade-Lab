// Common.h - helpers shared by the C++/CLI proxy classes.
#pragma once

#include <Standard_Failure.hxx>
#include <Standard_Version.hxx>
#include <TCollection_AsciiString.hxx>
#include <TCollection_ExtendedString.hxx>
#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Message_PrinterOStream.hxx>

#include <vcclr.h>
#include <exception>

namespace OcctProxy
{
  /// Exception thrown to .NET callers whenever an OCCT algorithm fails.
  public ref class OcctException : public System::Exception
  {
  public:
    OcctException(System::String^ message) : System::Exception(message) {}
  };

  /// Static information about / global settings of the native OCCT library.
  public ref class OcctInfo abstract sealed
  {
  public:
    /// OCCT version string compiled into the proxy, e.g. "8.0.1".
    static property System::String^ Version
    {
      System::String^ get() { return gcnew System::String(OCC_VERSION_COMPLETE); }
    }

    /// Enables / disables OCCT's own console output (e.g. STEP transfer statistics on stdout).
    static void SetConsoleOutput(bool enabled)
    {
      const Handle(Message_Messenger)& aMessenger = Message::DefaultMessenger();
      aMessenger->RemovePrinters(STANDARD_TYPE(Message_PrinterOStream));
      if (enabled)
      {
        aMessenger->AddPrinter(new Message_PrinterOStream());
      }
    }
  };

  // ---- string conversion ---------------------------------------------------

  /// System.String (UTF-16) -> TCollection_AsciiString (UTF-8).
  inline TCollection_AsciiString ToAscii(System::String^ theString)
  {
    if (theString == nullptr || theString->Length == 0)
    {
      return TCollection_AsciiString();
    }
    pin_ptr<const wchar_t> aPinned = PtrToStringChars(theString);
    return TCollection_AsciiString(static_cast<const wchar_t*>(aPinned));
  }

  /// TCollection_AsciiString (UTF-8) -> System.String.
  inline System::String^ ToManaged(const TCollection_AsciiString& theString)
  {
    TCollection_ExtendedString anExt(theString, true); // UTF-8 -> UTF-16
    return gcnew System::String(anExt.ToWideString());
  }

  inline System::String^ ToManaged(const char* theString)
  {
    return ToManaged(TCollection_AsciiString(theString));
  }
}

// Wrap OCCT calls: native exceptions become managed OcctException.
#define OCCT_TRY try {
#define OCCT_CATCH                                                                   \
  }                                                                                  \
  catch (const Standard_Failure& aFailure)                                           \
  {                                                                                  \
    const char* aMsg = aFailure.what();                                              \
    throw gcnew OcctProxy::OcctException(                                            \
      gcnew System::String(aMsg != nullptr && *aMsg != '\0' ? aMsg : "OCCT failure")); \
  }                                                                                  \
  catch (const std::exception& anStd)                                                \
  {                                                                                  \
    throw gcnew OcctProxy::OcctException(gcnew System::String(anStd.what()));        \
  }
