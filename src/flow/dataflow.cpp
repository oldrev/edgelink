#include "../pch.hpp"

#include "edgelink/edgelink.hpp"

RTTR_REGISTRATION {
    rttr::registration::class_<edgelink::INodeProvider>("edgelink::INodeProvider");
    rttr::registration::class_<edgelink::INodeDescriptor>("edgelink::INodeDescriptor");
}
