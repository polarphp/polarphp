// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/13.

#include "polarphp/utils/Error.h"

#include "polarphp/basic/adt/Twine.h"
#include "polarphp/utils/ErrorCode.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/ManagedStatics.h"
#include "../support/Error.h"
#include "gtest/gtest-spi.h"
#include "gtest/gtest.h"
#include <memory>

using namespace polar::basic;
using namespace polar::utils;
using namespace polar;
using namespace polar::unittest;

namespace {

// Custom error class with a default base class and some random 'info' attached.
class CustomError : public ErrorInfo<CustomError>
{
public:
   // Create an error with some info attached.
   CustomError(int Info) : Info(Info) {}

   // Get the info attached to this error.
   int getInfo() const { return Info; }

   // Log this error to a stream.
   void log(RawOutStream &outstream) const override {
      outstream << "CustomError {" << getInfo() << "}";
   }

   std::error_code convertToErrorCode() const override {
      polar_unreachable("CustomError doesn't support ECError conversion");
   }

   // Used by ErrorInfo::classID.
   static char sm_id;

protected:
   // This error is subclassed below, but we can't use inheriting constructors
   // yet, so we can't propagate the constructors through ErrorInfo. Instead
   // we have to have a default constructor and have the subclass initialize all
   // fields.
   CustomError() : Info(0) {}

   int Info;
};

char CustomError::sm_id = 0;

// Custom error class with a custom base class and some additional random
// 'info'.
class CustomSubError : public ErrorInfo<CustomSubError, CustomError> {
public:
   // Create a sub-error with some info attached.
   CustomSubError(int Info, int ExtraInfo) : ExtraInfo(ExtraInfo) {
      this->Info = Info;
   }

   // Get the extra info attached to this error.
   int getExtraInfo() const { return ExtraInfo; }

   // Log this error to a stream.
   void log(RawOutStream &outstream) const override {

      outstream << "CustomSubError { " << getInfo() << ", " << getExtraInfo() << "}";
   }

   std::error_code convertToErrorCode() const override {
      polar_unreachable("CustomSubError doesn't support ECError conversion");
   }

