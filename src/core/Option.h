//---------------------------------------------------------------------------
#ifndef OptionH
#define OptionH

#include <vector>
//---------------------------------------------------------------------------
enum TOptionType { otParam, otSwitch };
//---------------------------------------------------------------------------
class TOptions
{
public:
  TOptions();
  void ParseParams(const UnicodeString & Params);

  bool FindSwitch(const UnicodeString & Switch);
  bool FindSwitch(const UnicodeString & Switch, UnicodeString & Value);
  bool FindSwitch(const UnicodeString & Switch, int & ParamsStart,
    int & ParamsCount);
  bool FindSwitch(const UnicodeString & Switch, TStrings * Params,
    int ParamsMax = -1);
  void ParamsProcessed(int Position, int Count);
  UnicodeString SwitchValue(const UnicodeString & Switch, const UnicodeString & Default = L"");
  bool SwitchValue(const UnicodeString & Switch, bool Default);
  bool SwitchValue(const UnicodeString & Switch, bool Default, bool DefaultOnNonExistence);
  bool UnusedSwitch(UnicodeString & Switch) const;

  intptr_t GetParamCount() const { return FParamCount; }
  UnicodeString GetParam(intptr_t Index);
  void Clear() { FOptions.resize(0); FNoMoreSwitches = false; FParamCount = 0; }
  bool GetEmpty() const;
  UnicodeString GetSwitchMarks() const { return FSwitchMarks; }

protected:
  UnicodeString FSwitchMarks;
  UnicodeString FSwitchValueDelimiters;

  void Add(const UnicodeString & Option);

  bool FindSwitch(const UnicodeString & Switch,
    UnicodeString & Value, int & ParamsStart, int & ParamsCount);

private:
  struct TOption
  {
    TOptionType Type;
    UnicodeString Name;
    UnicodeString Value;
    bool Used;
  };

  std::vector<TOption> FOptions;
  bool FNoMoreSwitches;
  intptr_t FParamCount;
};
//---------------------------------------------------------------------------
#endif
