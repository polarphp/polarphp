//===--- Demangle.h - Interface to Swift symbol demangling ------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/28.
//
//===----------------------------------------------------------------------===//
//
// This file is the public API of the demangler library.
// Tools which use the demangler library (like lldb) must include this - and
// only this - header file.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_DEMANGLING_DEMANGLE_H
#define POLARPHP_DEMANGLING_DEMANGLE_H

#include <memory>
#include <string>
#include <cassert>
#include <cstdint>
#include "llvm/ADT/StringRef.h"

namespace llvm {
class raw_ostream;
}

namespace polar::demangling {

enum class SymbolicReferenceKind : uint8_t;

/// A simple default implementation that assigns letters to type parameters in
/// alphabetic order.
std::string generic_parameter_name(uint64_t depth, uint64_t index);

/// Display style options for the demangler.
struct DemangleOptions
{
   bool synthesizeSugarOnTypes = false;
   bool displayDebuggerGeneratedModule = true;
   bool qualifyEntities = true;
   bool displayExtensionContexts = true;
   bool displayUnmangledSuffix = true;
   bool displayModuleNames = true;
   bool displayGenericSpecializations = true;
   bool displayProtocolConformances = true;
   bool displayWhereClauses = true;
   bool displayEntityTypes = true;
   bool shortenPartialApply = false;
   bool shortenThunk = false;
   bool shortenValueWitness = false;
   bool shortenArchetype = false;
   bool showPrivateDiscriminators = true;
   bool showFunctionArgumentTypes = true;
   std::function<std::string(uint64_t, uint64_t)> genericParameterName =
         generic_parameter_name;

   DemangleOptions() {}

   static DemangleOptions SimplifiedUIDemangleOptions()
   {
      auto opt = DemangleOptions();
      opt.synthesizeSugarOnTypes = true;
      opt.qualifyEntities = true;
      opt.displayExtensionContexts = false;
      opt.displayUnmangledSuffix = false;
      opt.displayModuleNames = false;
      opt.displayGenericSpecializations = false;
      opt.displayProtocolConformances = false;
      opt.displayWhereClauses = false;
      opt.displayEntityTypes = false;
      opt.shortenPartialApply = true;
      opt.shortenThunk = true;
      opt.shortenValueWitness = true;
      opt.shortenArchetype = true;
      opt.showPrivateDiscriminators = false;
      opt.showFunctionArgumentTypes = false;
      return opt;
   }
};

class Node;
using NodePointer = Node *;

enum class FunctionSigSpecializationParamKind : unsigned
{
   // Option Flags use bits 0-5. This give us 6 bits implying 64 entries to
   // work with.
   ConstantPropFunction = 0,
   ConstantPropGlobal = 1,
   ConstantPropInteger = 2,
   ConstantPropFloat = 3,
   ConstantPropString = 4,
   ClosureProp = 5,
   BoxToValue = 6,
   BoxToStack = 7,

   // Option Set Flags use bits 6-31. This gives us 26 bits to use for option
   // flags.
   Dead = 1 << 6,
   OwnedToGuaranteed = 1 << 7,
   SROA = 1 << 8,
   GuaranteedToOwned = 1 << 9,
   ExistentialToGeneric = 1 << 10,
};

/// The pass that caused the specialization to occur. We use this to make sure
/// that two passes that generate similar changes do not yield the same
/// mangling. This currently cannot happen, so this is just a safety measure
/// that creates separate name spaces.
enum class SpecializationPass : uint8_t
{
   AllocBoxToStack,
   ClosureSpecializer,
   CapturePromotion,
   CapturePropagation,
   FunctionSignatureOpts,
   GenericSpecializer,
};

static inline char encode_specialization_pass(SpecializationPass pass)
{
   return char(uint8_t(pass)) + '0';
}

enum class ValueWitnessKind
{
#define VALUE_WITNESS(MANGLING, NAME) \
   NAME,
#include "polarphp/demangling/ValueWitnessManglingDef.h"
};

enum class Directness
{
   Direct, Indirect
};

class NodeFactory;
class Context;

class Node
{
public:
   enum class Kind : uint16_t
   {
#define NODE(ID) ID,
#include "polarphp/demangling/DemangleNodesDef.h"
   };

   using IndexType = uint64_t;

   friend class NodeFactory;

private:

   struct NodeVector
   {
      NodePointer *nodes;
      uint32_t number = 0;
      uint32_t capacity = 0;
   };

   union
   {
      llvm::StringRef text;
      IndexType index;
      NodePointer inlineChildren[2];
      NodeVector children;
   };


   Kind nodeKind;

   enum class PayloadKind : uint8_t
   {
      None, text, index, OneChild, TwoChildren, ManyChildren
   };

