#pragma once

#include "mavis/ExtensionManager.hpp"
#include "constants.hpp"
#include "json_key.hpp"

namespace mavis_tools
{
    // Maps mnemonics to the extensions that enable them
    class MnemonicExtensionMap
    {
      public:
        class Mnemonic
        {
          private:
            std::string parent_;
            std::vector<std::vector<std::string>> enabling_extensions_;

          public:
            Mnemonic(const std::string & mnemonic,
                     const std::vector<std::vector<std::string>> & enabling_extensions) :
                parent_(mnemonic),
                enabling_extensions_(enabling_extensions)
            {
            }

            void setParent(const boost::json::string & parent) { parent_ = parent.c_str(); }

            const std::string & dealias() const { return parent_; }

            const std::vector<std::vector<std::string>> & getExtensions() const
            {
                return enabling_extensions_;
            }

            // auto operator<=>(const Mnemonic & rhs) const
            //{
            //     return std::string_view(mnemonic_) <=> std::string_view(rhs.mnemonic_);
            // }

            // bool operator==(const Mnemonic & rhs) const { return mnemonic_ == rhs.mnemonic_; }
        };

      private:
        using MnemonicMap = std::map<std::string, Mnemonic>;

        JSONKey mnemonic_key_{MNEMONIC_KEY};
        MnemonicMap mnemonic_map_;

      public:
        // Requires a fully-specified ExtensionManager instance (i.e., the ISA string has been set
        // with setISA)
        template <typename ExtensionInfo, typename ExtensionState>
        explicit MnemonicExtensionMap(
            const mavis::extension_manager::ExtensionManager<ExtensionInfo, ExtensionState> &
                ext_man)
        {
            const auto & json_dir = ext_man.getMavisJSONDir();
            const auto extensions = ext_man.getEnabledExtensions(true, true);

            for (const auto & [name, ext] : extensions)
            {
                if (const auto & json = ext->getJSON(); !json.empty())
                {
                    const auto json_value = mavis::parseJSON(json_dir + "/" + json);
                    const auto & jobj = json_value.as_array();

                    for (const auto & inst_info : jobj)
                    {
                        const auto & inst_obj = inst_info.as_object();
                        const auto mnemonic =
                            boost::json::value_to<std::string>(inst_obj.at(mnemonic_key_.getKey()));

                        mnemonic_key_.updateWidth(mnemonic);

                        auto & mnemonic_info =
                            mnemonic_map_
                                .try_emplace(mnemonic, mnemonic,
                                             ext->isInternalOnly()
                                                 ? ext_man.getEnablingExtensions(ext)
                                                 : std::vector<std::vector<std::string>>{{name}})
                                .first->second;

                        if (const auto it = inst_obj.find("expand"); it != inst_obj.end())
                        {
                            mnemonic_info.setParent(it->value().as_string());
                        }
                    }
                }
            }
        }

        const JSONKey & getKey() const { return mnemonic_key_; }

        const auto & getMnemonics() const { return mnemonic_map_; }

        const std::vector<std::vector<std::string>> &
        getExtensions(const std::string & mnemonic) const
        {
            return mnemonic_map_.at(mnemonic).getExtensions();
        }

        friend inline std::ostream & operator<<(std::ostream & os,
                                                const MnemonicExtensionMap & mnemonic_info)
        {
            for (const auto & [mnemonic, _] : mnemonic_info.getMnemonics())
            {
                os << mnemonic << std::endl;
            }

            return os;
        }
    };
} // namespace mavis_tools
