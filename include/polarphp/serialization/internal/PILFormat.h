//===--- PILFormat.h - The internals of serialized PILs ---------*- C++ -*-===//
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
///
/// \file Contains various constants and helper types to deal with serialized
/// PILs.
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_SERIALIZATION_PILFORMAT_H
#define POLARPHP_SERIALIZATION_PILFORMAT_H

#include "polarphp/serialization/internal/ModuleFormat.h"

namespace polar {
namespace serialization {

using ValueID = DeclID;
using ValueIDField = DeclIDField;

using PILInstOpCodeField = BCFixed<8>;
using PILTypeCategoryField = BCFixed<2>;

enum PILStringEncoding : uint8_t {
   PIL_UTF8,
   PIL_UTF16,
   PIL_OBJC_SELECTOR,
   PIL_BYTES
};

enum PILLinkageEncoding : uint8_t {
   PIL_LINKAGE_PUBLIC,
   PIL_LINKAGE_PUBLIC_NON_ABI,
   PIL_LINKAGE_HIDDEN,
   PIL_LINKAGE_SHARED,
   PIL_LINKAGE_PRIVATE,
   PIL_LINKAGE_PUBLIC_EXTERNAL,
   PIL_LINKAGE_HIDDEN_EXTERNAL,
   PIL_LINKAGE_SHARED_EXTERNAL,
   PIL_LINKAGE_PRIVATE_EXTERNAL,
};
using PILLinkageField = BCFixed<4>;

enum PILVTableEntryKindEncoding : uint8_t {
   PIL_VTABLE_ENTRY_NORMAL,
   PIL_VTABLE_ENTRY_INHERITED,
   PIL_VTABLE_ENTRY_OVERRIDE,
};
using PILVTableEntryKindField = BCFixed<2>;

enum CastConsumptionKindEncoding : uint8_t {
   PIL_CAST_CONSUMPTION_TAKE_ALWAYS,
   PIL_CAST_CONSUMPTION_TAKE_ON_SUCCESS,
   PIL_CAST_CONSUMPTION_COPY_ON_SUCCESS,
   PIL_CAST_CONSUMPTION_BORROW_ALWAYS,
};

enum class KeyPathComponentKindEncoding : uint8_t {
   StoredProperty,
   TupleElement,
   GettableProperty,
   SettableProperty,
   OptionalChain,
   OptionalForce,
   OptionalWrap,
   Trivial,
};
enum class KeyPathComputedComponentIdKindEncoding : uint8_t {
   Property,
   Function,
   DeclRef,
};

/// The record types within the "sil-index" block.
///
/// \sa PIL_INDEX_BLOCK_ID
namespace sil_index_block {
// These IDs must \em not be renumbered or reordered without incrementing
// the module version.
enum RecordKind {
   PIL_FUNC_NAMES = 1,
   PIL_FUNC_OFFSETS,
   PIL_VTABLE_NAMES,
   PIL_VTABLE_OFFSETS,
   PIL_GLOBALVAR_NAMES,
   PIL_GLOBALVAR_OFFSETS,
   PIL_WITNESS_TABLE_NAMES,
   PIL_WITNESS_TABLE_OFFSETS,
   PIL_DEFAULT_WITNESS_TABLE_NAMES,
   PIL_DEFAULT_WITNESS_TABLE_OFFSETS,
   PIL_PROPERTY_OFFSETS,
};

using ListLayout = BCGenericRecordLayout<
BCFixed<4>,  // record ID
BCVBR<16>,  // table offset within the blob
BCBlob      // map from identifier strings to IDs.
>;

using OffsetLayout = BCGenericRecordLayout<
BCFixed<4>,  // record ID
BCArray<BitOffsetField>
>;
}

/// The record types within the "sil" block.
///
/// \sa PIL_BLOCK_ID
namespace sil_block {
// These IDs must \em not be renumbered or reordered without incrementing
// the module version.
enum RecordKind : uint8_t {
   PIL_FUNCTION = 1,
   PIL_BASIC_BLOCK,
   PIL_ONE_VALUE_ONE_OPERAND,
   PIL_ONE_TYPE,
   PIL_ONE_OPERAND,
   PIL_ONE_TYPE_ONE_OPERAND,
   PIL_ONE_TYPE_VALUES,
   PIL_TWO_OPERANDS,
   PIL_TAIL_ADDR,
   PIL_INST_APPLY,
   PIL_INST_NO_OPERAND,
   PIL_VTABLE,
   PIL_VTABLE_ENTRY,
   PIL_GLOBALVAR,
   PIL_INIT_EXISTENTIAL,
   PIL_WITNESS_TABLE,
   PIL_WITNESS_METHOD_ENTRY,
   PIL_WITNESS_BASE_ENTRY,
   PIL_WITNESS_ASSOC_PROTOCOL,
   PIL_WITNESS_ASSOC_ENTRY,
   PIL_WITNESS_CONDITIONAL_CONFORMANCE,
   PIL_DEFAULT_WITNESS_TABLE,
   PIL_DEFAULT_WITNESS_TABLE_NO_ENTRY,
   PIL_INST_WITNESS_METHOD,
   PIL_SPECIALIZE_ATTR,
   PIL_PROPERTY,
   PIL_ONE_OPERAND_EXTRA_ATTR,
   PIL_TWO_OPERANDS_EXTRA_ATTR,

