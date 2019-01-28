// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/05.

#ifndef POLARPHP_BASIC_ADT_INTRUSIVE_LIST_NODE_H
#define POLARPHP_BASIC_ADT_INTRUSIVE_LIST_NODE_H

#include "polarphp/basic/adt/IntrusiveListNodeBase.h"
#include "polarphp/basic/adt/IntrusiveListNodeOptions.h"

namespace polar {
namespace basic {

namespace ilist_internal {

struct NodeAccess;

} // end namespace ilist_internal

template <typename OptionsType, bool IsReverse, bool IsConst>
class IntrusiveListIterator;

template <typename OptionsType>
class IntrusiveListSentinel;

/// Implementation for an ilist node.
///
/// Templated on an appropriate \a ilist_internal::node_options, usually computed
/// by \a ilist_internal::ComputeNodeOptions.
///
/// This is a wrapper around \a IntrusiveListNode_base whose main purpose is to
/// provide type safety: you can't insert nodes of \a IntrusiveListNodeImpl into the
/// wrong \a simple_ilist or \a iplist.
template <typename OptionsType>
class IntrusiveListNodeImpl : OptionsType::NodeBaseType {
   using value_type = typename OptionsType::value_type;
   using NodeBaseType = typename OptionsType::NodeBaseType;
   using ListBaseType = typename OptionsType::ListBaseType;

   friend typename OptionsType::ListBaseType;
   friend struct ilist_internal::NodeAccess;
   friend class IntrusiveListSentinel<OptionsType>;
   friend class IntrusiveListIterator<OptionsType, false, false>;
   friend class IntrusiveListIterator<OptionsType, false, true>;
   friend class IntrusiveListIterator<OptionsType, true, false>;
   friend class IntrusiveListIterator<OptionsType, true, true>;

protected:
   using self_iterator = IntrusiveListIterator<OptionsType, false, false>;
   using const_self_iterator = IntrusiveListIterator<OptionsType, false, true>;
   using reverse_self_iterator = IntrusiveListIterator<OptionsType, true, false>;
   using const_reverse_self_iterator = IntrusiveListIterator<OptionsType, true, true>;

   IntrusiveListNodeImpl() = default;

private:
   IntrusiveListNodeImpl *getPrev()
   {
      return static_cast<IntrusiveListNodeImpl *>(NodeBaseType::getPrev());
   }

   IntrusiveListNodeImpl *getNext()
   {
      return static_cast<IntrusiveListNodeImpl *>(NodeBaseType::getNext());
   }

   const IntrusiveListNodeImpl *getPrev() const
   {
      return static_cast<IntrusiveListNodeImpl *>(NodeBaseType::getPrev());
   }

   const IntrusiveListNodeImpl *getNext() const
   {
      return static_cast<IntrusiveListNodeImpl *>(NodeBaseType::getNext());
   }

   void setPrev(IntrusiveListNodeImpl *node)
   {
      NodeBaseType::setPrev(node);
   }

   void setNext(IntrusiveListNodeImpl *node)
   {
      NodeBaseType::setNext(node);
   }

public:
   self_iterator getIterator()
   {
      return self_iterator(*this);
   }

   const_self_iterator getIterator() const
   {
      return const_self_iterator(*this);
   }

   reverse_self_iterator getReverseIterator()
   {
      return reverse_self_iterator(*this);
   }

   const_reverse_self_iterator getReverseIterator() const
   {
      return const_reverse_self_iterator(*this);
   }

   // Under-approximation, but always available for assertions.
   using NodeBaseType::isKnownSentinel;

   /// Check whether this is the sentinel node.
   ///
   /// This requires sentinel tracking to be explicitly enabled.  Use the
   /// IntrusiveListSentinel_tracking<true> option to get this API.
   bool isSentinel() const
   {
      static_assert(OptionsType::sm_isSentinelTrackingExplicit,
                    "Use IntrusiveListSentinelTracking<true> to enable isSentinel()");
      return NodeBaseType::isSentinel();
   }
};

/// An intrusive list node.
///
/// A base class to enable membership in intrusive lists, including \a
/// simple_ilist, \a iplist, and \a ilist.  The first template parameter is the
/// \a value_type for the list.
///
/// An ilist node can be configured with compile-time options to change
/// behaviour and/or add API.
///
/// By default, an \a IntrusiveListNode knows whether it is the list sentinel (an
/// instance of \a IntrusiveListSentinel) if and only if
/// LLVM_ENABLE_ABI_BREAKING_CHECKS.  The function \a isKnownSentinel() always
/// returns \c false tracking is off.  Sentinel tracking steals a bit from the
/// "prev" link, which adds a mask operation when decrementing an iterator, but
/// enables bug-finding assertions in \a IntrusiveListIterator.
///
/// To turn sentinel tracking on all the time, pass in the
/// IntrusiveListSentinel_tracking<true> template parameter.  This also enables the \a
/// isSentinel() function.  The same option must be passed to the intrusive
/// list.  (IntrusiveListSentinel_tracking<false> turns sentinel tracking off all the
/// time.)
///
/// A type can inherit from IntrusiveListNode multiple times by passing in different
/// \a ilist_tag options.  This allows a single instance to be inserted into
/// multiple lists simultaneously, where each list is given the same tag.
///
/// \example
/// struct A {};
/// struct B {};
/// struct N : IntrusiveListNode<N, ilist_tag<A>>, IntrusiveListNode<N, ilist_tag<B>> {};
///
/// void foo() {
///   simple_ilist<N, ilist_tag<A>> ListA;
///   simple_ilist<N, ilist_tag<B>> ListB;
///   N N1;
///   ListA.push_back(N1);
///   ListB.push_back(N1);
/// }
/// \endexample
///
/// See \a is_valid_option for steps on adding a new option.
template <typename T, typename... Options>
class IntrusiveListNode
      : public IntrusiveListNodeImpl<
      typename ilist_internal::ComputeNodeOptions<T, Options...>::type>
{
   static_assert(ilist_internal::CheckOptions<Options...>::value,
                 "Unrecognized node option!");
};

namespace ilist_internal
{

/// An access class for IntrusiveListNode private API.
///
/// This gives access to the private parts of ilist nodes.  Nodes for an ilist
/// should friend this class if they inherit privately from IntrusiveListNode.
///
/// Using this class outside of the ilist implementation is unsupported.
struct NodeAccess
{
protected:
   template <typename OptionsType>
   static IntrusiveListNodeImpl<OptionsType> *getNodePtr(typename OptionsType::pointer node)
   {
      return node;
   }

