//===--- TypeDecoder.h - Decode mangled type names --------------*- C++ -*-===//
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
// This file provides facilities to \c TypeDecoder, which decodes a mangled
// type name into a structured type representation.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_DEMANGLING_TYPEDECODER_H
#define POLARPHP_DEMANGLING_TYPEDECODER_H

#include "polarphp/demangling/Demangler.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/runtime/Unreachable.h"
#include "polarphp/global/NameStrings.h"
#include <vector>

namespace polar::demangling {

enum class ImplMetatypeRepresentation
{
   Thin,
   Thick
};

/// Describe a function parameter, parameterized on the type
/// representation.
template <typename BuiltType>
class FunctionParam
{
   StringRef Label;
   BuiltType Type;
   ParameterFlags Flags;

   FunctionParam(StringRef label, BuiltType type, ParameterFlags flags)
      : Label(label),
        Type(type),
        Flags(flags)
   {}

public:
   explicit FunctionParam() {}

   FunctionParam(BuiltType type) : Type(type) {}

   StringRef getLabel() const
   {
      return Label;
   }

   BuiltType getType() const
   {
      return Type;
   }

   ParameterFlags getFlags() const
   {
      return Flags;
   }

   void setLabel(StringRef label)
   {
      Label = label;
   }

   void setType(BuiltType type)
   {
      Type = type;
   }

   void setVariadic()
   {
      Flags = Flags.withVariadic(true);
   }

   void setAutoClosure()
   {
      Flags = Flags.withAutoClosure(true);
   }

   void setValueOwnership(ValueOwnership ownership)
   {
      Flags = Flags.withValueOwnership(ownership);
   }

   void setFlags(ParameterFlags flags)
   {
      Flags = flags;
   }

   FunctionParam withLabel(StringRef label) const
   {
      return FunctionParam(label, Type, Flags);
   }

   FunctionParam withType(BuiltType type) const
   {
      return FunctionParam(Label, type, Flags);
   }

   FunctionParam withFlags(ParameterFlags flags) const
   {
      return FunctionParam(Label, Type, flags);
   }
};

enum class ImplParameterConvention {
   Indirect_In,
   Indirect_In_Constant,
   Indirect_In_Guaranteed,
   Indirect_Inout,
   Indirect_InoutAliasable,
   Direct_Owned,
   Direct_Unowned,
   Direct_Guaranteed,
};

/// Describe a lowered function parameter, parameterized on the type
/// representation.
template <typename BuiltType>
class ImplFunctionParam
{
   ImplParameterConvention Convention;
   BuiltType Type;

public:
   using ConventionType = ImplParameterConvention;

   static Optional<ConventionType>
   getConventionFromString(StringRef conventionString)
   {
      if (conventionString == "@in")
         return ConventionType::Indirect_In;
      if (conventionString == "@in_constant")
         return ConventionType::Indirect_In_Constant;
      if (conventionString == "@in_guaranteed")
         return ConventionType::Indirect_In_Guaranteed;
      if (conventionString == "@inout")
         return ConventionType::Indirect_Inout;
      if (conventionString == "@inout_aliasable")
         return ConventionType::Indirect_InoutAliasable;
      if (conventionString == "@owned")
         return ConventionType::Direct_Owned;
      if (conventionString == "@unowned")
         return ConventionType::Direct_Unowned;
      if (conventionString == "@guaranteed")
         return ConventionType::Direct_Guaranteed;

      return None;
   }

   ImplFunctionParam(ImplParameterConvention convention, BuiltType type)
      : Convention(convention), Type(type)
   {}

   ImplParameterConvention getConvention() const
   {
      return Convention;
   }

   BuiltType getType() const
   {
      return Type;
   }
};

enum class ImplResultConvention
{
   Indirect,
   Owned,
   Unowned,
   UnownedInnerPointer,
   Autoreleased,
};

/// Describe a lowered function result, parameterized on the type
/// representation.
template <typename BuiltType>
class ImplFunctionResult
{
   ImplResultConvention Convention;
   BuiltType Type;

public:
   using ConventionType = ImplResultConvention;

