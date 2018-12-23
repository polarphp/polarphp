// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/01.

//===----------------------------------------------------------------------===//
//
// This file defines a simple interface that can be used to print out generic
// LLVM graphs to ".dot" files.  "dot" is a tool that is part of the AT&T
// graphviz package (http://www.research.att.com/sw/tools/graphviz/) which can
// be used to turn the files output by this interface into a variety of
// different graphics formats.
//
// Graphs do not need to implement any interface past what is already required
// by the GraphTraits template, but they can choose to implement specializations
// of the DOTGraphTraits template if they want to customize the graphs output in
// any way.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_GRAPH_WRITER_H
#define POLARPHP_UTILS_GRAPH_WRITER_H

#include "polarphp/basic/adt/GraphTraits.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/utils/DotGraphTraits.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/RawOutStream.h"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string>
#include <type_traits>
#include <vector>

namespace polar {
namespace utils {

using polar::basic::GraphTraits;
using polar::basic::Twine;

namespace dot {  // Private functions...

std::string escape_string(const std::string &label);

/// \brief Get a color string for this node number. Simply round-robin selects
/// from a reasonable number of colors.
StringRef get_color_string(unsigned nodeNumber);

} // end namespace DOT

namespace graphprogram {

enum Name {
   DOT,
   FDP,
   NEATO,
   TWOPI,
   CIRCO
};

} // end namespace graphprogram

bool display_graph(StringRef filename, bool wait = true,
                   graphprogram::Name program = graphprogram::DOT);

template<typename GraphType>
class GraphWriter
{
   RawOutStream &m_outstream;
   const GraphType &m_graph;

   using DOTTraits = DOTGraphTraits<GraphType>;
   using GTraits = GraphTraits<GraphType>;
   using NodeRef = typename GTraits::NodeRef;
   using node_iterator = typename GTraits::nodes_iterator;
   using child_iterator = typename GTraits::ChildIteratorType;
   DOTTraits m_dtraits;

   static_assert(std::is_pointer<NodeRef>::value,
                 "FIXME: Currently GraphWriter requires the NodeRef type to be "
                 "a pointer.\nThe pointer usage should be moved to "
                 "DOTGraphTraits, and removed from GraphWriter itself.");

   // Writes the edge labels of the node to outstream and returns true if there are any
   // edge labels not equal to the empty string "".
   bool getEdgeSourceLabels(RawOutStream &outstream, NodeRef node) {
      child_iterator eiter = GTraits::childBegin(node);
      child_iterator eend = GTraits::childEnd(node);
      bool hasEdgeSourceLabels = false;

      for (unsigned i = 0; eiter != eend && i != 64; ++eiter, ++i) {
         std::string label = m_dtraits.getEdgeSourceLabel(node, eiter);
         if (label.empty()) {
            continue;
         }
         hasEdgeSourceLabels = true;
         if (i) {
            outstream << "|";
         }
         outstream << "<s" << i << ">" << dot::escape_string(label);
      }

      if (eiter != eend && hasEdgeSourceLabels) {
         outstream << "|<s64>truncated...";
      }
      return hasEdgeSourceLabels;
   }

public:
   GraphWriter(RawOutStream &outstream, const GraphType &graph, bool sn)
      : m_outstream(outstream),
        m_graph(graph)
   {
      m_dtraits = DOTTraits(sn);
   }

   void writeGraph(const std::string &title = "")
   {
      // Output the header for the graph...
      writeHeader(title);

      // Emit all of the nodes in the graph...
      writeNodes();

      // Output any customizations on the graph
      DOTGraphTraits<GraphType>::addCustomGraphFeatures(m_graph, *this);

      // Output the end of the graph
      writeFooter();
   }

   void writeHeader(const std::string &title)
   {
      std::string graphName = m_dtraits.getGraphName(m_graph);

      if (!title.empty()) {
         m_outstream << "digraph \"" << dot::escape_string(title) << "\" {\n";
      } else if (!graphName.empty()) {
         m_outstream << "digraph \"" << dot::escape_string(graphName) << "\" {\n";
      } else {
         m_outstream << "digraph unnamed {\n";
      }
      if (m_dtraits.renderGraphFromBottomUp()) {
         m_outstream << "\trankdir=\"BT\";\n";
      }
      if (!title.empty()) {
         m_outstream << "\tlabel=\"" << dot::escape_string(title) << "\";\n";
      } else if (!graphName.empty()) {
         m_outstream << "\tlabel=\"" << dot::escape_string(graphName) << "\";\n";
      }

      m_outstream << m_dtraits.getGraphProperties(m_graph);
      m_outstream << "\n";
   }

