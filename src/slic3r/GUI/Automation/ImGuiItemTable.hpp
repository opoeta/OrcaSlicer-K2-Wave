#pragma once
#include <mutex>
#include <string>
#include <vector>

namespace Slic3r { namespace GUI { namespace Automation {

// One recorded ImGui item. Rect is in ImGui display coords; WxUiBackend maps it
// to screen coords using the canvas client origin + DPI scale.
struct ImGuiItemRecord {
    std::string window_name;
    std::string label;   // visible label / id
    std::string type;    // "button", "checkbox", "combo", "slider", "input", ...
    float       x = 0, y = 0, w = 0, h = 0;
    bool        enabled = true;
    bool        has_value = false;
    std::string value;
};

// A complete recorded frame: items + window-level info.
struct ImGuiWindowRecord {
    std::string name;
    float       x = 0, y = 0, w = 0, h = 0;
    bool        visible = true;
};

struct ImGuiFrameRecord {
    std::vector<ImGuiItemRecord>   items;
    std::vector<ImGuiWindowRecord> windows;
};

// Double-buffered recorder. The drawing code appends to the "back" frame; render()
// swaps it to "front" at frame end. Readers (GUI thread, after marshaling) read the
// front frame. All access is on the GUI thread, but we guard with a mutex anyway
// because the automation read may happen between frames.
class ImGuiItemTable {
public:
    static ImGuiItemTable& instance();

    // Called from ImGuiWrapper drawing hooks (GUI thread). No-op cheap append.
    void record_item(ImGuiItemRecord rec);
    void record_window(ImGuiWindowRecord rec);

    // Called at frame end (ImGuiWrapper::render). Promotes back -> front, clears back.
    void swap_frame();

    // Snapshot the latest complete frame for the backend to read.
    ImGuiFrameRecord snapshot() const;

private:
    mutable std::mutex m_mutex;
    ImGuiFrameRecord   m_back;   // accumulating
    ImGuiFrameRecord   m_front;  // last complete
};

}}} // namespace
