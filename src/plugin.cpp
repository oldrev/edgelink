#include "edgelink/edgelink.hpp"
#include "edgelink/plugin.hpp"

namespace edgelink {

//

}; // namespace edgelink

RTTR_REGISTRATION // remark the different registration macro!
{
    rttr::registration::class_<edgelink::MyPluginClass>("edgelink::MyPluginClass").constructor<>();
}