   void writeFooter()
   {
      // Finish off the graph
      m_outstream << "}\n";
   }

   void writeNodes()
   {
      // Loop over the graph, printing it out...
      for (const auto node : polar::basic::nodes<GraphType>(m_graph)) {
         if (!isNodeHidden(node)) {
            writeNode(node);
         }
      }
   }

   bool isNodeHidden(NodeRef node)
   {
      return m_dtraits.isNodeHidden(node);
   }

   void writeNode(NodeRef node)
   {
      std::string NodeAttributes = m_dtraits.getNodeAttributes(node, m_graph);

      m_outstream << "\tNode" << static_cast<const void*>(node) << " [shape=record,";
      if (!NodeAttributes.empty()) {
         m_outstream << NodeAttributes << ",";
      }
      m_outstream << "label=\"{";

      if (!m_dtraits.renderGraphFromBottomUp()) {
         m_outstream << dot::escape_string(m_dtraits.getNodeLabel(node, m_graph));

         // If we should include the address of the node in the label, do so now.
         std::string id = m_dtraits.getNodeIdentifierLabel(node, m_graph);
         if (!id.empty()) {
            m_outstream << "|" << dot::escape_string(id);
         }
         std::string nodeDesc = m_dtraits.getNodeDescription(node, m_graph);
         if (!nodeDesc.empty()) {
            m_outstream << "|" << dot::escape_string(nodeDesc);
         }
      }

      std::string edgeSourceLabel;
      RawStringOutStream edgeSourceLabels(edgeSourceLabel);
      bool hasEdgeSourceLabels = getEdgeSourceLabels(edgeSourceLabels, node);

      if (hasEdgeSourceLabels) {
         if (!m_dtraits.renderGraphFromBottomUp()) {
            m_outstream << "|";
         }
         m_outstream << "{" << edgeSourceLabels.getStr() << "}";
         if (m_dtraits.renderGraphFromBottomUp()) {
            m_outstream << "|";
         }
      }

      if (m_dtraits.renderGraphFromBottomUp()) {
         m_outstream << dot::escape_string(m_dtraits.getNodeLabel(node, m_graph));

         // If we should include the address of the node in the label, do so now.
         std::string id = m_dtraits.getNodeIdentifierLabel(node, m_graph);
         if (!id.empty()) {
            m_outstream << "|" << dot::escape_string(id);
         }

         std::string nodeDesc = m_dtraits.getNodeDescription(node, m_graph);
         if (!nodeDesc.empty()) {
            m_outstream << "|" << dot::escape_string(nodeDesc);
         }
      }

      if (m_dtraits.hasEdgeDestLabels()) {
         m_outstream << "|{";

         unsigned i = 0, e = m_dtraits.numEdgeDestLabels(node);
         for (; i != e && i != 64; ++i) {
            if (i) {
               m_outstream << "|";
            }
            m_outstream << "<d" << i << ">"
                        << dot::escape_string(m_dtraits.getEdgeDestLabel(node, i));
         }

         if (i != e) {
            m_outstream << "|<d64>truncated...";
         }
         m_outstream << "}";
      }

      m_outstream << "}\"];\n";   // Finish printing the "node" line

      // Output all of the edges now
      child_iterator eiter = GTraits::child_begin(node);
      child_iterator eend = GTraits::child_end(node);
      for (unsigned i = 0; eiter != eend && i != 64; ++eiter, ++i) {
         if (!m_dtraits.isNodeHidden(*eiter)) {
            writeEdge(node, i, eiter);
         }
      }
      for (; eiter != eend; ++eiter) {
         if (!m_dtraits.isNodeHidden(*eiter)) {
            writeEdge(node, 64, eiter);
         }
      }
   }

   void writeEdge(NodeRef node, unsigned edgeidx, child_iterator eiter)
   {
      if (NodeRef targetNode = *eiter) {
         int destPort = -1;
         if (m_dtraits.edgeTargetsEdgeSource(node, eiter)) {
            child_iterator targetIter = m_dtraits.getEdgeTarget(node, eiter);

            // Figure out which edge this targets...
            unsigned Offset =
                  (unsigned)std::distance(GTraits::child_begin(targetNode), targetIter);
            destPort = static_cast<int>(Offset);
         }

         if (m_dtraits.getEdgeSourceLabel(node, eiter).empty()) {
            edgeidx = -1;
         }
         emitEdge(static_cast<const void*>(node), edgeidx,
                  static_cast<const void*>(targetNode), destPort,
                  m_dtraits.getEdgeAttributes(node, eiter, m_graph));
      }
   }

