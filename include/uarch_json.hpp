#pragma once

#include <regex>
#include <string>
#include <unordered_map>

#include "command_line_param.hpp"
#include "filesystem.hpp"
#include "constants.hpp"
#include "mavis_tool_exceptions.hpp"
#include "json_key.hpp"

namespace mavis_tools
{
    class NotAJSONFileException : public MavisToolException
    {
      public:
        explicit NotAJSONFileException(const fs::path & path) :
            MavisToolException(path.string() + " is not a JSON file")
        {
        }
    };

    class NotAJSONDirectoryException : public MavisToolException
    {
      public:
        explicit NotAJSONDirectoryException(const fs::path & path) :
            MavisToolException(path.string() + " is not a directory")
        {
        }
    };

    class MissingUArchInfoException : public MavisToolException
    {
      public:
        explicit MissingUArchInfoException(const boost::json::string & mnemonic) :
            MissingUArchInfoException(std::string(mnemonic.c_str()))
        {
        }

        explicit MissingUArchInfoException(const std::string & mnemonic) :
            MavisToolException("Mnemonic " + mnemonic + " is not present in the uarch info")
        {
        }
    };

    class DuplicateUArchInfoException : public MavisToolException
    {
      public:
        DuplicateUArchInfoException(const boost::json::string & mnemonic,
                                    const fs::path & first_file, const fs::path & second_file) :
            MavisToolException("Duplicate entries found for mnemonic "
                               + std::string(mnemonic.c_str()) + " in files " + first_file.string()
                               + " and " + second_file.string())
        {
        }
    };

    // Holds Mavis uarch JSON data
    class UArchJSONInfo
    {
      private:
        // Maps a single uarch JSON key to all of its per-mnemonic values
        class UArchJSONEntry
        {
          private:
            JSONKey key_;
            std::unordered_map<std::string, boost::json::value> values_;

          public:
            class Ref
            {
              private:
                const JSONKey & key_;
                const boost::json::value & value_;

              public:
                Ref(const JSONKey & key, const boost::json::value & value) :
                    key_(key),
                    value_(value)
                {
                }

                friend inline std::ostream & operator<<(std::ostream & os, const Ref & ref)
                {
                    ref.key_.formatColumn(os, ref.value_);
                    return os;
                }
            };

            UArchJSONEntry() = default;

            explicit UArchJSONEntry(const JSONKey & key) : key_(key) {}

            void update(const boost::json::string & mnemonic, const boost::json::object & json_obj)
            {
                const auto & key = key_.getKey();
                const auto & value = json_obj.at(key);
                values_[mnemonic.c_str()] = value;
                key_.updateWidth(value);
            }

            const JSONKey & getKey() const { return key_; }

            Ref get(const std::string & mnemonic) const
            {
                const auto it = values_.find(mnemonic);

                if (__builtin_expect(it == values_.end(), 0))
                {
                    throw MissingUArchInfoException(mnemonic);
                }

                return Ref(key_, it->second);
            }

            friend inline std::istream & operator>>(std::istream & is, UArchJSONEntry & entry)
            {
                is >> entry.key_;
                return is;
            }
        };

        inline static constexpr CommandLineParam<std::vector<fs::path>> UARCH_JSON_PARAM_{
            "uarch_json", 'j', true, "path(s) to uarch JSON(s)"};
        inline static constexpr CommandLineParam<std::vector<fs::path>> UARCH_JSON_DIR_PARAM_{
            "uarch_json_dir", 'J', "path to uarch JSON directory"};
        inline static constexpr CommandLineParam<std::vector<UArchJSONEntry>> UARCH_JSON_KEY_PARAM_{
            "uarch_json_key", 'k', "uarch JSON key(s)"};
        inline static constexpr CommandLineParam<std::vector<std::string>>
            JSON_INCLUDE_REGEX_PARAM_{"include",
                                      "regex pattern(s) for files to include from JSON directory"};

        std::vector<UArchJSONEntry> uarch_info_;

