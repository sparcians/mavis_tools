#include <iostream>
#include <iomanip>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include "mavis/extension_managers/RISCVExtensionManager.hpp"
#include "mavis_path.hpp"
#include "mnemonic_extension_map.hpp"
#include "maximal_isa_string_generator.hpp"
#include "mavis_types.hpp"

int main(int argc, char* argv[])
{
    static const std::vector<uint32_t> ALLOWED_XLENS{32, 64};

    std::string mavis_path;
    std::string opcode_str;
    std::vector<uint32_t> xlens;

    boost::program_options::options_description optional_args("Optional arguments");
    boost::program_options::options_description required_args("Required arguments");

    optional_args.add_options()("help,h", "print this help message")(
        "mavis,m",
        boost::program_options::value<std::string>(&mavis_path)
            ->default_value(getMavisPath())
            ->value_name("path"),
        "path to mavis")(
        "xlen,x",
        boost::program_options::value<std::vector<uint32_t>>(&xlens)->composing()->value_name("xn"),
        "restrict search to given XLEN(s)");

    required_args.add_options()("opcode", boost::program_options::value<std::string>(&opcode_str),
                                "Opcode to find");

    boost::program_options::positional_options_description p;
    p.add("opcode", 1);

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
        std::cerr << "Usage: identify_opcode [OPTION] <opcode>" << std::endl
                  << "Identifies all RISC-V instructions that map to the given opcode" << std::endl
                  << std::endl
                  << optional_args << std::endl
                  << "Required arguments:" << std::endl
                  << std::setw(optional_args.get_option_column_width()) << std::left
                  << "  <opcode>";
        std::cerr.copyfmt(init);
        std::cerr << "opcode to find" << std::endl;
    };

    if (vm.count("help") != 0)
    {
        print_help();
        return 0;
    }

    if (vm.count("opcode") == 0)
    {
        std::cerr << "Opcode must be specified" << std::endl;
        print_help();
        return 1;
    }

    if (xlens.empty())
    {
        xlens = ALLOWED_XLENS;
    }
    else
    {
        for (const auto xlen : xlens)
        {
            if (std::find(ALLOWED_XLENS.begin(), ALLOWED_XLENS.end(), xlen) == ALLOWED_XLENS.end())
            {
                std::cerr << "Invalid XLEN specified: " << xlen << std::endl
                          << "Allowed XLENs:" << std::endl;

                for (const auto allowed_xlen : ALLOWED_XLENS)
                {
                    std::cerr << "- " << allowed_xlen << std::endl;
                }

                print_help();
                return 1;
            }
        }
    }

    const auto [json_path, isa_spec_json] = getRISCVJSONInfo(mavis_path);

    const uint32_t opcode = std::stoul(opcode_str, nullptr, 16);

    auto ext_manager = mavis::extension_manager::riscv::RISCVExtensionManager::fromISASpecJSON(
        isa_spec_json, json_path);
    const mavis_tools::MaximalISAStringGenerator isa_string_gen(ext_manager, xlens);

    // Maps XLEN -> mnemonic -> formatted string of extension / combination(s) of extensions that
    // enable the mnemonic
    // Using an std::set ensures we don't get any duplicates
    std::map<uint32_t, std::map<std::string, std::set<std::string>>> found_mnemonics;
    bool is_unknown = false;
    bool is_illegal = false;

    for (const auto xlen : xlens)
    {
        auto & xlen_found_mnemonics = found_mnemonics[xlen];

        for (const auto & isa_string : isa_string_gen.getISAStrings(xlen))
        {
            ext_manager.setISA(isa_string);
            auto mavis =
                ext_manager
                    .constructMavis<mavis_tools::StubInstType, mavis_tools::StubAnnotationType>({});

            try
            {
                const auto decode_info = mavis.getInfo(opcode);
                const auto & mnemonic = decode_info->opinfo->getMnemonic();

                const mavis_tools::MnemonicExtensionMap ext_map(ext_manager);
                const auto & extensions = ext_map.getExtensions(mnemonic);

                if (extensions.empty()) [[unlikely]]
                {
                    throw std::runtime_error("Could not find extension for " + mnemonic);
                }

                xlen_found_mnemonics[mnemonic].emplace(
                    [&extensions]
                    {
                        const bool multiple_terms = extensions.size() > 1;

                        // For mnemonics enabled by a single extension, just return the name of the
                        // extension
                        if (!multiple_terms && extensions.front().size() == 1)
                        {
                            return extensions.front().front();
                        }

                        std::ostringstream os;

                        // For mnemonics that are enabled by multiple extensions, or only enabled if
                        // multiple extensions are present, generate a logical expression
                        bool first_outer = true;
                        for (const auto & ext_combination : extensions)
                        {
                            if (first_outer)
                            {
                                first_outer = false;
                            }
                            else
                            {
                                // The outer vector specifies ORed terms
                                os << " || ";
                            }

                            const bool multiple_inner_terms =
                                multiple_terms && ext_combination.size() > 1;

                            if (multiple_inner_terms)
                            {
                                os << '(';
                            }

                            bool first_inner = true;
                            for (const auto & ext : ext_combination)
                            {
                                if (first_inner)
                                {
                                    first_inner = false;
                                }
                                else
                                {
                                    // The inner vector specifies ANDed terms
                                    os << " && ";
                                }
                                os << ext;
                            }

                            if (multiple_inner_terms)
                            {
                                os << ')';
                            }
                        }

                        return os.str();
                    }());
            }
            catch (const mavis::UnknownOpcode &)
            {
                is_unknown = true;
            }
            catch (const mavis::IllegalOpcode &)
            {
                is_illegal = true;
            }
        }
    }

    if (!found_mnemonics.empty())
    {
        std::cout << "Candidates:" << std::endl;

        for (const auto & [xlen, mnemonic_info] : found_mnemonics)
        {
            for (const auto & [mnemonic, all_extensions] : mnemonic_info)
            {
                for (const auto & extensions : all_extensions)
                {
                    std::cout << "- xlen: " << xlen << std::endl
                              << "  mnemonic: " << mnemonic << std::endl
                              << "  extension: " << extensions << std::endl;
                }
            }
        }
    }
    else if (is_unknown)
    {
        std::cerr << std::hex << opcode << " is an unknown opcode" << std::endl;
    }
    else if (is_illegal)
    {
        std::cerr << std::hex << opcode << " is an illegal opcode" << std::endl;
        return 1;
    }

    return 0;
}
