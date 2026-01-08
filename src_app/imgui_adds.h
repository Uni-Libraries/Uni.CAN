#pragma once

#include <cinttypes>
#include <cstdint>
#include <string_view>

#include <imgui.h>
#include <imgui_internal.h>

#include <magic_enum/magic_enum.hpp>

namespace ImGui {
    inline bool BeginCombo(std::string_view label, std::string_view preview_value, ImGuiComboFlags flags = 0) {
        return BeginCombo(label.data(), preview_value.data(), flags);
    }

    inline bool Button(std::string_view label, const ImVec2& size = ImVec2(0, 0)) {
        return Button(label.data(), size);
    }

    inline bool Checkbox(std::string_view label, bool* v) {
        return Checkbox(label.data(), v);
    }

    inline bool CollapsingHeader(std::string_view strv) {
        return CollapsingHeader(strv.data());
    }

    inline bool SliderFloat(std::string_view strv, float *v, float v_min, float v_max, const char *format = "%.3f",
                     ImGuiSliderFlags flags = 0) {
        return SliderFloat(strv.data(), v, v_min, v_max, format, flags);
    }

    inline bool Selectable(std::string_view label, bool selected = false, ImGuiSelectableFlags flags = 0, const ImVec2& size = ImVec2(0, 0)) {
        return Selectable(label.data(), selected, flags, size);
    }

    inline bool SliderFloatWithSteps(std::string_view strv, float* v, float v_min, float v_max, float v_step, const char* display_format = nullptr)
    {
        if (!display_format)
            display_format = "%.3f";

        char text_buf[64] = {};
        ImFormatString(text_buf, IM_ARRAYSIZE(text_buf), display_format, *v);

        // Map from [v_min,v_max] to [0,N]
        const int countValues = int((v_max-v_min)/v_step);
        int v_i = int((*v - v_min)/v_step);
        const bool value_changed = SliderInt(strv.data(), &v_i, 0, countValues, text_buf);

        // Remap from [0,N] to [v_min,v_max]
        *v = v_min + float(v_i) * v_step;
        return value_changed;
    }

    inline bool SliderDoubleWithSteps(std::string_view strv, double* v, double v_min, double v_max, double v_step, const char* display_format = nullptr)
    {
        if (!display_format)
            display_format = "%.3f";

        char text_buf[64] = {};
        if (*v > v_max)
        {
            strcpy(text_buf, "SNA");
        }
        else
        {
            ImFormatString(text_buf, IM_ARRAYSIZE(text_buf), display_format, *v);
        }

        // Map from [v_min,v_max] to [0,N]
        const int countValues = int((v_max-v_min)/v_step);
        int v_i = int((*v - v_min)/v_step);
        const bool value_changed = SliderInt(strv.data(), &v_i, 0, countValues+1, text_buf);

        // Remap from [0,N] to [v_min,v_max]
        *v = v_min + double(v_i) * v_step;
        return value_changed;
    }

    inline bool SliderUInt8(std::string_view label, uint64_t* v, uint64_t v_min, uint64_t v_max, const char* format = "%" PRIu64 , ImGuiSliderFlags flags = 0)
    {
        return SliderScalar(label.data(), ImGuiDataType_U64, v, &v_min, &v_max, format, flags);
    }

    inline bool InputInt(std::string_view label, int* v, int step = 1, int step_fast = 100, ImGuiInputTextFlags flags = 0) {
        return InputInt(label.data(), v, step, step_fast, flags);
    }

    inline bool InputUInt(const char* label, unsigned int* v, unsigned int step = 1, unsigned int step_fast = 100, ImGuiInputTextFlags flags = 0)
    {
        // Hexadecimal input provided as a convenience but the flag name is awkward. Typically you'd use InputText() to parse your own data, if you want to handle prefixes.
        const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%d";
        return InputScalar(label, ImGuiDataType_U32, (void*)v, (void*)(step > 0 ? &step : NULL), (void*)(step_fast > 0 ? &step_fast : NULL), format, flags);
    }

    inline bool InputUInt64(const char* label, uint64_t* v, uint64_t step = 1U, uint64_t step_fast = 100, ImGuiInputTextFlags flags = 0)
    {
        // Hexadecimal input provided as a convenience but the flag name is awkward. Typically you'd use InputText() to parse your own data, if you want to handle prefixes.
        const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%llX" : "%d";
        return InputScalar(label, ImGuiDataType_U64, (void*)v, (void*)(step > 0 ? &step : NULL), (void*)(step_fast > 0 ? &step_fast : NULL), format, flags);
    }


    inline bool InputUByte(std::string_view label, unsigned char* v, unsigned char step = 1, unsigned char step_fast = 10, ImGuiInputTextFlags flags = 0)
    {
        // Hexadecimal input provided as a convenience but the flag name is awkward. Typically you'd use InputText() to parse your own data, if you want to handle prefixes.
        const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%02X" : "%d";
        return InputScalar(label.data(), ImGuiDataType_U8, (void*)v, (void*)(step > 0 ? &step : NULL), (void*)(step_fast > 0 ? &step_fast : NULL), format, flags);
    }

    inline bool InputUShort(std::string_view label, unsigned short* v, unsigned short step = 1, unsigned short step_fast = 10, ImGuiInputTextFlags flags = 0)
    {
        // Hexadecimal input provided as a convenience but the flag name is awkward. Typically you'd use InputText() to parse your own data, if you want to handle prefixes.
        const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%04X" : "%d";
        return InputScalar(label.data(), ImGuiDataType_U16, (void*)v, (void*)(step > 0 ? &step : NULL), (void*)(step_fast > 0 ? &step_fast : NULL), format, flags);
    }

    inline void Text(std::string_view text) {
        Text("%s", text.data());
    }

    /// Draws a combo-box for an enum class.
    /// @returns true if the user picked a different value.
    template <typename E>
    bool EnumCombo(const char* label, E& value, float comboWidth = 0.0f)
    {
        static_assert(std::is_enum_v<E>, "EnumComboLeftLabel requires an enum type");

        // Compile-time reflection data
        constexpr auto names  = magic_enum::enum_names<E>();
        constexpr auto values = magic_enum::enum_values<E>();
        constexpr int  count  = static_cast<int>(values.size());

        int current = magic_enum::enum_index(value).value_or(0);
        bool changed = false;

        // ─── draw the label ───────────────────────────────────────────────────────
        ImGui::TextUnformatted(label);
        ImGui::SameLine();

        // ─── option: fixed width for nicer property grids ────────────────────────
        if (comboWidth > 0.0f)
            ImGui::SetNextItemWidth(comboWidth);

        // We need a unique ID for the widget but don’t want it printed,
        // so prepend "##" which hides the label part from the UI.
        ImGui::PushID(label);          // prevent ID collisions in tables/repeat blocks
        if (ImGui::BeginCombo("##combo", names[current].data()))
        {
            for (int i = 0; i < count; ++i)
            {
                bool selected = (current == i);
                if (ImGui::Selectable(names[i].data(), selected))
                {
                    current = i;
                    value   = values[i];
                    changed = true;
                }
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopID();
        return changed;
    }
}
