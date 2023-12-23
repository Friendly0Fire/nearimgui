#pragma once
#include <imgui.h>
#include <string_view>
#include <format>

namespace NGui
{
    namespace Detail
    {
        const char* ConvertString(std::string&& s)
        {
            thread_local std::string buf;
            buf = s;
            return buf.c_str();
        }

        const char* ConvertString(const std::string& s)
        {
            return s.c_str();
        }

        const char* ConvertString(std::string_view sv)
        {
            thread_local std::string buf;
            if (*(sv.data() + sv.size()) == '\0')
                return sv.data();
            else
            {
                buf.assign(sv);
                return buf.c_str();
            }
        }
    }

    class FormatArgs
    {
        const char* value_;

    public:
        template<typename... Args>
        FormatArgs(std::format_string<Args...> fmt, Args&& ...args)
            : value_(Detail::ConvertString(std::format(fmt, std::forward<Args>(args)...))) {}

        FormatArgs(std::string_view val)
            : value_(Detail::ConvertString(val)) {}

        FormatArgs(const char* val)
            : value_(val) {}

        FormatArgs(std::nullptr_t)
            : value_(nullptr) {}

        const char* GetValue() const { return value_; }
    };

    namespace Detail
    {
        template<typename E> requires std::is_enum_v<E>
        int Enum(E e)
        {
            return static_cast<int>(e);
        }

        class InvokeBase
        {
        protected:
            template<auto Begin, auto End, bool AlwaysCallEnd, typename... Args>
                requires std::invocable<decltype(Begin), const char*, Args...>
            void InvokeBlock(FormatArgs name, auto&& body, Args&& ...args) const
            {
                bool ret;
                if (ret = Begin(name.GetValue(), std::forward<Args>(args)...))
                    body();

                if (AlwaysCallEnd || ret)
                    End();
            }
            template<auto Push, auto Pop, typename... Args>
                requires std::invocable<decltype(Push), Args...>
            void InvokeStack(FormatArgs name, auto&& body, Args&& ...args) const
            {
                Push(std::forward<Args>(args)...);
                body();
                Pop();
            }
        };

        template<typename T>
        class CallableBlock : protected InvokeBase
        {
        public:
            constexpr CallableBlock() = default;

            void operator()(std::string_view name, auto&& body) const
            {
                static_cast<const T*>(this)->operator()(name, {}, std::move(body));
            }

        protected:
            using Base = CallableBlock<T>;
        };

        template<typename T>
        class Callable : protected InvokeBase
        {
        public:
            constexpr Callable() = default;

            void operator()(std::string_view name, auto&& body) const
            {
                static_cast<const T*>(this)->operator()(name, {}, std::move(body));
            }

        protected:
            using Base = Callable<T>;
        };

        template<typename T>
        constexpr ImGuiDataType GetDataType()
        {
            using Type = std::decay_t<T>;
            static_assert(std::is_arithmetic_v<Type>);

            if constexpr (std::is_integral_v<Type>)
            {
                constexpr bool isSigned = std::is_signed_v<Type>;
                switch (sizeof(Type))
                {
                case 1:
                    return isSigned ? ImGuiDataType_S8 : ImGuiDataType_U8;
                case 2:
                    return isSigned ? ImGuiDataType_S16 : ImGuiDataType_U16;
                case 4:
                    return isSigned ? ImGuiDataType_S32 : ImGuiDataType_U32;
                case 8:
                    return isSigned ? ImGuiDataType_S64 : ImGuiDataType_U64;
                }
            }
            else if constexpr (std::is_floating_point_v<Type>)
                return sizeof(Type) == 4 ? ImGuiDataType_Float : ImGuiDataType_Double;
        }
    }

    static constexpr class AutoFitT
    {
        inline constexpr operator float() { return 0.f; }
    } AutoFit;

    static constexpr class PreserveT
    {
        inline constexpr operator float() { return -1.f; }
    } Preserve;

