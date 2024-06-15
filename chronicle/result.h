#pragma once
#include <tuple>
#include <optional>
#include "Error.h"

template <typename T>
using Result = std::tuple<std::optional<T>, std::optional<Error>>;
