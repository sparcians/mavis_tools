#include <iostream>
#include <iomanip>
#include <string>
#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include "mavis/extension_managers/RISCVExtensionManager.hpp"
#include "mavis_path.hpp"
#include "mnemonic_extension_map.hpp"

int main(int argc, char* argv[])
{
    std::string mavis_path;
    std::string isa_string;

    boost::program_options::options_description optional_args("Optional arguments");
    boost::program_options::options_description required_args("Required arguments");

    optional_args.add_options()("help,h", "print this help message")(
        "mavis,m",
        boost::program_options::value<std::string>(&mavis_path)
            ->default_value(getMavisPath())
            ->value_name("path"),
        "path to mavis");

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

    const auto print_help = [&optional_args]()
    {
        std::ios init(NULL);
        init.copyfmt(std::cerr);
        std::cerr << "Usage: isa_dump [OPTION] <isa string>" << std::endl
                  << "Dumps all instruction mnemonics enabled by the given ISA string" << std::endl
                  << std::endl
                  << optional_args << std::endl
                  << "Required arguments:" << std::endl
                  << std::setw(optional_args.get_option_column_width()) << std::left
                  << "  <isa string>";
        std::cerr.copyfmt(init);
        std::cerr << "RISC-V ISA string to dump" << std::endl;
    };

    if (vm.count("help") != 0)
    {
        print_help();
        return 0;
    }

    if (vm.count("isa-string") == 0)
    {
        std::cerr << "ISA string must be specified" << std::endl;
        print_help();
        return 1;
    }

    const auto [json_path, isa_spec_json] = getRISCVJSONInfo(mavis_path);

    mavis_tools::MnemonicExtensionMap mnemonic_map(
        mavis::extension_manager::riscv::RISCVExtensionManager::fromISA(isa_string, isa_spec_json,
                                                                        json_path));

    for (const auto & mnemonic : mnemonic_map.getMnemonics())
    {
        std::cout << mnemonic << std::endl;
    }

    return 0;
}
