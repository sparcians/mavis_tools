#pragma once

#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include "filesystem.hpp"

class CouldNotFindExecutableException : public std::exception
{
    public:
        const char* what() const noexcept override
        {
            return "Failed to get path to exe";
        }
};

/**
 * Gets the path to the current executable
 */
inline fs::path getExecutablePath() {
#ifdef __APPLE__ // OSX
    static constexpr uint32_t DEFAULT_BUF_SIZE = 256;
    uint32_t buf_size = DEFAULT_BUF_SIZE;
    std::unique_ptr<char[]> path_buf = std::make_unique<char[]>(DEFAULT_BUF_SIZE);

    if(_NSGetExecutablePath(path_buf.get(), &buf_size) != 0) {
        // path_buf was too small, so reallocate and try again
        path_buf = std::make_unique<char[]>(buf_size);
        if(_NSGetExecutablePath(path_buf.get(), &buf_size) != 0)
        {
            throw CouldNotFindExecutableException();
        }
    }

    const char* const relative_path = path_buf.get();

    // fs::read_symlink will throw an exception if it isn't a symlink, so check first
    if(!fs::is_symlink(relative_path)) {
        return fs::canonical(relative_path);
    }
#else // Linux
    static const char* const relative_path = "/proc/self/exe"; // always a symlink
#endif
    return fs::canonical(fs::read_symlink(relative_path));
}

class CouldNotFindMavisException : public std::exception
{
    public:
        const char* what() const noexcept override
        {
            return "Could not find Mavis JSON files anywhere. Please define the MAVIS_PATH environment variable to point to your Mavis checkout.";
        }
};

/**
 * Gets a default path to Mavis
 */
inline std::string getMavisPath() {
    static constexpr std::string_view DEFAULT_RELATIVE_JSON_PATH_ = "../../../mavis"; /**< Default path to Mavis JSON files if building locally*/

    const auto mavis_path_env = getenv("MAVIS_PATH");
    if(mavis_path_env && fs::exists(mavis_path_env)) {
        return std::string(mavis_path_env);
    }

    const auto exe_dir = getExecutablePath().parent_path();

    const auto relative_path = exe_dir / DEFAULT_RELATIVE_JSON_PATH_;

    if(fs::exists(relative_path)) {
        return relative_path;
    }

    static constexpr std::string_view SHARE_RELATIVE_JSON_PATH_ = "../share/mavis_tools/mavis";

    const auto share_relative_path = exe_dir / SHARE_RELATIVE_JSON_PATH_;

    if(fs::exists(share_relative_path)) {
        return share_relative_path;
    }

    static constexpr std::string_view SHARE_GLOBAL_JSON_PATH_ = "/usr/share/mavis_tools/mavis";

    if(fs::exists(SHARE_GLOBAL_JSON_PATH_)) {
        return std::string(SHARE_GLOBAL_JSON_PATH_);
    }

    // Check for an existing stf_tools install if there isn't a global mavis_tools install
    static constexpr std::string_view STF_SHARE_RELATIVE_JSON_PATH_ = "../share/stf_tools/mavis";

    const auto stf_share_relative_path = exe_dir / STF_SHARE_RELATIVE_JSON_PATH_;

    if(fs::exists(stf_share_relative_path)) {
        return share_relative_path;
    }

    static constexpr std::string_view STF_SHARE_GLOBAL_JSON_PATH_ = "/usr/share/stf_tools/mavis";

    if(fs::exists(STF_SHARE_GLOBAL_JSON_PATH_)) {
        return std::string(STF_SHARE_GLOBAL_JSON_PATH_);
    }

#ifdef MAVIS_GLOBAL_PATH
    static constexpr std::string_view DEFAULT_GLOBAL_JSON_PATH_ = MAVIS_GLOBAL_PATH; /**< Default path to globally installed Mavis JSON files*/

    if(fs::exists(DEFAULT_GLOBAL_JSON_PATH_)) {
        return std::string(DEFAULT_GLOBAL_JSON_PATH_);
    }
#endif

    throw CouldNotFindMavisException();
}
