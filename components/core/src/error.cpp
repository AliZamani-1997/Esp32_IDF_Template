#include "core/error.hpp"

namespace core {

const char* Error::name() const noexcept
{
    return esp_err_to_name(code_);
}

std::string_view Error::message() const noexcept
{
    return std::string_view{esp_err_to_name(code_)};
}

} // namespace core
