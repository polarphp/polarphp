//===--- LLVMSwift.def ----------------------------------*- C++ -*---------===//
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

// KIND(Name, MemBehavior)
//
// This represents a specific equivalence class of LLVM instructions that have a
// Name and the same MemBehavior from a ModRef perspective.
//
// Name - The name of the kind.
// MemBehavior - One of NoModRef or ModRef.
//
#ifndef KIND
#define KIND(Name, MemBehavior)
#endif

// POLAR_FUNC(Name, MemBehavior, TextualName)
//
// This defines a special swift function known to the optimizer that may be
// present in either atomic or nonatomic form.
//
// Name - The name of the function
// MemBehavior - The MemBehavior of the instruction that can be known at compile time
// TextualName - The name of the function in the final binary.
#ifndef POLAR_FUNC
#define POLAR_FUNC(Name, MemBehavior, TextualName) KIND(Name, MemBehavior)
#endif

// POLAR_NEVER_NONATOMIC_FUNC(Name, MemBehavior, TextualName)
//
// This defines a special swift function known to the optimizer that does not
// have a nonatomic form.
//
// Name - The name of the function
// MemBehavior - The MemBehavior of the instruction that can be known at compile time
// TextualName - The name of the function in the final binary.
#ifndef POLAR_NEVER_NONATOMIC_FUNC
#define POLAR_NEVER_NONATOMIC_FUNC(Name, MemBehavior, TextualName) POLAR_FUNC(Name, MemBehavior, TextualName)
#endif

// POLAR_INTERNAL_NEVER_NONATOMIC_FUNC(Name, MemBehavior, TextualName)
//
// This defines a special swift function known to the optimizer that does not
// have a nonatomic form and has an internal prefix (i.e. '__').
//
// Name - The name of the function
// MemBehavior - The MemBehavior of the instruction that can be known at compile time
// TextualName - The name of the function in the final binary.
#ifndef POLAR_INTERNAL_FUNC_NEVER_NONATOMIC
#define POLAR_INTERNAL_FUNC_NEVER_NONATOMIC(Name, MemBehavior, TextualName) POLAR_FUNC(Name, MemBehavior, TextualName)
#endif

/// An instruction with this classification is known to not access (read or
/// write) memory.
KIND(NoMemoryAccessed, NoModRef)

/// void polar_retain(PolarHeapObject *object)
POLAR_FUNC(Retain, NoModRef, retain)

/// void polar_retain_n(SwiftHeapObject *object)
POLAR_FUNC(RetainN, NoModRef, retain_n)

/// void swift::polar_retainUnowned(HeapObject *object)
POLAR_FUNC(RetainUnowned, NoModRef, retainUnowned)

/// void polar_checkUnowned(HeapObject *object)
POLAR_FUNC(CheckUnowned, NoModRef, checkUnowned)

/// void polar_release(SwiftHeapObject *object)
POLAR_FUNC(Release, ModRef, release)

/// void polar_release_n(SwiftHeapObject *object)
POLAR_FUNC(ReleaseN, ModRef, release_n)

/// SwiftHeapObject *polar_allocObject(SwiftHeapMetadata *metadata,
///                                    size_t size, size_t alignment)
POLAR_NEVER_NONATOMIC_FUNC(AllocObject, NoModRef, allocObject)

/// void polar_unknownObjectRetain(%swift.refcounted* %P)
POLAR_FUNC(UnknownObjectRetain, NoModRef, unknownObjectRetain)

/// void polar_unknownObjectRetain_n(%swift.refcounted* %P)
POLAR_FUNC(UnknownObjectRetainN, NoModRef, unknownObjectRetain_n)

/// void polar_unknownObjectRelease(%swift.refcounted* %P)
POLAR_FUNC(UnknownObjectRelease, ModRef, unknownObjectRelease)

/// void polar_unknownObjectRelease_n(%swift.refcounted* %P)
POLAR_FUNC(UnknownObjectReleaseN, ModRef, unknownObjectRelease_n)

/// void __polar_fixLifetime(%swift.refcounted* %P)
POLAR_INTERNAL_FUNC_NEVER_NONATOMIC(FixLifetime, NoModRef, fixLifetime)

/// void polar_bridgeObjectRetain(%swift.refcounted* %P)
POLAR_FUNC(BridgeRetain, NoModRef, bridgeObjectRetain)

/// void polar_bridgeObjectRetain_n(%swift.refcounted* %P)
POLAR_FUNC(BridgeRetainN, NoModRef, bridgeObjectRetain_n)

/// void polar_bridgeObjectRelease(%swift.refcounted* %P)
POLAR_FUNC(BridgeRelease, ModRef, bridgeObjectRelease)

/// void polar_bridgeObjectRelease_n(%swift.refcounted* %P)
POLAR_FUNC(BridgeReleaseN, ModRef, bridgeObjectRelease_n)

/// borrow source is the value that was borrowed from. borrow_dest is the
/// borrowed ref.
///
/// TODO: We may want to communicate to the optimizer that this does not have
/// global effects.
///
/// void __polar_endBorrow(i8* %borrow_source, i8* %borrow_dest)
POLAR_INTERNAL_FUNC_NEVER_NONATOMIC(EndBorrow, ModRef, endBorrow)

/// This is not a runtime function that we support.  Maybe it is not a call,
/// or is a call to something we don't care about.
KIND(Unknown, ModRef)

#undef POLAR_INTERNAL_FUNC_NEVER_NONATOMIC
#undef POLAR_NEVER_NONATOMIC_FUNC
#undef POLAR_FUNC
#undef KIND
