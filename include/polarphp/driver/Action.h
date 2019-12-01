// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/26.

#ifndef POLARPHP_DRIVER_ACTION_H
#define POLARPHP_DRIVER_ACTION_H

#include "polarphp/basic/FileTypes.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/driver/Utils.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/Support/Chrono.h"

namespace llvm::opt {
class Arg;
} // llvm::opt

namespace polar::driver {

class Action
{
public:
   using size_type = ArrayRef<const Action *>::size_type;
   using iterator = ArrayRef<const Action *>::iterator;
   using const_iterator = ArrayRef<const Action *>::const_iterator;

   enum class Kind : unsigned
   {
      Input = 0,
      CompileJob,
      InterpretJob,
      BackendJob,
      MergeModuleJob,
      ModuleWrapJob,
      AutolinkExtractJob,
      REPLJob,
      DynamicLinkJob,
      StaticLinkJob,
      GenerateDSYMJob,
      VerifyDebugInfoJob,
      GeneratePCHJob,

      JobFirst = CompileJob,
      JobLast = GeneratePCHJob
   };

   static const char *getClassName(Kind AC);

private:
   unsigned m_rawKind : 4;
   unsigned m_type : 28;

   friend class Compilation;
   /// Actions must be created through Compilation::createAction.
   void *operator new(size_t size)
   {
      return ::operator new(size);
   }

protected:
   Action(Kind kind, filetypes::FileTypeId type)
      : m_rawKind(unsigned(kind)),
        m_type(type)
   {
      assert(kind == getKind() && "not enough bits");
      assert(type == getType() && "not enough bits");
   }

   virtual void anchor();

public:
   virtual ~Action() = default;
   const char *getClassName() const
   {
      return Action::getClassName(getKind());
   }

   Kind getKind() const
   {
      return static_cast<Kind>(m_rawKind);
   }

   filetypes::FileTypeId getType() const
   {
      return static_cast<filetypes::FileTypeId>(m_type);
   }
};

class InputAction : public Action
{
public:
   InputAction(const llvm::opt::Arg &input, filetypes::FileTypeId type)
      : Action(Action::Kind::Input, type),
        m_input(input)
   {}

   const llvm::opt::Arg &getInputArg() const
   {
      return m_input;
   }

   static bool classof(const Action *action)
   {
      return action->getKind() == Action::Kind::Input;
   }

protected:
   virtual void anchor();

private:
   const llvm::opt::Arg &m_input;
};

class JobAction : public Action
{
public:
   ArrayRef<const Action *> getInputs() const
   {
      return m_inputs;
   }

   void addInput(const Action *input)
   {
      m_inputs.push_back(input);
   }

   size_type size() const
   {
      return m_inputs.size();
   }

   iterator begin()
   {
      return m_inputs.begin();
   }

   iterator end()
   {
      return m_inputs.end();
   }

   const_iterator begin() const
   {
      return m_inputs.begin();
   }

   const_iterator end() const
   {
      return m_inputs.end();
   }

   // Returns the index of the input action's output file which is used as
   // (single) input to this action. Most actions produce only a single output
   // file, so we return 0 by default.
   virtual size_t getInputIndex() const
   {
      return 0;
   }

   static bool classof(const Action *action)
   {
      return (action->getKind() >= Kind::JobFirst &&
              action->getKind() <= Kind::JobLast);
   }

protected:
   JobAction(Kind kind, ArrayRef<const Action *> inputs,
             filetypes::FileTypeId type)
      : Action(kind, type),
        m_inputs(inputs)
   {}

   virtual void anchor();

private:
   TinyPtrVector<const Action *> m_inputs;
};

class CompileJobAction : public JobAction
{
public:
   struct InputInfo
   {
      enum Status
      {
         UpToDate,
         NeedsCascadingBuild,
         NeedsNonCascadingBuild,
         NewlyAdded
      };
      Status status = UpToDate;
      llvm::sys::TimePoint<> previousModTime;

      InputInfo() = default;
      InputInfo(Status stat, llvm::sys::TimePoint<> time)
         : status(stat),
           previousModTime(time)
      {}

      static InputInfo makeNewlyAdded()
      {
         return InputInfo(Status::NewlyAdded, llvm::sys::TimePoint<>::max());
      }
   };

public:
   CompileJobAction(filetypes::FileTypeId outputType)
      : JobAction(Action::Kind::CompileJob, None, outputType),
        m_inputInfo()
   {}

   CompileJobAction(Action *input, filetypes::FileTypeId outputType, InputInfo info)
      : JobAction(Action::Kind::CompileJob, input, outputType),
        m_inputInfo(info)
   {}

   InputInfo getInputInfo() const
   {
      return m_inputInfo;
   }

   static bool classof(const Action *action)
   {
      return action->getKind() == Action::Kind::CompileJob;
   }

protected:
   virtual void anchor();

private:
   InputInfo m_inputInfo;
};

class InterpretJobAction : public JobAction
{
public:
   explicit InterpretJobAction()
      : JobAction(Action::Kind::InterpretJob, llvm::None,
                  filetypes::TY_Nothing)
   {}

   static bool classof(const Action *action)
   {
      return action->getKind() == Action::Kind::InterpretJob;
   }

protected:
   virtual void anchor();
};

class BackendJobAction : public JobAction
{
public:
   BackendJobAction(const Action *input, filetypes::FileTypeId outputType,
                    size_t inputIndex)
      : JobAction(Action::Kind::BackendJob, input, outputType),
        m_inputIndex(inputIndex)
   {}