   // Used by ErrorInfo::classID.
   static char sm_id;

protected:
   int ExtraInfo;
};

char CustomSubError::sm_id = 0;

static Error handleCustomError(const CustomError &CE)
{
   return Error::getSuccess();
}

static void handleCustomErrorVoid(const CustomError &CE)
{}

static Error handleCustomErrorUP(std::unique_ptr<CustomError> CE)
{
   return Error::getSuccess();
}

static void handleCustomErrorUPVoid(std::unique_ptr<CustomError> CE)
{}

// Test that success values implicitly convert to false, and don't cause crashes
// once they've been implicitly converted.
TEST(ErrorTest, testCheckedSuccess)
{
   Error E = Error::getSuccess();
   EXPECT_FALSE(E) << "Unexpected error while testing Error 'Success'";
}

// Test that unchecked success values cause an abort.
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
TEST(ErrorTest, UncheckedSuccess) {
   EXPECT_DEATH({ Error E = Error::getSuccess(); },
                "Program aborted due to an unhandled Error:")
         << "Unchecked Error Succes value did not cause abort()";
}
#endif

// ErrorAsOutParameter tester.
void errAsOutParamHelper(Error &Err) {
   ErrorAsOutParameter ErrAsOutParam(&Err);
   // Verify that checked flag is raised - assignment should not crash.
   Err = Error::getSuccess();
   // Raise the checked bit manually - caller should still have to test the
   // error.
   (void)!!Err;
}

// Test that ErrorAsOutParameter sets the checked flag on construction.
TEST(ErrorTest, ErrorAsOutParameterChecked)
{
   Error E = Error::getSuccess();
   errAsOutParamHelper(E);
   (void)!!E;
}

// Test that ErrorAsOutParameter clears the checked flag on destruction.
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
TEST(ErrorTest, ErrorAsOutParameterUnchecked)
{
   EXPECT_DEATH({ Error E = Error::getSuccess(); errAsOutParamHelper(E); },
                "Program aborted due to an unhandled Error:")
         << "ErrorAsOutParameter did not clear the checked flag on destruction.";
}
#endif

// Check that we abort on unhandled failure cases. (Force conversion to bool
// to make sure that we don't accidentally treat checked errors as handled).
// Test runs in debug mode only.
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
TEST(ErrorTest, UncheckedError)
{
   auto DropUnhandledError = []() {
      Error E = make_error<CustomError>(42);
      (void)!E;
   };
   EXPECT_DEATH(DropUnhandledError(),
                "Program aborted due to an unhandled Error:")
         << "Unhandled Error failure value did not cause abort()";
}
#endif

// Check 'Error::isA<T>' method handling.
TEST(ErrorTest, testIsAHandling)
{
   // Check 'isA' handling.
   Error E = make_error<CustomError>(1);
   Error F = make_error<CustomSubError>(1, 2);
   Error G = Error::getSuccess();

   EXPECT_TRUE(E.isA<CustomError>());
   EXPECT_FALSE(E.isA<CustomSubError>());
   EXPECT_TRUE(F.isA<CustomError>());
   EXPECT_TRUE(F.isA<CustomSubError>());
   EXPECT_FALSE(G.isA<CustomError>());

   consume_error(std::move(E));
   consume_error(std::move(F));
   consume_error(std::move(G));
}

// Check that we can handle a custom error.
TEST(ErrorTest, testHandleCustomError)
{
   int CaughtErrorInfo = 0;
   handle_all_errors(make_error<CustomError>(42), [&](const CustomError &CE) {
      CaughtErrorInfo = CE.getInfo();
   });

   EXPECT_TRUE(CaughtErrorInfo == 42) << "Wrong result from CustomError handler";
}

// Check that handler type deduction also works for handlers
// of the following types:
// void (const Err&)
// Error (const Err&) mutable
// void (const Err&) mutable
// Error (Err&)
// void (Err&)
// Error (Err&) mutable
// void (Err&) mutable
// Error (unique_ptr<Err>)
// void (unique_ptr<Err>)
// Error (unique_ptr<Err>) mutable
// void (unique_ptr<Err>) mutable
TEST(ErrorTest, testHandlerTypeDeduction)
{

   handle_all_errors(make_error<CustomError>(42), [](const CustomError &CE) {});

   handle_all_errors(
            make_error<CustomError>(42),
            [](const CustomError &CE) mutable  -> Error { return Error::getSuccess(); });

   handle_all_errors(make_error<CustomError>(42),
                     [](const CustomError &CE) mutable {});

   handle_all_errors(make_error<CustomError>(42),
                     [](CustomError &CE) -> Error { return Error::getSuccess(); });

   handle_all_errors(make_error<CustomError>(42), [](CustomError &CE) {});

   handle_all_errors(make_error<CustomError>(42),
                     [](CustomError &CE) mutable -> Error { return Error::getSuccess(); });

   handle_all_errors(make_error<CustomError>(42), [](CustomError &CE) mutable {});

   handle_all_errors(
            make_error<CustomError>(42),
            [](std::unique_ptr<CustomError> CE) -> Error { return Error::getSuccess(); });

   handle_all_errors(make_error<CustomError>(42),
                     [](std::unique_ptr<CustomError> CE) {});

   handle_all_errors(
            make_error<CustomError>(42),
            [](std::unique_ptr<CustomError> CE) mutable -> Error { return Error::getSuccess(); });

   handle_all_errors(make_error<CustomError>(42),
                     [](std::unique_ptr<CustomError> CE) mutable {});

   // Check that named handlers of type 'Error (const Err&)' work.
   handle_all_errors(make_error<CustomError>(42), handleCustomError);

   // Check that named handlers of type 'void (const Err&)' work.
   handle_all_errors(make_error<CustomError>(42), handleCustomErrorVoid);

   // Check that named handlers of type 'Error (std::unique_ptr<Err>)' work.
   handle_all_errors(make_error<CustomError>(42), handleCustomErrorUP);

   // Check that named handlers of type 'Error (std::unique_ptr<Err>)' work.
   handle_all_errors(make_error<CustomError>(42), handleCustomErrorUPVoid);
}

// Test that we can handle errors with custom base classes.
TEST(ErrorTest, testHandleCustomErrorWithCustomBaseClass)
{
   int CaughtErrorInfo = 0;
   int CaughtErrorExtraInfo = 0;
   handle_all_errors(make_error<CustomSubError>(42, 7),
                     [&](const CustomSubError &SE) {
      CaughtErrorInfo = SE.getInfo();
      CaughtErrorExtraInfo = SE.getExtraInfo();
   });

   EXPECT_TRUE(CaughtErrorInfo == 42 && CaughtErrorExtraInfo == 7)
         << "Wrong result from CustomSubError handler";
}

// Check that we trigger only the first handler that applies.
TEST(ErrorTest, testFirstHandlerOnly)
{
   int DummyInfo = 0;
   int CaughtErrorInfo = 0;
   int CaughtErrorExtraInfo = 0;

   handle_all_errors(make_error<CustomSubError>(42, 7),
                     [&](const CustomSubError &SE) {
      CaughtErrorInfo = SE.getInfo();
      CaughtErrorExtraInfo = SE.getExtraInfo();
   },
   [&](const CustomError &CE) { DummyInfo = CE.getInfo(); });

   EXPECT_TRUE(CaughtErrorInfo == 42 && CaughtErrorExtraInfo == 7 &&
               DummyInfo == 0)
         << "Activated the wrong Error handler(s)";
}

// Check that general handlers shadow specific ones.
TEST(ErrorTest, testHandlerShadowing)
{
   int CaughtErrorInfo = 0;
   int DummyInfo = 0;
   int DummyExtraInfo = 0;

   handle_all_errors(
            make_error<CustomSubError>(42, 7),
            [&](const CustomError &CE) { CaughtErrorInfo = CE.getInfo(); },
   [&](const CustomSubError &SE) {
      DummyInfo = SE.getInfo();
      DummyExtraInfo = SE.getExtraInfo();
   });

   EXPECT_TRUE(CaughtErrorInfo == 42 && DummyInfo == 0 && DummyExtraInfo == 0)
         << "General Error handler did not shadow specific handler";
}

// Test join_errors.
TEST(ErrorTest, testCheckJoinErrors)
{
   int CustomErrorInfo1 = 0;
   int CustomErrorInfo2 = 0;
   int CustomErrorExtraInfo = 0;
   Error E =
         join_errors(make_error<CustomError>(7), make_error<CustomSubError>(42, 7));

   handle_all_errors(std::move(E),
                     [&](const CustomSubError &SE) {
      CustomErrorInfo2 = SE.getInfo();
      CustomErrorExtraInfo = SE.getExtraInfo();
   },
   [&](const CustomError &CE) {
      // Assert that the CustomError instance above is handled
      // before the
      // CustomSubError - join_errors should preserve error
      // ordering.
      EXPECT_EQ(CustomErrorInfo2, 0)
            << "CustomErrorInfo2 should be 0 here. "
               "join_errors failed to preserve ordering.\n";
      CustomErrorInfo1 = CE.getInfo();
   });

   EXPECT_TRUE(CustomErrorInfo1 == 7 && CustomErrorInfo2 == 42 &&
               CustomErrorExtraInfo == 7)
         << "Failed handling compound Error.";

   // Test appending a single item to a list.
   {
      int Sum = 0;
      handle_all_errors(
               join_errors(
                  join_errors(make_error<CustomError>(7),
                              make_error<CustomError>(7)),
                  make_error<CustomError>(7)),
               [&](const CustomError &CE) {
         Sum += CE.getInfo();
      });
      EXPECT_EQ(Sum, 21) << "Failed to correctly append error to error list.";
   }

   // Test prepending a single item to a list.
   {
      int Sum = 0;
      handle_all_errors(
               join_errors(
                  make_error<CustomError>(7),
                  join_errors(make_error<CustomError>(7),
                              make_error<CustomError>(7))),
               [&](const CustomError &CE) {
         Sum += CE.getInfo();
      });
      EXPECT_EQ(Sum, 21) << "Failed to correctly prepend error to error list.";
   }

   // Test concatenating two error lists.
   {
      int Sum = 0;
      handle_all_errors(
               join_errors(
                  join_errors(
                     make_error<CustomError>(7),
                     make_error<CustomError>(7)),
                  join_errors(
                     make_error<CustomError>(7),
                     make_error<CustomError>(7))),
               [&](const CustomError &CE) {
         Sum += CE.getInfo();
      });
      EXPECT_EQ(Sum, 28) << "Failed to correctly concatenate error lists.";
   }
}

// Test that we can consume success values.
TEST(ErrorTest, testConsumeSuccess)
{
   Error E = Error::getSuccess();
   consume_error(std::move(E));
}

TEST(ErrorTest, testConsumeError)
{
   Error E = make_error<CustomError>(7);
   consume_error(std::move(E));
}

// Test that handleAllUnhandledErrors crashes if an error is not caught.
// Test runs in debug mode only.
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
TEST(ErrorTest, FailureToHandle)
{
   auto FailToHandle = []() {

      handle_all_errors(make_error<CustomError>(7), [&](const CustomSubError &SE) {
         error_stream() << "This should never be called";
         exit(1);
      });
   };

   EXPECT_DEATH(FailToHandle(),
                "Failure value returned from cant_fail wrapped call")
         << "Unhandled Error in handle_all_errors call did not cause an "
            "abort()";
}
#endif

// Test that handleAllUnhandledErrors crashes if an error is returned from a
// handler.
// Test runs in debug mode only.
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
TEST(ErrorTest, FailureFromHandler)
{
   auto ReturnErrorFromHandler = []() {
      handle_all_errors(make_error<CustomError>(7),
                        [&](std::unique_ptr<CustomSubError> SE) {
         return Error(std::move(SE));
      });
   };

   EXPECT_DEATH(ReturnErrorFromHandler(),
                "Failure value returned from cant_fail wrapped call")
         << " Error returned from handler in handle_all_errors call did not "
            "cause abort()";
}
#endif

// Test that we can return values from handle_errors.
TEST(ErrorTest, testCatchErrorFromHandler)
{
   int ErrorInfo = 0;

   Error E = handle_errors(
            make_error<CustomError>(7),
            [&](std::unique_ptr<CustomError> CE) { return Error(std::move(CE)); });

   handle_all_errors(std::move(E),
                     [&](const CustomError &CE) { ErrorInfo = CE.getInfo(); });

   EXPECT_EQ(ErrorInfo, 7)
         << "Failed to handle Error returned from handle_errors.";
}

TEST(ErrorTest, testStringError)
{
   std::string Msg;
   RawStringOutStream S(Msg);
   log_all_unhandled_errors(make_error<StringError>("foo" + Twine(42),
                                                    inconvertible_error_code()),
                            S);
   EXPECT_EQ(S.getStr(), "foo42\n") << "Unexpected StringError log result";

   auto EC =
         error_to_error_code(make_error<StringError>("", ErrorCode::invalid_argument));
   EXPECT_EQ(EC, ErrorCode::invalid_argument)
         << "Failed to convert StringError to error_code.";
}

TEST(ErrorTest, testCreateStringError) {
   static const char *Bar = "bar";
   static const std::error_code EC = ErrorCode::invalid_argument;
   std::string Msg;
   RawStringOutStream S(Msg);
   log_all_unhandled_errors(create_string_error(EC, "foo%s%d0x%" PRIx8, Bar, 1, 0xff),
                            S);
   EXPECT_EQ(S.getStr(), "foobar10xff\n")
         << "Unexpected createStringError() log result";

   S.flush();
   Msg.clear();
   log_all_unhandled_errors(create_string_error(EC, Bar), S);
   EXPECT_EQ(S.getStr(), "bar\n")
         << "Unexpected createStringError() (overloaded) log result";

   S.flush();
   Msg.clear();
   auto Res = error_to_error_code(create_string_error(EC, "foo%s", Bar));
   EXPECT_EQ(Res, EC)
         << "Failed to convert createStringError() result to error_code.";
}

// Test that the ExitOnError utility works as expected.
TEST(ErrorTest, testExitOnError)
{
   ExitOnError ExitOnErr;
   ExitOnErr.setBanner("Error in tool:");
   ExitOnErr.setExitCodeMapper([](const Error &E) {
      if (E.isA<CustomSubError>())
         return 2;
      return 1;
   });

   // Make sure we don't bail on success.
   ExitOnErr(Error::getSuccess());
   EXPECT_EQ(ExitOnErr(Expected<int>(7)), 7)
         << "exitOnError returned an invalid value for Expected";

   int A = 7;
   int &B = ExitOnErr(Expected<int&>(A));
   EXPECT_EQ(&A, &B) << "ExitOnError failed to propagate reference";

   // Exit tests.
   EXPECT_EXIT(ExitOnErr(make_error<CustomError>(7)),
               ::testing::ExitedWithCode(1), "Error in tool:")
         << "exitOnError returned an unexpected error result";

   EXPECT_EXIT(ExitOnErr(Expected<int>(make_error<CustomSubError>(0, 0))),
               ::testing::ExitedWithCode(2), "Error in tool:")
         << "exitOnError returned an unexpected error result";
}

// Test that the ExitOnError utility works as expected.
TEST(ErrorTest, testCantFailSuccess)
{
   cant_fail(Error::getSuccess());

   int X = cant_fail(Expected<int>(42));
   EXPECT_EQ(X, 42) << "Expected value modified by cant_fail";

   int Dummy = 42;
   int &Y = cant_fail(Expected<int&>(Dummy));
   EXPECT_EQ(&Dummy, &Y) << "Reference mangled by cant_fail";
}

// Test that cant_fail results in a crash if you pass it a failure value.
#if POLAR_ENABLE_ABI_BREAKING_CHECKS && !defined(NDEBUG)
TEST(ErrorTest, testCantFailDeath)
{
   EXPECT_DEATH(
            cant_fail(make_error<StringError>("foo", inconvertible_error_code()),
                      "Cantfail call failed"),
            "Cantfail call failed")
         << "cant_fail(Error) did not cause an abort for failure value";

   EXPECT_DEATH(
   {
               auto IEC = inconvertible_error_code();
               int X = cant_fail(Expected<int>(make_error<StringError>("foo", IEC)));
               (void)X;
            },
            "Failure value returned from cant_fail wrapped call")
         << "cant_fail(Expected<int>) did not cause an abort for failure value";
}
#endif


// Test Checked Expected<T> in success mode.
TEST(ErrorTest, testCheckedExpectedInSuccessMode)
{
   Expected<int> A = 7;
   EXPECT_TRUE(!!A) << "Expected with non-error value doesn't convert to 'true'";
   // Access is safe in second test, since we checked the error in the first.
   EXPECT_EQ(*A, 7) << "Incorrect Expected non-error value";
}

// Test Expected with reference type.
TEST(ErrorTest, testExpectedWithReferenceType)
{
   int A = 7;
   Expected<int&> B = A;
   // 'Check' B.
   (void)!!B;
   int &C = *B;
   EXPECT_EQ(&A, &C) << "Expected failed to propagate reference";
}

// Test Unchecked Expected<T> in success mode.
// We expect this to blow up the same way Error would.
// Test runs in debug mode only.
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
TEST(ErrorTest, UncheckedExpectedInSuccessModeDestruction)
{
   EXPECT_DEATH({ Expected<int> A = 7; },
                "Expected<T> must be checked before access or destruction.")
         << "Unchecekd Expected<T> success value did not cause an abort().";
}
#endif

// Test Unchecked Expected<T> in success mode.
// We expect this to blow up the same way Error would.
// Test runs in debug mode only.
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
TEST(ErrorTest, UncheckedExpectedInSuccessModeAccess)
{
   EXPECT_DEATH({ Expected<int> A = 7; *A; },
                "Expected<T> must be checked before access or destruction.")
         << "Unchecekd Expected<T> success value did not cause an abort().";
}
#endif

// Test Unchecked Expected<T> in success mode.
// We expect this to blow up the same way Error would.
// Test runs in debug mode only.
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
TEST(ErrorTest, UncheckedExpectedInSuccessModeAssignment)
{
   EXPECT_DEATH({ Expected<int> A = 7; A = 7; },
                "Expected<T> must be checked before access or destruction.")
         << "Unchecekd Expected<T> success value did not cause an abort().";
}
#endif

// Test Expected<T> in failure mode.
TEST(ErrorTest, testExpectedInFailureMode)
{
   Expected<int> A = make_error<CustomError>(42);
   EXPECT_FALSE(!!A) << "Expected with error value doesn't convert to 'false'";
   Error E = A.takeError();
   EXPECT_TRUE(E.isA<CustomError>()) << "Incorrect Expected error value";
   consume_error(std::move(E));
}

// Check that an Expected instance with an error value doesn't allow access to
// operator*.
// Test runs in debug mode only.
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
TEST(ErrorTest, AccessExpectedInFailureMode)
{
   Expected<int> A = make_error<CustomError>(42);
   EXPECT_DEATH(*A, "Expected<T> must be checked before access or destruction.")
         << "Incorrect Expected error value";
   consume_error(A.takeError());
}
#endif

// Check that an Expected instance with an error triggers an abort if
// unhandled.
// Test runs in debug mode only.
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
TEST(ErrorTest, testUnhandledExpectedInFailureMode)
{
   EXPECT_DEATH({ Expected<int> A = make_error<CustomError>(42); },
                "Expected<T> must be checked before access or destruction.")
         << "Unchecked Expected<T> failure value did not cause an abort()";
}
#endif

// Test covariance of Expected.
TEST(ErrorTest, testExpectedCovariance)
{
   class B {};
   class D : public B {};

   Expected<B *> A1(Expected<D *>(nullptr));
   // Check A1 by converting to bool before assigning to it.
   (void)!!A1;
   A1 = Expected<D *>(nullptr);
   // Check A1 again before destruction.
   (void)!!A1;

   Expected<std::unique_ptr<B>> A2(Expected<std::unique_ptr<D>>(nullptr));
   // Check A2 by converting to bool before assigning to it.
   (void)!!A2;
   A2 = Expected<std::unique_ptr<D>>(nullptr);
   // Check A2 again before destruction.
   (void)!!A2;
}

// Test that handle_expected just returns success values.
TEST(ErrorTest, testHandleExpectedSuccess)
{
   auto ValOrErr =
         handle_expected(Expected<int>(42),
                         []() { return Expected<int>(43); });
   EXPECT_TRUE(!!ValOrErr)
         << "handle_expected should have returned a success value here";
   EXPECT_EQ(*ValOrErr, 42)
         << "handle_expected should have returned the original success value here";
}

enum FooStrategy { Aggressive, Conservative };

static Expected<int> foo(FooStrategy S) {
   if (S == Aggressive)
      return make_error<CustomError>(7);
   return 42;
}

// Test that handle_expected invokes the error path if errors are not handled.
TEST(ErrorTest, testHandleExpectedUnhandledError)
{
   // foo(Aggressive) should return a CustomError which should pass through as
   // there is no handler for CustomError.
   auto ValOrErr =
         handle_expected(
            foo(Aggressive),
            []() { return foo(Conservative); });

   EXPECT_FALSE(!!ValOrErr)
         << "handle_expected should have returned an error here";
   auto Err = ValOrErr.takeError();
   EXPECT_TRUE(Err.isA<CustomError>())
         << "handle_expected should have returned the CustomError generated by "
            "foo(Aggressive) here";
   consume_error(std::move(Err));
}

// Test that handle_expected invokes the fallback path if errors are handled.
TEST(ErrorTest, testHandleExpectedHandledError)
{
   // foo(Aggressive) should return a CustomError which should handle triggering
   // the fallback path.
   auto ValOrErr =
         handle_expected(
            foo(Aggressive),
            []() { return foo(Conservative); },
   [](const CustomError&) { /* do nothing */ });

   EXPECT_TRUE(!!ValOrErr)
         << "handle_expected should have returned a success value here";
   EXPECT_EQ(*ValOrErr, 42)
         << "handle_expected returned the wrong success value";
}

TEST(ErrorTest, testErrorCodeConversions)
{
   // Round-trip a success value to check that it converts correctly.
   EXPECT_EQ(error_to_error_code(error_code_to_error(std::error_code())),
             std::error_code())
         << "std::error_code() should round-trip via Error conversions";

   // Round-trip an error value to check that it converts correctly.
   EXPECT_EQ(error_to_error_code(error_code_to_error(ErrorCode::invalid_argument)),
             ErrorCode::invalid_argument)
         << "std::error_code error value should round-trip via Error "
            "conversions";

   // Round-trip a success value through OptionalError/Expected to check that it
   // converts correctly.
   {
      auto Orig = OptionalError<int>(42);
      auto RoundTripped =
            expected_to_optional_error(optional_error_to_expected(OptionalError<int>(42)));
      EXPECT_EQ(*Orig, *RoundTripped)
            << "OptionalError<T> success value should round-trip via Expected<T> "
               "conversions.";
   }

   // Round-trip a failure value through OptionalError/Expected to check that it
   // converts correctly.
   {
      auto Orig = OptionalError<int>(ErrorCode::invalid_argument);
      auto RoundTripped =
            expected_to_optional_error(
               optional_error_to_expected(OptionalError<int>(ErrorCode::invalid_argument)));
      EXPECT_EQ(Orig.getError(), RoundTripped.getError())
            << "OptionalError<T> failure value should round-trip via Expected<T> "
               "conversions.";
   }
}

// Test that error messages work.
TEST(ErrorTest, testErrorMessage)
{
   EXPECT_EQ(to_string(Error::getSuccess()).compare(""), 0);

   Error E1 = make_error<CustomError>(0);
   EXPECT_EQ(to_string(std::move(E1)).compare("CustomError {0}"), 0);

   Error E2 = make_error<CustomError>(0);
   handle_all_errors(std::move(E2), [](const CustomError &CE) {
      EXPECT_EQ(CE.message().compare("CustomError {0}"), 0);
   });

   Error E3 = join_errors(make_error<CustomError>(0), make_error<CustomError>(1));
   EXPECT_EQ(to_string(std::move(E3))
             .compare("CustomError {0}\n"
                      "CustomError {1}"),
             0);
}

TEST(ErrorTest, testStream)
{
   {
      Error OK = Error::getSuccess();
      std::string Buf;
      RawStringOutStream S(Buf);
      S << OK;
      EXPECT_EQ("success", S.getStr());
      consume_error(std::move(OK));
   }
   {
      Error E1 = make_error<CustomError>(0);
      std::string Buf;
      RawStringOutStream S(Buf);
      S << E1;
      EXPECT_EQ("CustomError {0}", S.getStr());
      consume_error(std::move(E1));
   }
}

TEST(ErrorTest, testErrorMatchers)
{
   EXPECT_THAT_ERROR(Error::getSuccess(), Succeeded());
   EXPECT_NONFATAL_FAILURE(
            EXPECT_THAT_ERROR(make_error<CustomError>(0), Succeeded()),
            "Expected: succeeded\n  Actual: failed  (CustomError {0})");

   EXPECT_THAT_ERROR(make_error<CustomError>(0), Failed());
   EXPECT_NONFATAL_FAILURE(EXPECT_THAT_ERROR(Error::getSuccess(), Failed()),
                           "Expected: failed\n  Actual: succeeded");

   EXPECT_THAT_ERROR(make_error<CustomError>(0), Failed<CustomError>());
   EXPECT_NONFATAL_FAILURE(
            EXPECT_THAT_ERROR(Error::getSuccess(), Failed<CustomError>()),
            "Expected: failed with Error of given type\n  Actual: succeeded");
   EXPECT_NONFATAL_FAILURE(
            EXPECT_THAT_ERROR(make_error<CustomError>(0), Failed<CustomSubError>()),
            "Error was not of given type");
   EXPECT_NONFATAL_FAILURE(
            EXPECT_THAT_ERROR(
               join_errors(make_error<CustomError>(0), make_error<CustomError>(1)),
               Failed<CustomError>()),
            "multiple errors");

   EXPECT_THAT_ERROR(
            make_error<CustomError>(0),
            Failed<CustomError>(testing::Property(&CustomError::getInfo, 0)));
   EXPECT_NONFATAL_FAILURE(
            EXPECT_THAT_ERROR(
               make_error<CustomError>(0),
               Failed<CustomError>(testing::Property(&CustomError::getInfo, 1))),
            "Expected: failed with Error of given type and the error is an object "
            "whose given property is equal to 1\n"
            "  Actual: failed  (CustomError {0})");
   EXPECT_THAT_ERROR(make_error<CustomError>(0), Failed<ErrorInfoBase>());

   EXPECT_THAT_EXPECTED(Expected<int>(0), Succeeded());
   EXPECT_NONFATAL_FAILURE(
            EXPECT_THAT_EXPECTED(Expected<int>(make_error<CustomError>(0)),
                                 Succeeded()),
            "Expected: succeeded\n  Actual: failed  (CustomError {0})");

   EXPECT_THAT_EXPECTED(Expected<int>(make_error<CustomError>(0)), Failed());
   EXPECT_NONFATAL_FAILURE(
            EXPECT_THAT_EXPECTED(Expected<int>(0), Failed()),
            "Expected: failed\n  Actual: succeeded with value 0");

   EXPECT_THAT_EXPECTED(Expected<int>(0), has_value(0));
   EXPECT_NONFATAL_FAILURE(
            EXPECT_THAT_EXPECTED(Expected<int>(make_error<CustomError>(0)),
                                 has_value(0)),
            "Expected: succeeded with value (is equal to 0)\n"
            "  Actual: failed  (CustomError {0})");
   EXPECT_NONFATAL_FAILURE(
            EXPECT_THAT_EXPECTED(Expected<int>(1), has_value(0)),
            "Expected: succeeded with value (is equal to 0)\n"
            "  Actual: succeeded with value 1, (isn't equal to 0)");

   EXPECT_THAT_EXPECTED(Expected<int &>(make_error<CustomError>(0)), Failed());
   int a = 1;
   EXPECT_THAT_EXPECTED(Expected<int &>(a), Succeeded());
   EXPECT_THAT_EXPECTED(Expected<int &>(a), has_value(testing::Eq(1)));

   EXPECT_THAT_EXPECTED(Expected<int>(1), has_value(testing::Gt(0)));
   EXPECT_NONFATAL_FAILURE(
            EXPECT_THAT_EXPECTED(Expected<int>(0), has_value(testing::Gt(1))),
            "Expected: succeeded with value (is > 1)\n"
            "  Actual: succeeded with value 0, (isn't > 1)");
   EXPECT_NONFATAL_FAILURE(
            EXPECT_THAT_EXPECTED(Expected<int>(make_error<CustomError>(0)),
                                 has_value(testing::Gt(1))),
            "Expected: succeeded with value (is > 1)\n"
            "  Actual: failed  (CustomError {0})");
}

TEST(Error, FileErrorTest) {
#if !defined(NDEBUG) && GTEST_HAS_DEATH_TEST
   EXPECT_DEATH(
   {
               Error S = Error::getSuccess();
               consume_error(create_file_error("file.bin", std::move(S)));
            },
            "");
#endif
   // Not allowed, would fail at compile-time
   //consumeError(create_file_error("file.bin", ErrorSuccess()));

   Error E1 = make_error<CustomError>(1);
   Error FE1 = create_file_error("file.bin", std::move(E1));
   EXPECT_EQ(to_string(std::move(FE1)).compare("'file.bin': CustomError {1}"), 0);

   Error E2 = make_error<CustomError>(2);
   Error FE2 = create_file_error("file.bin", std::move(E2));
   handle_all_errors(std::move(FE2), [](const FileError &F) {
      EXPECT_EQ(F.message().compare("'file.bin': CustomError {2}"), 0);
   });

   Error E3 = make_error<CustomError>(3);
   Error FE3 = create_file_error("file.bin", std::move(E3));
   auto E31 = handle_errors(std::move(FE3), [](std::unique_ptr<FileError> F) {
         return F->takeError();
});
   handle_all_errors(std::move(E31), [](const CustomError &C) {
      EXPECT_EQ(C.message().compare("CustomError {3}"), 0);
   });

   Error FE4 =
         join_errors(create_file_error("file.bin", make_error<CustomError>(41)),
                    create_file_error("file2.bin", make_error<CustomError>(42)));
   EXPECT_EQ(to_string(std::move(FE4))
             .compare("'file.bin': CustomError {41}\n"
                      "'file2.bin': CustomError {42}"),
             0);
}

enum class test_error_code {
   unspecified = 1,
   error_1,
   error_2,
};

} // end anon namespace

