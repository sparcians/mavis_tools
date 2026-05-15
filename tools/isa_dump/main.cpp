#include <iostream>
#include <iomanip>
#include <string>
#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include "mavis/extension_managers/RISCVExtensionManager.hpp"
#include "filesystem.hpp"
#include "mavis_path.hpp"
#include "mnemonic_extension_map.hpp"
#include "uarch_json.hpp"

int main(int argc, char* argv[])
{
    fs::path mavis_path;
    std::string isa_string;

    auto optional_args = mavis_tools::getDefaultOptionalArgs(mavis_path);
    boost::program_options::options_description required_args("Required arguments");

    mavis_tools::UArchJSONInfo::addOptions(optional_args);

    required_args.add_options()("isa-string",
                                boost::program_options::value<std::string>(&isa_string),
                                "ISA string to dump");

    boost::program_options::positional_options_description p;
    p.add("isa-string", 1);

    boost::program_options::options_description all_args("All arguments");
    all_args.add(optional_args).add(required_args);

    boost::program_options::variables_map vm;

    boost::program_options::store(boost::program_options::command_line_parser(argc, argv)
                                      .options(all_args)
                                      .positional(p)
                                      .run(),
                                  vm);
    boost::program_options::notify(vm);

    try
    {
        if (vm.count("help") != 0)
        {
            throw mavis_tools::CommandLineException();
        }

        if (vm.count("isa-string") == 0)
        {
            throw mavis_tools::CommandLineException("ISA string must be specified");
        }

        const auto [json_path, isa_spec_json] = mavis_tools::getRISCVJSONInfo(mavis_path);

        mavis_tools::MnemonicExtensionMap mnemonic_info(
            mavis::extension_manager::riscv::RISCVExtensionManager::fromISA(
                isa_string, isa_spec_json, json_path));

        const mavis_tools::UArchJSONInfo uarch_info(vm);

        if (uarch_info.empty())
        {
            std::cout << mnemonic_info;
        }
        else
        {
            const auto & mnemonic_key = mnemonic_info.getKey();

            std::cout << mnemonic_key;

            uarch_info.formatKeys(std::cout);

            std::cout << std::endl;

            for (const auto & [mnemonic, mnemonic_info] : mnemonic_info.getMnemonics())
            {
                mnemonic_key.formatColumn(std::cout, mnemonic);

                for (const auto & info : uarch_info)
                {
                    std::cout << info.get(mnemonic_info.dealias());
                }

                std::cout << std::endl;
            }
        }
    }
    catch (const mavis_tools::CommandLineException & ex)
    {
        if (ex.hasMessage())
        {
            std::cerr << ex.what() << std::endl;
        }

        std::ios init(NULL);
        init.copyfmt(std::cerr);
        std::cerr << "Usage: isa_dump [OPTION] [--] <isa string>" << std::endl
                  << "Dumps all instruction mnemonics enabled by the given ISA string" << std::endl
                  << std::endl
                  << optional_args << std::endl
                  << "Required arguments:" << std::endl
                  << std::setw(optional_args.get_option_column_width()) << std::left
                  << "  <isa string>";
        std::cerr.copyfmt(init);
        std::cerr << "RISC-V ISA string to dump" << std::endl;

        return ex.getReturnCode();
    }
    catch (const mavis_tools::MavisToolException & ex)
    {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
