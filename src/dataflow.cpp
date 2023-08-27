#include "pch.hpp"

#include "edgelink/edgelink.hpp"

RTTR_REGISTRATION 
{ 
    rttr::registration::class_<edgelink::ISourceProvider>("edgelink::ISourceProvider"); 
    rttr::registration::class_<edgelink::ISinkProvider>("edgelink::ISinkProvider"); 
    rttr::registration::class_<edgelink::IFilterProvider>("edgelink::IFilterProvider"); 
}
