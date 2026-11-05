#pragma once

#include "imgui.h"
#include <string>

namespace ewr_gui {

namespace colors {
    // Modern palette
    const ImVec4 Primary       = ImVec4(0.12f, 0.50f, 0.90f, 1.0f); // Vibrant Blue #1E80E6
    const ImVec4 PrimaryHover  = ImVec4(0.18f, 0.60f, 0.98f, 1.0f);
    const ImVec4 PrimaryActive = ImVec4(0.08f, 0.40f, 0.75f, 1.0f);

    const ImVec4 Success       = ImVec4(0.18f, 0.72f, 0.42f, 1.0f); // Emerald #2ECC71
    const ImVec4 Warning       = ImVec4(0.95f, 0.60f, 0.10f, 1.0f); // Amber #F39C12
    const ImVec4 Danger        = ImVec4(0.90f, 0.25f, 0.22f, 1.0f); // Coral Red #E74C3C
    const ImVec4 DangerHover   = ImVec4(0.96f, 0.35f, 0.30f, 1.0f);
    const ImVec4 DangerActive  = ImVec4(0.80f, 0.18f, 0.15f, 1.0f);

    const ImVec4 TextNormal    = ImVec4(0.92f, 0.93f, 0.95f, 1.0f);
    const ImVec4 TextMuted     = ImVec4(0.55f, 0.58f, 0.65f, 1.0f);
    const ImVec4 CardBg        = ImVec4(0.13f, 0.14f, 0.17f, 0.95f);
    const ImVec4 CardBgAlt     = ImVec4(0.15f, 0.17f, 0.21f, 0.95f);
    const ImVec4 HeaderBg      = ImVec4(0.10f, 0.11f, 0.14f, 1.0f);
    const ImVec4 Border        = ImVec4(0.22f, 0.25f, 0.30f, 0.75f);

    // Ink cartridge colors
    ImVec4 GetInkColor(const std::string& colorName);
    ImVec4 GetWastePadColor(double percent);
} // namespace colors

void SetupModernDarkTheme();
void DrawStatusBadge(const char* label, const ImVec4& color);
void DrawWasteGauge(const char* label, const char* kind, int used, int max, double percent);
void DrawInkGauge(const char* colorName, int level, const char* status);
void DrawSectionHeader(const char* title, const char* subtitle = nullptr);
void DrawTooltip(const char* text);

} // namespace ewr_gui
