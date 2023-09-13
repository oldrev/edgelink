
#include "edgelink/edgelink.hpp"

namespace edgelink {

IFlowNode* Flow::get_node(const std::string_view id) const {
    for (auto&& n : _nodes) {
        if (n->id() == id) {
            return n.get();
        }
    }
    throw std::runtime_error(fmt::format("找不到节点 ID：{0}", id));
}

}; // namespace edgelink