   /// emitSimpleNode - Outputs a simple (non-record) node
   void emitSimpleNode(const void *id, const std::string &attr,
                       const std::string &label, unsigned numEdgeSources = 0,
                       const std::vector<std::string> *edgeSourceLabels = nullptr)
   {
      m_outstream << "\tNode" << id << "[ ";
      if (!attr.empty()) {
         m_outstream << attr << ",";
      }
      m_outstream << " label =\"";
      if (numEdgeSources) m_outstream << "{";
      m_outstream << dot::escape_string(label);
      if (numEdgeSources) {
         m_outstream << "|{";
         for (unsigned i = 0; i != numEdgeSources; ++i) {
            if (i) {
               m_outstream << "|";
            }
            m_outstream << "<s" << i << ">";
            if (edgeSourceLabels) {
               m_outstream << dot::escape_string((*edgeSourceLabels)[i]);
            }
         }
         m_outstream << "}}";
      }
      m_outstream << "\"];\n";
   }

   /// emitEdge - Output an edge from a simple node into the graph...
   void emitEdge(const void *srcNodeID, int srcNodePort,
                 const void *destNodeID, int destNodePort,
                 const std::string &atrs)
   {
      if (srcNodePort  > 64) {
         return;             // Eminating from truncated part?
      }
      if (destNodePort > 64) {
         destNodePort = 64;  // Targeting the truncated part?
      }

      m_outstream << "\tNode" << srcNodeID;
      if (srcNodePort >= 0) {
         m_outstream << ":s" << srcNodePort;
      }

      m_outstream << " -> node" << destNodeID;
      if (destNodePort >= 0 && m_dtraits.hasEdgeDestLabels()) {
         m_outstream << ":d" << destNodePort;
      }
      if (!atrs.empty()) {
         m_outstream << "[" << atrs << "]";
      }

      m_outstream << ";\n";
   }

   /// getOStream - Get the raw output stream into the graph file. Useful to
   /// write fancy things using addCustomGraphFeatures().
   RawOutStream &getOStream()
   {
      return m_outstream;
   }
};

template<typename GraphType>
RawOutStream &write_graph(RawOutStream &m_outstream, const GraphType &m_graph,
                          bool shortNames = false,
                          const Twine &title = "") {
   // Start the graph emission process...
   GraphWriter<GraphType> writer(m_outstream, m_graph, shortNames);

   // Emit the graph.
   writer.writeGraph(title.getStr());

   return m_outstream;
}

std::string create_graph_filename(const Twine &name, int &fd);

/// Writes graph into a provided {@code filename}.
/// If {@code filename} is empty, generates a random one.
/// \return The resulting filename, or an empty string if writing
/// failed.
template <typename GraphType>
std::string write_graph(const GraphType &graph, const Twine &name,
                        bool shortNames = false, const Twine &title = "",
                        std::string filename = "") {
   int fd;
   // Windows can't always handle long paths, so limit the length of the name.
   std::string N = name.getStr();
   N = N.substr(0, std::min<std::size_t>(N.size(), 140));
   if (filename.empty()) {
      filename = create_graph_filename(N, fd);
   } else {
      std::error_code errorCode = polar::fs::open_file_for_write(filename, fd);

      // Writing over an existing file is not considered an error.
      if (errorCode == std::errc::file_exists) {
         error_stream() << "file exists, overwriting" << "\n";
      } else if (errorCode) {
         error_stream() << "error writing into file" << "\n";
         return "";
      }
   }
   RawFdOutStream out(fd, /*shouldClose=*/ true);

   if (fd == -1) {
      error_stream() << "error opening file '" << filename << "' for writing!\n";
      return "";
   }
   write_graph(out, graph, shortNames, title);
   error_stream() << " done. \n";
   return filename;
}

/// view_graph - Emit a dot graph, run 'dot', run gv on the postscript file,
/// then cleanup.  For use from the debugger.
///
template<typename GraphType>
void view_graph(const GraphType &graph, const Twine &name,
                bool shortNames = false, const Twine &title = "",
                graphprogram::Name program = graphprogram::DOT)
{
   std::string filename = write_graph(graph, name, shortNames, title);
   if (filename.empty()) {
      return;
   }
   display_graph(filename, false, program);
}

} // utils
} // polar

#endif // POLARPHP_UTILS_GRAPH_WRITER_H