   static Optional<ConventionType>
   getConventionFromString(StringRef conventionString)
   {
      if (conventionString == "@out")
         return ConventionType::Indirect;
      if (conventionString == "@owned")
         return ConventionType::Owned;
      if (conventionString == "@unowned")
         return ConventionType::Unowned;
      if (conventionString == "@unowned_inner_pointer")
         return ConventionType::UnownedInnerPointer;
      if (conventionString == "@autoreleased")
         return ConventionType::Autoreleased;

      return None;
   }

   ImplFunctionResult(ImplResultConvention convention, BuiltType type)
      : Convention(convention), Type(type) {}

   ImplResultConvention getConvention() const
   {
      return Convention;
   }

   BuiltType getType() const
   {
      return Type;
   }
};

enum class ImplFunctionRepresentation
{
   Thick,
   Block,
   Thin,
   CFunctionPointer,
   Method,
   WitnessMethod,
   Closure
};

class ImplFunctionTypeFlags
{
   unsigned Rep : 3;
   unsigned Pseudogeneric : 1;
   unsigned Escaping : 1;

public:
   ImplFunctionTypeFlags() : Rep(0), Pseudogeneric(0), Escaping(0) {}

   ImplFunctionTypeFlags(ImplFunctionRepresentation rep,
                         bool pseudogeneric, bool noescape)
      : Rep(unsigned(rep)), Pseudogeneric(pseudogeneric), Escaping(noescape)
   {}

   ImplFunctionTypeFlags
   withRepresentation(ImplFunctionRepresentation rep) const
   {
      return ImplFunctionTypeFlags(rep, Pseudogeneric, Escaping);
   }

   ImplFunctionTypeFlags
   withEscaping() const
   {
      return ImplFunctionTypeFlags(ImplFunctionRepresentation(Rep),
                                   Pseudogeneric, true);
   }

   ImplFunctionTypeFlags
   withPseudogeneric() const
   {
      return ImplFunctionTypeFlags(ImplFunctionRepresentation(Rep),
                                   true, Escaping);
   }

   ImplFunctionRepresentation getRepresentation() const
   {
      return ImplFunctionRepresentation(Rep);
   }

   bool isEscaping() const
   {
      return Escaping;
   }

   bool isPseudogeneric() const
   {
      return Pseudogeneric;
   }
};

/// Decode a mangled type to construct an abstract type, forming such
/// types by invoking a custom builder.
template <typename BuilderType>
class TypeDecoder
{
   using BuiltType = typename BuilderType::BuiltType;
   using BuiltTypeDecl = typename BuilderType::BuiltTypeDecl;
   using BuiltProtocolDecl = typename BuilderType::BuiltProtocolDecl;
   using NodeKind = demangling::Node::Kind;

   BuilderType &m_builder;

public:
   explicit TypeDecoder(BuilderType &builder)
      : m_builder(builder) {}