   static bool classof(const Action *action)
   {
      return action->getKind() == Action::Kind::BackendJob;
   }

   virtual size_t getInputIndex() const
   {
      return m_inputIndex;
   }

protected:
   virtual void anchor();

private:
   // In case of multi-threaded compilation, the compile-action produces multiple
   // output bitcode-files. For each bitcode-file a BackendJobAction is created.
   // This index specifies which of the files to select for the input.
   size_t m_inputIndex;
};

class REPLJobAction : public JobAction
{
public:
   enum class Mode
   {
      Integrated,
      PreferLLDB,
      RequireLLDB
   };
public:
   REPLJobAction(Mode mode)
      : JobAction(Action::Kind::REPLJob, llvm::None,
                  filetypes::TY_Nothing),
        m_requestedMode(mode)
   {}

   Mode getRequestedMode() const
   {
      return m_requestedMode;
   }

   static bool classof(const Action *action)
   {
      return action->getKind() == Action::Kind::REPLJob;
   }

protected:
   virtual void anchor();

private:
   Mode m_requestedMode;
};

class MergeModuleJobAction : public JobAction
{
public:
   MergeModuleJobAction(ArrayRef<const Action *> inputs)
      : JobAction(Action::Kind::MergeModuleJob, inputs,
                  filetypes::TY_PolarModuleFile)
   {}

   static bool classof(const Action *action)
   {
      return action->getKind() == Action::Kind::MergeModuleJob;
   }
protected:
   virtual void anchor();
};

class ModuleWrapJobAction : public JobAction
{
public:
   ModuleWrapJobAction(ArrayRef<const Action *> inputs)
      : JobAction(Action::Kind::ModuleWrapJob, inputs,
                  filetypes::TY_Object)
   {}

   static bool classof(const Action *action)
   {
      return action->getKind() == Action::Kind::ModuleWrapJob;
   }

protected:
   virtual void anchor();
};

class AutolinkExtractJobAction : public JobAction
{
public:
   AutolinkExtractJobAction(ArrayRef<const Action *> inputs)
      : JobAction(Action::Kind::AutolinkExtractJob, inputs,
                  filetypes::TY_AutolinkFile) {}

   static bool classof(const Action *action)
   {
      return action->getKind() == Action::Kind::AutolinkExtractJob;
   }

protected:
   virtual void anchor();
};

class GenerateDSYMJobAction : public JobAction
{
public:
   explicit GenerateDSYMJobAction(const Action *input)
      : JobAction(Action::Kind::GenerateDSYMJob, input,
                  filetypes::TY_dSYM)
   {}

   static bool classof(const Action *action)
   {
      return action->getKind() == Action::Kind::GenerateDSYMJob;
   }

protected:
   virtual void anchor();
};

class VerifyDebugInfoJobAction : public JobAction
{
public:
   explicit VerifyDebugInfoJobAction(const Action *input)
      : JobAction(Action::Kind::VerifyDebugInfoJob, input,
                  filetypes::TY_Nothing) {}

   static bool classof(const Action *action)
   {
      return action->getKind() == Action::Kind::VerifyDebugInfoJob;
   }

protected:
   virtual void anchor();
};

class GeneratePCHJobAction : public JobAction
{
public:
   GeneratePCHJobAction(const Action *input, StringRef persistentPCHDir)
      : JobAction(Action::Kind::GeneratePCHJob, input,
                  persistentPCHDir.empty() ? filetypes::TY_PCH
                                           : filetypes::TY_Nothing),
        m_persistentPCHDir(persistentPCHDir)
   {}

   bool isPersistentPCH() const
   {
      return !m_persistentPCHDir.empty();
   }

   StringRef getPersistentPCHDir() const
   {
      return m_persistentPCHDir;
   }

   static bool classof(const Action *action)
   {
      return action->getKind() == Action::Kind::GeneratePCHJob;
   }

protected:
   virtual void anchor();

private:
   std::string m_persistentPCHDir;
};

class DynamicLinkJobAction : public JobAction
{
public:
   DynamicLinkJobAction(ArrayRef<const Action *> inputs, LinkKind linkKind)
      : JobAction(Action::Kind::DynamicLinkJob, inputs, filetypes::TY_Image),
        m_kind(linkKind) {
      assert(m_kind != LinkKind::None && m_kind != LinkKind::StaticLibrary);
   }

   LinkKind getKind() const
   {
      return m_kind;
   }

   static bool classof(const Action *action)
   {
      return action->getKind() == Action::Kind::DynamicLinkJob;
   }

protected:
   virtual void anchor();

private:
   LinkKind m_kind;
};

class StaticLinkJobAction : public JobAction
{
   virtual void anchor();
public:
   StaticLinkJobAction(ArrayRef<const Action *> inputs, LinkKind linkKind)
      : JobAction(Action::Kind::StaticLinkJob, inputs, filetypes::TY_Image)
   {
      assert(linkKind == LinkKind::StaticLibrary);
   }

   static bool classof(const Action *action)
   {
      return action->getKind() == Action::Kind::StaticLinkJob;
   }
};

} // polar::driver

#endif // POLARPHP_DRIVER_ACTION_H
