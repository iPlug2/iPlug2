/*
    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#pragma once

#include "common.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/// Listing of various field types that can be used as variables in shaders
enum class VariableType {
    Invalid = 0,
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Float16,
    Float32,
    Float64,
    Bool
};

/// Convert from a C++ type to an element of \ref VariableType
template <typename T> constexpr VariableType get_type() {
    if constexpr (std::is_same_v<T, bool>)
        return VariableType::Bool;

    if constexpr (std::is_integral_v<T>) {
        if constexpr (sizeof(T) == 1)
            return std::is_signed_v<T> ? VariableType::Int8 : VariableType::UInt8;
        else if constexpr (sizeof(T) == 2)
            return std::is_signed_v<T> ? VariableType::Int16 : VariableType::UInt16;
        else if constexpr (sizeof(T) == 4)
            return std::is_signed_v<T> ? VariableType::Int32 : VariableType::UInt32;
        else if constexpr (sizeof(T) == 8)
            return std::is_signed_v<T> ? VariableType::Int64 : VariableType::UInt64;
    } else if constexpr (std::is_floating_point_v<T>) {
        if constexpr (sizeof(T) == 2)
            return VariableType::Float16;
        else if constexpr (sizeof(T) == 4)
            return VariableType::Float32;
        else if constexpr (sizeof(T) == 8)
            return VariableType::Float64;
    } else {
        return VariableType::Invalid;
    }
}

/// Return the size in bytes associated with a specific variable type
extern size_t type_size(VariableType type);

/// Return the name (e.g. "uint8") associated with a specific variable type
extern const char *type_name(VariableType type);

namespace detail {
    /// Detector pattern that is used to drive many type traits below
    template <typename SFINAE, template <typename> typename Op, typename Arg>
    struct detector : std::false_type { };

    template <template <typename> typename Op, typename Arg>
    struct detector<std::void_t<Op<Arg>>, Op, Arg>
        : std::true_type { };

    // template <typename T> using is_enoki_array_det    = std::enable_if_t<T::IsEnoki>;
    template <typename T> using is_nanogui_array_det  = std::enable_if_t<T::IsNanoGUI && !T::IsMatrix>;
    template <typename T> using is_nanogui_matrix_det = std::enable_if_t<T::IsNanoGUI && T::IsMatrix>;
}

template <template<typename> class Op, typename Arg>
constexpr bool is_detected_v = detail::detector<void, Op, Arg>::value;

template <typename T>
constexpr bool is_nanogui_array_v = is_detected_v<detail::is_nanogui_array_det, std::decay_t<T>>;

template <typename T>
constexpr bool is_nanogui_matrix_v = is_detected_v<detail::is_nanogui_matrix_det, std::decay_t<T>>;

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
