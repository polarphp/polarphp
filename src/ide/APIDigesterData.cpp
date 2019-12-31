//===--- APIDigesterData.cpp - api digester data implementation -----------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/StringSet.h"
#include "llvm/Support/YAMLParser.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm/Support/MemoryBuffer.h"
#include "polarphp/basic/JSONSerialization.h"
#include "polarphp/ide/APIDigesterData.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/ast/DiagnosticsDriver.h"

using namespace polar;
using namespace ide;
using namespace api;

raw_ostream &polar::ide::api::
operator<<(raw_ostream &Out, const SDKNodeKind Value) {
   switch (Value) {
#define NODE_KIND(Name, Value) case SDKNodeKind::Name: return Out << #Value;
#include "polarphp/ide/DigesterEnumsDef.h"
   }
   llvm_unreachable("Undefined SDK node kind.");
}

raw_ostream &polar::ide::api::
operator<<(raw_ostream &Out, const NodeAnnotation Value) {
#define NODE_ANNOTATION(X) if (Value == NodeAnnotation::X) { return Out << #X; }
#include "polarphp/ide/DigesterEnumsDef.h"
   llvm_unreachable("Undefined SDK node kind.");
}

StringRef polar::ide::api::getDeclKindStr(const DeclKind Value)  {
   switch (Value) {
#define DECL(X, PARENT) case DeclKind::X: return #X;
#include "polarphp/ast/DeclNodesDef.h"
   }
   llvm_unreachable("Unhandled DeclKind in switch.");
}

raw_ostream &polar::ide::api::operator<<(raw_ostream &Out,
                                         const DeclKind Value) {
   return Out << getDeclKindStr(Value);
}

Optional<SDKNodeKind> polar::ide::api::parseSDKNodeKind(StringRef Content) {
   return llvm::StringSwitch<Optional<SDKNodeKind>>(Content)
#define NODE_KIND(NAME, VALUE) .Case(#VALUE, SDKNodeKind::NAME)
#include "polarphp/ide/DigesterEnumsDef.h"
      .Default(None)
      ;
}

NodeAnnotation polar::ide::api::parseSDKNodeAnnotation(StringRef Content) {
   return llvm::StringSwitch<NodeAnnotation>(Content)
#define NODE_ANNOTATION_CHANGE_KIND(NAME) .Case(#NAME, NodeAnnotation::NAME)
#include "polarphp/ide/DigesterEnumsDef.h"
      ;
}

SpecialCaseId polar::ide::api::parseSpecialCaseId(StringRef Content) {
   return llvm::StringSwitch<SpecialCaseId>(Content)
#define SPECIAL_CASE_ID(NAME) .Case(#NAME, SpecialCaseId::NAME)
#include "polarphp/ide/DigesterEnumsDef.h"
      ;
}

polar::ide::api::CommonDiffItem::
CommonDiffItem(SDKNodeKind NodeKind, NodeAnnotation DiffKind,
               StringRef ChildIndex, StringRef LeftUsr, StringRef RightUsr,
               StringRef LeftComment, StringRef RightComment,
               StringRef ModuleName) : NodeKind(NodeKind),
                                       DiffKind(DiffKind), ChildIndex(ChildIndex), LeftUsr(LeftUsr),
                                       RightUsr(RightUsr), LeftComment(LeftComment),
                                       RightComment(RightComment), ModuleName(ModuleName) {
   assert(!ChildIndex.empty() && "Child index is empty.");
   llvm::SmallVector<StringRef, 4> Pieces;
   ChildIndex.split(Pieces, ":");
   std::transform(Pieces.begin(), Pieces.end(),
                  std::back_inserter(ChildIndexPieces),
                  [](StringRef Piece) { return std::stoi(Piece); });
}

StringRef polar::ide::api::CommonDiffItem::head() {
   return "SDK_CHANGE";
}