   /// Given a demangle tree, attempt to turn it into a type.
   BuiltType decodeMangledType(NodePointer node)
   {
      if (!node) return BuiltType();
      using NodeKind = demangling::Node::Kind;
      switch (node->getKind()) {
      case NodeKind::Global:
         if (node->getNumChildren() < 1) {
            return BuiltType();
         }
         return decodeMangledType(node->getChild(0));
      case NodeKind::TypeMangling:
         if (node->getNumChildren() < 1) {
            return BuiltType();
         }
         return decodeMangledType(node->getChild(0));
      case NodeKind::Type:
         if (node->getNumChildren() < 1) {
            return BuiltType();
         }
         return decodeMangledType(node->getChild(0));
      case NodeKind::Class:
      {
         LLVM_FALLTHROUGH;
      }
      case NodeKind::Enum:
      case NodeKind::Structure:
      case NodeKind::TypeAlias:
      case NodeKind::TypeSymbolicReference:
      {
         BuiltTypeDecl typeDecl = BuiltTypeDecl();
         BuiltType parent = BuiltType();
         bool typeAlias = false;
         if (!decodeMangledTypeDecl(node, typeDecl, parent, typeAlias)) {
            return BuiltType();
         }
         if (typeAlias) {
            return m_builder.createTypeAliasType(typeDecl, parent);
         }
         return m_builder.createNominalType(typeDecl, parent);
      }

      case NodeKind::BoundGenericEnum:
      case NodeKind::BoundGenericStructure:
      case NodeKind::BoundGenericClass:
      case NodeKind::BoundGenericTypeAlias:
      case NodeKind::BoundGenericOtherNominalType: {
         if (node->getNumChildren() < 2) {
            return BuiltType();
         }
         SmallVector<BuiltType, 8> args;
         const auto &genericArgs = node->getChild(1);
         if (genericArgs->getKind() != NodeKind::TypeList) {
            return BuiltType();
         }
         for (auto genericArg : *genericArgs) {
            auto paramType = decodeMangledType(genericArg);
            if (!paramType) {
               return BuiltType();
            }
            args.push_back(paramType);
         }

         auto childNode = node->getChild(0);
         if (childnode->getKind() == NodeKind::Type &&
             childnode->getNumChildren() > 0) {
            childNode = childnode->getChild(0);
         }
         BuiltTypeDecl typeDecl = BuiltTypeDecl();
         BuiltType parent = BuiltType();
         bool typeAlias = false;
         if (!decodeMangledTypeDecl(childNode, typeDecl,
                                    parent, typeAlias)) {
            return BuiltType();
         }
         return m_builder.createBoundGenericType(typeDecl, args, parent);
      }
      case NodeKind::BoundGenericProtocol: {
         // This is a special case. When you write a protocol typealias with a
         // concrete type base, for example:
         //
         // protocol P { typealias A<T> = ... }
         // struct S : P {}
         // let x: S.A<Int> = ...
         //
         // The mangling tree looks like this:
         //
         // BoundGenericProtocol ---> BoundGenericTypeAlias
         // |                         |
         // |                         |
         // --> protocol: P           --> TypeAlias: A
         // |                         |
         // --> TypeList:             --> TypeList:
         //     |                         |
         //     --> Structure: S          --> Structure: Int
         //
         // When resolving the mangling tree to a decl, we strip off the
         // BoundGenericProtocol's *argument*, leaving behind only the
         // protocol reference.
         //
         // But when resolving it to a type, we want to *keep* the argument
         // so that the parent type becomes 'S' and not 'P'.
         if (node->getNumChildren() < 2) {
            return BuiltType();
         }
         const auto &genericArgs = node->getChild(1);
         if (genericArgs->getNumChildren() != 1) {
            return BuiltType();
         }
         return decodeMangledType(genericArgs->getChild(0));
      }
      case NodeKind::BuiltinTypeName: {
         auto mangledName = demangling::mangle_node(node);
         return m_builder.createBuiltinType(node->getText(), mangledName);
      }
      case NodeKind::Metatype:
      case NodeKind::ExistentialMetatype: {
         unsigned i = 0;
         Optional<ImplMetatypeRepresentation> repr;

         // Handle lowered metatypes in a hackish way. If the representation
         // was not thin, force the resulting typeref to have a non-empty
         // representation.
         if (node->getNumChildren() >= 2) {
            auto reprNode = node->getChild(i++);
            if (reprnode->getKind() != NodeKind::MetatypeRepresentation ||
                !reprnode->hasText()) {
               return BuiltType();
            }
            if (reprnode->getText() == "@thin") {
               repr = ImplMetatypeRepresentation::Thin;
            } else if (reprnode->getText() == "@thick") {
               repr = ImplMetatypeRepresentation::Thick;
            }
         } else if (node->getNumChildren() < 1) {
            return BuiltType();
         }

         auto instance = decodeMangledType(node->getChild(i));
         if (!instance) {
            return BuiltType();
         }
         if (node->getKind() == NodeKind::Metatype) {
            return m_builder.createMetatypeType(instance, repr);
         } else if (node->getKind() == NodeKind::ExistentialMetatype) {
            return m_builder.createExistentialMetatypeType(instance, repr);
         } else {
            assert(false);
            return nullptr;
         }
      }
      case NodeKind::ProtocolList:
      case NodeKind::ProtocolListWithAnyObject:
      case NodeKind::ProtocolListWithClass: {
         if (node->getNumChildren() < 1) {
            return BuiltType();
         }
         // Find the protocol list.
         SmallVector<BuiltProtocolDecl, 8> protocols;
         auto typeList = node->getChild(0);
         if (typeList->getKind() == NodeKind::ProtocolList &&
             typeList->getNumChildren() >= 1) {
            typeList = typeList->getChild(0);
         }

         // demangling the protocol list.
         for (auto componentType : *typeList) {
            if (auto protocol = decodeMangledProtocolType(componentType)) {
               protocols.push_back(protocol);
            } else {
               return BuiltType();
            }
         }

         // superclass or AnyObject, if present.
         bool isClassBound = false;
         auto superclass = BuiltType();
         if (node->getKind() == NodeKind::ProtocolListWithClass) {
            if (node->getNumChildren() < 2)
               return BuiltType();

            auto superclassNode = node->getChild(1);
            superclass = decodeMangledType(superclassNode);
            if (!superclass) {
               return BuiltType();
            }
            isClassBound = true;
         } else if (node->getKind() == NodeKind::ProtocolListWithAnyObject) {
            isClassBound = true;
         }

         return m_builder.createProtocolCompositionType(protocols, superclass,
                                                        isClassBound);
      }

      case NodeKind::Protocol:
      case NodeKind::ProtocolSymbolicReference: {
         if (auto proto = decodeMangledProtocolType(node)) {
            return m_builder.createProtocolCompositionType(proto, BuiltType(),
                                                           /*isClassBound=*/false);
         }

         return BuiltType();
      }
      case NodeKind::DynamicSelf: {
         if (node->getNumChildren() != 1) {
            return BuiltType();
         }
         auto selfType = decodeMangledType(node->getChild(0));
         if (!selfType) {
            return BuiltType();
         }
         return m_builder.createDynamicSelfType(selfType);
      }
      case NodeKind::DependentGenericParamType: {
         auto depth = node->getChild(0)->getIndex();
         auto index = node->getChild(1)->getIndex();
         return m_builder.createGenericTypeParameterType(depth, index);
      }
      case NodeKind::ObjCBlock:
      case NodeKind::CFunctionPointer:
      case NodeKind::ThinFunctionType:
      case NodeKind::NoEscapeFunctionType:
      case NodeKind::AutoClosureType:
      case NodeKind::EscapingAutoClosureType:
      case NodeKind::FunctionType: {
         if (node->getNumChildren() < 2) {
            return BuiltType();
         }
         FunctionTypeFlags flags;
         if (node->getKind() == NodeKind::ObjCBlock) {
            flags = flags.withConvention(FunctionMetadataConvention::Block);
         } else if (node->getKind() == NodeKind::CFunctionPointer) {
            flags =
                  flags.withConvention(FunctionMetadataConvention::CFunctionPointer);
         } else if (node->getKind() == NodeKind::ThinFunctionType) {
            flags = flags.withConvention(FunctionMetadataConvention::Thin);
         }

         bool isThrow =
               node->getChild(0)->getKind() == NodeKind::ThrowsAnnotation;
         flags = flags.withThrows(isThrow);

         if (isThrow && node->getNumChildren() < 3) {
            return BuiltType();
         }
         bool hasParamFlags = false;
         SmallVector<FunctionParam<BuiltType>, 8> parameters;
         if (!decodeMangledFunctionInputType(node->getChild(isThrow ? 1 : 0),
                                             parameters, hasParamFlags)) {
            return BuiltType();
         }
         flags =
               flags.withNumParameters(parameters.size())
               .withParameterFlags(hasParamFlags)
               .withEscaping(
                  node->getKind() == NodeKind::FunctionType ||
                  node->getKind() == NodeKind::EscapingAutoClosureType);

         auto result = decodeMangledType(node->getChild(isThrow ? 2 : 1));
         if (!result) return BuiltType();
         return m_builder.createFunctionType(parameters, result, flags);
      }
      case NodeKind::ImplFunctionType: {
         auto calleeConvention = ImplParameterConvention::Direct_Unowned;
         SmallVector<ImplFunctionParam<BuiltType>, 8> parameters;
         SmallVector<ImplFunctionResult<BuiltType>, 8> results;
         SmallVector<ImplFunctionResult<BuiltType>, 8> errorResults;
         ImplFunctionTypeFlags flags;

         for (unsigned i = 0; i < node->getNumChildren(); i++) {
            auto child = node->getChild(i);

            if (child->getKind() == NodeKind::ImplConvention) {
               if (!child->hasText()) {
                  return BuiltType();
               }
               if (child->getText() == "@convention(thin)") {
                  flags =
                        flags.withRepresentation(ImplFunctionRepresentation::Thin);
               } else if (child->getText() == "@callee_guaranteed") {
                  calleeConvention = ImplParameterConvention::Direct_Guaranteed;
               }
            } else if (child->getKind() == NodeKind::ImplFunctionAttribute) {
               if (!child->hasText()) {
                  return BuiltType();
               }

               StringRef text = child->getText();
               if (text == "@convention(c)") {
                  flags =
                        flags.withRepresentation(ImplFunctionRepresentation::CFunctionPointer);
               } else if (text == "@convention(block)") {
                  flags =
                        flags.withRepresentation(ImplFunctionRepresentation::Block);
               }
            } else if (child->getKind() == NodeKind::ImplEscaping) {
               flags = flags.withEscaping();
            } else if (child->getKind() == NodeKind::ImplParameter) {
               if (decodeImplFunctionPart(child, parameters))
                  return BuiltType();
            } else if (child->getKind() == NodeKind::ImplResult) {
               if (decodeImplFunctionPart(child, results))
                  return BuiltType();
            } else if (child->getKind() == NodeKind::ImplErrorResult) {
               if (decodeImplFunctionPart(child, errorResults))
                  return BuiltType();
            } else {
               return BuiltType();
            }
         }

         Optional<ImplFunctionResult<BuiltType>> errorResult;
         switch (errorResults.size()) {
         case 0:
            break;
         case 1:
            errorResult = errorResults.front();
            break;
         default:
            return BuiltType();
         }

         // TODO: Some cases not handled above, but *probably* they cannot
         // appear as the types of values in SIL (yet?):
         // - functions with yield returns
         // - functions with generic signatures
         // - foreign error conventions
         return m_builder.createImplFunctionType(calleeConvention,
                                                 parameters, results,
                                                 errorResult, flags);
      }

      case NodeKind::ArgumentTuple:
         if (node->getNumChildren() < 1)
            return BuiltType();

         return decodeMangledType(node->getChild(0));

      case NodeKind::ReturnType:
         if (node->getNumChildren() < 1)
            return BuiltType();

         return decodeMangledType(node->getChild(0));

      case NodeKind::Tuple: {
         SmallVector<BuiltType, 8> elements;
         std::string labels;
         bool variadic = false;
         for (auto &element : *node) {
            if (element->getKind() != NodeKind::TupleElement) {
               return BuiltType();
            }
            // If the tuple element is labeled, add its label to 'labels'.
            unsigned typeChildIndex = 0;
            unsigned nameIdx = 0;
            if (element->getChild(nameIdx)->getKind() == NodeKind::VariadicMarker) {
               variadic = true;
               nameIdx = 1;
               typeChildIndex = 1;
            }
            if (element->getChild(nameIdx)->getKind() == NodeKind::TupleElementName) {
               // Add spaces to terminate all the previous labels if this
               // is the first we've seen.
               if (labels.empty()) {
                  labels.append(elements.size(), ' ');
               }

               // Add the label and its terminator.
               labels += element->getChild(0)->getText();
               labels += ' ';
               typeChildIndex++;

               // Otherwise, add a space if a previous element had a label.
            } else if (!labels.empty()) {
               labels += ' ';
            }

            // Decode the element type.
            BuiltType elementType =
                  decodeMangledType(element->getChild(typeChildIndex));
            if (!elementType) {
               return BuiltType();
            }

            elements.push_back(elementType);
         }
         return m_builder.createTupleType(elements, std::move(labels), variadic);
      }
      case NodeKind::TupleElement:
         if (node->getNumChildren() < 1) {
            return BuiltType();
         }
         if (node->getChild(0)->getKind() == NodeKind::TupleElementName) {
            if (node->getNumChildren() < 2) {
               return BuiltType();
            }
            return decodeMangledType(node->getChild(1));
         }
         return decodeMangledType(node->getChild(0));
      case NodeKind::DependentGenericType: {
         if (node->getNumChildren() < 2) {
            return BuiltType();
         }
         return decodeMangledType(node->getChild(1));
      }
      case NodeKind::DependentMemberType: {
         if (node->getNumChildren() < 2) {
            return BuiltType();
         }
         auto base = decodeMangledType(node->getChild(0));
         if (!base) {
            return BuiltType();
         }
         auto assocTypeChild = node->getChild(1);
         auto member = assocTypeChild->getFirstChild()->getText();
         if (assocTypeChild->getNumChildren() < 2) {
            return m_builder.createDependentMemberType(member, base);
         }
         auto protocol = decodeMangledProtocolType(assocTypeChild->getChild(1));
         if (!protocol) {
            return BuiltType();
         }
         return m_builder.createDependentMemberType(member, base, protocol);
      }
      case NodeKind::DependentAssociatedTypeRef: {
         if (node->getNumChildren() < 2) {
            return BuiltType();
         }
         return decodeMangledType(node->getChild(1));
      }
      case NodeKind::Unowned: {
         if (node->getNumChildren() < 1) {
            return BuiltType();
         }
         auto base = decodeMangledType(node->getChild(0));
         if (!base) {
            return BuiltType();
         }
         return m_builder.createUnownedStorageType(base);
      }
      case NodeKind::Unmanaged: {
         if (node->getNumChildren() < 1) {
            return BuiltType();
         }
         auto base = decodeMangledType(node->getChild(0));
         if (!base) {
            return BuiltType();
         }
         return m_builder.createUnmanagedStorageType(base);
      }
      case NodeKind::Weak: {
         if (node->getNumChildren() < 1) {
            return BuiltType();
         }
         auto base = decodeMangledType(node->getChild(0));
         if (!base) {
            return BuiltType();
         }
         return m_builder.createWeakStorageType(base);
      }
      case NodeKind::SILBoxType: {
         if (node->getNumChildren() < 1) {
            return BuiltType();
         }
         auto base = decodeMangledType(node->getChild(0));
         if (!base) {
            return BuiltType();
         }
         return m_builder.createSILBoxType(base);
      }
      case NodeKind::SILBoxTypeWithLayout: {
         // TODO: Implement SILBoxTypeRefs with layout. As a stopgap, specify the
         // NativeObject type ref.
         return m_builder.createBuiltinType("Builtin.NativeObject", "Bo");
      }
      case NodeKind::SugaredOptional: {
         if (node->getNumChildren() < 1) {
            return BuiltType();
         }
         auto base = decodeMangledType(node->getChild(0));
         if (!base) {
            return BuiltType();
         }
         return m_builder.createOptionalType(base);
      }
      case NodeKind::SugaredArray: {
         if (node->getNumChildren() < 1) {
            return BuiltType();
         }
         auto base = decodeMangledType(node->getChild(0));
         if (!base) {
            return BuiltType();
         }
         return m_builder.createArrayType(base);
      }
      case NodeKind::SugaredDictionary: {
         if (node->getNumChildren() < 2) {
            return BuiltType();
         }
         auto key = decodeMangledType(node->getChild(0));
         if (!key) {
            return BuiltType();
         }
         auto value = decodeMangledType(node->getChild(1));
         if (!key) {
            return BuiltType();
         }
         return m_builder.createDictionaryType(key, value);
      }
      case NodeKind::SugaredParen: {
         if (node->getNumChildren() < 1) {
            return BuiltType();
         }
         auto base = decodeMangledType(node->getChild(0));
         if (!base) {
            return BuiltType();
         }
         return m_builder.createParenType(base);
      }
      case NodeKind::OpaqueType: {
         if (node->getNumChildren() < 3) {
            return BuiltType();
         }
         auto descriptor = node->getChild(0);
         auto ordinalNode = node->getChild(1);
         if (ordinalNode->getKind() != NodeKind::Index
             || !ordinalNode->hasIndex()) {
            return BuiltType();
         }
         auto ordinal = ordinalNode->getIndex();

         std::vector<BuiltType> genericArgsBuf;
         std::vector<unsigned> genericArgsLevels;
         auto boundGenerics = node->getChild(2);
         for (unsigned i = 0; i < boundGenerics->getNumChildren(); ++i) {
            genericArgsLevels.push_back(genericArgsBuf.size());
            auto genericsNode = boundGenerics->getChild(i);
            if (genericsNode->getKind() != NodeKind::TypeList) {
               break;
            }

            for (auto argNode : *genericsNode) {
               auto arg = decodeMangledType(argNode);
               if (!arg) {
                  return BuiltType();
               }
               genericArgsBuf.push_back(arg);
            }
         }
         genericArgsLevels.push_back(genericArgsBuf.size());
         std::vector<ArrayRef<BuiltType>> genericArgs;
         for (unsigned i = 0; i < genericArgsLevels.size() - 1; ++i) {
            auto start = genericArgsLevels[i], end = genericArgsLevels[i+1];
            genericArgs.emplace_back(genericArgsBuf.data() + start,
                                     end - start);
         }

         return m_builder.resolveOpaqueType(descriptor, genericArgs, ordinal);
      }
         // TODO: Handle OpaqueReturnType, when we're in the middle of reconstructing
         // the defining decl
      default:
         return BuiltType();
      }
   }

private:
   template <typename T>
   bool decodeImplFunctionPart(demangling::NodePointer node,
                               SmallVectorImpl<T> &results)
   {
      if (node->getNumChildren() != 2) {
         return true;
      }
      if (node->getChild(0)->getKind() != Node::Kind::ImplConvention ||
          node->getChild(1)->getKind() != Node::Kind::Type) {
         return true;
      }
      StringRef conventionString = node->getChild(0)->getText();
      Optional<typename T::ConventionType> convention =
            T::getConventionFromString(conventionString);
      if (!convention) {
         return true;
      }

      BuiltType type = decodeMangledType(node->getChild(1));
      if (!type) {
         return true;
      }

      results.emplace_back(*convention, type);
      return false;
   }

