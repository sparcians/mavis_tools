#pragma once

#include <exception>
#include <string>
#include <sstream>

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

    class OpcodeException : public MavisToolException
    {
        protected:
            static inline std::string toHex_(const uint32_t val)
            {
                std::ostringstream os;
                os << std::hex << val;
                return os.str();
            }

            const uint32_t opcode_;

        public:
            OpcodeException(std::string&& msg, const uint32_t opcode) :
                MavisToolException(std::forward<std::string>(msg)),
                opcode_(opcode)
            {
            }

            uint32_t getOpcode() const { return opcode_; }
    };

} // namespace mavis_tools
