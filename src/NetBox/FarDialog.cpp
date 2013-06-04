#include <vcl.h>
#pragma hdrstop

#include <map.h>

#include "FarDialog.h"
#include "Common.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
TRect Rect(int Left, int Top, int Right, int Bottom)
{
  TRect Result = TRect(Left, Top, Right, Bottom);
  return Result;
}

//---------------------------------------------------------------------------
TFarDialog::TFarDialog(TCustomFarPlugin * AFarPlugin) :
  TObject(),
  FFarPlugin(NULL),
  FBounds(-1, -1, 40, 10),
  FFlags(0),
  FHelpTopic(),
  FVisible(false),
  FItems(NULL),
  FContainers(NULL),
  FHandle(0),
  FDefaultButton(NULL),
  FBorderBox(NULL),
  FNextItemPosition(ipNewLine),
  FDefaultGroup(0),
  FTag(0),
  FItemFocused(NULL),
  FOnKey(NULL),
  FDialogItems(NULL),
  FDialogItemsCapacity(0),
  FChangesLocked(0),
  FChangesPending(false),
  FResult(0),
  FNeedsSynchronize(false),
  FSynchronizeMethod(NULL)
{
  assert(AFarPlugin);
  FItems = new TObjectList();
  FContainers = new TObjectList();
  FFarPlugin = AFarPlugin;
  FFlags = 0;
  FHandle = 0;
  FDefaultGroup = 0;
  FDefaultButton = NULL;
  FNextItemPosition = ipNewLine;
  FDialogItems = NULL;
  FDialogItemsCapacity = 0;
  FChangesPending = false;
  FChangesLocked = 0;
  FResult = -1;
  FNeedsSynchronize = false;
  FSynchronizeObjects[0] = INVALID_HANDLE_VALUE;
  FSynchronizeObjects[1] = INVALID_HANDLE_VALUE;

  FBorderBox = new TFarBox(this);
  FBorderBox->SetBounds(TRect(3, 1, -4, -2));
  FBorderBox->SetDouble(true);
}
//---------------------------------------------------------------------------
TFarDialog::~TFarDialog()
{
  for (intptr_t I = 0; I < GetItemCount(); I++)
  {
    GetItem(I)->Detach();
  }
  delete FItems;
  nb_free(FDialogItems);
  FDialogItemsCapacity = 0;
  delete FContainers;
  if (FSynchronizeObjects[0] != INVALID_HANDLE_VALUE)
  {
    CloseHandle(FSynchronizeObjects[0]);
  }
  if (FSynchronizeObjects[1] != INVALID_HANDLE_VALUE)
  {
    CloseHandle(FSynchronizeObjects[1]);
  }
}
//---------------------------------------------------------------------------
void TFarDialog::SetBounds(TRect Value)
{
  if (GetBounds() != Value)
  {
    LockChanges();
    TRY_FINALLY (
    {
      FBounds = Value;
      if (GetHandle())
      {
        COORD Coord;
        Coord.X = static_cast<short int>(GetSize().x);
        Coord.Y = static_cast<short int>(GetSize().y);
        SendMessage(DM_RESIZEDIALOG, 0, reinterpret_cast<void *>(&Coord));
        Coord.X = static_cast<short int>(FBounds.Left);
        Coord.Y = static_cast<short int>(FBounds.Top);
        SendMessage(DM_MOVEDIALOG, (int)true, reinterpret_cast<void *>(&Coord));
      }
      for (intptr_t I = 0; I < GetItemCount(); I++)
      {
        GetItem(I)->DialogResized();
      }
    }
    ,
    {
      UnlockChanges();
    }
    );
  }
}
//---------------------------------------------------------------------------
TRect TFarDialog::GetClientRect() const
{
  TRect R;
  if (FBorderBox)
  {
    R = FBorderBox->GetBounds();
    R.Left += 2;
    R.Right -= 2;
    R.Top++;
    R.Bottom--;
  }
  else
  {
    R.Left = 0;
    R.Top = 0;
    R.Bottom = 0;
    R.Right = 0;
  }
  return R;
}
//---------------------------------------------------------------------------
TPoint TFarDialog::GetClientSize()
{
  TPoint S;
  if (FBorderBox)
  {
    TRect R = FBorderBox->GetActualBounds();
    S.x = R.Width() + 1;
    S.y = R.Height() + 1;
    S.x -= S.x > 4 ? 4 : S.x;
    S.y -= S.y > 2 ? 2 : S.y;
  }
  else
  {
    S = GetSize();
  }
  return S;
}
//---------------------------------------------------------------------------
TPoint TFarDialog::GetMaxSize()
{
  TPoint P = GetFarPlugin()->TerminalInfo();
  P.x -= 2;
  P.y -= 3;
  return P;
}
//---------------------------------------------------------------------------
void TFarDialog::SetHelpTopic(const UnicodeString & Value)
{
  if (FHelpTopic != Value)
  {
    assert(!GetHandle());
    FHelpTopic = Value;
  }
}
//---------------------------------------------------------------------------
void TFarDialog::SetFlags(const FARDIALOGITEMFLAGS Value)
{
  if (GetFlags() != Value)
  {
    assert(!GetHandle());
    FFlags = Value;
  }
}
//---------------------------------------------------------------------------
void TFarDialog::SetCentered(bool Value)
{
  if (GetCentered() != Value)
  {
    assert(!GetHandle());
    TRect B = GetBounds();
    B.Left = Value ? -1 : 0;
    B.Top = Value ? -1 : 0;
    SetBounds(B);
  }
}
//---------------------------------------------------------------------------
bool TFarDialog::GetCentered()
{
  return (GetBounds().Left < 0) && (GetBounds().Top < 0);
}
//---------------------------------------------------------------------------
TPoint TFarDialog::GetSize()
{
  if (GetCentered())
  {
    return TPoint(GetBounds().Right, GetBounds().Bottom);
  }
  else
  {
    return TPoint(GetBounds().Width() + 1, GetBounds().Height() + 1);
  }
}
//---------------------------------------------------------------------------
void TFarDialog::SetSize(TPoint Value)
{
  TRect B = GetBounds();
  if (GetCentered())
  {
    B.Right = Value.x;
    B.Bottom = Value.y;
  }
  else
  {
    B.Right = FBounds.Left + Value.x - 1;
    B.Bottom = FBounds.Top + Value.y - 1;
  }
  SetBounds(B);
}
//---------------------------------------------------------------------------
void TFarDialog::SetWidth(intptr_t Value)
{
  SetSize(TPoint(Value, GetHeight()));
}
//---------------------------------------------------------------------------
intptr_t TFarDialog::GetWidth()
{
  return GetSize().x;
}
//---------------------------------------------------------------------------
void TFarDialog::SetHeight(intptr_t Value)
{
  SetSize(TPoint(GetWidth(), Value));
}
//---------------------------------------------------------------------------
intptr_t TFarDialog::GetHeight()
{
  return GetSize().y;
}
//---------------------------------------------------------------------------
void TFarDialog::SetCaption(const UnicodeString & Value)
{
  if (GetCaption() != Value)
  {
    FBorderBox->SetCaption(Value);
  }
}
//---------------------------------------------------------------------------
UnicodeString TFarDialog::GetCaption()
{
  return FBorderBox->GetCaption();
}
//---------------------------------------------------------------------------
intptr_t TFarDialog::GetItemCount()
{
  return FItems->GetCount();
}
//---------------------------------------------------------------------------
intptr_t TFarDialog::GetItem(TFarDialogItem * Item) const
{
  if (!Item) return -1;
  return Item->GetItem();
}
//---------------------------------------------------------------------------
TFarDialogItem * TFarDialog::GetItem(intptr_t Index)
{
  TFarDialogItem * DialogItem;
  if (GetItemCount())
  {
    assert(Index >= 0 && Index < FItems->GetCount());
    DialogItem = dynamic_cast<TFarDialogItem *>((*GetItems())[Index]);
    assert(DialogItem);
  }
  else
  {
    DialogItem = NULL;
  }
  return DialogItem;
}
//---------------------------------------------------------------------------
void TFarDialog::Add(TFarDialogItem * DialogItem)
{
  TRect R = GetClientRect();
  intptr_t Left, Top;
  GetNextItemPosition(Left, Top);
  R.Left = static_cast<int>(Left);
  R.Top = static_cast<int>(Top);

  if (FDialogItemsCapacity == GetItems()->GetCount())
  {
    int DialogItemsDelta = 10;
    FarDialogItem * NewDialogItems;
    NewDialogItems = static_cast<FarDialogItem *>(
      nb_malloc(sizeof(FarDialogItem) * (GetItems()->GetCount() + DialogItemsDelta)));
    if (FDialogItems)
    {
      memmove(NewDialogItems, FDialogItems, FDialogItemsCapacity * sizeof(FarDialogItem));
      nb_free(FDialogItems);
    }
    memset(NewDialogItems + FDialogItemsCapacity, 0,
      DialogItemsDelta * sizeof(FarDialogItem));
    FDialogItems = NewDialogItems;
    FDialogItemsCapacity += DialogItemsDelta;
  }

  assert(DialogItem);
  DialogItem->SetItem(GetItems()->Add(DialogItem));

  R.Bottom = R.Top;
  DialogItem->SetBounds(R);
  DialogItem->SetGroup(GetDefaultGroup());
}
//---------------------------------------------------------------------------
void TFarDialog::Add(TFarDialogContainer * Container)
{
  FContainers->Add(Container);
}
//---------------------------------------------------------------------------
void TFarDialog::GetNextItemPosition(intptr_t & Left, intptr_t & Top)
{
  TRect R = GetClientRect();
  Left = R.Left;
  Top = R.Top;

  TFarDialogItem * LastItem = GetItem(GetItemCount() - 1);
  LastItem = LastItem == FBorderBox ? NULL : LastItem;

  if (LastItem)
  {
    switch (GetNextItemPosition())
    {
      case ipNewLine:
        Top = LastItem->GetBottom() + 1;
        break;

      case ipBelow:
        Top = LastItem->GetBottom() + 1;
        Left = LastItem->GetLeft();
        break;

      case ipRight:
        Top = LastItem->GetTop();
        Left = LastItem->GetRight() + 3;
        break;
    }
  }
}
//---------------------------------------------------------------------------
intptr_t WINAPI TFarDialog::DialogProcGeneral(HANDLE Handle, intptr_t Msg, intptr_t Param1, void * Param2)
{
  TFarPluginEnvGuard Guard;

  static rde::map<HANDLE, void *> Dialogs;
  TFarDialog * Dialog = NULL;
  LONG_PTR Result = 0;
  if (Msg == DN_INITDIALOG)
  {
    assert(Dialogs.find(Handle) == Dialogs.end());
    Dialogs[Handle] = Param2;
    Dialog = reinterpret_cast<TFarDialog *>(Param2);
    Dialog->FHandle = Handle;
  }
  else
  {
    if (Dialogs.find(Handle) == Dialogs.end())
    {
      // DM_CLOSE is sent after DN_CLOSE, if the dialog was closed programatically
      // by SendMessage(DM_CLOSE, ...)
      assert(Msg == DM_CLOSE);
      Result = static_cast<LONG_PTR>(0);
    }
    else
    {
      Dialog = reinterpret_cast<TFarDialog *>(Dialogs[Handle]);
    }
  }

  if (Dialog != NULL)
  {
    Result = Dialog->DialogProc(Msg, static_cast<intptr_t>(Param1), Param2);
  }

  if ((Msg == DN_CLOSE) && Result)
  {
    if (Dialog != NULL)
    {
        Dialog->FHandle = 0;
    }
    Dialogs.erase(Handle);
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TFarDialog::DialogProc(intptr_t Msg, intptr_t Param1, void * Param2)
{

  intptr_t Result = 0;
  bool Handled = false;

  try
  {
    if (FNeedsSynchronize)
    {
      try
      {
        FNeedsSynchronize = false;
        FSynchronizeMethod();
        ReleaseSemaphore(FSynchronizeObjects[0], 1, NULL);
        BreakSynchronize();
      }
      catch (...)
      {
        assert(false);
      }
    }

    bool Changed = false;

    switch (Msg)
    {
      case DN_BTNCLICK:
      case DN_EDITCHANGE:
      case DN_GOTFOCUS:
      case DN_KILLFOCUS:
      case DN_LISTCHANGE:
        Changed = true;

      // case DN_MOUSECLICK:
      case DN_CTLCOLORDLGITEM:
      case DN_CTLCOLORDLGLIST:
      case DN_DRAWDLGITEM:
      case DN_HOTKEY:
      case DN_CONTROLINPUT:
      // case DN_KEY:
        if (Param1 >= 0)
        {
          TFarDialogItem * I = GetItem(Param1);
          try
          {
            Result = I->ItemProc(Msg, Param2);
          }
          catch (Exception & E)
          {
            Handled = true;
            DEBUG_PRINTF(L"before GetFarPlugin()->HandleException");
            GetFarPlugin()->HandleException(&E);
            Result = I->FailItemProc(Msg, Param2);
          }

          if (!Result && (Msg == DN_CONTROLINPUT))
          {
            INPUT_RECORD *Rec = reinterpret_cast<INPUT_RECORD *>(Param2);
            const KEY_EVENT_RECORD &Event = Rec->Event.KeyEvent;
            Result = Key(I, static_cast<long>(Event.wVirtualKeyCode | (Event.dwControlKeyState << 16)));
          }
          Handled = true;
        }

        // FAR WORKAROUND
        // When pressing Enter FAR forces dialog to close without calling
        // DN_BTNCLICK on default button. This fixes the scenario.
        // (first check if focused dialog item is not another button)
        if (!Result && (Msg == DN_CONTROLINPUT) &&
            (reinterpret_cast<intptr_t>(Param2) == VK_RETURN) &&
            ((Param1 < 0) ||
             ((Param1 >= 0) && (dynamic_cast<TFarButton *>(GetItem(Param1)) == NULL))) &&
            GetDefaultButton()->GetEnabled() &&
            (GetDefaultButton()->GetOnClick()))
        {
          bool Close = (GetDefaultButton()->GetResult() != 0);
          GetDefaultButton()->GetOnClick()(GetDefaultButton(), Close);
          Handled = true;
          if (!Close)
          {
            Result = static_cast<int>(true);
          }
        }
        break;
        case DN_INPUT:
        // case DN_MOUSEEVENT:
        Result = MouseEvent(reinterpret_cast<MOUSE_EVENT_RECORD *>(Param2));
        Handled = true;
        break;
    }
    if (!Handled)
    {
      switch (Msg)
      {
        case DN_INITDIALOG:
          Init();
          Result = static_cast<int>(true);
          break;

        case DN_DRAGGED:
          if (Param1 == 1)
          {
            RefreshBounds();
          }
          break;

        case DN_DRAWDIALOG:
          // before drawing the dialog, make sure we know correct coordinates
          // (especially while the dialog is being dragged)
          RefreshBounds();
          break;

        case DN_CLOSE:
          Result = static_cast<int>(true);
          if (Param1 >= 0)
          {
            TFarButton * Button = dynamic_cast<TFarButton *>(GetItem(Param1));
            // FAR WORKAROUND
            // FAR 1.70 alpha 6 calls DN_CLOSE even for non-button dialog items
            // (list boxes in particular), while FAR 1.70 beta 5 used ID of
            // default button in such case.
            // Particularly for listbox, we can prevent closing dialog using
            // flag DIF_LISTNOCLOSE.
            if (Button == NULL)
            {
              assert(dynamic_cast<TFarListBox *>(GetItem(Param1)) != NULL);
              Result = static_cast<intptr_t>(false);
            }
            else
            {
              FResult = Button->GetResult();
            }
          }
          else
          {
            FResult = -1;
          }
          if (Result)
          {
            Result = CloseQuery();
            if (!Result)
            {
              FResult = -1;
            }
          }
          Handled = true;
          break;

        case DN_ENTERIDLE:
          Idle();
          break;
      }

      if (!Handled)
      {
        Result = DefaultDialogProc(Msg, Param1, Param2);
      }
    }
    if (Changed)
    {
      Change();
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF(L"before GetFarPlugin()->HandleException");
    GetFarPlugin()->HandleException(&E);
    if (!Handled)
    {
      Result = FailDialogProc(Msg, Param1, Param2);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TFarDialog::DefaultDialogProc(intptr_t Msg, intptr_t Param1, void * Param2)
{
  TFarEnvGuard Guard;
  return GetFarPlugin()->GetStartupInfo()->DefDlgProc(GetHandle(), Msg, static_cast<int>(Param1), Param2);
}
//---------------------------------------------------------------------------
intptr_t TFarDialog::FailDialogProc(intptr_t Msg, intptr_t Param1, void * Param2)
{
  intptr_t Result = 0;
  switch (Msg)
  {
    case DN_CLOSE:
      Result = static_cast<int>(false);
      break;

    default:
      Result = DefaultDialogProc(Msg, Param1, Param2);
      break;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TFarDialog::Idle()
{
  // nothing
}
//---------------------------------------------------------------------------
bool TFarDialog::MouseEvent(MOUSE_EVENT_RECORD * Event)
{
  bool Result = true;
  bool Handled = false;
  if (FLAGSET(Event->dwEventFlags, MOUSE_MOVED))
  {
    int X = Event->dwMousePosition.X - GetBounds().Left;
    int Y = Event->dwMousePosition.Y - GetBounds().Top;
    TFarDialogItem * Item = ItemAt(X, Y);
    if (Item != NULL)
    {
      Result = Item->MouseMove(X, Y, Event);
      Handled = true;
    }
  }
  else
  {
    Handled = false;
  }

  if (!Handled)
  {
        INPUT_RECORD Rec = {0};
        Rec.EventType = MOUSE_EVENT;
        memmove(&Rec.Event.MouseEvent, Event, sizeof(*Event));
        Result = DefaultDialogProc(DN_INPUT, 0, static_cast<void *>(&Rec)) != 0;
  }

  return Result;
}
//---------------------------------------------------------------------------
bool TFarDialog::Key(TFarDialogItem * Item, LONG_PTR KeyCode)
{
  bool Result = false;
  if (FOnKey)
  {
    FOnKey(this, Item, static_cast<long>(KeyCode), Result);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TFarDialog::HotKey(uintptr_t Key, uintptr_t ControlState)
{
  bool Result = false;
  char HotKey = 0;
  if ((ControlState & ALTMASK) &&
        ('A' <= Key) && (Key <= 'Z'))
  {
    Result = true;
    HotKey = static_cast<char>('a' + static_cast<char>(Key - 'A'));
  }

  if (Result)
  {
    Result = false;
    for (intptr_t I = 0; I < GetItemCount(); I++)
    {
      if (GetItem(I)->HotKey(HotKey))
      {
        Result = true;
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
TFarDialogItem * TFarDialog::ItemAt(int X, int Y)
{
  TFarDialogItem * Result = NULL;
  for (intptr_t I = 0; I < GetItemCount(); I++)
  {
    TRect Bounds = GetItem(I)->GetActualBounds();
    if ((Bounds.Left <= X) && (X <= Bounds.Right) &&
        (Bounds.Top <= Y) && (Y <= Bounds.Bottom))
    {
      Result = GetItem(I);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TFarDialog::CloseQuery()
{
  bool Result = true;
  for (intptr_t I = 0; I < GetItemCount() && Result; I++)
  {
    if (!GetItem(I)->CloseQuery())
    {
      Result = false;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TFarDialog::RefreshBounds()
{
  SMALL_RECT Rect = {0};
  SendMessage(DM_GETDLGRECT, 0, reinterpret_cast<void *>(&Rect));
  FBounds.Left = Rect.Left;
  FBounds.Top = Rect.Top;
  FBounds.Right = Rect.Right;
  FBounds.Bottom = Rect.Bottom;
}
//---------------------------------------------------------------------------
void TFarDialog::Init()
{
  for (intptr_t I = 0; I < GetItemCount(); I++)
  {
    GetItem(I)->Init();
  }

  RefreshBounds();

  Change();
}
//---------------------------------------------------------------------------
intptr_t TFarDialog::ShowModal()
{
  FResult = -1;

  TFarDialog * PrevTopDialog = GetFarPlugin()->FTopDialog;
  GetFarPlugin()->FTopDialog = this;
  HANDLE Handle = INVALID_HANDLE_VALUE;
  TRY_FINALLY (
  {
    assert(GetDefaultButton());
    assert(GetDefaultButton()->GetDefault());

    UnicodeString AHelpTopic = GetHelpTopic();
    intptr_t BResult = 0;

    {
      TFarEnvGuard Guard;
      TRect Bounds = GetBounds();
      Handle = GetFarPlugin()->GetStartupInfo()->DialogInit(
              &MainGuid, &MainGuid,
              Bounds.Left, Bounds.Top, Bounds.Right, Bounds.Bottom,
              AHelpTopic.c_str(), FDialogItems,
              GetItemCount(), 0, GetFlags(),
              DialogProcGeneral,
              reinterpret_cast<void *>(this));
      BResult = GetFarPlugin()->GetStartupInfo()->DialogRun(Handle);
    }

    if (BResult >= 0)
    {
      TFarButton * Button = dynamic_cast<TFarButton *>(GetItem(BResult));
      assert(Button);
      // correct result should be already set by TFarButton
      assert(FResult == Button->GetResult());
      FResult = Button->GetResult();
    }
    else
    {
      // allow only one negative value = -1
      FResult = -1;
    }
  }
  ,
  {
    GetFarPlugin()->FTopDialog = PrevTopDialog;
    if (Handle != INVALID_HANDLE_VALUE)
      GetFarPlugin()->GetStartupInfo()->DialogFree(Handle);
  }
  );

  return FResult;
}
//---------------------------------------------------------------------------
void TFarDialog::BreakSynchronize()
{
  SetEvent(FSynchronizeObjects[1]);
}
//---------------------------------------------------------------------------
void TFarDialog::Synchronize(TThreadMethod Event)
{
  if (FSynchronizeObjects[0] == INVALID_HANDLE_VALUE)
  {
    FSynchronizeObjects[0] = CreateSemaphore(NULL, 0, 2, NULL);
    FSynchronizeObjects[1] = CreateEvent(NULL, false, false, NULL);
  }
  FSynchronizeMethod = Event;
  FNeedsSynchronize = true;
  WaitForMultipleObjects(LENOF(FSynchronizeObjects),
                         reinterpret_cast<HANDLE *>(&FSynchronizeObjects), false, INFINITE);
}
//---------------------------------------------------------------------------
void TFarDialog::Close(TFarButton * Button)
{
  assert(Button != NULL);
  SendMessage(DM_CLOSE, Button->GetItem(), NULL);
}
//---------------------------------------------------------------------------
void TFarDialog::Change()
{
  if (FChangesLocked > 0)
  {
    FChangesPending = true;
  }
  else
  {
    std::auto_ptr<TList> NotifiedContainers(new TList());
    for (intptr_t I = 0; I < GetItemCount(); I++)
    {
      TFarDialogItem * DItem = GetItem(I);
      DItem->Change();
      if (DItem->GetContainer() && NotifiedContainers->IndexOf(DItem->GetContainer()) == NPOS)
      {
        NotifiedContainers->Add(DItem->GetContainer());
      }
    }

    for (intptr_t Index = 0; Index < NotifiedContainers->GetCount(); ++Index)
    {
      (static_cast<TFarDialogContainer *>((*NotifiedContainers)[Index]))->Change();
    }
  }
}
//---------------------------------------------------------------------------
intptr_t TFarDialog::SendMessage(intptr_t Msg, intptr_t Param1, void * Param2)
{
  assert(GetHandle());
  TFarEnvGuard Guard;
  return GetFarPlugin()->GetStartupInfo()->SendDlgMessage(GetHandle(),
    Msg, static_cast<int>(Param1), Param2);
}
//---------------------------------------------------------------------------
FarColor TFarDialog::GetSystemColor(PaletteColors colorId)
{
    FarColor color = {0};
    if (GetFarPlugin()->FarAdvControl(ACTL_GETCOLOR, colorId, &color) != 0)
    {
        // TODO: throw error
    }
    return color;
}
//---------------------------------------------------------------------------
void TFarDialog::Redraw()
{
  SendMessage(DM_REDRAW, 0, NULL);
}
//---------------------------------------------------------------------------
void TFarDialog::ShowGroup(intptr_t Group, bool Show)
{
  ProcessGroup(Group, MAKE_CALLBACK(TFarDialog::ShowItem, this), &Show);
}
//---------------------------------------------------------------------------
void TFarDialog::EnableGroup(intptr_t Group, bool Enable)
{
  ProcessGroup(Group, MAKE_CALLBACK(TFarDialog::EnableItem, this), &Enable);
}
//---------------------------------------------------------------------------
void TFarDialog::ProcessGroup(intptr_t Group, TFarProcessGroupEvent Callback,
  void * Arg)
{
  LockChanges();
  TRY_FINALLY (
  {
    for (intptr_t I = 0; I < GetItemCount(); I++)
    {
      TFarDialogItem * Item = GetItem(I);
      if (Item->GetGroup() == Group)
      {
        Callback(Item, Arg);
      }
    }
  }
  ,
  {
    UnlockChanges();
  }
  );
}
//---------------------------------------------------------------------------
void TFarDialog::ShowItem(TFarDialogItem * Item, void * Arg)
{
  Item->SetVisible(*static_cast<bool *>(Arg));
}
//---------------------------------------------------------------------------
void TFarDialog::EnableItem(TFarDialogItem * Item, void * Arg)
{
  Item->SetEnabled(*static_cast<bool *>(Arg));
}
//---------------------------------------------------------------------------
void TFarDialog::SetItemFocused(TFarDialogItem * Value)
{
  if (Value != GetItemFocused())
  {
    assert(Value);
    Value->SetFocus();
  }
}
//---------------------------------------------------------------------------
UnicodeString TFarDialog::GetMsg(intptr_t MsgId)
{
  return FFarPlugin->GetMsg(MsgId);
}
//---------------------------------------------------------------------------
void TFarDialog::LockChanges()
{
  assert(FChangesLocked < 10);
  FChangesLocked++;
  if (FChangesLocked == 1)
  {
    assert(!FChangesPending);
    if (GetHandle())
    {
      SendMessage(DM_ENABLEREDRAW, 0, 0);
    }
  }
}
//---------------------------------------------------------------------------
void TFarDialog::UnlockChanges()
{
  assert(FChangesLocked > 0);
  FChangesLocked--;
  if (FChangesLocked == 0)
  {
    TRY_FINALLY (
    {
      if (FChangesPending)
      {
        FChangesPending = false;
        Change();
      }
    }
    ,
    {
      if (GetHandle())
      {
        SendMessage(DM_ENABLEREDRAW, TRUE, 0);
      }
    }
    );
  }
}
//---------------------------------------------------------------------------
bool TFarDialog::ChangesLocked()
{
  return (FChangesLocked > 0);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarDialogContainer::TFarDialogContainer(TFarDialog * ADialog) :
  TObject(),
  FLeft(0),
  FTop(0),
  FItems(NULL),
  FDialog(NULL),
  FEnabled(false)
{
  assert(ADialog);

  FItems = new TObjectList();
  FItems->SetOwnsObjects(false);
  FDialog = ADialog;
  FEnabled = true;

  GetDialog()->Add(this);
  GetDialog()->GetNextItemPosition(FLeft, FTop);
}
//---------------------------------------------------------------------------
TFarDialogContainer::~TFarDialogContainer()
{
  delete FItems;
}
//---------------------------------------------------------------------------
UnicodeString TFarDialogContainer::GetMsg(int MsgId)
{
  return GetDialog()->GetMsg(MsgId);
}
//---------------------------------------------------------------------------
void TFarDialogContainer::Add(TFarDialogItem * Item)
{
  assert(FItems->IndexOf(Item) == NPOS);
  Item->SetContainer(this);
  if (FItems->IndexOf(Item) == NPOS)
    FItems->Add(Item);
}
//---------------------------------------------------------------------------
void TFarDialogContainer::Remove(TFarDialogItem * Item)
{
  assert(FItems->IndexOf(Item) != NPOS);
  Item->SetContainer(NULL);
  FItems->Remove(Item);
  if (FItems->GetCount() == 0)
  {
    delete this;
  }
}
//---------------------------------------------------------------------------
void TFarDialogContainer::SetPosition(intptr_t Index, intptr_t Value)
{
  intptr_t & Position = Index ? FTop : FLeft;
  if (Position != Value)
  {
    Position = Value;
    for (intptr_t Index = 0; Index < GetItemCount(); ++Index)
    {
      dynamic_cast<TFarDialogItem *>((*FItems)[Index])->DialogResized();
    }
  }
}
//---------------------------------------------------------------------------
void TFarDialogContainer::Change()
{
}
//---------------------------------------------------------------------------
void TFarDialogContainer::SetEnabled(bool Value)
{
  if (FEnabled != Value)
  {
    FEnabled = true;
    for (intptr_t Index = 0; Index < GetItemCount(); ++Index)
    {
      dynamic_cast<TFarDialogItem *>((*FItems)[Index])->UpdateEnabled();
    }
  }
}
//---------------------------------------------------------------------------
intptr_t TFarDialogContainer::GetItemCount() const
{
  return FItems->GetCount();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarDialogItem::TFarDialogItem(TFarDialog * ADialog, FARDIALOGITEMTYPES AType) :
  TObject(),
  FDefaultType(AType),
  FGroup(0),
  FTag(0),
  FOnExit(NULL),
  FOnMouseClick(NULL),
  FDialog(ADialog),
  FEnabledFollow(NULL),
  FEnabledDependency(NULL),
  FEnabledDependencyNegative(NULL),
  FContainer(NULL),
  FItem(NPOS),
  FEnabled(true),
  FIsEnabled(true),
  FColors(0),
  FColorMask(0)
{
  assert(ADialog);
  GetDialog()->Add(this);

  GetDialogItem()->Type = AType;
}
//---------------------------------------------------------------------------
TFarDialogItem::~TFarDialogItem()
{
  assert(!GetDialog());
  if (GetDialog())
  {
    nb_free((void*)GetDialogItem()->Data);
  }
}

const FarDialogItem * TFarDialogItem::GetDialogItem() const
{
  return const_cast<TFarDialogItem *>(this)->GetDialogItem();
}
//---------------------------------------------------------------------------
FarDialogItem * TFarDialogItem::GetDialogItem()
{
  assert(GetDialog());
  return &GetDialog()->FDialogItems[GetItem()];
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetBounds(const TRect & Value)
{
  if (FBounds != Value)
  {
    FBounds = Value;
    UpdateBounds();
  }
}
//---------------------------------------------------------------------------
void TFarDialogItem::Detach()
{
  nb_free((void*)GetDialogItem()->Data);
  FDialog = NULL;
}
//---------------------------------------------------------------------------
void TFarDialogItem::DialogResized()
{
  UpdateBounds();
}
//---------------------------------------------------------------------------
void TFarDialogItem::ResetBounds()
{
  TRect B = FBounds;
  FarDialogItem * DItem = GetDialogItem();
  #define BOUND(DIB, BB, DB, CB) DItem->DIB = B.BB >= 0 ? \
    (GetContainer() ? GetContainer()->CB : 0) + B.BB : GetDialog()->GetSize().DB + B.BB
  BOUND(X1, Left, x, GetLeft());
  BOUND(Y1, Top, y, GetTop());
  BOUND(X2, Right, x, GetLeft());
  BOUND(Y2, Bottom, y, GetTop());
  #undef BOUND
}
//---------------------------------------------------------------------------
void TFarDialogItem::UpdateBounds()
{
  ResetBounds();

  if (GetDialog()->GetHandle())
  {
    TRect B = GetActualBounds();
    SMALL_RECT Rect = {0};
    Rect.Left = static_cast<short int>(B.Left);
    Rect.Top = static_cast<short int>(B.Top);
    Rect.Right = static_cast<short int>(B.Right);
    Rect.Bottom = static_cast<short int>(B.Bottom);
    SendMessage(DM_SETITEMPOSITION, reinterpret_cast<void *>(&Rect));
  }
}
//---------------------------------------------------------------------------
char TFarDialogItem::GetColor(intptr_t Index) const
{
  return *((reinterpret_cast<const char *>(&FColors)) + Index);
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetColor(intptr_t Index, char Value)
{
  if (GetColor(Index) != Value)
  {
    *((reinterpret_cast<char *>(&FColors)) + Index) = Value;
    FColorMask |= (0xFF << (Index * 8));
  }
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetFlags(FARDIALOGITEMFLAGS Value)
{
  if (GetFlags() != Value)
  {
    assert(!GetDialog()->GetHandle());
    UpdateFlags(Value);
  }
}
//---------------------------------------------------------------------------
void TFarDialogItem::UpdateFlags(FARDIALOGITEMFLAGS Value)
{
  if (GetFlags() != Value)
  {
    GetDialogItem()->Flags = Value;
    DialogChange();
  }
}
//---------------------------------------------------------------------------
TRect TFarDialogItem::GetActualBounds() const
{
  return TRect(GetDialogItem()->X1, GetDialogItem()->Y1,
               GetDialogItem()->X2, GetDialogItem()->Y2);
}
//---------------------------------------------------------------------------
FARDIALOGITEMFLAGS TFarDialogItem::GetFlags() const
{
  return GetDialogItem()->Flags;
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetDataInternal(const UnicodeString & Value)
{
  UnicodeString FarData = Value.c_str();
  // DEBUG_PRINTF(L"GetDialogItem()->Data = %s", GetDialogItem()->Data);
  if (GetDialog()->GetHandle())
  {
    SendMessage(DM_SETTEXTPTR, static_cast<void *>(const_cast<wchar_t *>(FarData.c_str())));
  }
  nb_free((void*)GetDialogItem()->Data);
  GetDialogItem()->Data = TCustomFarPlugin::DuplicateStr(FarData, true);

  // DEBUG_PRINTF(L"GetDialogItem()->Data = %s", GetDialogItem()->Data);
  DialogChange();
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetData(const UnicodeString & Value)
{
  if (GetData() != Value)
  {
    SetDataInternal(Value);
  }
}
//---------------------------------------------------------------------------
void TFarDialogItem::UpdateData(const UnicodeString & Value)
{
  UnicodeString FarData = Value.c_str();
  nb_free((void*)GetDialogItem()->Data);
  GetDialogItem()->Data = TCustomFarPlugin::DuplicateStr(FarData, true);

}
//---------------------------------------------------------------------------
UnicodeString TFarDialogItem::GetData() const
{
  return const_cast<TFarDialogItem *>(this)->GetData();
}
//---------------------------------------------------------------------------
UnicodeString TFarDialogItem::GetData()
{
  UnicodeString Result;
  if (GetDialogItem()->Data)
  {
    Result = GetDialogItem()->Data;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetType(FARDIALOGITEMTYPES Value)
{
  if (GetType() != Value)
  {
    assert(!GetDialog()->GetHandle());
    GetDialogItem()->Type = Value;
  }
}
//---------------------------------------------------------------------------
FARDIALOGITEMTYPES TFarDialogItem::GetType() const
{
  return static_cast<FARDIALOGITEMTYPES>(GetDialogItem()->Type);
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetAlterType(FARDIALOGITEMTYPES Index, bool Value)
{
  if (GetAlterType(Index) != Value)
  {
    SetType(Value ? Index : FDefaultType);
  }
}
//---------------------------------------------------------------------------
bool TFarDialogItem::GetAlterType(FARDIALOGITEMTYPES Index) const
{
  return const_cast<TFarDialogItem *>(this)->GetAlterType(Index);
}
//---------------------------------------------------------------------------
bool TFarDialogItem::GetAlterType(FARDIALOGITEMTYPES Index)
{
  return (GetType() == Index);
}
//---------------------------------------------------------------------------
bool TFarDialogItem::GetFlag(FARDIALOGITEMFLAGS Index) const
{
  bool Result = (GetFlags() & (Index & 0xFFFFFFFFFFFFFF00ULL)) != 0;
  if (Index & 0x000000FFUL)
  {
    Result = !Result;
  }
    // bool Result = (GetFlags() & Index) != 0;
  return Result;
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetFlag(FARDIALOGITEMFLAGS Index, bool Value)
{
  if (GetFlag(Index) != Value)
  {
    if (Index & DIF_INVERSE)
    {
      Value = !Value;
    }

    FARDIALOGITEMFLAGS F = GetFlags();
    FARDIALOGITEMFLAGS Flag = Index & 0xFFFFFFFFFFFFFF00ULL;
    bool ToHandle = true;

    switch (Flag)
    {
      case DIF_DISABLE:
        if (GetDialog()->GetHandle())
        {
          SendMessage(DM_ENABLE, reinterpret_cast<void *>(!Value));
        }
        break;

      case DIF_HIDDEN:
        if (GetDialog()->GetHandle())
        {
          SendMessage(DM_SHOWITEM, reinterpret_cast<void *>(!Value));
        }
        break;

      case DIF_3STATE:
        if (GetDialog()->GetHandle())
        {
          SendMessage(DM_SET3STATE, reinterpret_cast<void *>(Value));
        }
        break;
    }

    if (ToHandle)
    {
      if (Value)
      {
        F |= Flag;
      }
      else
      {
        F &= ~Flag;
      }
      UpdateFlags(F);
    }
  }
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetEnabledFollow(TFarDialogItem * Value)
{
  if (GetEnabledFollow() != Value)
  {
    FEnabledFollow = Value;
    Change();
  }
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetEnabledDependency(TFarDialogItem * Value)
{
  if (GetEnabledDependency() != Value)
  {
    FEnabledDependency = Value;
    Change();
  }
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetEnabledDependencyNegative(TFarDialogItem * Value)
{
  if (GetEnabledDependencyNegative() != Value)
  {
    FEnabledDependencyNegative = Value;
    Change();
  }
}
//---------------------------------------------------------------------------
bool TFarDialogItem::GetIsEmpty() const
{
  return GetData().IsEmpty();
}
//---------------------------------------------------------------------------
intptr_t TFarDialogItem::FailItemProc(intptr_t Msg, void * Param)
{
  intptr_t Result = 0;
  switch (Msg)
  {
    case DN_KILLFOCUS:
      Result = static_cast<intptr_t>(GetItem());
      break;

    default:
      Result = DefaultItemProc(Msg, Param);
      break;
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TFarDialogItem::ItemProc(intptr_t Msg, void * Param)
{
  intptr_t Result = 0;
  bool Handled = false;

  if (Msg == DN_GOTFOCUS)
  {
    DoFocus();
    UpdateFocused(true);
  }
  else if (Msg == DN_KILLFOCUS)
  {
    DoExit();
    UpdateFocused(false);
  }
  else if (Msg == DN_CONTROLINPUT)
  {
    INPUT_RECORD *Rec = reinterpret_cast<INPUT_RECORD *>(Param);
    MOUSE_EVENT_RECORD *Event = &Rec->Event.MouseEvent;
    if (FLAGCLEAR(Event->dwEventFlags, MOUSE_MOVED))
    {
      Result = MouseClick(Event);
      Handled = true;
    }
  }

  if (!Handled)
  {
    Result = DefaultItemProc(Msg, Param);
  }

  if (Msg == DN_CTLCOLORDLGITEM && FColorMask)
  {
    Result &= ~FColorMask;
    Result |= (FColors & FColorMask);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TFarDialogItem::DoFocus()
{
}
//---------------------------------------------------------------------------
void TFarDialogItem::DoExit()
{
  if (FOnExit)
  {
    FOnExit(this);
  }
}
//---------------------------------------------------------------------------
intptr_t TFarDialogItem::DefaultItemProc(intptr_t Msg, void * Param)
{
  TFarEnvGuard Guard;
  return GetDialog()->GetFarPlugin()->GetStartupInfo()->DefDlgProc(GetDialog()->GetHandle(), Msg, static_cast<int>(GetItem()), Param);
}
//---------------------------------------------------------------------------
intptr_t TFarDialogItem::DefaultDialogProc(intptr_t Msg, intptr_t Param1, void * Param2)
{
  TFarEnvGuard Guard;
  return GetDialog()->GetFarPlugin()->GetStartupInfo()->DefDlgProc(GetDialog()->GetHandle(), Msg, static_cast<int>(Param1), Param2);
}
//---------------------------------------------------------------------------
void TFarDialogItem::Change()
{
  if (GetEnabledFollow() || GetEnabledDependency() || GetEnabledDependencyNegative())
  {
    UpdateEnabled();
  }
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetEnabled(bool Value)
{
  if (GetEnabled() != Value)
  {
    FEnabled = Value;
    UpdateEnabled();
  }
}
//---------------------------------------------------------------------------
void TFarDialogItem::UpdateEnabled()
{
  bool Value =
    GetEnabled() &&
    (!GetEnabledFollow() || GetEnabledFollow()->GetIsEnabled()) &&
    (!GetEnabledDependency() ||
     (!GetEnabledDependency()->GetIsEmpty() && GetEnabledDependency()->GetIsEnabled())) &&
    (!GetEnabledDependencyNegative() ||
     (GetEnabledDependencyNegative()->GetIsEmpty() || !GetEnabledDependencyNegative()->GetIsEnabled())) &&
    (!GetContainer() || GetContainer()->GetEnabled());

  if (Value != GetIsEnabled())
  {
    FIsEnabled = Value;
    SetFlag(DIF_DISABLE | DIF_INVERSE, GetIsEnabled());
  }
}
//---------------------------------------------------------------------------
void TFarDialogItem::DialogChange()
{
  assert(GetDialog());
  GetDialog()->Change();
}
//---------------------------------------------------------------------------
intptr_t TFarDialogItem::SendDialogMessage(intptr_t Msg, intptr_t Param1, void * Param2)
{
  return GetDialog()->SendMessage(Msg, Param1, Param2);
}
//---------------------------------------------------------------------------
intptr_t TFarDialogItem::SendMessage(intptr_t Msg, void * Param)
{
  return GetDialog()->SendMessage(Msg, GetItem(), Param);
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetSelected(intptr_t Value)
{
  if (GetSelected() != Value)
  {
    if (GetDialog()->GetHandle())
    {
      SendMessage(DM_SETCHECK, reinterpret_cast<void *>(Value));
    }
    UpdateSelected(Value);
  }
}
//---------------------------------------------------------------------------
void TFarDialogItem::UpdateSelected(intptr_t Value)
{
  if (GetSelected() != Value)
  {
    GetDialogItem()->Selected = Value;
    DialogChange();
  }
}
//---------------------------------------------------------------------------
intptr_t TFarDialogItem::GetSelected() const
{
  return static_cast<intptr_t>(GetDialogItem()->Selected);
}
//---------------------------------------------------------------------------
bool TFarDialogItem::GetFocused() const
{
  return GetFlag(DIF_FOCUS);
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetFocused(bool Value)
{
  SetFlag(DIF_FOCUS, Value);
}
//---------------------------------------------------------------------------
bool TFarDialogItem::GetChecked() const
{
  return GetSelected() == BSTATE_CHECKED;
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetChecked(bool Value)
{
  SetSelected(Value ? BSTATE_CHECKED : BSTATE_UNCHECKED);
}
//---------------------------------------------------------------------------
void TFarDialogItem::Move(intptr_t DeltaX, intptr_t DeltaY)
{
  TRect R = GetBounds();

  R.Left += static_cast<int>(DeltaX);
  R.Right += static_cast<int>(DeltaX);
  R.Top += static_cast<int>(DeltaY);
  R.Bottom += static_cast<int>(DeltaY);

  SetBounds(R);
}
//---------------------------------------------------------------------------
void TFarDialogItem::MoveAt(intptr_t X, intptr_t Y)
{
  Move(X - GetLeft(), Y - GetTop());
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetCoordinate(intptr_t Index, intptr_t Value)
{
  assert(sizeof(TRect) == sizeof(intptr_t) * 4);
  TRect R = GetBounds();
  intptr_t * D = reinterpret_cast<intptr_t *>(&R);
  D += Index;
  *D = static_cast<int>(Value);
  SetBounds(R);
}
//---------------------------------------------------------------------------
intptr_t TFarDialogItem::GetCoordinate(intptr_t Index) const
{
  assert(sizeof(TRect) == sizeof(intptr_t) * 4);
  TRect R = GetBounds();
  intptr_t * D = reinterpret_cast<intptr_t *>(&R);
  D += Index;
  return static_cast<intptr_t>(*D);
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetWidth(intptr_t Value)
{
  TRect R = GetBounds();
  if (R.Left >= 0)
  {
    R.Right = R.Left + static_cast<int>(Value - 1);
  }
  else
  {
    assert(R.Right < 0);
    R.Left = R.Right - static_cast<int>(Value + 1);
  }
  SetBounds(R);
}
//---------------------------------------------------------------------------
intptr_t TFarDialogItem::GetWidth() const
{
  return static_cast<intptr_t>(GetActualBounds().Width() + 1);
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetHeight(intptr_t Value)
{
  TRect R = GetBounds();
  if (R.Top >= 0)
  {
    R.Bottom = static_cast<int>(R.Top + Value - 1);
  }
  else
  {
    assert(R.Bottom < 0);
    R.Top = static_cast<int>(R.Bottom - Value + 1);
  }
  SetBounds(R);
}
//---------------------------------------------------------------------------
intptr_t TFarDialogItem::GetHeight() const
{
  return static_cast<intptr_t>(GetActualBounds().Height() + 1);
}
//---------------------------------------------------------------------------
bool TFarDialogItem::CanFocus() const
{
  FARDIALOGITEMTYPES Type = GetType();
  return GetVisible() && GetEnabled() && GetTabStop() &&
    (Type == DI_EDIT || Type == DI_PSWEDIT || Type == DI_FIXEDIT ||
     Type == DI_BUTTON || Type == DI_CHECKBOX || Type == DI_RADIOBUTTON ||
     Type == DI_COMBOBOX || Type == DI_LISTBOX || Type == DI_USERCONTROL);
}
//---------------------------------------------------------------------------
bool TFarDialogItem::Focused() const
{
  return GetFocused();
}
//---------------------------------------------------------------------------
void TFarDialogItem::UpdateFocused(bool Value)
{
  SetFocused(Value);
  assert(GetDialog());
  GetDialog()->SetItemFocused(Value ? this : NULL);
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetFocus()
{
  assert(CanFocus());
  if (!Focused())
  {
    if (GetDialog()->GetHandle())
    {
      SendMessage(DM_SETFOCUS, NULL);
    }
    else
    {
      if (GetDialog()->GetItemFocused())
      {
        assert(GetDialog()->GetItemFocused() != this);
        GetDialog()->GetItemFocused()->UpdateFocused(false);
      }
      UpdateFocused(true);
    }
  }
}
//---------------------------------------------------------------------------
void TFarDialogItem::Init()
{
  if (GetFlag(DIF_CENTERGROUP))
  {
    SMALL_RECT Rect;
    ClearStruct(Rect);

    // at least for "text" item, returned item size is not correct (on 1.70 final)
    SendMessage(DM_GETITEMPOSITION, reinterpret_cast<void *>(&Rect));

    TRect B = GetBounds();
    B.Left = Rect.Left;
    B.Right = Rect.Right;
    SetBounds(B);
  }
}
//---------------------------------------------------------------------------
bool TFarDialogItem::CloseQuery()
{
  if (Focused() && (GetDialog()->GetResult() >= 0))
  {
    DoExit();
  }
  return true;
}
//---------------------------------------------------------------------------
TPoint TFarDialogItem::MouseClientPosition(MOUSE_EVENT_RECORD * Event)
{
  TPoint Result;
  if (GetType() == DI_USERCONTROL)
  {
    Result = TPoint(Event->dwMousePosition.X, Event->dwMousePosition.Y);
  }
  else
  {
    Result = TPoint(
      static_cast<int>(Event->dwMousePosition.X - GetDialog()->GetBounds().Left - GetLeft()),
      static_cast<int>(Event->dwMousePosition.Y - GetDialog()->GetBounds().Top - GetTop()));
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TFarDialogItem::MouseClick(MOUSE_EVENT_RECORD * Event)
{
  if (FOnMouseClick)
  {
    FOnMouseClick(this, Event);
  }
  INPUT_RECORD Rec = {0};
  Rec.EventType = MOUSE_EVENT;
  memmove(&Rec.Event.MouseEvent, Event, sizeof(*Event));
  return DefaultItemProc(DN_CONTROLINPUT, static_cast<void *>(&Rec)) != 0;
}
//---------------------------------------------------------------------------
bool TFarDialogItem::MouseMove(int /*X*/, int /*Y*/,
  MOUSE_EVENT_RECORD * Event)
{
  INPUT_RECORD Rec = {0};
  Rec.EventType = MOUSE_EVENT;
  memmove(&Rec.Event.MouseEvent, Event, sizeof(*Event));
  return DefaultDialogProc(DN_INPUT, 0, reinterpret_cast<void *>(&Rec)) != 0;
}
//---------------------------------------------------------------------------
void TFarDialogItem::Text(int X, int Y, const FarColor & Color, const UnicodeString & Str)
{
  TFarEnvGuard Guard;
  GetDialog()->GetFarPlugin()->GetStartupInfo()->Text(
    static_cast<int>(GetDialog()->GetBounds().Left + GetLeft() + X),
    static_cast<int>(GetDialog()->GetBounds().Top + GetTop() + Y),
    &Color, Str.c_str());
}
//---------------------------------------------------------------------------
void TFarDialogItem::Redraw()
{
  // do not know how to force redraw of the item only
  GetDialog()->Redraw();
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetContainer(TFarDialogContainer * Value)
{
  if (GetContainer() != Value)
  {
    TFarDialogContainer * PrevContainer = GetContainer();
    FContainer = Value;
    if (PrevContainer)
    {
      PrevContainer->Remove(this);
    }
    if (GetContainer())
    {
      GetContainer()->Add(this);
    }
    UpdateBounds();
    UpdateEnabled();
  }
}
//---------------------------------------------------------------------------
bool TFarDialogItem::HotKey(char /*HotKey*/)
{
  return false;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarBox::TFarBox(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_SINGLEBOX)
{
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarButton::TFarButton(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_BUTTON)
{
  FResult = 0;
  FOnClick = NULL;
  FBrackets = brNormal;
}
//---------------------------------------------------------------------------
void TFarButton::SetDataInternal(const UnicodeString & Value)
{
  UnicodeString AValue;
  switch (FBrackets)
  {
    case brTight:
      AValue = L"[" + Value + L"]";
      break;

    case brSpace:
      AValue = L" " + Value + L" ";
      break;

    default:
      AValue = Value;
      break;
  }

  TFarDialogItem::SetDataInternal(AValue);

  if ((GetLeft() >= 0) || (GetRight() >= 0))
  {
    int Margin = 0;
    switch (FBrackets)
    {
      case brNone:
        Margin = 0;
        break;

      case brTight:
      case brSpace:
        Margin = 1;
        break;

      case brNormal:
        Margin = 2;
        break;
    }
    SetWidth(Margin + StripHotkey(AValue).GetLength() + Margin);
  }
}
//---------------------------------------------------------------------------
UnicodeString TFarButton::GetData()
{
  UnicodeString Result = TFarDialogItem::GetData();
  if ((FBrackets == brTight) || (FBrackets == brSpace))
  {
    bool HasBrackets = (Result.Length() >= 2) &&
      (Result[1] == ((FBrackets == brSpace) ? L' ' : L'[')) &&
      (Result[Result.Length()] == ((FBrackets == brSpace) ? L' ' : L']'));
    assert(HasBrackets);
    if (HasBrackets)
    {
      Result = Result.SubString(2, Result.Length() - 2);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TFarButton::SetDefault(bool Value)
{
  if (GetDefault() != Value)
  {
    assert(!GetDialog()->GetHandle());
    SetFlag(DIF_DEFAULTBUTTON, Value);
    if (Value)
    {
      if (GetDialog()->GetDefaultButton() && (GetDialog()->GetDefaultButton() != this))
      {
        GetDialog()->GetDefaultButton()->SetDefault(false);
      }
      GetDialog()->FDefaultButton = this;
    }
    else if (GetDialog()->GetDefaultButton() == this)
    {
      GetDialog()->FDefaultButton = NULL;
    }
    DialogChange();
  }
}
//---------------------------------------------------------------------------
bool TFarButton::GetDefault() const
{
  return GetFlag(DIF_DEFAULTBUTTON);
}
//---------------------------------------------------------------------------
void TFarButton::SetBrackets(TFarButtonBrackets Value)
{
  if (FBrackets != Value)
  {
    UnicodeString AData = GetData();
    SetFlag(DIF_NOBRACKETS, (Value != brNormal));
    FBrackets = Value;
    SetDataInternal(AData);
  }
}
//---------------------------------------------------------------------------
intptr_t TFarButton::ItemProc(intptr_t Msg, void * Param)
{
  if (Msg == DN_BTNCLICK)
  {
    if (!GetEnabled())
    {
      return 1;
    }
    else
    {
      bool Close = (GetResult() != 0);
      if (FOnClick)
      {
        FOnClick(this, Close);
      }
      if (!Close)
      {
        return 1;
      }
    }
  }
  return TFarDialogItem::ItemProc(Msg, Param);
}
//---------------------------------------------------------------------------
bool TFarButton::HotKey(char HotKey)
{
  intptr_t P = GetCaption().Pos(L'&');
  bool Result =
    GetVisible() && GetEnabled() &&
    (P > 0) && (P < GetCaption().Length()) &&
    (GetCaption()[P + 1] == HotKey);
  if (Result)
  {
    bool Close = (GetResult() != 0);
    if (FOnClick)
    {
      FOnClick(this, Close);
    }

    if (Close)
    {
      GetDialog()->Close(this);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarCheckBox::TFarCheckBox(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_CHECKBOX),
  FOnAllowChange(NULL)
{
}
//---------------------------------------------------------------------------
intptr_t TFarCheckBox::ItemProc(intptr_t Msg, void * Param)
{
  if (Msg == DN_BTNCLICK)
  {
    bool Allow = true;
    if (FOnAllowChange)
    {
      FOnAllowChange(this, Param, Allow);
    }
    if (Allow)
    {
      UpdateSelected(reinterpret_cast<intptr_t>(Param));
    }
    return static_cast<intptr_t>(Allow);
  }
  else
  {
    return TFarDialogItem::ItemProc(Msg, Param);
  }
}
//---------------------------------------------------------------------------
bool TFarCheckBox::GetIsEmpty() const
{
  return GetChecked() != BSTATE_CHECKED;
}
//---------------------------------------------------------------------------
void TFarCheckBox::SetData(const UnicodeString & Value)
{
  TFarDialogItem::SetData(Value);
  if (GetLeft() >= 0 || GetRight() >= 0)
  {
    SetWidth(4 + StripHotkey(Value).Length());
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarRadioButton::TFarRadioButton(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_RADIOBUTTON),
  FOnAllowChange(NULL)
{
}
//---------------------------------------------------------------------------
intptr_t TFarRadioButton::ItemProc(intptr_t Msg, void * Param)
{
  if (Msg == DN_BTNCLICK)
  {
    bool Allow = true;
    if (FOnAllowChange)
    {
      FOnAllowChange(this, Param, Allow);
    }
    if (Allow)
    {
      // FAR WORKAROUND
      // This does not correspond to FAR API Manual, but it works so.
      // Manual says that Param should contain ID of previously selected dialog item
      UpdateSelected(reinterpret_cast<intptr_t>(Param));
    }
    return static_cast<intptr_t>(Allow);
  }
  else
  {
    return TFarDialogItem::ItemProc(Msg, Param);
  }
}
//---------------------------------------------------------------------------
bool TFarRadioButton::GetIsEmpty() const
{
  return !GetChecked();
}
//---------------------------------------------------------------------------
void TFarRadioButton::SetData(const UnicodeString & Value)
{
  TFarDialogItem::SetData(Value);
  if (GetLeft() >= 0 || GetRight() >= 0)
  {
    SetWidth(4 + StripHotkey(Value).Length());
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarEdit::TFarEdit(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_EDIT)
{
  SetAutoSelect(false);
}
//---------------------------------------------------------------------------
void TFarEdit::Detach()
{
  nb_free((void*)GetDialogItem()->Mask);
  nb_free((void *)GetDialogItem()->History);
  TFarDialogItem::Detach();
}
//---------------------------------------------------------------------------
intptr_t TFarEdit::ItemProc(intptr_t Msg, void * Param)
{
  if (Msg == DN_EDITCHANGE)
  {
    UnicodeString Data = (reinterpret_cast<FarDialogItem *>(Param))->Data;
    nb_free((void*)GetDialogItem()->Data);
    GetDialogItem()->Data = TCustomFarPlugin::DuplicateStr(Data, true);
  }
  return TFarDialogItem::ItemProc(Msg, Param);
}
//---------------------------------------------------------------------------
UnicodeString TFarEdit::GetHistoryMask(size_t Index) const
{
  UnicodeString Result =
    ((Index == 0) && (GetFlags() & DIF_HISTORY)) ||
    ((Index == 1) && (GetFlags() & DIF_MASKEDIT)) ? GetDialogItem()->Mask : L"";
  return Result;
}
//---------------------------------------------------------------------------
void TFarEdit::SetHistoryMask(size_t Index, const UnicodeString & Value)
{
  if (GetHistoryMask(Index) != Value)
  {
    assert(!GetDialog()->GetHandle());
    FarDialogItem * Item = GetDialogItem();
    // assert(&GetDialogItem()->Mask == &GetDialogItem()->History);

    nb_free((void*)Item->Mask);
    nb_free((void*)Item->History);
    if (Value.IsEmpty())
    {
      Item->Mask = NULL;
      Item->History = NULL;
    }
    else
    {
      Item->Mask = TCustomFarPlugin::DuplicateStr(Value);
      Item->History = TCustomFarPlugin::DuplicateStr(Value);
    }
    bool PrevHistory = !GetHistory().IsEmpty();
    SetFlag(DIF_HISTORY, (Index == 0) && !Value.IsEmpty());
    bool Masked = (Index == 1) && !Value.IsEmpty();
    SetFlag(DIF_MASKEDIT, Masked);
    if (Masked)
    {
      SetFixed(true);
    }
    bool CurrHistory = !GetHistory().IsEmpty();
    if (PrevHistory != CurrHistory)
    {
      // add/remove space for history arrow
      SetWidth(GetWidth() + (CurrHistory ? -1 : 1));
    }
    DialogChange();
  }
}
//---------------------------------------------------------------------------
void TFarEdit::SetAsInteger(intptr_t Value)
{
  intptr_t Int = GetAsInteger();
  if (!Int || (Int != Value))
  {
    SetText(::IntToStr(Value));
    DialogChange();
  }
}
//---------------------------------------------------------------------------
intptr_t TFarEdit::GetAsInteger()
{
  return ::StrToIntDef(::Trim(GetText()), 0);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarSeparator::TFarSeparator(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_TEXT)
{
  SetLeft(-1);
  SetFlag(DIF_SEPARATOR, true);
}
//---------------------------------------------------------------------------
void TFarSeparator::ResetBounds()
{
  TFarDialogItem::ResetBounds();
  if (GetBounds().Left < 0)
  {
    GetDialogItem()->X1 = -1;
  }
}
//---------------------------------------------------------------------------
void TFarSeparator::SetDouble(bool Value)
{
  if (GetDouble() != Value)
  {
    assert(!GetDialog()->GetHandle());
    SetFlag(DIF_SEPARATOR, !Value);
    SetFlag(DIF_SEPARATOR2, Value);
  }
}
//---------------------------------------------------------------------------
bool TFarSeparator::GetDouble()
{
  return GetFlag(DIF_SEPARATOR2);
}
//---------------------------------------------------------------------------
void TFarSeparator::SetPosition(intptr_t Value)
{
  TRect R = GetBounds();
  R.Top = static_cast<int>(Value);
  R.Bottom = static_cast<int>(Value);
  SetBounds(R);
}
//---------------------------------------------------------------------------
int TFarSeparator::GetPosition()
{
  return GetBounds().Top;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarText::TFarText(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_TEXT)
{
}
//---------------------------------------------------------------------------
void TFarText::SetData(const UnicodeString & Value)
{
  TFarDialogItem::SetData(Value);
  if (GetLeft() >= 0 || GetRight() >= 0)
  {
    SetWidth(StripHotkey(Value).Length());
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarList::TFarList(TFarDialogItem * ADialogItem) :
  TStringList()
{
  assert((ADialogItem == NULL) ||
    (ADialogItem->GetType() == DI_COMBOBOX) || (ADialogItem->GetType() == DI_LISTBOX));
  FDialogItem = ADialogItem;
  FListItems = static_cast<FarList *>(nb_malloc(sizeof(FarList)));
  memset(FListItems, 0, sizeof(FarList));
  FListItems->StructSize = sizeof(FarList);
  FNoDialogUpdate = false;
}
//---------------------------------------------------------------------------
TFarList::~TFarList()
{
  for (intptr_t Index = 0; Index < FListItems->ItemsNumber; ++Index)
  {
    nb_free((void*)FListItems->Items[Index].Text);
  }
  nb_free(FListItems->Items);
  nb_free(FListItems);
}
//---------------------------------------------------------------------------
void TFarList::Assign(const TPersistent * Source)
{
  TStringList::Assign(Source);

  const TFarList * FarList = dynamic_cast<const TFarList *>(Source);
  if (FarList != NULL)
  {
    for (intptr_t Index = 0; Index < FarList->GetCount(); ++Index)
    {
      SetFlags(Index, FarList->GetFlags(Index));
    }
  }
}
//---------------------------------------------------------------------------
void TFarList::UpdateItem(intptr_t Index)
{
  FarListItem * ListItem = &FListItems->Items[Index];
  nb_free((void*)ListItem->Text);
  ListItem->Text = TCustomFarPlugin::DuplicateStr(GetString(Index), true);

  FarListUpdate ListUpdate;
  ClearStruct(ListUpdate);
    ListUpdate.StructSize = sizeof(FarListUpdate);
  ListUpdate.Index = static_cast<int>(Index);
  ListUpdate.Item = *ListItem;
  GetDialogItem()->SendMessage(DM_LISTUPDATE, reinterpret_cast<void *>(&ListUpdate));
}
//---------------------------------------------------------------------------
void TFarList::Put(intptr_t Index, const UnicodeString & S)
{
  if ((GetDialogItem() != NULL) && GetDialogItem()->GetDialog()->GetHandle())
  {
    FNoDialogUpdate = true;
    TRY_FINALLY (
    {
      TStringList::SetString(Index, S);
      if (GetUpdateCount() == 0)
      {
        UpdateItem(Index);
      }
    }
    ,
    {
      FNoDialogUpdate = false;
    }
    );
  }
  else
  {
    TStringList::SetString(Index, S);
  }
}
//---------------------------------------------------------------------------
void TFarList::Changed()
{
  TStringList::Changed();

  if ((GetUpdateCount() == 0) && !FNoDialogUpdate)
  {
    intptr_t PrevSelected = 0;
    intptr_t PrevTopIndex = 0;
    if ((GetDialogItem() != NULL) && GetDialogItem()->GetDialog()->GetHandle())
    {
      PrevSelected = GetSelected();
      PrevTopIndex = GetTopIndex();
    }
    intptr_t Count = GetCount();
    if (FListItems->ItemsNumber != Count)
    {
      FarListItem * Items = FListItems->Items;
      intptr_t ItemsNumber = FListItems->ItemsNumber;
      if (Count)
      {
        FListItems->Items = static_cast<FarListItem *>(
          nb_calloc(1, sizeof(FarListItem) * Count));
        for (intptr_t Index = 0; Index < Count; ++Index)
        {
          if (Index < FListItems->ItemsNumber)
          {
            FListItems->Items[Index].Flags = Items[Index].Flags;
          }
        }
      }
      else
      {
        FListItems->Items = NULL;
      }
      for (intptr_t Index = 0; Index < ItemsNumber; ++Index)
      {
        nb_free((void*)Items[Index].Text);
      }
      nb_free(Items);
      FListItems->ItemsNumber = static_cast<int>(GetCount());
    }
    for (intptr_t I = 0; I < GetCount(); I++)
    {
      FListItems->Items[I].Text = TCustomFarPlugin::DuplicateStr(GetString(I), true);
    }
    if ((GetDialogItem() != NULL) && GetDialogItem()->GetDialog()->GetHandle())
    {
      GetDialogItem()->GetDialog()->LockChanges();
      TRY_FINALLY (
      {
        GetDialogItem()->SendMessage(DM_LISTSET, reinterpret_cast<void *>(FListItems));
        if (PrevTopIndex + GetDialogItem()->GetHeight() > GetCount())
        {
          PrevTopIndex = GetCount() > GetDialogItem()->GetHeight() ? GetCount() - GetDialogItem()->GetHeight() : 0;
        }
        SetCurPos((PrevSelected >= GetCount()) ? (GetCount() - 1) : PrevSelected,
          PrevTopIndex);
      }
      ,
      {
        GetDialogItem()->GetDialog()->UnlockChanges();
      }
      );
    }
  }
}
//---------------------------------------------------------------------------
void TFarList::SetSelected(intptr_t Value)
{
  assert(GetDialogItem() != NULL);
  if (GetSelectedInt(false) != Value)
  {
    if (GetDialogItem()->GetDialog()->GetHandle())
    {
      UpdatePosition(Value);
    }
    else
    {
      GetDialogItem()->SetData(GetString(Value));
    }
  }
}
//---------------------------------------------------------------------------
void TFarList::UpdatePosition(intptr_t Position)
{
  if (Position >= 0)
  {
    intptr_t ATopIndex = GetTopIndex();
    // even if new position is visible already, FAR will scroll the view so
    // that the new selected item is the last one, following prevents the scroll
    if ((ATopIndex <= Position) && (Position < ATopIndex + GetVisibleCount()))
    {
      SetCurPos(Position, ATopIndex);
    }
    else
    {
      SetCurPos(Position, -1);
    }
  }
}
//---------------------------------------------------------------------------
void TFarList::SetCurPos(intptr_t Position, intptr_t TopIndex)
{
  assert(GetDialogItem() != NULL);
  assert(GetDialogItem()->GetDialog()->GetHandle());
  FarListPos ListPos;
    ListPos.StructSize = sizeof(FarListPos);
  ListPos.SelectPos = Position;
  ListPos.TopPos = TopIndex;
  GetDialogItem()->SendMessage(DM_LISTSETCURPOS, reinterpret_cast<void *>(&ListPos));
}
//---------------------------------------------------------------------------
void TFarList::SetTopIndex(intptr_t Value)
{
  if (Value != GetTopIndex())
  {
    SetCurPos(NPOS, Value);
  }
}
//---------------------------------------------------------------------------
intptr_t TFarList::GetPosition() const
{
  assert(GetDialogItem() != NULL);
  return GetDialogItem()->SendMessage(DM_LISTGETCURPOS, NULL);
}
//---------------------------------------------------------------------------
intptr_t TFarList::GetTopIndex() const
{
  intptr_t Result;
  if (GetCount() == 0)
  {
    Result = -1;
  }
  else
  {
    FarListPos ListPos;
    ClearStruct(ListPos);
    ListPos.StructSize = sizeof(FarListPos);
    assert(GetDialogItem() != NULL);
    GetDialogItem()->SendMessage(DM_LISTGETCURPOS, reinterpret_cast<void *>(&ListPos));
    Result = static_cast<intptr_t>(ListPos.TopPos);
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TFarList::GetMaxLength() const
{
  intptr_t Result = 0;
  for (intptr_t I = 0; I < GetCount(); I++)
  {
    if (Result < GetString(I).Length())
    {
      Result = GetString(I).Length();
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TFarList::GetVisibleCount() const
{
  assert(GetDialogItem() != NULL);
  return GetDialogItem()->GetHeight() - (GetDialogItem()->GetFlag(DIF_LISTNOBOX) ? 0 : 2);
}
//---------------------------------------------------------------------------
intptr_t TFarList::GetSelectedInt(bool Init) const
{
  intptr_t Result = NPOS;
  assert(GetDialogItem() != NULL);
  if (GetCount() == 0)
  {
    Result = NPOS;
  }
  else if (GetDialogItem()->GetDialog()->GetHandle() && !Init)
  {
    Result = GetPosition();
  }
  else
  {
    const wchar_t *Data = GetDialogItem()->GetDialogItem()->Data;
    if (Data)
    {
      Result = IndexOf(Data);
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
intptr_t TFarList::GetSelected() const
{
  intptr_t Result = GetSelectedInt(false);

  if ((Result == NPOS) && (GetCount() > 0))
  {
    Result = 0;
  }

  return Result;
}
//---------------------------------------------------------------------------
LISTITEMFLAGS TFarList::GetFlags(intptr_t Index) const
{
  return FListItems->Items[Index].Flags;
}
//---------------------------------------------------------------------------
void TFarList::SetFlags(intptr_t Index, LISTITEMFLAGS Value)
{
  if (FListItems->Items[Index].Flags != Value)
  {
    FListItems->Items[Index].Flags = Value;
    if ((GetDialogItem() != NULL) && GetDialogItem()->GetDialog()->GetHandle() && (GetUpdateCount() == 0))
    {
      UpdateItem(Index);
    }
  }
}
//---------------------------------------------------------------------------
bool TFarList::GetFlag(intptr_t Index, LISTITEMFLAGS Flag) const
{
  return FLAGSET(GetFlags(Index), Flag);
}
//---------------------------------------------------------------------------
void TFarList::SetFlag(intptr_t Index, LISTITEMFLAGS Flag, bool Value)
{
  SetFlags(Index, (GetFlags(Index) & ~Flag) | FLAGMASK(Value, Flag));
}
//---------------------------------------------------------------------------
void TFarList::Init()
{
  UpdatePosition(GetSelectedInt(true));
}
//---------------------------------------------------------------------------
intptr_t TFarList::ItemProc(intptr_t Msg, void * Param)
{
  assert(GetDialogItem() != NULL);
  if (Msg == DN_LISTCHANGE)
  {
    if ((Param == 0) && (GetCount() == 0))
    {
      GetDialogItem()->UpdateData(L"");
    }
    else
    {
      intptr_t param = reinterpret_cast<intptr_t>(Param);
      assert(param >= 0 && param < GetCount());
      GetDialogItem()->UpdateData(GetString(param));
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarListBox::TFarListBox(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_LISTBOX),
  FDenyClose(NULL)
{
  FList = new TFarList(this);
  GetDialogItem()->ListItems = FList->GetListItems();
  FAutoSelect = asOnlyFocus;
}
//---------------------------------------------------------------------------
TFarListBox::~TFarListBox()
{
  SAFE_DESTROY(FList);
}
//---------------------------------------------------------------------------
intptr_t TFarListBox::ItemProc(intptr_t Msg, void * Param)
{
  intptr_t Result = 0;
  if (Msg == DN_CONTROLINPUT)
  {
    const INPUT_RECORD *Rec = static_cast<const INPUT_RECORD *>(Param);
    const KEY_EVENT_RECORD &Event = Rec->Event.KeyEvent;
    if (GetDialog()->HotKey(Event.wVirtualKeyCode, Event.dwControlKeyState))
    {
      Result = 1;
    }
  }
  else if (FList->ItemProc(Msg, Param))
  {
    Result = 1;
  }
  else
  {
    Result = TFarDialogItem::ItemProc(Msg, Param);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TFarListBox::Init()
{
  TFarDialogItem::Init();
  GetItems()->Init();
  UpdateMouseReaction();
}
//---------------------------------------------------------------------------
void TFarListBox::SetAutoSelect(TFarListBoxAutoSelect Value)
{
  if (GetAutoSelect() != Value)
  {
    FAutoSelect = Value;
    if (GetDialog()->GetHandle())
    {
      UpdateMouseReaction();
    }
  }
}
//---------------------------------------------------------------------------
void TFarListBox::UpdateMouseReaction()
{
  SendMessage(DIF_LISTTRACKMOUSE, reinterpret_cast<void *>(GetAutoSelect()));
}
//---------------------------------------------------------------------------
void TFarListBox::SetItems(TStrings * Value)
{
  FList->Assign(Value);
}
//---------------------------------------------------------------------------
void TFarListBox::SetList(TFarList * Value)
{
  SetItems(Value);
}
//---------------------------------------------------------------------------
bool TFarListBox::CloseQuery()
{
  return true;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarComboBox::TFarComboBox(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_COMBOBOX)
{
  FList = new TFarList(this);
  GetDialogItem()->ListItems = FList->GetListItems();
  SetAutoSelect(false);
}
//---------------------------------------------------------------------------
TFarComboBox::~TFarComboBox()
{
  SAFE_DESTROY(FList);
}
//---------------------------------------------------------------------------
void TFarComboBox::ResizeToFitContent()
{
  SetWidth(FList->GetMaxLength());
}
//---------------------------------------------------------------------------
intptr_t TFarComboBox::ItemProc(intptr_t Msg, void * Param)
{
  if (Msg == DN_EDITCHANGE)
  {
    UnicodeString Data = (reinterpret_cast<FarDialogItem *>(Param))->Data;
    nb_free((void*)GetDialogItem()->Data);
    GetDialogItem()->Data = TCustomFarPlugin::DuplicateStr(Data, true);
  }

  if (FList->ItemProc(Msg, Param))
  {
    return 1;
  }
  else
  {
    return TFarDialogItem::ItemProc(Msg, Param);
  }
}
//---------------------------------------------------------------------------
void TFarComboBox::Init()
{
  TFarDialogItem::Init();
  GetItems()->Init();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarLister::TFarLister(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_USERCONTROL),
  FItems(new TStringList()),
  FTopIndex(0)
{
  FItems->SetOnChange(MAKE_CALLBACK(TFarLister::ItemsChange, this));
}
//---------------------------------------------------------------------------
TFarLister::~TFarLister()
{
  delete FItems;
}
//---------------------------------------------------------------------------
void TFarLister::ItemsChange(TObject * /*Sender*/)
{
  FTopIndex = 0;
  if (GetDialog()->GetHandle())
  {
    Redraw();
  }
}
//---------------------------------------------------------------------------
bool TFarLister::GetScrollBar() const
{
  return (GetItems()->GetCount() > GetHeight());
}
//---------------------------------------------------------------------------
void TFarLister::SetTopIndex(intptr_t Value)
{
  if (GetTopIndex() != Value)
  {
    FTopIndex = Value;
    Redraw();
  }
}
//---------------------------------------------------------------------------
TStrings * TFarLister::GetItems() const
{
  return FItems;
}
//---------------------------------------------------------------------------
void TFarLister::SetItems(const TStrings * Value)
{
  if (!FItems->Equals(Value))
  {
    FItems->Assign(Value);
  }
}
//---------------------------------------------------------------------------
void TFarLister::DoFocus()
{
  TFarDialogItem::DoFocus();
  // TODO: hide cursor
}
//---------------------------------------------------------------------------
intptr_t TFarLister::ItemProc(intptr_t Msg, void * Param)
{
  intptr_t Result = 0;

  if (Msg == DN_DRAWDLGITEM)
  {
    bool AScrollBar = GetScrollBar();
    intptr_t ScrollBarPos = 0;
    if (GetItems()->GetCount() > GetHeight())
    {
      ScrollBarPos = static_cast<intptr_t>((static_cast<float>(GetHeight() - 3) * (static_cast<float>(FTopIndex) / (GetItems()->GetCount() - GetHeight())))) + 1;
    }
    intptr_t DisplayWidth = GetWidth() - (AScrollBar ? 1 : 0);
    FarColor Color = GetDialog()->GetSystemColor(
      FLAGSET(GetDialog()->GetFlags(), FDLG_WARNING) ? COL_WARNDIALOGLISTTEXT : COL_DIALOGLISTTEXT);
    UnicodeString Buf;
    for (intptr_t Row = 0; Row < GetHeight(); Row++)
    {
      intptr_t Index = GetTopIndex() + Row;
      Buf = L" ";
      if (Index < GetItems()->GetCount())
      {
        UnicodeString Value = GetItems()->GetString(Index).SubString(1, DisplayWidth - 1);
        Buf += Value;
      }
      UnicodeString Value = ::StringOfChar(' ', DisplayWidth - Buf.Length());
      Value.SetLength(DisplayWidth - Buf.Length());
      Buf += Value;
      if (AScrollBar)
      {
        if (Row == 0)
        {
          Buf += static_cast<wchar_t>(0x25B2); // L'\x1E'; // ucUpScroll
        }
        else if (Row == ScrollBarPos)
        {
          Buf += static_cast<wchar_t>(0x2592); // L'\xB2'; // ucBox50
        }
        else if (Row == GetHeight() - 1)
        {
          Buf += static_cast<wchar_t>(0x25BC); // L'\x1F'; // ucDnScroll
        }
        else
        {
          Buf += static_cast<wchar_t>(0x2591); // '\xB0'; // ucBox25
        }
      }
      Text(0, (int)Row, Color, Buf);
    }
  }
  else if (Msg == DN_CONTROLINPUT)
  {
    Result = static_cast<int>(true);
    INPUT_RECORD *Rec = reinterpret_cast<INPUT_RECORD *>(Param);
    if (Rec->EventType == KEY_EVENT)
    {
      KEY_EVENT_RECORD *KeyEvent = &Rec->Event.KeyEvent;

      intptr_t NewTopIndex = GetTopIndex();

      WORD Key = KeyEvent->wVirtualKeyCode;
      if ((Key == VK_UP) || (Key == VK_LEFT))
      {
        if (NewTopIndex > 0)
        {
          NewTopIndex--;
        }
        else
        {
          INPUT_RECORD Rec = {0};
          Rec.EventType = KEY_EVENT;
          Rec.Event.KeyEvent.wVirtualKeyCode = VK_TAB;
          Rec.Event.KeyEvent.dwControlKeyState = SHIFT_PRESSED;
          SendDialogMessage(DN_CONTROLINPUT, 1, static_cast<void *>(&Rec));
        }
      }
      else if ((Key == VK_DOWN) || (Key == VK_RIGHT))
      {
        if (NewTopIndex < GetItems()->GetCount() - GetHeight())
        {
          ++NewTopIndex;
        }
        else
        {
          INPUT_RECORD Rec = {0};
          Rec.EventType = KEY_EVENT;
          Rec.Event.KeyEvent.wVirtualKeyCode = VK_TAB;
          Rec.Event.KeyEvent.dwControlKeyState = 0;
          SendDialogMessage(DN_CONTROLINPUT, 1, static_cast<void *>(&Rec));
        }
      }
      else if (Key == VK_PRIOR)
      {
        if (NewTopIndex > GetHeight() - 1)
        {
          NewTopIndex -= GetHeight() - 1;
        }
        else
        {
          NewTopIndex = 0;
        }
      }
      else if (Key == VK_NEXT)
      {
        if (NewTopIndex < GetItems()->GetCount() - GetHeight() - GetHeight() + 1)
        {
          NewTopIndex += GetHeight() - 1;
        }
        else
        {
          NewTopIndex = GetItems()->GetCount() - GetHeight();
        }
      }
      else if (Key == VK_HOME)
      {
        NewTopIndex = 0;
      }
      else if (Key == VK_END)
      {
        NewTopIndex = GetItems()->GetCount() - GetHeight();
      }
      else
      {
        Result = TFarDialogItem::ItemProc(Msg, Param);
      }

      SetTopIndex(NewTopIndex);
    }
    else if (Rec->EventType == MOUSE_EVENT)
    {
      if (!Focused() && CanFocus())
      {
        SetFocus();
      }

      MOUSE_EVENT_RECORD *Event = &Rec->Event.MouseEvent;
      TPoint P = MouseClientPosition(Event);

      if (FLAGSET(Event->dwEventFlags, DOUBLE_CLICK) &&
              (P.x < GetWidth() - 1))
      {
        Result = TFarDialogItem::ItemProc(Msg, Param);
      }
      else
      {
        intptr_t NewTopIndex = GetTopIndex();

        if (((P.x == static_cast<int>(GetWidth()) - 1) && (P.y == 0)) ||
                ((P.x < static_cast<int>(GetWidth() - 1)) && (P.y < static_cast<int>(GetHeight() / 2))))
        {
          if (NewTopIndex > 0)
          {
              NewTopIndex--;
          }
        }
        else if (((P.x == GetWidth() - 1) && (P.y == static_cast<int>(GetHeight() - 1))) ||
                 ((P.x < GetWidth() - 1) && (P.y >= static_cast<int>(GetHeight() / 2))))
        {
          if (NewTopIndex < GetItems()->GetCount() - GetHeight())
          {
            ++NewTopIndex;
          }
        }
        else
        {
          assert(P.x == GetWidth() - 1);
          assert((P.y > 0) && (P.y < static_cast<int>(GetHeight() - 1)));
          NewTopIndex = static_cast<intptr_t>(ceil(static_cast<float>(P.y - 1) / (GetHeight() - 2) * (GetItems()->GetCount() - GetHeight() + 1)));
        }

        Result = static_cast<int>(true);

        SetTopIndex(NewTopIndex);
      }
    }
  }
  else
  {
    Result = TFarDialogItem::ItemProc(Msg, Param);
  }

  return Result;
}
