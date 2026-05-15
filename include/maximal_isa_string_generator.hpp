#pragma once

#include <map>
#include <string>
#include <vector>

#include <boost/graph/subgraph.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/sequential_vertex_coloring.hpp>

#include "mavis/extension_managers/RISCVExtensionManager.hpp"

namespace mavis_tools
{
    // Generates enough different ISA strings to fully cover all ISA extensions while accounting for
    // conflicts. This is implemented as its own class in case we want to pre-generate the strings
    // at build time in the future. Right now, this is fast enough to do at runtime.
    class MaximalISAStringGenerator
    {
      private:
        template <typename ExtensionInfo, typename ExtensionState, typename SkipExtensionCallback,
                  typename GenBaseISAStringCallback, typename AppendExtensionCallback>
        void generate_(
            const mavis::extension_manager::ExtensionManager<ExtensionInfo, ExtensionState> & man,
            const std::vector<uint32_t> & arches, SkipExtensionCallback && skip_extension,
            GenBaseISAStringCallback && gen_base_isa_string,
            AppendExtensionCallback && append_extension)
        {
            // Helper lambda to get a const property map from a non-const graph
            const auto get_const_property_map =
                []<typename PropertyType>(const PropertyType property,
                                          const mavis::extension_manager::ConflictGraph & graph)
            { return get(property, graph); };

            isa_strings_.clear();

            for (const auto arch_id : arches)
            {
                auto ext_graph = man.getConflictGraph(arch_id);

                // Divide the graph into two subgraphs:
                // 1. Extensions that have no conflicts - these can always be part of the ISA string
                // 2. Extensions that have at least 1 conflict - these need to be in separate ISA
                // strings
                mavis::extension_manager::ConflictGraph & always_enabled_extensions =
                    ext_graph.create_subgraph();
                mavis::extension_manager::ConflictGraph & conflicting_extensions =
                    ext_graph.create_subgraph();

                for (auto [it, end] = boost::vertices(ext_graph); it != end; ++it)
                {
                    const auto vertex = *it;
                    if (boost::degree(vertex, ext_graph) != 0)
                    {
                        boost::add_vertex(vertex, conflicting_extensions);
                    }
                    else
                    {
                        boost::add_vertex(vertex, always_enabled_extensions);
                    }
                }

                // If there are no conflicts, there is only 1 color needed
                size_t num_colors = 1;

                std::vector<mavis::extension_manager::ConflictGraph::vertices_size_type> color_vec;

                if (const auto num_conflicts = boost::num_vertices(conflicting_extensions);
                    num_conflicts != 0)
                {
                    // Create a color entry for every vertex in the conflict graph
                    color_vec.resize(num_conflicts);
                    const auto color = boost::make_iterator_property_map(
                        color_vec.begin(),
                        get_const_property_map(boost::vertex_index, conflicting_extensions));

                    // Perform a simple graph coloring. Each conflicting extension will be assigned
                    // a different color. Each color corresponds to a unique ISA string.
                    num_colors = boost::sequential_vertex_coloring(conflicting_extensions, color);
                }

                std::string base_isa_string = gen_base_isa_string(arch_id);

                const auto name_props =
                    get_const_property_map(boost::vertex_name, always_enabled_extensions);

                for (auto [it, end] = boost::vertices(always_enabled_extensions); it != end; ++it)
                {
                    const auto & name = get(name_props, *it);

                    if (skip_extension(name))
                    {
                        continue;
                    }

                    append_extension(base_isa_string, name);
                }

                const auto emplace_result =
                    isa_strings_.try_emplace(arch_id, num_colors, base_isa_string);
                if (!emplace_result.second) [[unlikely]]
                {
                    throw std::runtime_error("Arch ID " + std::to_string(arch_id)
                                             + " was given multiple times");
                }

                auto & isa_strings = emplace_result.first->second;

                const auto conflict_name_props =
                    get_const_property_map(boost::vertex_name, conflicting_extensions);

                for (uint32_t i = 0; i < color_vec.size(); ++i)
                {
                    append_extension(
                        isa_strings[color_vec[i]],
                        get(conflict_name_props, boost::vertex(i, conflicting_extensions)));
                }
            }
        }

        std::map<uint32_t, std::vector<std::string>> isa_strings_;

      public:
        // Requires an extension manager that has at least had its ISA specification set with
        // setISASpecJSON
        MaximalISAStringGenerator(
            const mavis::extension_manager::riscv::RISCVExtensionManager & man,
            const std::vector<uint32_t> & arches)
        {
            generate_(
                man, arches, [](const std::string & ext) { return ext == "i"; },
                [](const uint32_t xlen) { return "rv" + std::to_string(xlen) + "i"; },
                [](std::string & isa_string, const std::string & ext) { isa_string += '_' + ext; });
        }

        const std::vector<std::string> & getISAStrings(const uint32_t arch_id) const
        {
            return isa_strings_.at(arch_id);
        }
    };
} // namespace mavis_tools