   PayloadKind nodePayloadKind;

   Node(Kind k)
      : nodeKind(k), nodePayloadKind(PayloadKind::None)
   {
   }

   Node(Kind k, llvm::StringRef t)
      : nodeKind(k), nodePayloadKind(PayloadKind::text)
   {
      text = t;
   }

   Node(Kind k, IndexType index)
      : nodeKind(k), nodePayloadKind(PayloadKind::index)
   {
      this->index = index;
   }

   Node(const Node &) = delete;
   Node &operator=(const Node &) = delete;

public:
   Kind getKind() const
   {
      return nodeKind;
   }

   bool hasText() const
   {
      return nodePayloadKind == PayloadKind::text;
   }

   llvm::StringRef getText() const
   {
      assert(hasText());
      return text;
   }

   bool hasIndex() const
   {
      return nodePayloadKind == PayloadKind::index;
   }

   uint64_t getIndex() const
   {
      assert(hasIndex());
      return index;
   }

   using iterator = const NodePointer *;

   size_t getNumChildren() const;

   bool hasChildren() const
   {
      return getNumChildren() != 0;
   }

   iterator begin() const;

   iterator end() const;

   NodePointer getFirstChild() const
   {
      return getChild(0);
   }

   NodePointer getChild(size_t index) const
   {
      assert(getNumChildren() > index);
      return begin()[index];
   }

   // Only to be used by the demangler parsers.
   void addChild(NodePointer child, NodeFactory &factory);
   // Only to be used by the demangler parsers.
   void removeChildAt(unsigned pos, NodeFactory &factory);

   // Reverses the order of children.
   void reverseChildren(size_t startingAt = 0);

   /// Prints the whole node tree in readable form to stderr.
   ///
   /// Useful to be called from the debugger.
   void dump();
};

/// Returns the length of the polarphp mangling prefix of the \p SymbolName.
///
/// Returns 0 if \p SymbolName is not a mangled polarphp (>= polarphp 4.x) name.
int get_mangling_prefix_length(llvm::StringRef mangledName);

/// Returns true if \p SymbolName is a mangled polarphp name.
///
/// This does not include the old (<= polarphp 3.x) mangling prefix "_T".
inline bool is_mangled_name(llvm::StringRef mangledName)
{
   return get_mangling_prefix_length(mangledName) != 0;
}

/// Returns true if the mangledName starts with the polarphp mangling prefix.
bool is_polarphp_symbol(llvm::StringRef mangledName);

/// Returns true if the mangledName starts with the polarphp mangling prefix.
bool is_polarphp_symbol(const char *mangledName);

/// Drops the Swift mangling prefix from the given mangled name, if there is
/// one.
///
/// This does not include the old (<= polarphp 3.x) mangling prefix "_T".
llvm::StringRef drop_polarphp_mangling_prefix(llvm::StringRef mangledName);

/// Returns true if the mangled name is an alias type name.
///
/// \param mangledName A null-terminated string containing a mangled name.
bool is_alias(llvm::StringRef mangledName);

/// Returns true if the mangled name is a class type name.
///
/// \param mangledName A null-terminated string containing a mangled name.
bool is_class(llvm::StringRef mangledName);

/// Returns true if the mangled name is an enum type name.
///
/// \param mangledName A null-terminated string containing a mangled name.
bool is_enum(llvm::StringRef mangledName);

/// Returns true if the mangled name is a protocol type name.
///
/// \param mangledName A null-terminated string containing a mangled name.
bool is_protocol(llvm::StringRef mangledName);

/// Returns true if the mangled name is a structure type name.
///
/// \param mangledName A null-terminated string containing a mangled name.
bool is_struct(llvm::StringRef mangledName);

class Demangler;

/// The demangler context.
///
/// It owns the allocated nodes which are created during demangling.
/// It is always preferable to use the demangling via this context class as it
/// ensures efficient memory management. Especially if demangling is done for
/// multiple symbols. Typical usage:
/// \code
///   Context Ctx;
///   for (...) {
///      NodePointer root = Ctx.demangleSymbolAsNode(mangledName);
///      // Do something with root
///      Ctx.clear(); // deallocates root
///   }
/// \endcode
/// Declaring the context out of the loop minimizes the amount of needed memory
/// allocations.
///
class Context
{
   Demangler *m_demangler;

   friend class Node;

public:
   Context();

   ~Context();

   /// Demangle the given symbol and return the parse tree.
   ///
   /// \param mangledName The mangled symbol string, which start a mangling
   /// prefix: _T, _T0, $S, _$S.
   ///
   /// \returns A parse tree for the demangled string - or a null pointer
   /// on failure.
   /// The lifetime of the returned node tree ends with the lifetime of the
   /// context or with a call of clear().
   NodePointer demangleSymbolAsNode(llvm::StringRef mangledName);

