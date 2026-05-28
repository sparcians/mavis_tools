#pragma once

#include <boost/json.hpp>
#include <iomanip>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

namespace mavis_tools
{
    // Holds a JSON key name and calculates the optimal width for a column-formatted view
    class JSONKey
    {
      private:
        static constexpr uint32_t COLUMN_PADDING_ = 4;

        std::string key_;
        size_t column_width_{key_.size() + COLUMN_PADDING_};

        void updateWidth_(const size_t new_width)
        {
            column_width_ = std::max(column_width_, new_width + COLUMN_PADDING_);
        }

        void formatColumn_(std::ostream & os) const { os << std::left << std::setw(column_width_); }

      public:
        JSONKey() = default;

        explicit JSONKey(const std::string_view key) : key_(key) {}
        explicit JSONKey(const boost::json::string_view key) : key_(key) {}

        const std::string & getKey() const { return key_; }

        void updateWidth(const boost::json::string & str) { updateWidth_(str.size()); }

        void updateWidth(const std::string & str) { updateWidth_(str.size()); }

        void updateWidth(const boost::json::value & val)
        {
            if (val.is_string())
            {
                updateWidth(val.as_string());
            }
            else
            {
                updateWidth(boost::json::serialize(val));
            }
        }

        void formatColumn(std::ostream & os, const boost::json::value & val) const
        {
            if (val.is_string())
            {
                formatColumn(os, val.as_string());
            }
            else
            {
                formatColumn_(os);
                os << val;
            }
        }

        void formatColumn(std::ostream & os, const boost::json::string & str) const
        {
            formatColumn_(os);
            os << str.c_str();
        }

        void formatColumn(std::ostream & os, const std::string & str) const
        {
            formatColumn_(os);
            os << str;
        }

        friend inline std::istream & operator>>(std::istream & is, JSONKey & key_info)
        {
            is >> key_info.key_;
            key_info.updateWidth(key_info.key_);
            return is;
        }

        friend inline std::ostream & operator<<(std::ostream & os, const JSONKey & key_info)
        {
            key_info.formatColumn(os, key_info.key_);
            return os;
        }
    };
} // namespace mavis_tools
