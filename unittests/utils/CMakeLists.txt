# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/10/12.

polar_add_unittest(PolarBaseLibTests BasicUtilsTest
   ../TestEntry.cpp
   AlignOfTest.cpp
   AllocatorTest.cpp
   ArrayRecyclerTest.cpp
   BinaryStreamTest.cpp
   BranchProbabilityTest.cpp
   CachePruningTest.cpp
   CastingTest.cpp
   ChronoTest.cpp
   CommandLineTest.cpp
   CompressionTest.cpp
   ConvertUtfTest.cpp
   CrashRecoveryTest.cpp
   DataExtractorTest.cpp
   DebugTest.cpp
   DebugCounterTest.cpp
   EndianStreamTest.cpp
   EndianTest.cpp
   ErrorNumberTest.cpp
   FormatVariadicTest.cpp
   FormattedStreamTest.cpp
   )

target_link_libraries(BasicUtilsTest PRIVATE TestSupport)
