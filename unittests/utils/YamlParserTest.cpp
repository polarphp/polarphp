// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/15.

#include "polarphp/utils/yaml/YamlParser.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/Casting.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/SourceMgr.h"

#include "gtest/gtest.h"

using namespace polar;
using namespace polar::utils;
using namespace polar::basic;

namespace {

static void SuppressDiagnosticsOutput(const SMDiagnostic &, void *) {
   // Prevent SourceMgr from writing errors to stderr
   // to reduce noise in unit test runs.
}

// Assumes Ctx is an SMDiagnostic where Diag can be stored.
static void CollectDiagnosticsOutput(const SMDiagnostic &Diag, void *Ctx) {
   SMDiagnostic* DiagOut = static_cast<SMDiagnostic*>(Ctx);
   *DiagOut = Diag;
}

// Checks that the given input gives a parse error. Makes sure that an error
// text is available and the parse fails.
static void ExpectParseError(StringRef Message, StringRef Input) {
   SourceMgr SM;
   yaml::Stream Stream(Input, SM);
   SM.setDiagHandler(SuppressDiagnosticsOutput);
   EXPECT_FALSE(Stream.validate()) << Message.getStr() << ": " << Input.getStr();
   EXPECT_TRUE(Stream.failed()) << Message.getStr() << ": " << Input.getStr();
}

// Checks that the given input can be parsed without error.
static void ExpectParseSuccess(StringRef Message, StringRef Input) {
   SourceMgr SM;
   yaml::Stream Stream(Input, SM);
   EXPECT_TRUE(Stream.validate()) << Message << ": " << Input;
}

TEST(YamlParserTest, testParsesEmptyArray)
{
   ExpectParseSuccess("Empty array", "[]");
}

TEST(YamlParserTest, testFailsIfNotClosingArray)
{
   ExpectParseError("Not closing array", "[");
   ExpectParseError("Not closing array", "  [  ");
   ExpectParseError("Not closing array", "  [x");
}

TEST(YamlParserTest, testParsesEmptyArrayWithWhitespace)
{
   ExpectParseSuccess("Array with spaces", "  [  ]  ");
   ExpectParseSuccess("All whitespaces", "\t\r\n[\t\n \t\r ]\t\r \n\n");
}

TEST(YamlParserTest, testParsesEmptyObject)
{
   ExpectParseSuccess("Empty object", "[{}]");
}

TEST(YamlParserTest, testParsesObject)
{
   ExpectParseSuccess("Object with an entry", "[{\"a\":\"/b\"}]");
}

TEST(YamlParserTest, testParsesMultipleKeyValuePairsInObject)
{
   ExpectParseSuccess("Multiple key, value pairs",
                      "[{\"a\":\"/b\",\"c\":\"d\",\"e\":\"f\"}]");
}

TEST(YamlParserTest, testFailsIfNotClosingObject)
{
   ExpectParseError("Missing close on empty", "[{]");
   ExpectParseError("Missing close after pair", "[{\"a\":\"b\"]");
}

TEST(YamlParserTest, testFailsIfMissingColon)
{
   ExpectParseError("Missing colon between key and value", "[{\"a\"\"/b\"}]");
   ExpectParseError("Missing colon between key and value", "[{\"a\" \"b\"}]");
}

TEST(YamlParserTest, testFailsOnMissingQuote)
{
   ExpectParseError("Missing open quote", "[{a\":\"b\"}]");
   ExpectParseError("Missing closing quote", "[{\"a\":\"b}]");
}

TEST(YamlParserTest, testParsesEscapedQuotes)
{
   ExpectParseSuccess("Parses escaped string in key and value",
                      "[{\"a\":\"\\\"b\\\"  \\\" \\\"\"}]");
}

TEST(YamlParserTest, testParsesEmptyString)
{
   ExpectParseSuccess("Parses empty string in value", "[{\"a\":\"\"}]");
}

TEST(YamlParserTest, testParsesMultipleObjects)
{
   ExpectParseSuccess(
            "Multiple objects in array",
            "["
            " { \"a\" : \"b\" },"
            " { \"a\" : \"b\" },"
            " { \"a\" : \"b\" }"
            "]");
}

TEST(YamlParserTest, testFailsOnMissingComma)
{
   ExpectParseError(
            "Missing comma",
            "["
            " { \"a\" : \"b\" }"
            " { \"a\" : \"b\" }"
            "]");
}

TEST(YamlParserTest, testParsesSpacesInBetweenTokens)
{
   ExpectParseSuccess(
            "Various whitespace between tokens",
            " \t \n\n \r [ \t \n\n \r"
            " \t \n\n \r { \t \n\n \r\"a\"\t \n\n \r :"
            " \t \n\n \r \"b\"\t \n\n \r } \t \n\n \r,\t \n\n \r"
            " \t \n\n \r { \t \n\n \r\"a\"\t \n\n \r :"
            " \t \n\n \r \"b\"\t \n\n \r } \t \n\n \r]\t \n\n \r");
}

TEST(YamlParserTest, testParsesArrayOfArrays)
{
   ExpectParseSuccess("Array of arrays", "[[]]");
}

TEST(YamlParserTest, testParsesBlockLiteralScalars)
{
   ExpectParseSuccess("Block literal scalar", "test: |\n  Hello\n  World\n");
   ExpectParseSuccess("Block literal scalar EOF", "test: |\n  Hello\n  World");
   ExpectParseSuccess("Empty block literal scalar header EOF", "test: | ");
   ExpectParseSuccess("Empty block literal scalar", "test: |\ntest2: 20");
   ExpectParseSuccess("Empty block literal scalar 2", "- | \n  \n\n \n- 42");
   ExpectParseSuccess("Block literal scalar in sequence",
                      "- |\n  Testing\n  Out\n\n- 22");
   ExpectParseSuccess("Block literal scalar in document",
                      "--- |\n  Document\n...");
   ExpectParseSuccess("Empty non indented lines still count",
                      "- |\n  First line\n \n\n  Another line\n\n- 2");
   ExpectParseSuccess("Comment in block literal scalar header",
                      "test: | # Comment \n  No Comment\ntest 2: | # Void");
   ExpectParseSuccess("Chomping indicators in block literal scalar header",
                      "test: |- \n  Hello\n\ntest 2: |+ \n\n  World\n\n\n");
   ExpectParseSuccess("Indent indicators in block literal scalar header",
                      "test: |1 \n  \n Hello \n  World\n");
   ExpectParseSuccess("Chomping and indent indicators in block literals",
                      "test: |-1\n Hello\ntest 2: |9+\n         World");
   ExpectParseSuccess("Trailing comments in block literals",
                      "test: |\n  Content\n # Trailing\n  #Comment\ntest 2: 3");
   ExpectParseError("Invalid block scalar header", "test: | failure");
   ExpectParseError("Invalid line indentation", "test: |\n  First line\n Error");
   ExpectParseError("Long leading space line", "test: |\n   \n  Test\n");
}

TEST(YamlParserTest, testNullTerminatedBlockScalars)
{
   SourceMgr SM;
   yaml::Stream Stream("test: |\n  Hello\n  World\n", SM);
   yaml::Document &Doc = *Stream.begin();
   yaml::MappingNode *Map = cast<yaml::MappingNode>(Doc.getRoot());
   StringRef Value =
         cast<yaml::BlockScalarNode>(Map->begin()->getValue())->getValue();

   EXPECT_EQ(Value, "Hello\nWorld\n");
   EXPECT_EQ(Value.getData()[Value.size()], '\0');
}

TEST(YamlParserTest, testHandlesEndOfFileGracefully)
{
   ExpectParseError("In string starting with EOF", "[\"");
   ExpectParseError("In string hitting EOF", "[\"   ");
   ExpectParseError("In string escaping EOF", "[\"  \\");
   ExpectParseError("In array starting with EOF", "[");
   ExpectParseError("In array element starting with EOF", "[[], ");
   ExpectParseError("In array hitting EOF", "[[] ");
   ExpectParseError("In array hitting EOF", "[[]");
   ExpectParseError("In object hitting EOF", "{\"\"");
}

TEST(YamlParserTest, testHandlesNullValuesInKeyValueNodesGracefully)
{
   ExpectParseError("KeyValueNode with null key", "? \"\n:");
   ExpectParseError("KeyValueNode with null value", "test: '");
}

// Checks that the given string can be parsed into an identical string inside
// of an array.
static void ExpectCanParseString(StringRef String)
{
   std::string StringInArray = (Twine("[\"") + String + "\"]").getStr();
   SourceMgr SM;
   yaml::Stream Stream(StringInArray, SM);
   yaml::SequenceNode *ParsedSequence
         = dyn_cast<yaml::SequenceNode>(Stream.begin()->getRoot());
   StringRef ParsedString
         = dyn_cast<yaml::ScalarNode>(
            static_cast<yaml::Node*>(ParsedSequence->begin()))->getRawValue();
   ParsedString = ParsedString.substr(1, ParsedString.size() - 2);
   EXPECT_EQ(String, ParsedString.getStr());
}

// Checks that parsing the given string inside an array fails.
static void ExpectCannotParseString(StringRef String)
{
   std::string StringInArray = (Twine("[\"") + String + "\"]").getStr();
   ExpectParseError((Twine("When parsing string \"") + String + "\"").getStr(),
                    StringInArray);
}

TEST(YamlParserTest, testParsesStrings)
{
   ExpectCanParseString("");
   ExpectCannotParseString("\\");
   ExpectCannotParseString("\"");
   ExpectCanParseString(" ");
   ExpectCanParseString("\\ ");
   ExpectCanParseString("\\\"");
   ExpectCannotParseString("\"\\");
   ExpectCannotParseString(" \\");
   ExpectCanParseString("\\\\");
   ExpectCannotParseString("\\\\\\");
   ExpectCanParseString("\\\\\\\\");
   ExpectCanParseString("\\\" ");
   ExpectCannotParseString("\\\\\" ");
   ExpectCanParseString("\\\\\\\" ");
   ExpectCanParseString("    \\\\  \\\"  \\\\\\\"   ");
}

TEST(YamlParserTest, testWorksWithIteratorAlgorithms)
{
   SourceMgr SM;
   yaml::Stream Stream("[\"1\", \"2\", \"3\", \"4\", \"5\", \"6\"]", SM);
   yaml::SequenceNode *Array
         = dyn_cast<yaml::SequenceNode>(Stream.begin()->getRoot());
   EXPECT_EQ(6, std::distance(Array->begin(), Array->end()));
}

TEST(YamlParserTest, testDefaultDiagnosticFilename)
{
   SourceMgr SM;

   SMDiagnostic GeneratedDiag;
   SM.setDiagHandler(CollectDiagnosticsOutput, &GeneratedDiag);

   // When we construct a YAML stream over an unnamed string,
   // the filename is hard-coded as "YAML".
   yaml::Stream UnnamedStream("[]", SM);
   UnnamedStream.printError(UnnamedStream.begin()->getRoot(), "Hello, World!");
   EXPECT_EQ("YAML", GeneratedDiag.getFilename());
}

TEST(YamlParserTest, testDiagnosticFilenameFromBufferID)
{
   SourceMgr SM;

   SMDiagnostic GeneratedDiag;
   SM.setDiagHandler(CollectDiagnosticsOutput, &GeneratedDiag);

   // When we construct a YAML stream over a named buffer,
   // we get its ID as filename in diagnostics.
   std::unique_ptr<MemoryBuffer> Buffer =
         MemoryBuffer::getMemBuffer("[]", "buffername.yaml");
   yaml::Stream Stream(Buffer->getMemBufferRef(), SM);
   Stream.printError(Stream.begin()->getRoot(), "Hello, World!");
   EXPECT_EQ("buffername.yaml", GeneratedDiag.getFilename());
}

TEST(YamlParserTest, testSameNodeIteratorOperatorNotEquals)
{
   SourceMgr SM;
   yaml::Stream Stream("[\"1\", \"2\"]", SM);

   yaml::SequenceNode *Node = dyn_cast<yaml::SequenceNode>(
            Stream.begin()->getRoot());

   auto Begin = Node->begin();
   auto End = Node->end();

   EXPECT_TRUE(Begin != End);
   EXPECT_FALSE(Begin != Begin);
   EXPECT_FALSE(End != End);
}

TEST(YamlParserTest, testSameNodeIteratorOperatorEquals)
{
   SourceMgr SM;
   yaml::Stream Stream("[\"1\", \"2\"]", SM);

   yaml::SequenceNode *Node = dyn_cast<yaml::SequenceNode>(
            Stream.begin()->getRoot());

   auto Begin = Node->begin();
   auto End = Node->end();

   EXPECT_FALSE(Begin == End);
   EXPECT_TRUE(Begin == Begin);
   EXPECT_TRUE(End == End);
}

TEST(YamlParserTest, testDifferentNodesIteratorOperatorNotEquals)
{
   SourceMgr SM;
   yaml::Stream Stream("[\"1\", \"2\"]", SM);
   yaml::Stream AnotherStream("[\"1\", \"2\"]", SM);

   yaml::SequenceNode *Node = dyn_cast<yaml::SequenceNode>(
            Stream.begin()->getRoot());
   yaml::SequenceNode *AnotherNode = dyn_cast<yaml::SequenceNode>(
            AnotherStream.begin()->getRoot());

   auto Begin = Node->begin();
   auto End = Node->end();

   auto AnotherBegin = AnotherNode->begin();
   auto AnotherEnd = AnotherNode->end();

   EXPECT_TRUE(Begin != AnotherBegin);
   EXPECT_TRUE(Begin != AnotherEnd);
   EXPECT_FALSE(End != AnotherEnd);
}

TEST(YamlParserTest, testDifferentNodesIteratorOperatorEquals)
{
   SourceMgr SM;
   yaml::Stream Stream("[\"1\", \"2\"]", SM);
   yaml::Stream AnotherStream("[\"1\", \"2\"]", SM);

   yaml::SequenceNode *Node = dyn_cast<yaml::SequenceNode>(
            Stream.begin()->getRoot());
   yaml::SequenceNode *AnotherNode = dyn_cast<yaml::SequenceNode>(
            AnotherStream.begin()->getRoot());

   auto Begin = Node->begin();
   auto End = Node->end();

   auto AnotherBegin = AnotherNode->begin();
   auto AnotherEnd = AnotherNode->end();

   EXPECT_FALSE(Begin == AnotherBegin);
   EXPECT_FALSE(Begin == AnotherEnd);
   EXPECT_TRUE(End == AnotherEnd);
}


} // anonymous namespace