   bool decodeMangledTypeDecl(demangling::NodePointer node,
                              BuiltTypeDecl &typeDecl,
                              BuiltType &parent,
                              bool &typeAlias)
   {
      if (node->getKind() == NodeKind::Type) {
         return decodeMangledTypeDecl(node->getChild(0), typeDecl,
                                      parent, typeAlias);
      }
      demangling::NodePointer declNode;
      if (node->getKind() == NodeKind::TypeSymbolicReference) {
         // A symbolic reference can be directly resolved to a nominal type.
         declNode = node;
      } else {
         if (node->getNumChildren() < 2)
            return false;

         auto parentContext = node->getChild(0);

         // Nested types are handled a bit funny here because a
         // nominal typeref always stores its full mangled name,
         // in addition to a reference to the parent type. The
         // mangled name already includes the module and parent
         // types, if any.
         declNode = node;
         switch (parentContext->getKind()) {
         case Node::Kind::Module:
            break;
         case Node::Kind::Extension:
            // Decode the type being extended.
            if (parentContext->getNumChildren() < 2)
               return false;
            parentContext = parentContext->getChild(1);
            LLVM_FALLTHROUGH;
         default:
            parent = decodeMangledType(parentContext);
            // Remove any generic arguments from the context node, producing a
            // node that references the nominal type declaration.
            declNode = demangling::get_unspecialized(node, m_builder.getNodeFactory());
            break;
         }
      }
      typeDecl = m_builder.createTypeDecl(declNode, typeAlias);
      if (!typeDecl) {
         return false;
      }
      return true;
   }

