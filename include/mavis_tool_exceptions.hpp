#pragma once

#include <exception>
#include <string>

namespace mavis_tools
{
    class MavisToolException : public std::exception
    {
      protected:
        const std::string msg_;

      public:
        MavisToolException() = default;

        explicit MavisToolException(std::string && msg) : msg_(std::move(msg)) {}

        const char* what() const noexcept override final { return msg_.c_str(); }
    };

    class CommandLineException : public MavisToolException
    {
      private:
        const int return_code_;

      public:
        explicit CommandLineException(const int return_code = 0) :
            MavisToolException(),
            return_code_(return_code)
        {
        }

        explicit CommandLineException(std::string && msg, const int return_code = 1) :
            MavisToolException(std::forward<std::string>(msg)),
            return_code_(return_code)
        {
        }

        bool hasMessage() const { return !msg_.empty(); }

        int getReturnCode() const { return return_code_; }
    };
} // namespace mavis_tools