    static constexpr class WindowT : public Detail::CallableBlock<WindowT>
    {
    public:
        struct Params
        {
            bool* open = nullptr;
            ImGuiWindowFlags_ flags = ImGuiWindowFlags_None;
        };

        using Base::operator();
        void operator()(FormatArgs name, Params&& params, auto&& body) const
        {
            if (params.open && !*params.open)
                return;

            InvokeBlock<ImGui::Begin, ImGui::End, true>(name, std::move(body), params.open, Detail::Enum(params.flags));
        }

        [[nodiscard]] bool IsAppearing() const { return ImGui::IsWindowAppearing(); }
        [[nodiscard]] bool IsCollapsed() const { return ImGui::IsWindowCollapsed(); }
        [[nodiscard]] bool IsFocused(ImGuiFocusedFlags_ flags = ImGuiFocusedFlags_None) const { return ImGui::IsWindowFocused(Detail::Enum(flags)); }
        [[nodiscard]] bool IsHovered(ImGuiFocusedFlags_ flags = ImGuiFocusedFlags_None) const { return ImGui::IsWindowHovered(Detail::Enum(flags)); }

        [[nodiscard]] float GetDPIScale() const { return ImGui::GetWindowDpiScale(); }
        [[nodiscard]] const auto& GetPosition() const { return ImGui::GetWindowPos(); }
        [[nodiscard]] const auto& GetSize() const { return ImGui::GetWindowSize(); }
        [[nodiscard]] float GetWidth() const { return ImGui::GetWindowWidth(); }
        [[nodiscard]] float GetHeight() const { return ImGui::GetWindowHeight(); }

        [[nodiscard]] auto* GetDrawList() const { return ImGui::GetWindowDrawList(); }
        [[nodiscard]] auto* GetViewport() const { return ImGui::GetWindowViewport(); }

        struct Builder
        {
            [[nodiscard]] Builder&& Position(const ImVec2& pos, ImGuiCond_ cond = ImGuiCond_Always, const ImVec2& pivot = { 0.f, 0.f })
            {
                ImGui::SetNextWindowPos(pos, cond, pivot);
                return std::move(*this);
            }

            [[nodiscard]] Builder&& Size(const ImVec2& size, ImGuiCond_ cond = ImGuiCond_Always)
            {
                ImGui::SetNextWindowSize(size, cond);
                return std::move(*this);
            }

            [[nodiscard]] Builder&& SizeConstraints(const ImVec2& sizeMin, const ImVec2& sizeMax)
            {
                ImGui::SetNextWindowSizeConstraints(sizeMin, sizeMax);
                return std::move(*this);
            }

            [[nodiscard]] Builder&& SizeConstraints(std::invocable<ImGuiSizeCallbackData*> auto&& callback)
            {
                ImGui::SetNextWindowSizeConstraints({}, {}, callback);
                return std::move(*this);
            }
        };
    } Window;

    static constexpr class RegionT : public Detail::CallableBlock<RegionT>
    {
    public:
        struct Params
        {
            ImVec2 size = { 0.f, 0.f };
            bool border = false;
            ImGuiWindowFlags_ flags = ImGuiWindowFlags_None;
        };

        using Base::operator();
        void operator()(FormatArgs name, Params&& params, auto&& body) const
        {
            InvokeBlock<ImGui::BeginChild, ImGui::EndChild, true>(name, std::move(body), params.size, params.border, Detail::Enum(params.flags));
        }
    } Region;

    static constexpr struct TextT
    {
        void operator()(FormatArgs fmt) const
        {
            ImGui::TextUnformatted(fmt.GetValue());
        }
    } Text;

    static constexpr struct ButtonT
    {
        bool operator()(FormatArgs fmt, const ImVec2& size = {}) const
        {
            return ImGui::Button(fmt.GetValue(), size);
        }
    } Button;

    static constexpr struct SliderT
    {
        struct Params
        {
            FormatArgs format = nullptr;
            ImGuiSliderFlags_ flags = ImGuiSliderFlags_None;
        };

        template<typename T>
        bool operator()(FormatArgs fmt, T& value, const T& min, const T& max) const
        {
            return ImGui::SliderScalar(fmt.GetValue(), Detail::GetDataType<T>(), &value, &min, &max);
        }

        template<typename T>
        bool operator()(FormatArgs fmt, T& value, const T& min, const T& max, Params&& params) const
        {
            return ImGui::SliderScalar(fmt.GetValue(), Detail::GetDataType<T>(), &value, &min, &max, params.format.GetValue(), params.flags);
        }
    } Slider;
}