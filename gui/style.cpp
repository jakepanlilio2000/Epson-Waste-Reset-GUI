#include "style.h"
#include <algorithm>
#include <cctype>
#include <cstdio>

namespace ewr_gui {

namespace {
    std::string ToLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }
}

namespace colors {

ImVec4 GetInkColor(const std::string& colorName)
{
    const std::string name = ToLower(colorName);
    if (name.find("photo black") != std::string::npos || name.find("matte black") != std::string::npos)
        return ImVec4(0.20f, 0.20f, 0.24f, 1.0f);
    if (name.find("black") != std::string::npos || name == "bk")
        return ImVec4(0.12f, 0.12f, 0.15f, 1.0f);
    if (name.find("light cyan") != std::string::npos || name == "lc")
        return ImVec4(0.35f, 0.80f, 0.98f, 1.0f);
    if (name.find("cyan") != std::string::npos || name == "c")
        return ImVec4(0.00f, 0.68f, 0.94f, 1.0f);
    if (name.find("light magenta") != std::string::npos || name == "lm")
        return ImVec4(0.95f, 0.48f, 0.74f, 1.0f);
    if (name.find("magenta") != std::string::npos || name == "m")
        return ImVec4(0.88f, 0.12f, 0.52f, 1.0f);
    if (name.find("yellow") != std::string::npos || name == "y")
        return ImVec4(0.96f, 0.82f, 0.10f, 1.0f);
    if (name.find("gray") != std::string::npos || name.find("grey") != std::string::npos)
        return ImVec4(0.55f, 0.58f, 0.64f, 1.0f);
    if (name.find("red") != std::string::npos)
        return ImVec4(0.90f, 0.22f, 0.22f, 1.0f);
    if (name.find("blue") != std::string::npos)
        return ImVec4(0.18f, 0.38f, 0.88f, 1.0f);

    return ImVec4(0.35f, 0.65f, 0.85f, 1.0f);
}

ImVec4 GetWastePadColor(double percent)
{
    if (percent >= 98.0)
        return Danger;
    if (percent >= 75.0)
        return Warning;
    return Success;
}

} // namespace colors

void SetupModernDarkTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* c = style.Colors;

    // Compact, clean layout geometry
    style.WindowRounding    = 8.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 5.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 5.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize  = 1.0f;
    style.PopupBorderSize  = 1.0f;
    style.FrameBorderSize  = 0.0f;
    style.TabBorderSize    = 0.0f;

    style.WindowPadding    = ImVec2(10.0f, 10.0f);
    style.FramePadding     = ImVec2(8.0f, 5.0f);
    style.ItemSpacing      = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.ScrollbarSize    = 12.0f;
    style.GrabMinSize      = 10.0f;

    // Modern Deep Slate Dark Palette
    c[ImGuiCol_Text]                  = ImVec4(0.92f, 0.94f, 0.96f, 1.00f);
    c[ImGuiCol_TextDisabled]          = ImVec4(0.52f, 0.55f, 0.62f, 1.00f);
    c[ImGuiCol_WindowBg]              = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
    c[ImGuiCol_ChildBg]               = ImVec4(0.13f, 0.14f, 0.17f, 0.90f);
    c[ImGuiCol_PopupBg]               = ImVec4(0.14f, 0.15f, 0.18f, 0.98f);
    c[ImGuiCol_Border]                = ImVec4(0.22f, 0.25f, 0.30f, 0.70f);
    c[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_FrameBg]               = ImVec4(0.17f, 0.19f, 0.23f, 1.00f);
    c[ImGuiCol_FrameBgHovered]        = ImVec4(0.22f, 0.25f, 0.30f, 1.00f);
    c[ImGuiCol_FrameBgActive]         = ImVec4(0.26f, 0.30f, 0.36f, 1.00f);
    c[ImGuiCol_TitleBg]               = ImVec4(0.08f, 0.09f, 0.11f, 1.00f);
    c[ImGuiCol_TitleBgActive]         = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.08f, 0.09f, 0.11f, 0.60f);
    c[ImGuiCol_MenuBarBg]             = ImVec4(0.11f, 0.12f, 0.15f, 1.00f);
    c[ImGuiCol_ScrollbarBg]           = ImVec4(0.10f, 0.11f, 0.13f, 0.30f);
    c[ImGuiCol_ScrollbarGrab]         = ImVec4(0.24f, 0.27f, 0.32f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.32f, 0.36f, 0.43f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.38f, 0.43f, 0.52f, 1.00f);
    c[ImGuiCol_CheckMark]             = ImVec4(0.18f, 0.60f, 0.98f, 1.00f);
    c[ImGuiCol_SliderGrab]            = ImVec4(0.18f, 0.60f, 0.98f, 1.00f);
    c[ImGuiCol_SliderGrabActive]      = ImVec4(0.25f, 0.68f, 1.00f, 1.00f);
    c[ImGuiCol_Button]                = ImVec4(0.17f, 0.20f, 0.26f, 1.00f);
    c[ImGuiCol_ButtonHovered]         = ImVec4(0.23f, 0.28f, 0.36f, 1.00f);
    c[ImGuiCol_ButtonActive]          = ImVec4(0.15f, 0.55f, 0.95f, 1.00f);
    c[ImGuiCol_Header]                = ImVec4(0.17f, 0.21f, 0.27f, 1.00f);
    c[ImGuiCol_HeaderHovered]         = ImVec4(0.22f, 0.28f, 0.37f, 1.00f);
    c[ImGuiCol_HeaderActive]          = ImVec4(0.15f, 0.55f, 0.95f, 0.80f);
    c[ImGuiCol_Separator]             = ImVec4(0.20f, 0.23f, 0.28f, 0.80f);
    c[ImGuiCol_SeparatorHovered]      = ImVec4(0.28f, 0.38f, 0.52f, 1.00f);
    c[ImGuiCol_SeparatorActive]       = ImVec4(0.15f, 0.55f, 0.95f, 1.00f);
    c[ImGuiCol_ResizeGrip]            = ImVec4(0.20f, 0.24f, 0.30f, 0.40f);
    c[ImGuiCol_ResizeGripHovered]     = ImVec4(0.15f, 0.55f, 0.95f, 0.60f);
    c[ImGuiCol_ResizeGripActive]      = ImVec4(0.15f, 0.55f, 0.95f, 1.00f);
    c[ImGuiCol_Tab]                   = ImVec4(0.13f, 0.15f, 0.18f, 1.00f);
    c[ImGuiCol_TabHovered]            = ImVec4(0.20f, 0.25f, 0.33f, 1.00f);
    c[ImGuiCol_TabActive]             = ImVec4(0.16f, 0.38f, 0.68f, 1.00f);
    c[ImGuiCol_TabUnfocused]          = ImVec4(0.11f, 0.12f, 0.15f, 1.00f);
    c[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.14f, 0.22f, 0.33f, 1.00f);
    c[ImGuiCol_TableHeaderBg]         = ImVec4(0.14f, 0.16f, 0.20f, 1.00f);
    c[ImGuiCol_TableBorderStrong]     = ImVec4(0.22f, 0.25f, 0.30f, 1.00f);
    c[ImGuiCol_TableBorderLight]      = ImVec4(0.16f, 0.18f, 0.22f, 0.80f);
    c[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.02f);
    c[ImGuiCol_TextSelectedBg]        = ImVec4(0.15f, 0.55f, 0.95f, 0.35f);
    c[ImGuiCol_DragDropTarget]        = ImVec4(0.15f, 0.55f, 0.95f, 0.90f);
    c[ImGuiCol_NavHighlight]          = ImVec4(0.15f, 0.55f, 0.95f, 1.00f);
    c[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.04f, 0.05f, 0.06f, 0.75f);
}