   /// Demangle the given type and return the parse tree.
   ///
   /// \param mangledName The mangled symbol string, which start a mangling
   /// prefix: _T, _T0, $S, _$S.
   ///
   /// \returns A parse tree for the demangled string - or a null pointer
   /// on failure.
   /// The lifetime of the returned node tree ends with the lifetime of the
   /// context or with a call of clear().
   NodePointer demangleTypeAsNode(llvm::StringRef mangledName);

   /// Demangle the given symbol and return the readable name.
   ///
   /// \param mangledName The mangled symbol string, which start a mangling
   /// prefix: _T, _T0, $S, _$S.
   ///
   /// \returns The demangled string.
   std::string demangle_symbol_as_string(
         llvm::StringRef mangledName,
         const DemangleOptions &options = DemangleOptions());

   /// Demangle the given type and return the readable name.
   ///
   /// \param mangledName The mangled type string, which does _not_ start with
   /// a mangling prefix.
   ///
   /// \returns The demangled string.
   std::string
   demangle_type_as_string(llvm::StringRef mangledName,
                           const DemangleOptions &options = DemangleOptions());

   /// Returns true if the mangledName refers to a thunk function.
   ///
   /// Thunk functions are either (ObjC) partial apply forwarder, polarphp-as-ObjC
   /// or ObjC-as-polarphp thunks or allocating init functions.
   bool isThunkSymbol(llvm::StringRef mangledName);

   /// Returns the mangled name of the target of a thunk.
   ///
   /// \returns Returns the remaining name after removing the thunk mangling
   /// characters from \p mangledName. If \p mangledName is not a thunk symbol
   /// or the thunk target cannot be derived from the mangling, an empty string
   /// is returned.
   std::string getThunkTarget(llvm::StringRef mangledName);

   /// Returns true if the \p mangledName refers to a function which conforms to
   /// the Swift calling convention.
   ///
   /// The return value is unspecified if the \p mangledName does not refer to a
   /// function symbol.
   bool hasPolarphpCallingConvention(llvm::StringRef mangledName);

   /// Demangle the given symbol and return the module name of the symbol.
   ///
   /// \param mangledName The mangled symbol string, which start a mangling
   /// prefix: _T, _T0, $S, _$S.
   ///
   /// \returns The module name.
   std::string getModuleName(llvm::StringRef mangledName);

