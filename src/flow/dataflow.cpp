#include "edgelink/edgelink.hpp"

RTTR_REGISTRATION {
    rttr::registration::class_<edgelink::IFlowNodeProvider>("edgelink::IFlowNodeProvider");
    rttr::registration::class_<edgelink::INodeDescriptor>("edgelink::INodeDescriptor");
}