      public:
        explicit UArchJSONInfo(const boost::program_options::variables_map & vm) :
            uarch_info_(UARCH_JSON_KEY_PARAM_.getValue(vm))
        {
            if (JSON_INCLUDE_REGEX_PARAM_.exists(vm) && !UARCH_JSON_DIR_PARAM_.exists(vm)
                && !UARCH_JSON_PARAM_.exists(vm))
            {
                throw CommandLineException("--include argument requires at some uarch JSONs to be "
                                           "specified with -J or -j");
            }

            if (!UARCH_JSON_KEY_PARAM_.exists(vm))
            {
                if (UARCH_JSON_PARAM_.exists(vm))
                {
                    throw CommandLineException(
                        "-j argument requires at least one uarch JSON key to be specified with -k");
                }
                else if (UARCH_JSON_DIR_PARAM_.exists(vm))
                {
                    throw CommandLineException(
                        "-J argument requires at least one uarch JSON key to be specified with -k");
                }
            }
            else
            {
                const auto uarch_jsons = UARCH_JSON_PARAM_.getValue(vm);
                const auto uarch_json_dirs = UARCH_JSON_DIR_PARAM_.getValue(vm);
                const auto json_include_regex =
                    JSON_INCLUDE_REGEX_PARAM_.getValue<std::vector<std::regex>>(vm);

                const auto canonicalize = [](const fs::path & path) { return fs::canonical(path); };

                std::set<fs::path> json_results;

                const auto append_json =
                    [&json_include_regex, &json_results, &canonicalize](const fs::path & uarch_json)
                {
                    const auto is_json = [](const fs::path & abs_path)
                    { return fs::is_regular_file(abs_path) && abs_path.extension() == ".json"; };

                    const auto is_included = [&json_include_regex](const fs::path & abs_path)
                    {
                        if (json_include_regex.empty())
                        {
                            return true;
                        }
                        const auto filename = abs_path.filename();
                        const auto & filename_str = filename.native();
                        return std::any_of(json_include_regex.begin(), json_include_regex.end(),
                                           [&filename_str](const auto & regex)
                                           { return std::regex_search(filename_str, regex); });
                    };

                    if (is_included(uarch_json))
                    {
                        const auto canonicalized_json = canonicalize(uarch_json);

                        if (!is_json(canonicalized_json))
                        {
                            throw NotAJSONFileException(uarch_json);
                        }

                        json_results.emplace(canonicalized_json);
                    }
                };

                for (const auto & uarch_json : uarch_jsons)
                {
                    append_json(uarch_json);
                }

                for (const auto & uarch_json_dir : uarch_json_dirs)
                {
                    const auto & abs_path = canonicalize(uarch_json_dir);

                    if (!fs::is_directory(abs_path))
                    {
                        throw NotAJSONDirectoryException(uarch_json_dir);
                    }

                    for (const auto & uarch_json : fs::directory_iterator{abs_path})
                    {
                        append_json(uarch_json);
                    }
                }

                if (json_results.empty())
                {
                    throw CommandLineException(
                        "-k argument requires at least one uarch JSON to be specified with -j/-J");
                }

                std::unordered_map<boost::json::string, std::set<fs::path>::const_iterator>
                    mnemonic_to_json_map;

                for (auto it = json_results.begin(); it != json_results.end(); ++it)
                {
                    const auto & uarch_json = *it;
                    const auto json_value = mavis::parseJSON(uarch_json);
                    for (const auto & elem : json_value.as_array())
                    {
                        const auto & json_obj = elem.as_object();
                        const auto & mnemonic = json_obj.at(MNEMONIC_KEY).as_string();
                        const auto dup_check_result =
                            mnemonic_to_json_map.try_emplace(mnemonic, it);
                        if (!dup_check_result.second)
                        {
                            throw DuplicateUArchInfoException(
                                mnemonic, *dup_check_result.first->second, uarch_json);
                        }

                        for (auto & info : uarch_info_)
                        {
                            info.update(mnemonic, json_obj);
                        }
                    }
                }
            }
        }

        bool empty() const { return uarch_info_.empty(); }

        void formatKeys(std::ostream & os) const
        {
            for (const auto & info : uarch_info_)
            {
                os << info.getKey();
            }
        }

        auto begin() const { return uarch_info_.begin(); }

        auto end() const { return uarch_info_.end(); }

        inline static constexpr void
        addOptions(boost::program_options::options_description & optional_args)
        {
            UARCH_JSON_PARAM_.addOption(optional_args);
            UARCH_JSON_DIR_PARAM_.addOption(optional_args);
            JSON_INCLUDE_REGEX_PARAM_.addOption(optional_args);
            UARCH_JSON_KEY_PARAM_.addOption(optional_args);
        }
    };
} // namespace mavis_tools