   /// Deallocates all nodes.
   ///
   /// The memory which is used for nodes is not freed but recycled for the next
   /// demangling operation.
   void clear();
};

/// Standalone utility function to demangle the given symbol as string.
///
/// If performance is an issue when demangling multiple symbols,
/// Context::demangle_symbol_as_string should be used instead.
/// \param mangledName The mangled name string pointer.
/// \param mangledNameLength The length of the mangledName string.
/// \returns The demangled string.
std::string
demangle_symbol_as_string(const char *mangledName, size_t mangledNameLength,
                          const DemangleOptions &options = DemangleOptions());

/// Standalone utility function to demangle the given symbol as string.
///
/// If performance is an issue when demangling multiple symbols,
/// Context::demangle_symbol_as_string should be used instead.
/// \param mangledName The mangled name string.
/// \returns The demangled string.
inline std::string
demangle_symbol_as_string(const std::string &mangledName,
                          const DemangleOptions &options = DemangleOptions())
{
   return demangle_symbol_as_string(mangledName.data(), mangledName.size(),
                                    options);
}

/// Standalone utility function to demangle the given symbol as string.
///
/// If performance is an issue when demangling multiple symbols,
/// Context::demangle_symbol_as_string should be used instead.
/// \param mangledName The mangled name string.
/// \returns The demangled string.
inline std::string
demangle_symbol_as_string(llvm::StringRef mangledName,
                          const DemangleOptions &options = DemangleOptions())
{
   return demangle_symbol_as_string(mangledName.data(),
                                    mangledName.size(), options);
}

/// Standalone utility function to demangle the given type as string.
///
/// If performance is an issue when demangling multiple symbols,
/// Context::demangle_type_as_string should be used instead.
/// \param mangledName The mangled name string pointer.
/// \param mangledNameLength The length of the mangledName string.
/// \returns The demangled string.
std::string
demangle_type_as_string(const char *mangledName, size_t mangledNameLength,
                        const DemangleOptions &options = DemangleOptions());

/// Standalone utility function to demangle the given type as string.
///
/// If performance is an issue when demangling multiple symbols,
/// Context::demangle_type_as_string should be used instead.
/// \param mangledName The mangled name string.
/// \returns The demangled string.
inline std::string
demangle_type_as_string(const std::string &mangledName,
                        const DemangleOptions &options = DemangleOptions())
{
   return demangle_type_as_string(mangledName.data(), mangledName.size(), options);
}

/// Standalone utility function to demangle the given type as string.
///
/// If performance is an issue when demangling multiple symbols,
/// Context::demangle_type_as_string should be used instead.
/// \param mangledName The mangled name string.
/// \returns The demangled string.
inline std::string
demangle_type_as_string(llvm::StringRef mangledName,
                        const DemangleOptions &options = DemangleOptions())
{
   return demangle_type_as_string(mangledName.data(),
                                  mangledName.size(), options);
}


enum class OperatorKind
{
   NotOperator,
   Prefix,
   Postfix,
   Infix,
};

/// Remangle a demangled parse tree.
std::string mangle_node(NodePointer root);

using SymbolicResolver =
llvm::function_ref<demangling::NodePointer (SymbolicReferenceKind,
const void *)>;

/// Remangle a demangled parse tree, using a callback to resolve
/// symbolic references.
std::string mangle_node(NodePointer root, SymbolicResolver resolver);

/// Remangle a demangled parse tree, using a callback to resolve
/// symbolic references.
///
/// The returned string is owned by \p factory. This means \p factory must stay
/// alive as long as the returned string is used.
llvm::StringRef mangle_node(NodePointer root, SymbolicResolver resolver,
                            NodeFactory &factory);

/// Remangle in the old mangling scheme.
///
/// This is only used for objc-runtime names.
std::string mangle_node_old(NodePointer root);

/// Remangle in the old mangling scheme.
///
/// This is only used for objc-runtime names.
/// The returned string is owned by \p factory. This means \p factory must stay
/// alive as long as the returned string is used.
llvm::StringRef mangle_node_old(NodePointer node, NodeFactory &factory);


/// Transform the node structure to a string.
///
/// Typical usage:
/// \code
///   std::string aDemangledName =
/// polarphp::Demangler::node_to_string(aNode)
/// \endcode
///
/// \param root A pointer to a parse tree generated by the demangler.
/// \param options An object encapsulating options to use to perform this demangling.
///
/// \returns A string representing the demangled name.
///
std::string node_to_string(NodePointer root,
                           const DemangleOptions &options = DemangleOptions());

/// A class for printing to a std::string.
class DemanglerPrinter
{
public:
   DemanglerPrinter() = default;

   DemanglerPrinter &operator<<(llvm::StringRef value) &
   {
      m_stream.append(value.data(), value.size());
      return *this;
   }

   DemanglerPrinter &operator<<(char c) &
   {
      m_stream.push_back(c);
      return *this;
   }

   DemanglerPrinter &operator<<(unsigned long long n) &;
   DemanglerPrinter &operator<<(long long n) &;

   DemanglerPrinter &operator<<(unsigned long n) &
   {
      return *this << (unsigned long long)n;
   }

   DemanglerPrinter &operator<<(long n) &
   {
      return *this << (long long)n;
   }

   DemanglerPrinter &operator<<(unsigned n) &
   {
      return *this << (unsigned long long)n;
   }

   DemanglerPrinter &operator<<(int n) &
   {
      return *this << (long long)n;
   }

   template<typename T>
   DemanglerPrinter &&operator<<(T &&x) &&
   {
      return std::move(*this << std::forward<T>(x));
   }

   DemanglerPrinter &writeHex(unsigned long long n) &;

   std::string &&str() &&
   {
      return std::move(m_stream);
   }

   llvm::StringRef getStringRef() const
   {
      return m_stream;
   }

   /// Shrinks the buffer.
   void resetSize(size_t toPos)
   {
      assert(toPos <= m_stream.size());
      m_stream.resize(toPos);
   }

private:
   std::string m_stream;
};

/// Returns a the node kind \p k as string.
const char *get_node_kind_string(polar::demangling::Node::Kind k);

/// Prints the whole node tree \p root in readable form into a std::string.
///
/// Useful for debugging.
std::string get_node_tree_as_string(NodePointer root);

bool node_consumes_generic_args(Node *node);

bool is_specialized(Node *node);

NodePointer get_unspecialized(Node *node, NodeFactory &factory);

/// Returns true if the node \p kind refers to a context node, e.g. a nominal
/// type or a function.
bool is_context(Node::Kind kind);

/// Returns true if the node \p kind refers to a node which is placed before a
/// function node, e.g. a specialization attribute.
bool is_functionAttr(Node::Kind kind);

/// Form a StringRef around the mangled name starting at base, if the name may
/// contain symbolic references.
llvm::StringRef make_symbolic_mangled_name_string_ref(const char *base);

} // polar::demangling

#endif // POLARPHP_DEMANGLING_DEMANGLE_H