bool polar::ide::api::CommonDiffItem::operator<(CommonDiffItem Other) const {
   if (auto UsrCompare = LeftUsr.compare(Other.LeftUsr))
      return UsrCompare < 0;
   if (NodeKind != Other.NodeKind)
      return NodeKind < Other.NodeKind;
   if (DiffKind != Other.DiffKind)
      return DiffKind < Other.DiffKind;
   if (auto ChildCompare = ChildIndex.compare(Other.ChildIndex))
      return ChildCompare < 0;
   return false;
}

void polar::ide::api::CommonDiffItem::describe(llvm::raw_ostream &os) {
   os << "#ifndef " << head() << "\n";
   os << "#define " << head() << "(NODE_KIND, DIFF_KIND, CHILD_INDEX, LEFT_USR, "
                                 "RIGHT_USR, LEFT_COMMENT, RIGHT_COMMENT, "
                                 "MODULENAME)\n";
   os << "#endif\n";
}

void polar::ide::api::CommonDiffItem::undef(llvm::raw_ostream &os) {
   os << "#undef " << head() << "\n";
}

void polar::ide::api::CommonDiffItem::streamDef(llvm::raw_ostream &S) const {
   S << head() << "(" << NodeKind << ", " << DiffKind << ", \"" << ChildIndex
     << "\", \"" << LeftUsr << "\", \"" << RightUsr << "\", \""
     << LeftComment << "\", \"" << RightComment
     << "\", \"" << ModuleName << "\")";
}

StringRef polar::ide::api::TypeMemberDiffItem::head() {
   return "SDK_CHANGE_TYPE_MEMBER";
}

TypeMemberDiffItemSubKind
polar::ide::api::TypeMemberDiffItem::getSubKind() const {
   DeclNameViewer OldName = getOldName();
   DeclNameViewer NewName = getNewName();
   if (!OldName.isFunction()) {
      assert(!NewName.isFunction());
      if (oldTypeName.empty())
         return TypeMemberDiffItemSubKind::SimpleReplacement;
      else
         return TypeMemberDiffItemSubKind::QualifiedReplacement;
   }
   assert(OldName.isFunction());
   bool ToProperty = !NewName.isFunction();
   if (selfIndex) {
      if (removedIndex) {
         if (ToProperty)
            llvm_unreachable("unknown situation");
         else {
            assert(NewName.argSize() + 2 == OldName.argSize());
            return TypeMemberDiffItemSubKind::HoistSelfAndRemoveParam;
         }
      } else if (ToProperty) {
         assert(OldName.argSize() == 1);
         return TypeMemberDiffItemSubKind::HoistSelfAndUseProperty;
      } else if (oldTypeName.empty()) {
         assert(NewName.argSize() + 1 == OldName.argSize());
         return TypeMemberDiffItemSubKind::HoistSelfOnly;
      } else {
         assert(NewName.argSize() == OldName.argSize());
         return TypeMemberDiffItemSubKind::QualifiedReplacement;
      }
   } else if (ToProperty) {
      assert(OldName.argSize() == 0);
      assert(!removedIndex);
      return TypeMemberDiffItemSubKind::GlobalFuncToStaticProperty;
   } else if (oldTypeName.empty()) {
      // we can handle this as a simple function rename.
      assert(NewName.argSize() == OldName.argSize());
      return TypeMemberDiffItemSubKind::FuncRename;
   } else {
      assert(NewName.argSize() == OldName.argSize());
      return TypeMemberDiffItemSubKind::QualifiedReplacement;
   }
}

void polar::ide::api::TypeMemberDiffItem::describe(llvm::raw_ostream &os) {
   os << "#ifndef " << head() << "\n";
   os << "#define " << head() << "(USR, NEW_TYPE_NAME, NEW_PRINTED_NAME, "
                                 "SELF_INDEX, OLD_PRINTED_NAME)\n";
   os << "#endif\n";
}

void polar::ide::api::TypeMemberDiffItem::undef(llvm::raw_ostream &os) {
   os << "#undef " << head() << "\n";
}

void polar::ide::api::TypeMemberDiffItem::streamDef(llvm::raw_ostream &os) const {
   std::string IndexContent = selfIndex.hasValue() ?
                              std::to_string(selfIndex.getValue()) : "";
   os << head() << "("
      << "\"" << usr << "\"" << ", "
      << "\"" << newTypeName << "\"" << ", "
      << "\"" << newPrintedName << "\"" << ", "
      << "\"" << IndexContent << "\"" << ", "
      << "\"" << oldPrintedName << "\""
      << ")";
}