   template <typename OptionsType>
   static const IntrusiveListNodeImpl<OptionsType> *
   getNodePtr(typename OptionsType::const_pointer node)
   {
      return node;
   }

   template <typename OptionsType>
   static typename OptionsType::pointer getValuePtr(IntrusiveListNodeImpl<OptionsType> *node)
   {
      return static_cast<typename OptionsType::pointer>(node);
   }

   template <typename OptionsType>
   static typename OptionsType::const_pointer
   getValuePtr(const IntrusiveListNodeImpl<OptionsType> *node)
   {
      return static_cast<typename OptionsType::const_pointer>(node);
   }

   template <typename OptionsType>
   static IntrusiveListNodeImpl<OptionsType> *getPrev(IntrusiveListNodeImpl<OptionsType> &node)
   {
      return node.getPrev();
   }

   template <typename OptionsType>
   static IntrusiveListNodeImpl<OptionsType> *getNext(IntrusiveListNodeImpl<OptionsType> &node)
   {
      return node.getNext();
   }

   template <typename OptionsType>
   static const IntrusiveListNodeImpl<OptionsType> *
   getPrev(const IntrusiveListNodeImpl<OptionsType> &node)
   {
      return node.getPrev();
   }

   template <typename OptionsType>
   static const IntrusiveListNodeImpl<OptionsType> *
   getNext(const IntrusiveListNodeImpl<OptionsType> &node)
   {
      return node.getNext();
   }
};

template <typename OptionsType>
struct SpecificNodeAccess : NodeAccess
{
protected:
   using pointer = typename OptionsType::pointer;
   using const_pointer = typename OptionsType::const_pointer;
   using node_type = IntrusiveListNodeImpl<OptionsType>;

   static node_type *getNodePtr(pointer node)
   {
      return NodeAccess::getNodePtr<OptionsType>(node);
   }

   static const node_type *getNodePtr(const_pointer node)
   {
      return NodeAccess::getNodePtr<OptionsType>(node);
   }

   static pointer getValuePtr(node_type *node)
   {
      return NodeAccess::getValuePtr<OptionsType>(node);
   }

   static const_pointer getValuePtr(const node_type *node)
   {
      return NodeAccess::getValuePtr<OptionsType>(node);
   }
};

} // end namespace ilist_internal

template <typename OptionsType>
class IntrusiveListSentinel : public IntrusiveListNodeImpl<OptionsType>
{
public:
   IntrusiveListSentinel()
   {
      this->initializeSentinel();
      reset();
   }

   void reset()
   {
      this->setPrev(this);
      this->setNext(this);
   }

   bool empty() const
   {
      return this == this->getPrev();
   }
};

/// An ilist node that can access its parent list.
///
/// Requires \c NodeType to have \a getParent() to find the parent node, and the
/// \c ParentType to have \a getSublistAccess() to get a reference to the list.
template <typename NodeType, typename ParentType, class... Options>
class IntrusiveListNodeWithParent : public IntrusiveListNode<NodeType, Options...>
{
protected:
   IntrusiveListNodeWithParent() = default;

private:
   /// Forward to NodeType::getParent().
   ///
   /// Note: do not use the name "getParent()".  We want a compile error
   /// (instead of recursion) when the subclass fails to implement \a
   /// getParent().
   const ParentType *getNodeParent() const
   {
      return static_cast<const NodeType *>(this)->getParent();
   }

public:
   /// @name Adjacent Node Accessors
   /// @{
   /// \brief Get the previous node, or \c nullptr for the list head.
   NodeType *getPrevNode()
   {
      // Should be separated to a reused function, but then we couldn't use auto
      // (and would need the type of the list).
      const auto &list =
            getNodeParent()->*(ParentType::getSublistAccess((NodeType *)nullptr));
      return list.getPrevNode(*static_cast<NodeType *>(this));
   }

   /// \brief Get the previous node, or \c nullptr for the list head.
   const NodeType *getPrevNode() const
   {
      return const_cast<IntrusiveListNodeWithParent *>(this)->getPrevNode();
   }

   /// \brief Get the next node, or \c nullptr for the list tail.
   NodeType *getNextNode()
   {
      // Should be separated to a reused function, but then we couldn't use auto
      // (and would need the type of the list).
      const auto &list =
            getNodeParent()->*(ParentType::getSublistAccess((NodeType *)nullptr));
      return list.getNextNode(*static_cast<NodeType *>(this));
   }

   /// \brief Get the next node, or \c nullptr for the list tail.
   const NodeType *getNextNode() const
   {
      return const_cast<IntrusiveListNodeWithParent *>(this)->getNextNode();
   }
   /// @}
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_INTRUSIVE_LIST_NODE_H
