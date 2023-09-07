#include "edgelink/edgelink.hpp"


RTTR_REGISTRATION {
    rttr::registration::class_<edgelink::INodeDescriptor>("edgelink::INodeDescriptor");
    rttr::registration::class_<edgelink::INodeProvider>("edgelink::IStandaloneNodeProvider");
    rttr::registration::class_<edgelink::IStandaloneNodeProvider>("edgelink::IStandaloneNodeProvider");
    rttr::registration::class_<edgelink::IFlowNodeProvider>("edgelink::IFlowNodeProvider");
}