bool polar::ide::api::TypeMemberDiffItem::
operator<(TypeMemberDiffItem Other) const {
   return usr.compare(Other.usr) < 0;
}

StringRef polar::ide::api::NoEscapeFuncParam::head() {
   return "NOESCAPE_FUNC_PARAM";
}

void polar::ide::api::NoEscapeFuncParam::describe(llvm::raw_ostream &os) {
   os << "#ifndef " << head() << "\n";
   os << "#define " << head() << "(USR, Index)\n";
   os << "#endif\n";
}

void polar::ide::api::NoEscapeFuncParam::undef(llvm::raw_ostream &os) {
   os << "#undef " << head() << "\n";
}

void polar::ide::api::NoEscapeFuncParam::
streamDef(llvm::raw_ostream &os) const {
   os << head() << "(" << "\"" << Usr << "\"" << ", "
      << "\"" << Index << "\"" << ")";
}

bool polar::ide::api::NoEscapeFuncParam::
operator<(NoEscapeFuncParam Other) const {
   if (Usr != Other.Usr)
      return Usr.compare(Other.Usr) < 0;
   return Index < Other.Index;
}

StringRef polar::ide::api::OverloadedFuncInfo::head() {
   return "OVERLOAD_FUNC_TRAILING_CLOSURE";
}

void polar::ide::api::OverloadedFuncInfo::describe(llvm::raw_ostream &os) {
   os << "#ifndef " << head() << "\n";
   os << "#define " << head() << "(USR)\n";
   os << "#endif\n";
}

void polar::ide::api::OverloadedFuncInfo::undef(llvm::raw_ostream &os) {
   os << "#undef " << head() << "\n";
}

void polar::ide::api::OverloadedFuncInfo::
streamDef(llvm::raw_ostream &os) const {
   os << head() << "(" << "\"" << Usr << "\"" << ")";
}

bool polar::ide::api::OverloadedFuncInfo::
operator<(OverloadedFuncInfo Other) const {
   return Usr.compare(Other.Usr) < 0;
}

#define DIFF_ITEM_KIND(NAME)                                                   \
bool polar::ide::api::NAME::classof(const APIDiffItem *D) {                    \
  return D->getKind() == APIDiffItemKind::ADK_##NAME;                          \
}
#include "polarphp/ide/DigesterEnumsDef.h"

bool APIDiffItem::operator==(const APIDiffItem &Other) const {
   if (getKind() != Other.getKind())
      return false;
   if (getKey() != Other.getKey())
      return false;
   switch(getKind()) {
      case APIDiffItemKind::ADK_CommonDiffItem: {
         auto *Left = static_cast<const CommonDiffItem*>(this);
         auto *Right = static_cast<const CommonDiffItem*>(&Other);
         return
            Left->DiffKind == Right->DiffKind &&
            Left->ChildIndex == Right->ChildIndex;
      }
      case APIDiffItemKind::ADK_NoEscapeFuncParam: {
         auto *Left = static_cast<const NoEscapeFuncParam*>(this);
         auto *Right = static_cast<const NoEscapeFuncParam*>(&Other);
         return Left->Index == Right->Index;
      }
      case APIDiffItemKind::ADK_TypeMemberDiffItem:
      case APIDiffItemKind::ADK_OverloadedFuncInfo:
      case APIDiffItemKind::ADK_SpecialCaseDiffItem:
         return true;
   }
   llvm_unreachable("unhandled kind");
}

