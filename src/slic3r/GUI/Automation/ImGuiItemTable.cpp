#include "ImGuiItemTable.hpp"

namespace Slic3r { namespace GUI { namespace Automation {

ImGuiItemTable& ImGuiItemTable::instance() {
    static ImGuiItemTable t;
    return t;
}

void ImGuiItemTable::record_item(ImGuiItemRecord rec) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_back.items.push_back(std::move(rec));
}

void ImGuiItemTable::record_window(ImGuiWindowRecord rec) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_back.windows.push_back(std::move(rec));
}

void ImGuiItemTable::swap_frame() {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_front = std::move(m_back);
    m_back  = ImGuiFrameRecord{};
}

ImGuiFrameRecord ImGuiItemTable::snapshot() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_front;
}

}}} // namespace