   // We also share these layouts from the decls block. Their enumerators must
   // not overlap with ours.
      ABSTRACT_PROTOCOL_CONFORMANCE = decls_block::ABSTRACT_PROTOCOL_CONFORMANCE,
   NORMAL_PROTOCOL_CONFORMANCE = decls_block::NORMAL_PROTOCOL_CONFORMANCE,
   SPECIALIZED_PROTOCOL_CONFORMANCE
   = decls_block::SPECIALIZED_PROTOCOL_CONFORMANCE,
   INHERITED_PROTOCOL_CONFORMANCE
   = decls_block::INHERITED_PROTOCOL_CONFORMANCE,
   INVALID_PROTOCOL_CONFORMANCE = decls_block::INVALID_PROTOCOL_CONFORMANCE,
   GENERIC_REQUIREMENT = decls_block::GENERIC_REQUIREMENT,
   LAYOUT_REQUIREMENT = decls_block::LAYOUT_REQUIREMENT,
};

using PILInstNoOperandLayout = BCRecordLayout<
   PIL_INST_NO_OPERAND,
   PILInstOpCodeField
>;

using VTableLayout = BCRecordLayout<
PIL_VTABLE,
DeclIDField,   // Class Decl
BCFixed<1>     // IsSerialized.
>;

using VTableEntryLayout = BCRecordLayout<
PIL_VTABLE_ENTRY,
DeclIDField,  // PILFunction name
PILVTableEntryKindField,  // Kind
BCArray<ValueIDField> // PILDeclRef
>;

using PropertyLayout = BCRecordLayout<
PIL_PROPERTY,
DeclIDField,          // Property decl
BCFixed<1>,           // Is serialized
BCArray<ValueIDField> // Encoded key path component
// Any substitutions or conformances required for the key path component
// follow.
>;

using WitnessTableLayout = BCRecordLayout<
PIL_WITNESS_TABLE,
PILLinkageField,     // Linkage
BCFixed<1>,          // Is this a declaration. We represent this separately
// from whether or not we have entries since we can
// have empty witness tables.
BCFixed<1>           // IsSerialized.
// Conformance follows
// Witness table entries will be serialized after.
>;

using WitnessMethodEntryLayout = BCRecordLayout<
PIL_WITNESS_METHOD_ENTRY,
DeclIDField,  // PILFunction name
BCArray<ValueIDField> // PILDeclRef
>;

using WitnessBaseEntryLayout = BCRecordLayout<
   PIL_WITNESS_BASE_ENTRY,
   DeclIDField  // ID of protocol decl
   // Trailed by the conformance itself.
>;

using WitnessAssocProtocolLayout = BCRecordLayout<
   PIL_WITNESS_ASSOC_PROTOCOL,
   TypeIDField, // ID of associated type
   DeclIDField  // ID of ProtocolDecl
   // Trailed by the conformance itself if appropriate.
>;

using WitnessAssocEntryLayout = BCRecordLayout<
   PIL_WITNESS_ASSOC_ENTRY,
   DeclIDField,  // ID of AssociatedTypeDecl
   TypeIDField
>;

using WitnessConditionalConformanceLayout = BCRecordLayout<
   PIL_WITNESS_CONDITIONAL_CONFORMANCE,
   TypeIDField // ID of associated type
   // Trailed by the conformance itself if appropriate.
>;

using DefaultWitnessTableLayout = BCRecordLayout<
   PIL_DEFAULT_WITNESS_TABLE,
   DeclIDField,  // ID of ProtocolDecl
   PILLinkageField  // Linkage
   // Default witness table entries will be serialized after.
>;

using DefaultWitnessTableNoEntryLayout = BCRecordLayout<
   PIL_DEFAULT_WITNESS_TABLE_NO_ENTRY
>;

using PILGlobalVarLayout = BCRecordLayout<
PIL_GLOBALVAR,
PILLinkageField,
BCFixed<1>,          // serialized
BCFixed<1>,          // Is this a declaration.
BCFixed<1>,          // Is this a let variable.
TypeIDField,
DeclIDField
>;

using PILFunctionLayout =
BCRecordLayout<PIL_FUNCTION, PILLinkageField,
BCFixed<1>,  // transparent
BCFixed<2>,  // serialized
BCFixed<2>,  // thunks: signature optimized/reabstraction
BCFixed<1>,  // without_actually_escaping
BCFixed<1>,  // global_init
BCFixed<2>,  // inlineStrategy
BCFixed<2>,  // optimizationMode
BCFixed<3>,  // side effect info.
BCVBR<8>,    // number of specialize attributes
BCFixed<1>,  // has qualified ownership
BCFixed<1>,  // force weak linking
BC_AVAIL_TUPLE, // availability for weak linking
BCFixed<1>,  // is dynamically replacable
BCFixed<1>,  // exact self class
TypeIDField, // PILFunctionType
DeclIDField,  // PILFunction name or 0 (replaced function)
GenericSignatureIDField,
DeclIDField, // ClangNode owner
BCArray<IdentifierIDField> // Semantics Attribute
// followed by specialize attributes
// followed by generic param list, if any
>;

using PILSpecializeAttrLayout =
BCRecordLayout<PIL_SPECIALIZE_ATTR,
BCFixed<1>, // exported
BCFixed<1>, // specialization kind
GenericSignatureIDField // specialized signature
>;

// Has an optional argument list where each argument is a typed valueref.
using PILBasicBlockLayout = BCRecordLayout<
PIL_BASIC_BLOCK,
BCArray<DeclIDField> // The array contains type-value pairs.
>;

// PIL instructions with one valueref and one typed valueref.
// (store)
using PILOneValueOneOperandLayout = BCRecordLayout<
PIL_ONE_VALUE_ONE_OPERAND,
PILInstOpCodeField,
BCFixed<2>,          // Optional attributes
ValueIDField,
TypeIDField,
PILTypeCategoryField,
ValueIDField
>;

// PIL instructions with one type and one typed valueref.
using PILOneTypeOneOperandLayout = BCRecordLayout<
PIL_ONE_TYPE_ONE_OPERAND,
PILInstOpCodeField,
BCFixed<2>,          // Optional attributes
TypeIDField,
PILTypeCategoryField,
TypeIDField,
PILTypeCategoryField,
ValueIDField
>;

// PIL instructions that construct existential values.
using PILInitExistentialLayout = BCRecordLayout<
PIL_INIT_EXISTENTIAL,
PILInstOpCodeField,   // opcode
TypeIDField,          // result type
PILTypeCategoryField, // result type category
TypeIDField,          // operand type
PILTypeCategoryField, // operand type category
ValueIDField,         // operand id
TypeIDField,          // formal concrete type
BCVBR<5>              // # of protocol conformances
// Trailed by protocol conformance info (if any)
>;

// PIL instructions with one type and a list of values.
using PILOneTypeValuesLayout = BCRecordLayout<
PIL_ONE_TYPE_VALUES,
PILInstOpCodeField,
TypeIDField,
PILTypeCategoryField,
BCArray<ValueIDField>
>;

enum ApplyKind : unsigned {
   PIL_APPLY = 0,
   PIL_PARTIAL_APPLY,
   PIL_BUILTIN,
   PIL_TRY_APPLY,
   PIL_NON_THROWING_APPLY,
   PIL_BEGIN_APPLY,
   PIL_NON_THROWING_BEGIN_APPLY
};

using PILInstApplyLayout = BCRecordLayout<
PIL_INST_APPLY,
BCFixed<3>,           // ApplyKind
SubstitutionMapIDField,  // substitution map
TypeIDField,          // callee unsubstituted type
TypeIDField,          // callee substituted type
ValueIDField,         // callee value
BCArray<ValueIDField> // a list of arguments
>;

// PIL instructions with one type. (alloc_stack)
using PILOneTypeLayout = BCRecordLayout<
PIL_ONE_TYPE,
PILInstOpCodeField,
BCFixed<2>,          // Optional attributes
TypeIDField,
PILTypeCategoryField
>;

// PIL instructions with one typed valueref. (dealloc_stack, return)
using PILOneOperandLayout = BCRecordLayout<
PIL_ONE_OPERAND,
PILInstOpCodeField,
BCFixed<2>,          // Optional attributes
TypeIDField,
PILTypeCategoryField,
ValueIDField
>;

using PILOneOperandExtraAttributeLayout = BCRecordLayout<
PIL_ONE_OPERAND_EXTRA_ATTR,
PILInstOpCodeField,
BCFixed<6>, // Optional attributes
TypeIDField, PILTypeCategoryField, ValueIDField
>;

// PIL instructions with two typed values.
using PILTwoOperandsLayout = BCRecordLayout<
PIL_TWO_OPERANDS,
PILInstOpCodeField,
BCFixed<2>,          // Optional attributes
TypeIDField,
PILTypeCategoryField,
ValueIDField,
TypeIDField,
PILTypeCategoryField,
ValueIDField
>;

using PILTwoOperandsExtraAttributeLayout = BCRecordLayout<
PIL_TWO_OPERANDS_EXTRA_ATTR,
PILInstOpCodeField,
BCFixed<6>,          // Optional attributes
TypeIDField,
PILTypeCategoryField,
ValueIDField,
TypeIDField,
PILTypeCategoryField,
ValueIDField
>;

// The tail_addr instruction.
using PILTailAddrLayout = BCRecordLayout<
   PIL_TAIL_ADDR,
   PILInstOpCodeField,
   TypeIDField,          // Base operand
   ValueIDField,
   TypeIDField,          // Count operand
   ValueIDField,
   TypeIDField           // Result type
>;

using PILInstWitnessMethodLayout = BCRecordLayout<
PIL_INST_WITNESS_METHOD,
TypeIDField,           // result type
PILTypeCategoryField,
BCFixed<1>,            // volatile?
TypeIDField,           // lookup type
PILTypeCategoryField,
TypeIDField,           // Optional
PILTypeCategoryField,  // opened
ValueIDField,          // existential
BCArray<ValueIDField>  // PILDeclRef
// may be trailed by an inline protocol conformance
>;
}

} // end namespace serialization
} // end namespace polar

#endif