namespace {
enum class DiffItemKeyKind {
#define DIFF_ITEM_KEY_KIND(NAME) KK_##NAME,
#include "polarphp/ide/DigesterEnumsDef.h"
};

static const char* getKeyContent(DiffItemKeyKind KK) {
   switch (KK) {
#define DIFF_ITEM_KEY_KIND(NAME) case DiffItemKeyKind::KK_##NAME: return #NAME;
#include "polarphp/ide/DigesterEnumsDef.h"
   }
   llvm_unreachable("unhandled kind");
}

static DiffItemKeyKind parseKeyKind(StringRef Content) {
   return llvm::StringSwitch<DiffItemKeyKind>(Content)
#define DIFF_ITEM_KEY_KIND(NAME) .Case(#NAME, DiffItemKeyKind::KK_##NAME)
#include "polarphp/ide/DigesterEnumsDef.h"
      ;
}

static APIDiffItemKind parseDiffItemKind(StringRef Content) {
   return llvm::StringSwitch<APIDiffItemKind>(Content)
#define DIFF_ITEM_KIND(NAME) .Case(#NAME, APIDiffItemKind::ADK_##NAME)
#include "polarphp/ide/DigesterEnumsDef.h"
      ;
}

static StringRef getScalarString(llvm::yaml::Node *N) {
   auto WithQuote = cast<llvm::yaml::ScalarNode>(N)->getRawValue();
   return WithQuote.substr(1, WithQuote.size() - 2);
}

static int getScalarInt(llvm::yaml::Node *N) {
   return std::stoi(cast<llvm::yaml::ScalarNode>(N)->getRawValue());
}

static APIDiffItem*
serializeDiffItem(llvm::BumpPtrAllocator &Alloc,
                  llvm::yaml::MappingNode* Node) {
#define DIFF_ITEM_KEY_KIND_STRING(NAME) StringRef NAME;
#define DIFF_ITEM_KEY_KIND_INT(NAME) Optional<int> NAME;
#include "polarphp/ide/DigesterEnumsDef.h"
   for (auto &Pair : *Node) {
      switch(parseKeyKind(getScalarString(Pair.getKey()))) {
#define DIFF_ITEM_KEY_KIND_STRING(NAME)                                       \
    case DiffItemKeyKind::KK_##NAME:                                          \
      NAME = getScalarString(Pair.getValue()); break;
#define DIFF_ITEM_KEY_KIND_INT(NAME)                                          \
    case DiffItemKeyKind::KK_##NAME:                                          \
      NAME = getScalarInt(Pair.getValue()); break;
#include "polarphp/ide/DigesterEnumsDef.h"
      }
   }
   switch (parseDiffItemKind(DiffItemKind)) {
      case APIDiffItemKind::ADK_CommonDiffItem: {
         return new (Alloc.Allocate<CommonDiffItem>())
            CommonDiffItem(*parseSDKNodeKind(NodeKind),
                           parseSDKNodeAnnotation(NodeAnnotation), ChildIndex,
                           LeftUsr, RightUsr, LeftComment, RightComment, ModuleName);
      }
      case APIDiffItemKind::ADK_TypeMemberDiffItem: {
         Optional<uint8_t> SelfIndexShort;
         Optional<uint8_t> RemovedIndexShort;
         if (SelfIndex)
            SelfIndexShort = SelfIndex.getValue();
         if (RemovedIndex)
            RemovedIndexShort = RemovedIndex.getValue();
         return new (Alloc.Allocate<TypeMemberDiffItem>())
            TypeMemberDiffItem(Usr, NewTypeName, NewPrintedName, SelfIndexShort,
                               RemovedIndexShort, OldTypeName, OldPrintedName);
      }
      case APIDiffItemKind::ADK_NoEscapeFuncParam: {
         return new (Alloc.Allocate<NoEscapeFuncParam>())
            NoEscapeFuncParam(Usr, Index.getValue());
      }
      case APIDiffItemKind::ADK_OverloadedFuncInfo: {
         return new (Alloc.Allocate<OverloadedFuncInfo>()) OverloadedFuncInfo(Usr);
      }
      case APIDiffItemKind::ADK_SpecialCaseDiffItem: {
         return new (Alloc.Allocate<SpecialCaseDiffItem>())
            SpecialCaseDiffItem(Usr, SpecialCaseId);
      }
   }
   llvm_unreachable("unhandled kind");
}
} // end anonymous namespace