namespace std {
template <>
struct is_error_code_enum<test_error_code> : std::true_type {};
} // namespace std

namespace {

const std::error_category &TErrorCategory();

inline std::error_code make_error_code(test_error_code E) {
   return std::error_code(static_cast<int>(E), TErrorCategory());
}

class TestDebugError : public ErrorInfo<TestDebugError, StringError> {
public:
   using ErrorInfo<TestDebugError, StringError >::ErrorInfo; // inherit constructors
   TestDebugError(const Twine &S) : ErrorInfo(S, test_error_code::unspecified) {}
   static char sm_id;
};

class TestErrorCategory : public std::error_category {
public:
   const char *name() const noexcept override { return "error"; }
   std::string message(int Condition) const override {
      switch (static_cast<test_error_code>(Condition)) {
      case test_error_code::unspecified:
         return "An unknown error has occurred.";
      case test_error_code::error_1:
         return "Error 1.";
      case test_error_code::error_2:
         return "Error 2.";
      }
      polar_unreachable("Unrecognized test_error_code");
   }
};

static polar::utils::ManagedStatic<TestErrorCategory> TestErrCategory;
const std::error_category &TErrorCategory() { return *TestErrCategory; }

char TestDebugError::sm_id;

TEST(ErrorTest, testSubtypeStringErrorTest)
{
   auto E1 = make_error<TestDebugError>(test_error_code::error_1);
   EXPECT_EQ(to_string(std::move(E1)).compare("Error 1."), 0);

   auto E2 = make_error<TestDebugError>(test_error_code::error_1,
                                        "Detailed information");
   EXPECT_EQ(to_string(std::move(E2)).compare("Error 1. Detailed information"),
             0);

   auto E3 = make_error<TestDebugError>(test_error_code::error_2);
   handle_all_errors(std::move(E3), [](const TestDebugError &F) {
      EXPECT_EQ(F.message().compare("Error 2."), 0);
   });

   auto E4 = join_errors(make_error<TestDebugError>(test_error_code::error_1,
                                                   "Detailed information"),
                        make_error<TestDebugError>(test_error_code::error_2));
   EXPECT_EQ(to_string(std::move(E4))
             .compare("Error 1. Detailed information\n"
                      "Error 2."),
             0);
}


} // anonymous namespace

