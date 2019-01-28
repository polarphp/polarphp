# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/10/18.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

polar_add_unittest(PolarBaseLibTests BasicAdtTest
   ../../TestEntry.cpp
   ApIntTest.cpp
   ApSIntTest.cpp
   ApFloatTest.cpp
   ArrayRefTest.cpp
   BitVectorTest.cpp
   BitMaskEnumTest.cpp
   BreadthFirstIteratorTest.cpp
   BumpPtrListTest.cpp
   DenseMapTest.cpp
   DenseSetTest.cpp
   DagDeltaAlgorithmTest.cpp
   DeltaAlgorithmTest.cpp
   DepthFirstIteratorTest.cpp
   EquivalenceClassesTest.cpp
   FoldingSetTest.cpp
   FuncExtrasTest.cpp
   FuncRefTest.cpp
   HashingTest.cpp
   IteratorTest.cpp
   ImmutableMapTest.cpp
   ImmutableSetTest.cpp
   IntervalMapTest.cpp
   IntrusiveListBaseTest.cpp
   IntrusiveListIteratorTest.cpp
   IntrusiveListNodeBaseTest.cpp
   IntrusiveListNodeTest.cpp
   IntrusiveListSentienelTest.cpp
   IntrusiveListTest.cpp
   IntrusiveRefCntPtrTest.cpp
   ImmutableListTest.cpp
   MappedIteratorTest.cpp
   MapVectorTest.cpp
   PackedVectorTest.cpp
   PointerEmbeddedIntTest.cpp
   PointerIntPairTest.cpp
   PointerSumTypeTest.cpp
   PointerUnionTest.cpp
   PriorityWorkListTest.cpp
   RangeAdapterTest.cpp
   SccIteratorTest.cpp
   ScopeExitTest.cpp
   SequenceTest.cpp
   SetVectorTest.cpp
   SimpleIntrusiveListTest.cpp
   StringRefTest.cpp
   StringExtrasTest.cpp
   STLExtrasTest.cpp
   StringMapTest.cpp
   SmallStringTest.cpp
   SmallVectorTest.cpp
   SmallSetTest.cpp
   SmallPtrSetTest.cpp
   SparseBitVectorTest.cpp
   SparseMultiSetTest.cpp
   StringSwitchTest.cpp
   TinyPtrVectorTest.cpp
   TwineTest.cpp
   TripleTest.cpp
   )

target_link_libraries(BasicAdtTest PRIVATE TestSupport)
