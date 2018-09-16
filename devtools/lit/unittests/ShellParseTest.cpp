// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/16.

#include <gtest/gtest.h>
#include "ShellUtil.h"
#include <filesystem>

using polar::lit::ShParser;
using polar::lit::ShellTokenType;
using polar::lit::Pipeline;
using polar::lit::Seq;
using polar::lit::Command;
using polar::lit::AbstractCommand;

TEST(ShellParseTest, testBasic)
{
   {
      std::shared_ptr<AbstractCommand> command = ShParser("echo hello").parse();
      ASSERT_EQ(command->getCommandType(), AbstractCommand::Type::Pipeline);
      Pipeline *pipeCommand = dynamic_cast<Pipeline *>(command.get());
      ASSERT_TRUE(pipeCommand != nullptr);
      ASSERT_EQ(pipeCommand->getCommands().size(), 1);
      ASSERT_FALSE(pipeCommand->isNegate());
      std::shared_ptr<AbstractCommand> subAbstractCommand = pipeCommand->getCommands().front();
      ASSERT_EQ(subAbstractCommand->getCommandType(), AbstractCommand::Type::Command);
      Command *subCommand = dynamic_cast<Command *>(subAbstractCommand.get());
      ASSERT_TRUE(subCommand != nullptr);
      ASSERT_EQ(subCommand->getArgs().size(), 2);
      const std::list<std::any> &args = subCommand->getArgs();
      std::list<std::any>::const_iterator argIter = args.begin();
      ASSERT_TRUE(argIter->has_value());
      ASSERT_EQ(argIter->type(), typeid(ShellTokenType));
      ASSERT_EQ(std::get<0>(std::any_cast<ShellTokenType>(*argIter)), "echo");
      ++argIter;
      ASSERT_TRUE(argIter->has_value());
      ASSERT_EQ(argIter->type(), typeid(ShellTokenType));
      ASSERT_EQ(std::get<0>(std::any_cast<ShellTokenType>(*argIter)), "hello");
   }
   {
      std::shared_ptr<AbstractCommand> command = ShParser("echo \"\"").parse();
      ASSERT_EQ(command->getCommandType(), AbstractCommand::Type::Pipeline);
      Pipeline *pipeCommand = dynamic_cast<Pipeline *>(command.get());
      ASSERT_TRUE(pipeCommand != nullptr);
      ASSERT_EQ(pipeCommand->getCommands().size(), 1);
      ASSERT_FALSE(pipeCommand->isNegate());
      std::shared_ptr<AbstractCommand> subAbstractCommand = pipeCommand->getCommands().front();
      ASSERT_EQ(subAbstractCommand->getCommandType(), AbstractCommand::Type::Command);
      Command *subCommand = dynamic_cast<Command *>(subAbstractCommand.get());
      ASSERT_TRUE(subCommand != nullptr);
      ASSERT_EQ(subCommand->getArgs().size(), 2);
      const std::list<std::any> &args = subCommand->getArgs();
      std::list<std::any>::const_iterator argIter = args.begin();
      ASSERT_TRUE(argIter->has_value());
      ASSERT_EQ(argIter->type(), typeid(ShellTokenType));
      ASSERT_EQ(std::get<0>(std::any_cast<ShellTokenType>(*argIter)), "echo");
      ++argIter;
      ASSERT_TRUE(argIter->has_value());
      ASSERT_EQ(argIter->type(), typeid(ShellTokenType));
      ASSERT_EQ(std::get<0>(std::any_cast<ShellTokenType>(*argIter)), "");
   }
   {
      std::shared_ptr<AbstractCommand> command = ShParser(R"(echo -DFOO='a')").parse();
      ASSERT_EQ(command->getCommandType(), AbstractCommand::Type::Pipeline);
      Pipeline *pipeCommand = dynamic_cast<Pipeline *>(command.get());
      ASSERT_TRUE(pipeCommand != nullptr);
      ASSERT_EQ(pipeCommand->getCommands().size(), 1);
      ASSERT_FALSE(pipeCommand->isNegate());
      std::shared_ptr<AbstractCommand> subAbstractCommand = pipeCommand->getCommands().front();
      ASSERT_EQ(subAbstractCommand->getCommandType(), AbstractCommand::Type::Command);
      Command *subCommand = dynamic_cast<Command *>(subAbstractCommand.get());
      ASSERT_TRUE(subCommand != nullptr);
      ASSERT_EQ(subCommand->getArgs().size(), 2);
      const std::list<std::any> &args = subCommand->getArgs();
      std::list<std::any>::const_iterator argIter = args.begin();
      ASSERT_TRUE(argIter->has_value());
      ASSERT_EQ(argIter->type(), typeid(ShellTokenType));
      ASSERT_EQ(std::get<0>(std::any_cast<ShellTokenType>(*argIter)), "echo");
      ++argIter;
      ASSERT_TRUE(argIter->has_value());
      ASSERT_EQ(argIter->type(), typeid(ShellTokenType));
      ASSERT_EQ(std::get<0>(std::any_cast<ShellTokenType>(*argIter)), "-DFOO=a");
   }
   {
      std::shared_ptr<AbstractCommand> command = ShParser(R"(echo -DFOO="a")").parse();
      ASSERT_EQ(command->getCommandType(), AbstractCommand::Type::Pipeline);
      Pipeline *pipeCommand = dynamic_cast<Pipeline *>(command.get());
      ASSERT_TRUE(pipeCommand != nullptr);
      ASSERT_EQ(pipeCommand->getCommands().size(), 1);
      ASSERT_FALSE(pipeCommand->isNegate());
      std::shared_ptr<AbstractCommand> subAbstractCommand = pipeCommand->getCommands().front();
      ASSERT_EQ(subAbstractCommand->getCommandType(), AbstractCommand::Type::Command);
      Command *subCommand = dynamic_cast<Command *>(subAbstractCommand.get());
      ASSERT_TRUE(subCommand != nullptr);
      ASSERT_EQ(subCommand->getArgs().size(), 2);
      const std::list<std::any> &args = subCommand->getArgs();
      std::list<std::any>::const_iterator argIter = args.begin();
      ASSERT_TRUE(argIter->has_value());
      ASSERT_EQ(argIter->type(), typeid(ShellTokenType));
      ASSERT_EQ(std::get<0>(std::any_cast<ShellTokenType>(*argIter)), "echo");
      ++argIter;
      ASSERT_TRUE(argIter->has_value());
      ASSERT_EQ(argIter->type(), typeid(ShellTokenType));
      ASSERT_EQ(std::get<0>(std::any_cast<ShellTokenType>(*argIter)), "-DFOO=a");
   }
}