void DrawStatusBadge(const char* label, const ImVec4& color)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 textSize = ImGui::CalcTextSize(label);
    const float padX = 7.0f;
    const float padY = 2.0f;
    const ImVec2 rectMin = pos;
    const ImVec2 rectMax = ImVec2(pos.x + textSize.x + padX * 2.0f, pos.y + textSize.y + padY * 2.0f);

    const ImU32 bgCol = ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, 0.20f));
    const ImU32 borderCol = ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, 0.75f));
    const ImU32 textCol = ImGui::ColorConvertFloat4ToU32(ImVec4(color.x * 1.15f, color.y * 1.15f, color.z * 1.15f, 1.0f));

    drawList->AddRectFilled(rectMin, rectMax, bgCol, 4.0f);
    drawList->AddRect(rectMin, rectMax, borderCol, 4.0f, 0, 1.0f);
    drawList->AddText(ImVec2(pos.x + padX, pos.y + padY), textCol, label);

    ImGui::Dummy(ImVec2(textSize.x + padX * 2.0f, textSize.y + padY * 2.0f));
}

void DrawWasteGauge(const char* label, const char* kind, int used, int max, double percent)
{
    ImGui::BeginGroup();

    // Line 1: Pad Label and formatted percentage count
    ImGui::TextColored(colors::TextNormal, "%s", label);
    ImGui::SameLine();
    ImGui::TextDisabled("[%s]", (kind && *kind) ? kind : "counter");

    char countBuf[128];
    if (max > 0)
        std::snprintf(countBuf, sizeof(countBuf), "%d / %d  (%.1f%%)", used, max, percent);
    else
        std::snprintf(countBuf, sizeof(countBuf), "Raw Count: %d", used);

    const float countWidth = ImGui::CalcTextSize(countBuf).x;
    const float avail = ImGui::GetContentRegionAvail().x;
    if (avail > countWidth + 10.0f)
    {
        ImGui::SameLine(ImGui::GetCursorPosX() + avail - countWidth);
        const ImVec4 valColor = (max > 0) ? colors::GetWastePadColor(percent) : colors::TextMuted;
        ImGui::TextColored(valColor, "%s", countBuf);
    }

    // Gauge bar with nice height and border
    const float fraction = (max > 0) ? static_cast<float>(std::min(std::max(percent / 100.0, 0.0), 1.0)) : 0.0f;
    const ImVec4 barColor = colors::GetWastePadColor(percent);

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.16f, 0.18f, 0.22f, 0.90f));
    ImGui::ProgressBar(fraction, ImVec2(-1.0f, 20.0f), "");
    ImGui::PopStyleColor(2);

    // Warning alert if full
    if (percent >= 100.0)
    {
        ImGui::Spacing();
        ImGui::TextColored(colors::Danger, "  * Counter reached service limit (100%%) - Reset required to resume printing");
    }
    else if (percent >= 80.0)
    {
        ImGui::Spacing();
        ImGui::TextColored(colors::Warning, "  * Counter is high (%.1f%%) - Prepare to reset or service absorbent pad", percent);
    }

    ImGui::EndGroup();
}

void DrawInkGauge(const char* colorName, int level, const char* status)
{
    ImGui::BeginGroup();

    const ImVec4 inkColor = colors::GetInkColor(colorName);

    // Ink name & status
    ImGui::TextColored(colors::TextNormal, "%s", colorName);
    ImGui::SameLine();
    if (status && *status)
    {
        if (std::string(status) == "OK")
            ImGui::TextColored(colors::Success, "%d%% [OK]", level);
        else
            ImGui::TextColored(colors::Danger, "%d%% [%s]", level, status);
    }
    else
    {
        ImGui::TextColored(colors::TextMuted, "%d%%", level);
    }

    // Visual bar
    const float fraction = static_cast<float>(std::min(std::max(level / 100.0, 0.0), 1.0));
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, inkColor);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.16f, 0.18f, 0.22f, 0.90f));
    ImGui::ProgressBar(fraction, ImVec2(-1.0f, 15.0f), "");
    ImGui::PopStyleColor(2);

    ImGui::EndGroup();
}

void DrawSectionHeader(const char* title, const char* subtitle)
{
    ImGui::Spacing();
    ImGui::TextColored(colors::PrimaryHover, "%s", title);
    if (subtitle && *subtitle)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("- %s", subtitle);
    }
    ImGui::Separator();
    ImGui::Spacing();
}

void DrawTooltip(const char* text)
{
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 26.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

} // namespace ewr_gui
