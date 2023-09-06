#include "Debug.h"

#include <stdexcept>

void Common::throwError(const std::string& vMessage)
{
    throw std::runtime_error(vMessage);
}
