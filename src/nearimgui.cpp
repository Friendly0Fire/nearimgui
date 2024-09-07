#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "nearimgui.h"

#include <vector>
#include <mutex>

namespace NGui::ImGuiExt
{
    // Those MIN/MAX values are not define because we need to point to them
    static const signed char    IM_S8_MIN = -128;
    static const signed char    IM_S8_MAX = 127;
    static const unsigned char  IM_U8_MIN = 0;
    static const unsigned char  IM_U8_MAX = 0xFF;
    static const signed short   IM_S16_MIN = -32768;
    static const signed short   IM_S16_MAX = 32767;
    static const unsigned short IM_U16_MIN = 0;
    static const unsigned short IM_U16_MAX = 0xFFFF;
    static const ImS32          IM_S32_MIN = INT_MIN;    // (-2147483647 - 1), (0x80000000);
    static const ImS32          IM_S32_MAX = INT_MAX;    // (2147483647), (0x7FFFFFFF)
    static const ImU32          IM_U32_MIN = 0;
    static const ImU32          IM_U32_MAX = UINT_MAX;   // (0xFFFFFFFF)
#ifdef LLONG_MIN
    static const ImS64          IM_S64_MIN = LLONG_MIN;  // (-9223372036854775807ll - 1ll);
    static const ImS64          IM_S64_MAX = LLONG_MAX;  // (9223372036854775807ll);
#else
    static const ImS64          IM_S64_MIN = -9223372036854775807LL - 1;
    static const ImS64          IM_S64_MAX = 9223372036854775807LL;
#endif
    static const ImU64          IM_U64_MIN = 0;
#ifdef ULLONG_MAX
    static const ImU64          IM_U64_MAX = ULLONG_MAX; // (0xFFFFFFFFFFFFFFFFull);
#else
    static const ImU64          IM_U64_MAX = (2ULL * 9223372036854775807LL + 1);
#endif
    static const float          DRAG_MOUSE_THRESHOLD_FACTOR = 0.50f;    // Multiplier for the default value of io.MouseDragThreshold to make DragFloat/DragInt react faster to mouse drags.

    static float GetMinimumStepAtDecimalPrecision(int decimal_precision)
    {
        static const float min_steps[10] = { 1.0f, 0.1f, 0.01f, 0.001f, 0.0001f, 0.00001f, 0.000001f, 0.0000001f, 0.00000001f, 0.000000001f };
        if (decimal_precision < 0)
            return FLT_MIN;
        return (decimal_precision < IM_ARRAYSIZE(min_steps)) ? min_steps[decimal_precision] : ImPow(10.0f, (float)-decimal_precision);
    }

