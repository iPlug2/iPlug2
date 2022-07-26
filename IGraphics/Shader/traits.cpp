#include "traits.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

size_t type_size(VariableType type) {
    switch (type) {
        case VariableType::UInt8:
        case VariableType::Int8:
        case VariableType::Bool:
            return 1;

        case VariableType::UInt16:
        case VariableType::Int16:
        case VariableType::Float16:
            return 2;

        case VariableType::UInt32:
        case VariableType::Int32:
        case VariableType::Float32:
            return 4;

        case VariableType::UInt64:
        case VariableType::Int64:
        case VariableType::Float64:
            return 8;

        default:
            throw std::runtime_error("Unknown type!");
    }
}

const char *type_name(VariableType type) {
    switch (type) {
        case VariableType::Bool:    return "bool";
        case VariableType::UInt8:   return "uint8";
        case VariableType::Int8:    return "int8";
        case VariableType::UInt16:  return "uint16";
        case VariableType::Int16:   return "int16";
        case VariableType::UInt32:  return "uint32";
        case VariableType::Int32:   return "int32";
        case VariableType::UInt64:  return "uint64";
        case VariableType::Int64:   return "int64";
        case VariableType::Float16: return "float16";
        case VariableType::Float32: return "float32";
        case VariableType::Float64: return "float64";
        default: return "invalid";
    }
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
