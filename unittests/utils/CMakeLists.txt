# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/10/12.

add_subdirectory(dynamiclibrary)

polar_add_unittest(PolarBaseLibTests UtilsTest
   ../TestEntry.cpp
   AlignOfTest.cpp
   AllocatorTest.cpp
   ArrayRecyclerTest.cpp
   ARMAttributeParserTest.cpp
   BinaryStreamTest.cpp
   BranchProbabilityTest.cpp
   CachePruningTest.cpp
   CastingTest.cpp
   ChronoTest.cpp
   CommandLineTest.cpp
   CompressionTest.cpp
   ConvertUtfTest.cpp
   CrashRecoveryTest.cpp
   CrcTest.cpp
   DataExtractorTest.cpp
   DebugTest.cpp
   DebugCounterTest.cpp
   FileCheckTest.cpp
   EndianStreamTest.cpp
   EndianTest.cpp
   ErrorNumberTest.cpp
   ErrorTest.cpp
   OptionalErrorTest.cpp
   FastHashTest.cpp
   FileOutputBufferTest.cpp
   FormattedRawOutStreamTest.cpp
   FormatVariadicTest.cpp
   GlobPatternTest.cpp
   HostTest.cpp
   KnownBitsTest.cpp
   LEB128Test.cpp
   LineIteratorTest.cpp
   LockFileManagerTest.cpp
   ManagedStaticTest.cpp
   MathExtrasTest.cpp
   Md5Test.cpp
   MemoryBufferTest.cpp
   MemoryTest.cpp
   NativeFormatTests.cpp
   PathTest.cpp
   ProcessTest.cpp
   ProgramTest.cpp
   RawOutStreamTest.cpp
   RawPwriteStreamTest.cpp
   RawSha1OutStreamTest.cpp
   ReplaceFileTest.cpp
   ReverseIterationTest.cpp
   ScaledNumberTest.cpp
   SourceMgrTest.cpp
   SpecialCaseListTest.cpp
   StringPoolTest.cpp
   SwapByteOrderTest.cpp
   TargetParserTest.cpp
   TarWriterTest.cpp
   TimerTest.cpp
   TrailingObjectsTest.cpp
   TrigramIndexTest.cpp
   TypeNameTest.cpp
   TypeTraitsTest.cpp
   UnicodeTest.cpp
   YamlIOTest.cpp
   YamlParserTest.cpp
   VersionTupleTest.cpp
   ThreadPoolTest.cpp
   TaskQueueTest.cpp
   VirtualFileSystemTest.cpp
   )

target_link_libraries(UtilsTest PRIVATE TestSupport filecheckerkernel)
