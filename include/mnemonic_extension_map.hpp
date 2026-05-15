#pragma once

#include "mavis/ExtensionManager.hpp"

namespace mavis_tools
{
    // Maps mnemonics to the extensions that enable them
    class MnemonicExtensionMap
    {
      private:
        std::map<std::string, std::vector<std::vector<std::string>>> mnemonic_map_;
        mutable std::vector<std::string> mnemonics_;

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
                        const auto mnemonic = boost::json::value_to<std::string>(
                            inst_info.as_object().at("mnemonic"));

                        if (ext->isInternalOnly())
                        {
                            mnemonic_map_[mnemonic] = ext_man.getEnablingExtensions(ext);
                        }

                        else
                        {
                            mnemonic_map_[mnemonic] = {{name}};
                        }
                    }
                }
            }
        }

        const std::vector<std::string> & getMnemonics() const
        {
            if (mnemonics_.empty()) [[unlikely]]
            {
                for (const auto & [mnemonic, _] : mnemonic_map_)
                {
                    mnemonics_.emplace_back(mnemonic);
                }
            }

            return mnemonics_;
        }

        const std::vector<std::vector<std::string>> &
        getExtensions(const std::string & mnemonic) const
        {
            return mnemonic_map_.at(mnemonic);
        }
    };
} // namespace mavis_tools