namespace polar {
namespace json {
template<>
struct ScalarEnumerationTraits<APIDiffItemKind> {
   static void enumeration(Output &out, APIDiffItemKind &value) {
#define DIFF_ITEM_KIND(X) out.enumCase(value, #X, APIDiffItemKind::ADK_##X);
#include "polarphp/ide/DigesterEnumsDef.h"
   }
};

template<>
struct ScalarEnumerationTraits<NodeAnnotation> {
   static void enumeration(Output &out, NodeAnnotation &value) {
#define NODE_ANNOTATION(X) out.enumCase(value, #X, NodeAnnotation::X);
#include "polarphp/ide/DigesterEnumsDef.h"
   }
};
template<>
struct ObjectTraits<APIDiffItem*> {
   static void mapping(Output &out, APIDiffItem *&value) {
      switch (value->getKind()) {
         case APIDiffItemKind::ADK_CommonDiffItem: {
            CommonDiffItem *Item = cast<CommonDiffItem>(value);
            auto ItemKind = Item->getKind();
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_DiffItemKind), ItemKind);
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_NodeKind),
                            Item->NodeKind);
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_NodeAnnotation),
                            Item->DiffKind);
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_ChildIndex),
                            Item->ChildIndex);
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_LeftUsr),
                            Item->LeftUsr);
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_LeftComment),
                            Item->LeftComment);
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_RightUsr),
                            Item->RightUsr);
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_RightComment),
                            Item->RightComment);
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_ModuleName),
                            Item->ModuleName);
            return;
         }
         case APIDiffItemKind::ADK_TypeMemberDiffItem: {
            TypeMemberDiffItem *Item = cast<TypeMemberDiffItem>(value);
            auto ItemKind = Item->getKind();
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_DiffItemKind),
                            ItemKind);
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_Usr), Item->usr);
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_OldPrintedName),
                            Item->oldPrintedName);
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_OldTypeName),
                            Item->oldTypeName);
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_NewPrintedName),
                            Item->newPrintedName);
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_NewTypeName),
                            Item->newTypeName);
            out.mapOptional(getKeyContent(DiffItemKeyKind::KK_SelfIndex),
                            Item->selfIndex);
            return;
         }
         case APIDiffItemKind::ADK_NoEscapeFuncParam: {
            NoEscapeFuncParam *Item = cast<NoEscapeFuncParam>(value);
            auto ItemKind = Item->getKind();
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_DiffItemKind), ItemKind);
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_Usr), Item->Usr);
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_Index), Item->Index);
            return;
         }
         case APIDiffItemKind::ADK_OverloadedFuncInfo: {
            OverloadedFuncInfo *Item = cast<OverloadedFuncInfo>(value);
            auto ItemKind = Item->getKind();
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_DiffItemKind), ItemKind);
            out.mapRequired(getKeyContent(DiffItemKeyKind::KK_Usr), Item->Usr);
            return;
         }
         case APIDiffItemKind::ADK_SpecialCaseDiffItem:
            llvm_unreachable("This entry should be authored only.");
      }
   }
};
template<>
struct ArrayTraits<ArrayRef<APIDiffItem*>> {
static size_t size(Output &out, ArrayRef<APIDiffItem *> &seq) {
   return seq.size();
}
static APIDiffItem *&element(Output &, ArrayRef<APIDiffItem *> &seq,
                             size_t index) {
   return const_cast<APIDiffItem *&>(seq[index]);
}
};

template<>
struct ObjectTraits<NameCorrectionInfo> {
   static void mapping(Output &out, NameCorrectionInfo &value) {
      out.mapRequired(getKeyContent(DiffItemKeyKind::KK_OldPrintedName),value.OriginalName);
      out.mapRequired(getKeyContent(DiffItemKeyKind::KK_NewPrintedName), value.CorrectedName);
      out.mapRequired(getKeyContent(DiffItemKeyKind::KK_ModuleName), value.ModuleName);
   }
};
template<>
struct ArrayTraits<ArrayRef<NameCorrectionInfo>> {
static size_t size(Output &out, ArrayRef<NameCorrectionInfo> &seq) {
   return seq.size();
}
static NameCorrectionInfo &element(Output &, ArrayRef<NameCorrectionInfo> &seq,
                                   size_t index) {
   return const_cast<NameCorrectionInfo&>(seq[index]);
}
};
} // namespace json
} // namespace polar