   BuiltProtocolDecl decodeMangledProtocolType(demangling::NodePointer node) {
      if (node->getKind() == NodeKind::Type)
         return decodeMangledProtocolType(node->getChild(0));

      if ((node->getNumChildren() < 2 || node->getKind() != NodeKind::protocol)
          && node->getKind() != NodeKind::ProtocolSymbolicReference)
         return BuiltProtocolDecl();

      return m_builder.createProtocolDecl(node);
   }

   bool decodeMangledFunctionInputType(
         demangling::NodePointer node,
         SmallVectorImpl<FunctionParam<BuiltType>> &params,
         bool &hasParamFlags)
   {
      // Look through a couple of sugar nodes.
      if (node->getKind() == NodeKind::Type ||
          node->getKind() == NodeKind::ArgumentTuple) {
         return decodeMangledFunctionInputType(node->getFirstChild(), params,
                                               hasParamFlags);
      }

      auto decodeParamTypeAndFlags =
            [&](demangling::NodePointer typeNode,
            FunctionParam<BuiltType> &param) -> bool {
         demangling::NodePointer node = typeNode;

         auto setOwnership = [&](ValueOwnership ownership) {
            param.setValueOwnership(ownership);
            node = node->getFirstChild();
            hasParamFlags = true;
         };
         switch (node->getKind()) {
         case NodeKind::InOut:
            setOwnership(ValueOwnership::InOut);
            break;

         case NodeKind::Shared:
            setOwnership(ValueOwnership::Shared);
            break;

         case NodeKind::Owned:
            setOwnership(ValueOwnership::Owned);
            break;

         case NodeKind::AutoClosureType:
         case NodeKind::EscapingAutoClosureType: {
            param.setAutoClosure();
            hasParamFlags = true;
            break;
         }

         default:
            break;
         }

         auto paramType = decodeMangledType(node);
         if (!paramType) {
            return false;
         }
         param.setType(paramType);
         return true;
      };

      auto decodeParam = [&](NodePointer paramNode)
            -> Optional<FunctionParam<BuiltType>> {
         if (paramnode->getKind() != NodeKind::TupleElement)
            return None;

         FunctionParam<BuiltType> param;
         for (const auto &child : *paramNode) {
            switch (child->getKind()) {
            case NodeKind::TupleElementName:
               param.setLabel(child->getText());
               break;

            case NodeKind::VariadicMarker:
               param.setVariadic();
               hasParamFlags = true;
               break;

            case NodeKind::Type:
               if (!decodeParamTypeAndFlags(child->getFirstChild(), param))
                  return None;
               break;

            default:
               return None;
            }
         }

         return param;
      };

      // Expand a single level of tuple.
      if (node->getKind() == NodeKind::Tuple) {
         // Decode all the elements as separate arguments.
         for (const auto &elt : *node) {
            auto param = decodeParam(elt);
            if (!param)
               return false;

            params.push_back(std::move(*param));
         }

         return true;
      }

      // Otherwise, handle the type as a single argument.
      FunctionParam<BuiltType> param;
      if (!decodeParamTypeAndFlags(node, param))
         return false;

      params.push_back(std::move(param));
      return true;
   }
};

template<typename BuilderType>
inline typename BuilderType::BuiltType
decodeMangledType(BuilderType &m_builder,
                  NodePointer Node)
{
   return TypeDecoder<BuilderType>(m_builder).decodeMangledType(Node);
}

} // polar::demangling

#endif // POLARPHP_DEMANGLING_TYPEDECODER_H