    // This is called by DragBehavior() when the widget is active (held by mouse or being manipulated with Nav controls)
    template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
    bool AngularDragBehaviorT(const ImRect& frame, ImGuiDataType data_type, TYPE* v, float v_speed, const TYPE v_min, const TYPE v_max, const char* format, ImGuiSliderFlags flags)
    {
        using namespace ImGui;

        ImGuiContext& g = *GImGui;
        const ImGuiAxis axis = (flags & ImGuiSliderFlags_Vertical) ? ImGuiAxis_Y : ImGuiAxis_X;
        const bool is_bounded = (v_min < v_max);
        const bool is_wrapped = is_bounded && (flags & ImGuiSliderFlags_WrapAround);
        const bool is_logarithmic = (flags & ImGuiSliderFlags_Logarithmic) != 0;
        const bool is_floating_point = (data_type == ImGuiDataType_Float) || (data_type == ImGuiDataType_Double);

        // Default tweak speed
        if (v_speed == 0.0f && is_bounded && (v_max - v_min < FLT_MAX))
            v_speed = (float)((v_max - v_min) * g.DragSpeedDefaultRatio);

        // Inputs accumulates into g.DragCurrentAccum, which is flushed into the current value as soon as it makes a difference with our precision settings
        float adjust_delta = 0.0f;
        if (g.ActiveIdSource == ImGuiInputSource_Mouse && IsMousePosValid() && IsMouseDragPastThreshold(0, g.IO.MouseDragThreshold * DRAG_MOUSE_THRESHOLD_FACTOR))
        {
            const ImVec2 c = frame.GetCenter();

            ImVec2 prevMouseVec = g.IO.MousePosPrev - c;
            prevMouseVec *= ImInvLength(prevMouseVec, 1.f);

            ImVec2 currentMouseVec = g.IO.MousePos - c;
            currentMouseVec *= ImInvLength(currentMouseVec, 1.f);

            adjust_delta = atan2(currentMouseVec.y * prevMouseVec.x - currentMouseVec.x * prevMouseVec.y, ImDot(currentMouseVec, prevMouseVec));
            printf("delta = %f\n", adjust_delta);
            if (g.IO.KeyAlt)
                adjust_delta *= 1.0f / 100.0f;
            if (g.IO.KeyShift)
                adjust_delta *= 10.0f;
        }
        else if (g.ActiveIdSource == ImGuiInputSource_Keyboard || g.ActiveIdSource == ImGuiInputSource_Gamepad)
        {
            const int decimal_precision = is_floating_point ? ImParseFormatPrecision(format, 3) : 0;
            const bool tweak_slow = IsKeyDown((g.NavInputSource == ImGuiInputSource_Gamepad) ? ImGuiKey_NavGamepadTweakSlow : ImGuiKey_NavKeyboardTweakSlow);
            const bool tweak_fast = IsKeyDown((g.NavInputSource == ImGuiInputSource_Gamepad) ? ImGuiKey_NavGamepadTweakFast : ImGuiKey_NavKeyboardTweakFast);
            const float tweak_factor = tweak_slow ? 1.0f / 10.0f : tweak_fast ? 10.0f : 1.0f;
            adjust_delta = GetNavTweakPressedAmount(axis) * tweak_factor;
            v_speed = ImMax(v_speed, GetMinimumStepAtDecimalPrecision(decimal_precision));

            // For vertical drag we currently assume that Up=higher value (like we do with vertical sliders). This may become a parameter.
            if (axis == ImGuiAxis_Y)
                adjust_delta = -adjust_delta;
        }
        adjust_delta *= v_speed;

        // For logarithmic use our range is effectively 0..1 so scale the delta into that range
        if (is_logarithmic && (v_max - v_min < FLT_MAX) && ((v_max - v_min) > 0.000001f)) // Epsilon to avoid /0
            adjust_delta /= (float)(v_max - v_min);

        // Clear current value on activation
        // Avoid altering values and clamping when we are _already_ past the limits and heading in the same direction, so e.g. if range is 0..255, current value is 300 and we are pushing to the right side, keep the 300.
        const bool is_just_activated = g.ActiveIdIsJustActivated;
        const bool is_already_past_limits_and_pushing_outward = is_bounded && !is_wrapped && ((*v >= v_max && adjust_delta > 0.0f) || (*v <= v_min && adjust_delta < 0.0f));
        if (is_just_activated || is_already_past_limits_and_pushing_outward)
        {
            g.DragCurrentAccum = 0.0f;
            g.DragCurrentAccumDirty = false;
        }
        else if (adjust_delta != 0.0f)
        {
            g.DragCurrentAccum += adjust_delta;
            g.DragCurrentAccumDirty = true;
        }

        if (!g.DragCurrentAccumDirty)
            return false;

        TYPE v_cur = *v;
        FLOATTYPE v_old_ref_for_accum_remainder = (FLOATTYPE)0.0f;

        float logarithmic_zero_epsilon = 0.0f; // Only valid when is_logarithmic is true
        const float zero_deadzone_halfsize = 0.0f; // Drag widgets have no deadzone (as it doesn't make sense)
        if (is_logarithmic)
        {
            // When using logarithmic sliders, we need to clamp to avoid hitting zero, but our choice of clamp value greatly affects slider precision. We attempt to use the specified precision to estimate a good lower bound.
            const int decimal_precision = is_floating_point ? ImParseFormatPrecision(format, 3) : 1;
            logarithmic_zero_epsilon = ImPow(0.1f, (float)decimal_precision);

            // Convert to parametric space, apply delta, convert back
            float v_old_parametric = ScaleRatioFromValueT<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, v_cur, v_min, v_max, is_logarithmic, logarithmic_zero_epsilon, zero_deadzone_halfsize);
            float v_new_parametric = v_old_parametric + g.DragCurrentAccum;
            v_cur = ScaleValueFromRatioT<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, v_new_parametric, v_min, v_max, is_logarithmic, logarithmic_zero_epsilon, zero_deadzone_halfsize);
            v_old_ref_for_accum_remainder = v_old_parametric;
        }
        else
        {
            v_cur += (SIGNEDTYPE)g.DragCurrentAccum;
        }

        // Round to user desired precision based on format string
        if (is_floating_point && !(flags & ImGuiSliderFlags_NoRoundToFormat))
            v_cur = RoundScalarWithFormatT<TYPE>(format, data_type, v_cur);

        // Preserve remainder after rounding has been applied. This also allow slow tweaking of values.
        g.DragCurrentAccumDirty = false;
        if (is_logarithmic)
        {
            // Convert to parametric space, apply delta, convert back
            float v_new_parametric = ScaleRatioFromValueT<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, v_cur, v_min, v_max, is_logarithmic, logarithmic_zero_epsilon, zero_deadzone_halfsize);
            g.DragCurrentAccum -= (float)(v_new_parametric - v_old_ref_for_accum_remainder);
        }
        else
        {
            g.DragCurrentAccum -= (float)((SIGNEDTYPE)v_cur - (SIGNEDTYPE)*v);
        }

        // Lose zero sign for float/double
        if (v_cur == (TYPE)-0)
            v_cur = (TYPE)0;

        if (*v != v_cur && is_bounded)
        {
            if (is_wrapped)
            {
                // Wrap values
                if (v_cur < v_min)
                    v_cur += v_max - v_min + (is_floating_point ? 0 : 1);
                if (v_cur > v_max)
                    v_cur -= v_max - v_min + (is_floating_point ? 0 : 1);
            }
            else
            {
                // Clamp values + handle overflow/wrap-around for integer types.
                if (v_cur < v_min || (v_cur > *v && adjust_delta < 0.0f && !is_floating_point))
                    v_cur = v_min;
                if (v_cur > v_max || (v_cur < *v && adjust_delta > 0.0f && !is_floating_point))
                    v_cur = v_max;
            }
        }

        // Apply result
        if (*v == v_cur)
            return false;
        *v = v_cur;
        return true;
    }

    bool AngularDragBehavior(ImGuiID id, const ImRect& frame, ImGuiDataType data_type, void* p_v, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
    {
        using namespace ImGui;

        // Read imgui.cpp "API BREAKING CHANGES" section for 1.78 if you hit this assert.
        IM_ASSERT((flags == 1 || (flags & ImGuiSliderFlags_InvalidMask_) == 0) && "Invalid ImGuiSliderFlags flags! Has the legacy 'float power' argument been mistakenly cast to flags? Call function with ImGuiSliderFlags_Logarithmic flags instead.");

        ImGuiContext& g = *GImGui;
        if (g.ActiveId == id)
        {
            // Those are the things we can do easily outside the DragBehaviorT<> template, saves code generation.
            if (g.ActiveIdSource == ImGuiInputSource_Mouse && !g.IO.MouseDown[0])
                ClearActiveID();
            else if ((g.ActiveIdSource == ImGuiInputSource_Keyboard || g.ActiveIdSource == ImGuiInputSource_Gamepad) && g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
                ClearActiveID();
        }
        if (g.ActiveId != id)
            return false;
        if ((g.LastItemData.InFlags & ImGuiItemFlags_ReadOnly) || (flags & ImGuiSliderFlags_ReadOnly))
            return false;

        GetWindowDrawList()->PushClipRectFullScreen();
        GetWindowDrawList()->AddLine(frame.GetCenter(), GetMousePos(), 0xFFFFFFFF);
        GetWindowDrawList()->AddCircleFilled(frame.GetCenter(), frame.GetHeight() * 0.65f, GetColorU32(ImGuiCol_ButtonActive, 0.5f));
        GetWindowDrawList()->AddCircleFilled(GetMousePos(), frame.GetHeight() * 0.65f, GetColorU32(ImGuiCol_ButtonActive, 0.5f));
        GetWindowDrawList()->PopClipRect();

        switch (data_type)
        {
        case ImGuiDataType_S8: { ImS32 v32 = (ImS32) * (ImS8*)p_v;  bool r = AngularDragBehaviorT<ImS32, ImS32, float>(frame, ImGuiDataType_S32, &v32, v_speed, p_min ? *(const ImS8*)p_min : IM_S8_MIN, p_max ? *(const ImS8*)p_max : IM_S8_MAX, format, flags); if (r) *(ImS8*)p_v = (ImS8)v32; return r; }
        case ImGuiDataType_U8: { ImU32 v32 = (ImU32) * (ImU8*)p_v;  bool r = AngularDragBehaviorT<ImU32, ImS32, float>(frame, ImGuiDataType_U32, &v32, v_speed, p_min ? *(const ImU8*)p_min : IM_U8_MIN, p_max ? *(const ImU8*)p_max : IM_U8_MAX, format, flags); if (r) *(ImU8*)p_v = (ImU8)v32; return r; }
        case ImGuiDataType_S16: { ImS32 v32 = (ImS32) * (ImS16*)p_v; bool r = AngularDragBehaviorT<ImS32, ImS32, float>(frame, ImGuiDataType_S32, &v32, v_speed, p_min ? *(const ImS16*)p_min : IM_S16_MIN, p_max ? *(const ImS16*)p_max : IM_S16_MAX, format, flags); if (r) *(ImS16*)p_v = (ImS16)v32; return r; }
        case ImGuiDataType_U16: { ImU32 v32 = (ImU32) * (ImU16*)p_v; bool r = AngularDragBehaviorT<ImU32, ImS32, float>(frame, ImGuiDataType_U32, &v32, v_speed, p_min ? *(const ImU16*)p_min : IM_U16_MIN, p_max ? *(const ImU16*)p_max : IM_U16_MAX, format, flags); if (r) *(ImU16*)p_v = (ImU16)v32; return r; }
        case ImGuiDataType_S32:    return AngularDragBehaviorT<ImS32, ImS32, float >(frame, data_type, (ImS32*)p_v, v_speed, p_min ? *(const ImS32*)p_min : IM_S32_MIN, p_max ? *(const ImS32*)p_max : IM_S32_MAX, format, flags);
        case ImGuiDataType_U32:    return AngularDragBehaviorT<ImU32, ImS32, float >(frame, data_type, (ImU32*)p_v, v_speed, p_min ? *(const ImU32*)p_min : IM_U32_MIN, p_max ? *(const ImU32*)p_max : IM_U32_MAX, format, flags);
        case ImGuiDataType_S64:    return AngularDragBehaviorT<ImS64, ImS64, double>(frame, data_type, (ImS64*)p_v, v_speed, p_min ? *(const ImS64*)p_min : IM_S64_MIN, p_max ? *(const ImS64*)p_max : IM_S64_MAX, format, flags);
        case ImGuiDataType_U64:    return AngularDragBehaviorT<ImU64, ImS64, double>(frame, data_type, (ImU64*)p_v, v_speed, p_min ? *(const ImU64*)p_min : IM_U64_MIN, p_max ? *(const ImU64*)p_max : IM_U64_MAX, format, flags);
        case ImGuiDataType_Float:  return AngularDragBehaviorT<float, float, float >(frame, data_type, (float*)p_v, v_speed, p_min ? *(const float*)p_min : -FLT_MAX, p_max ? *(const float*)p_max : FLT_MAX, format, flags);
        case ImGuiDataType_Double: return AngularDragBehaviorT<double, double, double>(frame, data_type, (double*)p_v, v_speed, p_min ? *(const double*)p_min : -DBL_MAX, p_max ? *(const double*)p_max : DBL_MAX, format, flags);
        case ImGuiDataType_COUNT:  break;
        }
        IM_ASSERT(0);
        return false;
    }

    bool AngularDragScalar(const char* label, ImGuiDataType data_type, void* p_data, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
    {
        using namespace ImGui;

        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const float w = CalcItemWidth();

        const ImVec2 label_size = CalcTextSize(label, NULL, true);
        const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
        const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

        const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
        ItemSize(total_bb, style.FramePadding.y);
        if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
            return false;

        // Default format string when passing NULL
        if (format == NULL)
            format = DataTypeGetInfo(data_type)->PrintFmt;

        const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.InFlags);
        bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
        if (!temp_input_is_active)
        {
            // Tabbing or CTRL-clicking on Drag turns it into an InputText
            const bool clicked = hovered && IsMouseClicked(0, ImGuiInputFlags_None, id);
            const bool double_clicked = (hovered && g.IO.MouseClickedCount[0] == 2 && TestKeyOwner(ImGuiKey_MouseLeft, id));
            const bool make_active = (clicked || double_clicked || g.NavActivateId == id);
            if (make_active && (clicked || double_clicked))
                SetKeyOwner(ImGuiKey_MouseLeft, id);
            if (make_active && temp_input_allowed)
                if ((clicked && g.IO.KeyCtrl) || double_clicked || (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
                    temp_input_is_active = true;

            // (Optional) simple click (without moving) turns Drag into an InputText
            if (g.IO.ConfigDragClickToInputText && temp_input_allowed && !temp_input_is_active)
                if (g.ActiveId == id && hovered && g.IO.MouseReleased[0] && !IsMouseDragPastThreshold(0, g.IO.MouseDragThreshold * DRAG_MOUSE_THRESHOLD_FACTOR))
                {
                    g.NavActivateId = id;
                    g.NavActivateFlags = ImGuiActivateFlags_PreferInput;
                    temp_input_is_active = true;
                }

            if (make_active && !temp_input_is_active)
            {
                SetActiveID(id, window);
                SetFocusID(id, window);
                FocusWindow(window);
                g.ActiveIdUsingNavDirMask = (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
            }
        }

        if (temp_input_is_active)
        {
            // Only clamp CTRL+Click input when ImGuiSliderFlags_AlwaysClamp is set
            const bool is_clamp_input = (flags & ImGuiSliderFlags_AlwaysClamp) != 0 && (p_min == NULL || p_max == NULL || DataTypeCompare(data_type, p_min, p_max) < 0);
            return TempInputScalar(frame_bb, id, label, data_type, p_data, format, is_clamp_input ? p_min : NULL, is_clamp_input ? p_max : NULL);
        }

        // Draw frame
        const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
        RenderNavHighlight(frame_bb, id);
        RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, style.FrameRounding);

        // Drag behavior
        const bool value_changed = AngularDragBehavior(id, frame_bb, data_type, p_data, v_speed, p_min, p_max, format, flags);
        if (value_changed)
            MarkItemEdited(id);

        // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
        char value_buf[64];
        const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
        if (g.LogEnabled)
            LogSetNextTextDecoration("{", "}");
        RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

        if (label_size.x > 0.0f)
            RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
        return value_changed;
    }
}

namespace NGui::Detail
{

const char* CacheString(std::string_view sv)
{
    thread_local std::string buf;
    buf.assign(sv);
    return buf.c_str();
}

const char* CacheString(std::string&& s)
{
    thread_local std::string buf;
    buf = s;
    return buf.c_str();
}

std::vector<std::unique_ptr<BaseCallback>> callbacks;
std::mutex callbacksLock;

BaseCallback* CacheCallback(std::unique_ptr<BaseCallback>&& callback)
{
    std::lock_guard guard(callbacksLock);
    callbacks.push_back(std::move(callback));
    return callbacks.back().get();
}

}
namespace NGui
{
    void NewFrame()
    {
        Detail::callbacks.clear();
    }
}
