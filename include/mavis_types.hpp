#pragma once

#include <memory>
#include <ostream>
#include <boost/json.hpp>

namespace mavis_tools
{
    // Minimal InstType class for instantiating a Mavis instance
    class StubInstType
    {
      public:
        using PtrType = std::shared_ptr<StubInstType>;
    };

    // Minimal AnnotationType class for instantiating a Mavis instance
    class StubAnnotationType
    {
      public:
        using PtrType = std::shared_ptr<StubAnnotationType>;

        StubAnnotationType() = default;

        StubAnnotationType(const boost::json::object &) {}

        void update(const boost::json::object &) {}

        friend inline std::ostream & operator<<(std::ostream & os, const StubAnnotationType &)
        {
            return os;
        }
    };
} // namespace mavis_tools