void polar::ide::api::APIDiffItemStore::
serialize(llvm::raw_ostream &os, ArrayRef<APIDiffItem*> Items) {
   json::Output yout(os);
   yout << Items;
}

void polar::ide::api::APIDiffItemStore::
serialize(llvm::raw_ostream &os, ArrayRef<NameCorrectionInfo> Items) {
   json::Output yout(os);
   yout << Items;
}

struct polar::ide::api::APIDiffItemStore::Implementation {
private:
   DiagnosticEngine &Diags;
   llvm::SmallVector<std::unique_ptr<llvm::MemoryBuffer>, 2> AllBuffer;
   llvm::BumpPtrAllocator Allocator;

   static bool shouldInclude(APIDiffItem *Item) {
      if (auto *CI = dyn_cast<CommonDiffItem>(Item)) {
         if (CI->rightCommentUnderscored())
            return false;

         // Ignore constructor's return value rewritten.
         if (CI->DiffKind == NodeAnnotation::TypeRewritten &&
             CI->NodeKind == SDKNodeKind::DeclConstructor &&
             CI->getChildIndices().front() == 0) {
            return false;
         }
      }
      return true;
   }

public:
   Implementation(DiagnosticEngine &Diags): Diags(Diags) {}
   llvm::StringMap<std::vector<APIDiffItem*>> Data;
   bool PrintUsr;
   std::vector<APIDiffItem*> AllItems;
   llvm::StringSet<> PrintedUsrs;
   void addStorePath(StringRef FileName) {
      llvm::MemoryBuffer *pMemBuffer = nullptr;
      {
         auto FileBufOrErr = llvm::MemoryBuffer::getFileOrSTDIN(FileName);
         if (!FileBufOrErr) {
            Diags.diagnose(SourceLoc(), diag::cannot_find_migration_script,
                           FileName);
            return;
         }
         pMemBuffer = FileBufOrErr->get();
         AllBuffer.push_back(std::move(FileBufOrErr.get()));
      }
      assert(pMemBuffer);
      StringRef Buffer = pMemBuffer->getBuffer();
      llvm::SourceMgr SM;
      llvm::yaml::Stream Stream(Buffer, SM);
      for (auto DI = Stream.begin(); DI != Stream.end(); ++ DI) {
         auto Array = cast<llvm::yaml::SequenceNode>(DI->getRoot());
         for (auto It = Array->begin(); It != Array->end(); ++ It) {
            APIDiffItem *Item = serializeDiffItem(Allocator,
                                                  cast<llvm::yaml::MappingNode>(&*It));
            auto &Bag = Data[Item->getKey()];
            if (shouldInclude(Item) && std::find_if(Bag.begin(), Bag.end(),
                                                    [&](APIDiffItem* I) { return *Item == *I; }) == Bag.end()) {
               Bag.push_back(Item);
               AllItems.push_back(Item);
            }
         }
      }
   }
};

ArrayRef<APIDiffItem*> polar::ide::api::APIDiffItemStore::
getDiffItems(StringRef Key) const {
   if (Impl.PrintUsr && !Impl.PrintedUsrs.count(Key)) {
      llvm::outs() << Key << "\n";
      Impl.PrintedUsrs.insert(Key);
   }
   return Impl.Data[Key];
}

ArrayRef<APIDiffItem*> polar::ide::api::APIDiffItemStore::
getAllDiffItems() const { return Impl.AllItems; }

polar::ide::api::APIDiffItemStore::APIDiffItemStore(DiagnosticEngine &Diags) :
   Impl(*new Implementation(Diags)) {}

polar::ide::api::APIDiffItemStore::~APIDiffItemStore() { delete &Impl; }

void polar::ide::api::APIDiffItemStore::addStorePath(StringRef Path) {
   Impl.addStorePath(Path);
}

void polar::ide::api::APIDiffItemStore::printIncomingUsr(bool print) {
   Impl.PrintUsr = print;
}
