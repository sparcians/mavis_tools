#pragma once

#include <vector>
#include <boost/program_options.hpp>

#include "mavis_path.hpp"

namespace mavis_tools
{
    // Gets a scalar command line parameter value from boost::program_options::variables_map
    template <typename ValueType> struct CommandLineParamGetter
    {
        template <typename FinalType = ValueType>
        static constexpr FinalType getValue(const std::string_view name,
                                            const boost::program_options::variables_map & vm)
        {
            const auto value = vm[std::string(name)].as<ValueType>();

            if constexpr (std::is_same_v<ValueType, FinalType>)
            {
                return value;
            }
            else
            {
                return FinalType(value);
            }
        }
    };

    // Gets a vector command line parameter value from boost::program_options::variables_map
    template <typename ValueType> struct CommandLineParamGetter<std::vector<ValueType>>
    {
        template <typename FinalType = std::vector<ValueType>>
        static constexpr FinalType getValue(const std::string_view name,
                                            const boost::program_options::variables_map & vm)
        {
            const auto value = vm[std::string(name)].as<std::vector<ValueType>>();

            if constexpr (std::is_same_v<std::vector<ValueType>, FinalType>)
            {
                return value;
            }
            else
            {
                return FinalType(value.begin(), value.end());
            }
        }
    };

    // Helper class for defining and getting command line parameters
    template <typename ValueType> class CommandLineParam
    {
      private:
        const std::string_view name_;
        const char short_param_;
        const bool multitoken_;
        const std::string_view help_msg_;

      public:
        consteval CommandLineParam(const std::string_view name, const std::string_view help_msg) :
            name_(name),
            short_param_('\0'),
            multitoken_(false),
            help_msg_(help_msg)
        {
        }

        consteval CommandLineParam(const std::string_view name, const char short_param,
                                   const std::string_view help_msg) :
            name_(name),
            short_param_(short_param),
            multitoken_(false),
            help_msg_(help_msg)
        {
        }

        consteval CommandLineParam(const std::string_view name, const bool multitoken,
                                   const std::string_view help_msg) :
            name_(name),
            short_param_('\0'),
            multitoken_(multitoken),
            help_msg_(help_msg)
        {
        }

        consteval CommandLineParam(const std::string_view name, const char short_param,
                                   const bool multitoken, const std::string_view help_msg) :
            name_(name),
            short_param_(short_param),
            multitoken_(multitoken),
            help_msg_(help_msg)
        {
        }

        constexpr bool exists(const boost::program_options::variables_map & vm) const
        {
            return vm.count(std::string(name_)) != 0;
        }

        constexpr void addOption(boost::program_options::options_description & args) const
        {
            std::string param_str{name_};
            if (short_param_)
            {
                param_str += ',';
                param_str += short_param_;
            }

            auto value = boost::program_options::value<ValueType>()->value_name(std::string(name_));

            if (multitoken_)
            {
                value->multitoken();
            }

            args.add_options()(param_str.c_str(), value, help_msg_.data());
        }

        template <typename FinalType = ValueType>
        constexpr FinalType getValue(const boost::program_options::variables_map & vm) const
        {
            if (exists(vm))
            {
                return CommandLineParamGetter<ValueType>::template getValue<FinalType>(name_, vm);
            }

            return {};
        }
    };

    inline boost::program_options::options_description getDefaultOptionalArgs(fs::path & mavis_path)
    {
        boost::program_options::options_description optional_args("Optional arguments");
        optional_args.add_options()("help,h", "print this help message")(
            "mavis,m",
            boost::program_options::value<fs::path>(&mavis_path)
                ->default_value(mavis_tools::getMavisPath())
                ->value_name("path"),
            "path to mavis");
        return optional_args;
    }
} // namespace mavis_tools
