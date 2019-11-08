//===--- SyntaxCollection.h -------------------------------------*- C++ -*-===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/08.

#ifndef POLARPHP_SYNTAX_SYNTAXCOLLECTION_H
#define POLARPHP_SYNTAX_SYNTAXCOLLECTION_H

#include "polarphp/syntax/Syntax.h"
#include <iterator>

namespace polar::syntax {

template <SyntaxKind collectionKind, typename Element>
class SyntaxCollection;

template <SyntaxKind collectionKind, typename Element>
struct SyntaxCollectionIterator
{
   const SyntaxCollection<collectionKind, Element> &collection;
   size_t index;

   Element operator*()
   {
      return collection[index];
   }

   SyntaxCollectionIterator<collectionKind, Element> &operator++()
   {
      ++index;
      return *this;
   }

   bool
   operator==(const SyntaxCollectionIterator<collectionKind, Element> &other)
   {
      return collection.hasSameIdentityAs(other.collection) &&
            index == other.index;
   }

   bool
   operator!=(const SyntaxCollectionIterator<collectionKind, Element> &other)
   {
      return !operator==(other);

   }
};

/// A generic unbounded collection of syntax nodes
template <SyntaxKind collectionKind, typename Element>
class SyntaxCollection : public Syntax
{
private:
   static RefCountPtr<SyntaxData>
   makeData(std::initializer_list<Element> &elements)
   {
      std::vector<RefCountPtr<RawSyntax>> list;
      list.reserve(elements.size());
      for (auto &element : elements) {
         list.push_back(element.getRaw());
      }
      auto raw = RawSyntax::make(collectionKind, list,
                                 SourcePresence::Present);
      return SyntaxData::make(raw);
   }

   SyntaxCollection(const RefCountPtr<SyntaxData> root)
      : Syntax(root, root.get())
   {}

public:

   SyntaxCollection(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {}

   SyntaxCollection(std::initializer_list<Element> list)
      : SyntaxCollection(SyntaxCollection::makeData(list))
   {}

   /// Returns true if the collection is empty.
   bool empty() const
   {
      return size() == 0;
   }

   /// Returns the number of elements in the collection.
   size_t size() const
   {
      return getRaw()->getLayout().size();
   }

   SyntaxCollectionIterator<collectionKind, Element> begin() const
   {
      return SyntaxCollectionIterator<collectionKind, Element> {
         *this,
         0,
      };
   }

   SyntaxCollectionIterator<collectionKind, Element> end() const
   {
      return SyntaxCollectionIterator<collectionKind, Element> {
         *this,
         getRaw()->getLayout().size(),
      };
   }

   /// Return the element at the given index.
   ///
   /// Precondition: index < size()
   /// Precondition: !empty()
   Element operator[](const size_t index) const
   {
      assert(index < size());
      assert(!empty());
      return { m_root, m_data->getChild(index).get() };
   }

   /// Return a new collection with the given element added to the end.
   SyntaxCollection<collectionKind, Element>
   appending(Element element) const
   {
      auto oldLayout = getRaw()->getLayout();
      std::vector<RefCountPtr<RawSyntax>> newLayout;
      newLayout.reserve(oldLayout.size() + 1);
      std::copy(oldLayout.begin(), oldLayout.end(), std::back_inserter(newLayout));
      newLayout.push_back(element.getRaw());
      auto raw = RawSyntax::make(collectionKind, newLayout, getRaw()->getPresence());
      return m_data->replaceSelf<SyntaxCollection<collectionKind, Element>>(raw);
   }

   /// Return a new collection with an element removed from the end.
   ///
   /// Precondition: !empty()
   SyntaxCollection<collectionKind, Element> removingLast() const
   {
      assert(!empty());
      auto newLayout = getRaw()->getLayout().drop_back();
      auto raw = RawSyntax::make(collectionKind, newLayout, getRaw()->getPresence());
      return m_data->replaceSelf<SyntaxCollection<collectionKind, Element>>(raw);
   }

   /// Return a new collection with the given element appended to the front.
   SyntaxCollection<collectionKind, Element>
   prepending(Element element) const
   {
      auto oldLayout = getRaw()->getLayout();
      std::vector<RefCountPtr<RawSyntax>> newLayout = { element.getRaw() };
      std::copy(oldLayout.begin(), oldLayout.end(),
                std::back_inserter(newLayout));
      auto raw = RawSyntax::make(collectionKind, newLayout, getRaw()->getPresence());
      return m_data->replaceSelf<SyntaxCollection<collectionKind, Element>>(raw);
   }

   /// Return a new collection with an element removed from the end.
   ///
   /// Precondition: !empty()
   SyntaxCollection<collectionKind, Element> removingFirst() const
   {
      assert(!empty());
      auto newLayout = getRaw()->getLayout().drop_front();
      auto raw = RawSyntax::make(collectionKind, newLayout, getRaw()->getPresence());
      return m_data->replaceSelf<SyntaxCollection<collectionKind, Element>>(raw);
   }

   /// Return a new collection with the Element inserted at index i.
   ///
   /// Precondition: i <= size()
   SyntaxCollection<collectionKind, Element>
   inserting(size_t i, Element element) const
   {
      assert(i <= size());
      auto oldLayout = getRaw()->getLayout();
      std::vector<RefCountPtr<RawSyntax>> newLayout;
      newLayout.reserve(oldLayout.size() + 1);

      std::copy(oldLayout.begin(), oldLayout.begin() + i,
                std::back_inserter(newLayout));
      newLayout.push_back(element.getRaw());
      std::copy(oldLayout.begin() + i, oldLayout.end(),
                std::back_inserter(newLayout));
      auto raw = RawSyntax::make(collectionKind, newLayout, getRaw()->getPresence());
      return m_data->replaceSelf<SyntaxCollection<collectionKind, Element>>(raw);
   }

   /// Return a new collection with the element removed at index i.
   SyntaxCollection<collectionKind, Element> removing(size_t i) const
   {
      assert(i <= size());
      std::vector<RefCountPtr<RawSyntax>> newLayout = getRaw()->getLayout();
      auto iterator = newLayout.begin();
      std::advance(iterator, i);
      newLayout.erase(iterator);
      auto raw = RawSyntax::make(collectionKind, newLayout, getRaw()->getPresence());
      return m_data->replaceSelf<SyntaxCollection<collectionKind, Element>>(raw);
   }

   /// Return an empty syntax collection of this type.
   SyntaxCollection<collectionKind, Element> cleared() const
   {
      auto raw = RawSyntax::make(collectionKind, {}, getRaw()->getPresence());
      return m_data->replaceSelf<SyntaxCollection<collectionKind, Element>>(raw);
   }

   static bool kindOf(SyntaxKind kind)
   {
      return kind == collectionKind;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend struct SyntaxFactory;
   friend class Syntax;
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAXCOLLECTION_H
