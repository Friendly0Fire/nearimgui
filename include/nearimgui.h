#pragma once
#include <imgui.h>
#include <imgui_internal.h>
#include <string_view>
#include <format>
#include <optional>
#include <variant>
#include <span>
#include <utility>
#include <functional>
#include <misc/cpp/imgui_stdlib.h>

namespace NGui
{
    template<typename E> requires std::is_enum_v<E>
    constexpr bool EnableFlagOperators = false;

    template<> constexpr bool EnableFlagOperators<ImGuiWindowFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiChildFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiInputTextFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiTreeNodeFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiPopupFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiSelectableFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiComboFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiTabBarFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiTabItemFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiFocusedFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiHoveredFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiDockNodeFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiDragDropFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiButtonFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiColorEditFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiSliderFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiTableFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiTableColumnFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiTableRowFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiSeparatorFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiFocusRequestFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiTextFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiTooltipFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiNextWindowDataFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiNextItemDataFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiInputFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiActivateFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiScrollFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiNavHighlightFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiNavMoveFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiTypingSelectFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiOldColumnFlags_> = true;
    template<> constexpr bool EnableFlagOperators<ImGuiDebugLogFlags_> = true;

    template<typename E, typename EP> requires std::is_enum_v<E> && std::is_enum_v<EP>
    constexpr bool IsEquivalentEnum = false;
    template<typename E> constexpr bool IsEquivalentEnum<E, E> = true;

    template<> constexpr bool IsEquivalentEnum<ImGuiHoveredFlags_, ImGuiHoveredFlagsPrivate_> = true;
    template<> constexpr bool IsEquivalentEnum<ImGuiInputTextFlags_, ImGuiInputTextFlagsPrivate_> = true;
    template<> constexpr bool IsEquivalentEnum<ImGuiButtonFlags_, ImGuiButtonFlagsPrivate_> = true;
    template<> constexpr bool IsEquivalentEnum<ImGuiComboFlags_, ImGuiComboFlagsPrivate_> = true;
    template<> constexpr bool IsEquivalentEnum<ImGuiSliderFlags_, ImGuiSliderFlagsPrivate_> = true;
    template<> constexpr bool IsEquivalentEnum<ImGuiSelectableFlags_, ImGuiSelectableFlagsPrivate_> = true;
    template<> constexpr bool IsEquivalentEnum<ImGuiTreeNodeFlags_, ImGuiTreeNodeFlagsPrivate_> = true;
    template<> constexpr bool IsEquivalentEnum<ImGuiDockNodeFlags_, ImGuiDockNodeFlagsPrivate_> = true;
    template<> constexpr bool IsEquivalentEnum<ImGuiTabBarFlags_, ImGuiTabBarFlagsPrivate_> = true;
    template<> constexpr bool IsEquivalentEnum<ImGuiTabItemFlags_, ImGuiTabItemFlagsPrivate_> = true;

    template<typename E> requires std::is_enum_v<E>
    constexpr auto ToUnderlying(E e)
    {
        return static_cast<std::underlying_type_t<E>>(e);
    }

    template<typename E, typename E2> requires EnableFlagOperators<E> && IsEquivalentEnum<E, E2>
    constexpr auto operator|(E a, E2 b)
    {
        return static_cast<E>(ToUnderlying(a) | ToUnderlying(b));
    }

    template<typename E, typename E2> requires EnableFlagOperators<E>&& IsEquivalentEnum<E, E2>
    constexpr auto operator&(E a, E2 b)
    {
        return static_cast<E>(ToUnderlying(a) & ToUnderlying(b));
    }

    template<typename E, typename E2> requires EnableFlagOperators<E>&& IsEquivalentEnum<E, E2>
    constexpr auto& operator|=(E& a, E2 b)
    {
        a = static_cast<E>(ToUnderlying(a) | ToUnderlying(b));
        return a;
    }

    template<typename E, typename E2> requires EnableFlagOperators<E>&& IsEquivalentEnum<E, E2>
    constexpr auto& operator&=(E& a, E2 b)
    {
        a = static_cast<E>(ToUnderlying(a) & ToUnderlying(b));
        return a;
    }

    template<typename T, typename F = std::function<bool(T)>> requires std::convertible_to<std::invoke_result_t<F, const T&>, bool>
    class Validated
    {
        T value_{};
        T cached_{};
        F validator_ = [](T) { return true; };

    public:
        using Type = T;
        using Validator = F;

        Validated(T&& initialValue, F&& validator)
            : value_(std::move(initialValue)), cached_(value_), validator_(std::move(validator)) {}

        Validated(F&& validator)
            : validator_(std::move(validator)) {}

        Validated() {}

        const T& get() const { return cached_; }
        operator const T&() const { return get(); }
        T& ref() { return value_; }

        bool update(bool changed)
        {
            if (changed && validator_(value_))
                cached_ = value_;

            return changed;
        }
    };

    static constexpr struct AutoFitT
    {
        static constexpr float value = 0.f;
    } AutoFit;

    static constexpr struct PreserveT
    {
        static constexpr float value = -1.f;
    } Preserve;

    namespace Detail
    {
        template<typename T>
        concept IsValidated = std::same_as<T, Validated<typename T::Type, typename T::Validator>>;

        template<typename T>
        concept Numeric = std::integral<T> || std::floating_point<T>;

        template<typename T>
        concept IsValidatedNumber = IsValidated<T> && Numeric<typename T::Type>;

        template<class... Ts>
        struct Overloaded : Ts... { using Ts::operator()...; };

        const char* CacheString(std::string_view sv);
        const char* CacheString(std::string&& s);

        inline const char* CacheString(const std::string& s)
        {
            return s.c_str();
        }

        inline const char* CacheString(const char* s)
        {
            return s;
        }

        struct BaseCallback
        {
            BaseCallback(const BaseCallback&) = delete;
            BaseCallback(BaseCallback&&) = delete;
            virtual ~BaseCallback() = default;

        protected:
            BaseCallback() = default;
        };

        template<typename F>
        struct Callback : BaseCallback
        {
            Callback(F&& cb) : callback(std::move(cb)) {}
            ~Callback() override = default;

            F callback;
        };

        template<typename C>
        struct CallbackTraits {};

        template<typename R, typename... Args>
        struct CallbackTraits<R(*)(Args...)>
        {
            using ReturnValue = R;
            using Arguments = std::tuple<Args...>;
            static constexpr size_t ArgumentCount = sizeof...(Args);
        };

        template<typename T>
        concept IsVoidPtr = std::same_as<std::remove_cvref_t<T>, void*>;

        template<typename C>
        concept IsDataPointerStyleCallback = requires(std::tuple_element_t<0, typename CallbackTraits<C>::Arguments> data) { { data->UserData } -> IsVoidPtr; };

        template<typename C>
        concept IsVoidPointerStyleCallback = IsVoidPtr<std::tuple_element_t<CallbackTraits<C>::ArgumentCount - 1, typename CallbackTraits<C>::Arguments>>;

        BaseCallback* CacheCallback(std::unique_ptr<BaseCallback>&& callback);

        /**
         * Given a callable with no state and which can be implicitly converted to a function pointer, passes callback along unchanged and does not set the user data void pointer.
         * @tparam C ImGui callback type to match.
         * @tparam F Provided callable type.
         * @param callback The callback.
         * @return A pair containing the function pointer callback and the user data pointer (here, always null).
        */
        template<typename C, bool Cache, typename F>
        requires std::convertible_to<F, C>
        std::pair<C, void*> ThunkCallback(F&& callback)
        {
            return std::make_pair(callback, nullptr);
        }

        /**
         * Given a callable which cannot be implictly converted to a function pointer, caches it and returns a thunk which wraps the call to it
         * into a function pointer-convertible callback leveraging ImGui's user data pointer.
         * This overload works on callbacks with a void* user_data as the last argument.
         * @tparam C ImGui callback type to match.
         * @tparam F Provided callable type.
         * @param callback The callback.
         * @return A pair containing the function pointer callback and the user data pointer storing the stateful callback.
        */
        template<typename C, bool Cache, typename F>
        requires (!std::convertible_to<F, C>) && IsVoidPointerStyleCallback<C>
        auto ThunkCallback(F&& callback)
        {
            auto cb = [&] {
                auto c = std::make_unique<Callback<F>>(std::move(callback));
                if constexpr (Cache)
                    return static_cast<Callback<F>*>(CacheCallback(std::move(c)));
                else
                    return std::move(c);
                }();

            return [=]<typename R, typename... Args>(R(*)(Args..., void*)) {
                return std::make_pair([](Args&& ...args, void* user_data) {
                    return static_cast<Callback<F>*>(user_data)->callback(std::forward<Args>(args)...);
                    }, std::move(cb));
            }(static_cast<C>(nullptr));
        }

        /**
         * Given a callable which cannot be implictly converted to a function pointer, caches it and returns a thunk which wraps the call to it
         * into a function pointer-convertible callback leveraging ImGui's user data pointer.
         * This overload works on callbacks with a Data* pointer which contains a void* pointer named UserData.
         * @tparam C ImGui callback type to match.
         * @tparam F Provided callable type.
         * @param callback The callback.
         * @return A pair containing the function pointer callback and the user data pointer storing the stateful callback.
        */
        template<typename C, bool Cache, typename F>
            requires (!std::convertible_to<F, C>) && IsDataPointerStyleCallback<C>
        auto ThunkCallback(F&& callback)
        {
            auto cb = [&] {
                auto c = std::make_unique<Callback<F>>(std::move(callback));
                if constexpr (Cache)
                    return static_cast<Callback<F>*>(CacheCallback(std::move(c)));
                else
                    return std::move(c);
                }();

            using Data = std::tuple_element_t<0, typename CallbackTraits<C>::Arguments>;
            return std::make_pair([](Data data) {
                return static_cast<Callback<F>*>(data->UserData)->callback(data);
                }, std::move(cb));
        }
    }

    void NewFrame();

    class FormatArgs
    {
        const char* value_;

    public:
        template<typename... Args>
        FormatArgs(std::format_string<Args...> fmt, Args&& ...args)
            : value_(Detail::CacheString(std::format(fmt, std::forward<Args>(args)...))) {}

        FormatArgs(std::string_view val)
            : value_(Detail::CacheString(val)) {}

        FormatArgs(const char* val)
            : value_(Detail::CacheString(val)) {}

        FormatArgs(std::nullptr_t)
            : value_(nullptr) {}

        const char* GetValue() const { return value_; }
    };

    class FormatArgsWithEnd
    {
        const char* value_;
        const char* valueEnd_ = nullptr;

    public:
        template<typename... Args>
        FormatArgsWithEnd(std::format_string<Args...> fmt, Args&& ...args)
            : value_(Detail::CacheString(std::format(fmt, std::forward<Args>(args)...))) {}

        FormatArgsWithEnd(std::string_view val)
            : value_(val.data()), valueEnd_(val.data() + val.size()) {}

        FormatArgsWithEnd(const char* val)
            : value_(val) {}

        FormatArgsWithEnd(const char* val, const char* valEnd)
            : value_(val), valueEnd_(valEnd) {}

        FormatArgsWithEnd(std::nullptr_t)
            : value_(nullptr) {}

        const char* GetValue() const { return value_; }
        const char* GetValueEnd() const { return valueEnd_; }
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
            void InvokeBlock(auto&& body, Args&& ...args) const
            {
                bool ret;
                if (ret = Begin(std::forward<Args>(args)...))
                    body();

                if (AlwaysCallEnd || ret)
                    End();
            }

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
            void InvokeStack(auto&& body, Args&& ...args) const
            {
                Push(std::forward<Args>(args)...);
                body();
                if constexpr (requires() { Pop(1); })
                    Pop(1);
                else
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
        consteval ImGuiDataType GetDataType()
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

            return ImGuiDataType_COUNT;
        }

        template<typename T>
        class BaseItem
        {
        public:
            const T& operator[](float width) const
            {
                ImGui::SetNextItemWidth(width);
                return static_cast<const T&>(*this);
            }
        };
    }

    template<typename T>
    class CommonWindowT
    {
    public:
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

        [[nodiscard]] auto GetContentRegionMin() const { return ImGui::GetWindowContentRegionMin(); }
        [[nodiscard]] auto GetContentRegionMax() const { return ImGui::GetWindowContentRegionMax(); }
        [[nodiscard]] auto GetScrollX() const { return ImGui::GetScrollX(); }
        [[nodiscard]] auto GetScrollY() const { return ImGui::GetScrollY(); }
        [[nodiscard]] auto GetScrollMaxX() const { return ImGui::GetScrollMaxX(); }
        [[nodiscard]] auto GetScrollMaxY() const { return ImGui::GetScrollMaxY(); }

        [[nodiscard]] const auto& Position(const ImVec2& pos, ImGuiCond_ cond = ImGuiCond_Always, const ImVec2& pivot = { 0.f, 0.f }) const
        {
            ImGui::SetNextWindowPos(pos, cond, pivot);
            return static_cast<const T&>(*this);
        }

        [[nodiscard]] const auto& Size(const ImVec2& size, ImGuiCond_ cond = ImGuiCond_Always) const
        {
            ImGui::SetNextWindowSize(size, cond);
            return static_cast<const T&>(*this);
        }

        [[nodiscard]] const auto& SizeConstraints(const ImVec2& sizeMin, const ImVec2& sizeMax) const
        {
            ImGui::SetNextWindowSizeConstraints(sizeMin, sizeMax);
            return static_cast<const T&>(*this);
        }

        [[nodiscard]] const auto& SizeConstraints(std::invocable<ImGuiSizeCallbackData*> auto&& callback) const
        {
            auto&& [cb, data] = Detail::ThunkCallback<ImGuiSizeCallback, true>(std::move(callback));
            ImGui::SetNextWindowSizeConstraints({}, {}, cb, data);

            return static_cast<const T&>(*this);
        }

        [[nodiscard]] const auto& ContentSize(const ImVec2& size) const
        {
            ImGui::SetNextWindowContentSize(size);
            return static_cast<const T&>(*this);
        }

        [[nodiscard]] const auto& Focus() const
        {
            ImGui::SetNextWindowFocus();
            return static_cast<const T&>(*this);
        }

        [[nodiscard]] const auto& Scroll(const ImVec2& scroll) const
        {
            ImGui::SetNextWindowScroll(scroll);
            return static_cast<const T&>(*this);
        }

        [[nodiscard]] const auto& SetBackgroundAlpha(float alpha) const
        {
            ImGui::SetNextWindowBgAlpha(alpha);
            return static_cast<const T&>(*this);
        }

        const class CursorT
        {
        public:
            auto GetStartPosition() const { return ImGui::GetCursorStartPos(); }
            auto GetPosition() const { return ImGui::GetCursorPos(); }
            auto GetX() const { return ImGui::GetCursorPosX(); }
            auto GetY() const { return ImGui::GetCursorPosY(); }

            auto SetPosition(const ImVec2& windowPosition) const { return ImGui::SetCursorPos(windowPosition); }
            auto SetX(float windowX) const { return ImGui::SetCursorPosX(windowX); }
            auto SetY(float windowY) const { return ImGui::SetCursorPosY(windowY); }
        } Cursor;
    };

    static constexpr class WindowT : public Detail::CallableBlock<WindowT>, public CommonWindowT<WindowT>
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

            InvokeBlock<ImGui::Begin, ImGui::End, true>(name, std::forward<decltype(body)>(body), params.open, Detail::Enum(params.flags));
        }

        [[nodiscard]] const auto& Collapsed(bool collapsed, ImGuiCond cond = ImGuiCond_Always) const
        {
            ImGui::SetNextWindowCollapsed(collapsed, cond);
            return *this;
        }

        [[nodiscard]] const auto& Viewport(ImGuiID viewportID) const
        {
            ImGui::SetNextWindowViewport(viewportID);
            return *this;
        }

    } Window;

    static constexpr class RegionT : public Detail::CallableBlock<RegionT>, public CommonWindowT<RegionT>
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
            InvokeBlock<ImGui::BeginChild, ImGui::EndChild, true>(name, std::forward<decltype(body)>(body), params.size, params.border, Detail::Enum(params.flags));
        }
    } Region;

    struct StyleVarX
    {
        ImGuiStyleVar_ v;
    };

    struct StyleVarY
    {
        ImGuiStyleVar_ v;
    };

    struct Color
    {
    private:
        struct BaseCol
        {
            ImVec4 col;

            BaseCol(const ImVec4& col) : col(col) {}
            BaseCol(ImU32 col) : col(ImGui::ColorConvertU32ToFloat4(col)) {}

            friend class Style;
        };

    public:
#define NGUI_DECLARE_STYLE_COL(Name_) struct Name_ : BaseCol { private: static constexpr ImGuiCol_ NGuiCol = ImGuiCol_##Name_; friend class Style; }

        NGUI_DECLARE_STYLE_COL(Text);
        NGUI_DECLARE_STYLE_COL(TextDisabled);
        NGUI_DECLARE_STYLE_COL(WindowBg);              // Background of normal windows
        NGUI_DECLARE_STYLE_COL(ChildBg);               // Background of child windows
        NGUI_DECLARE_STYLE_COL(PopupBg);               // Background of popups, menus, tooltips windows
        NGUI_DECLARE_STYLE_COL(Border);
        NGUI_DECLARE_STYLE_COL(BorderShadow);
        NGUI_DECLARE_STYLE_COL(FrameBg);               // Background of checkbox, radio button, plot, slider, text input
        NGUI_DECLARE_STYLE_COL(FrameBgHovered);
        NGUI_DECLARE_STYLE_COL(FrameBgActive);
        NGUI_DECLARE_STYLE_COL(TitleBg);               // Title bar
        NGUI_DECLARE_STYLE_COL(TitleBgActive);         // Title bar when focused
        NGUI_DECLARE_STYLE_COL(TitleBgCollapsed);      // Title bar when collapsed
        NGUI_DECLARE_STYLE_COL(MenuBarBg);
        NGUI_DECLARE_STYLE_COL(ScrollbarBg);
        NGUI_DECLARE_STYLE_COL(ScrollbarGrab);
        NGUI_DECLARE_STYLE_COL(ScrollbarGrabHovered);
        NGUI_DECLARE_STYLE_COL(ScrollbarGrabActive);
        NGUI_DECLARE_STYLE_COL(CheckMark);             // Checkbox tick and RadioButton circle
        NGUI_DECLARE_STYLE_COL(SliderGrab);
        NGUI_DECLARE_STYLE_COL(SliderGrabActive);
        NGUI_DECLARE_STYLE_COL(Button);
        NGUI_DECLARE_STYLE_COL(ButtonHovered);
        NGUI_DECLARE_STYLE_COL(ButtonActive);
        NGUI_DECLARE_STYLE_COL(Header);                // Header* colors are used for CollapsingHeader, TreeNode, Selectable, MenuItem
        NGUI_DECLARE_STYLE_COL(HeaderHovered);
        NGUI_DECLARE_STYLE_COL(HeaderActive);
        NGUI_DECLARE_STYLE_COL(Separator);
        NGUI_DECLARE_STYLE_COL(SeparatorHovered);
        NGUI_DECLARE_STYLE_COL(SeparatorActive);
        NGUI_DECLARE_STYLE_COL(ResizeGrip);            // Resize grip in lower-right and lower-left corners of windows.
        NGUI_DECLARE_STYLE_COL(ResizeGripHovered);
        NGUI_DECLARE_STYLE_COL(ResizeGripActive);
        NGUI_DECLARE_STYLE_COL(TabHovered);            // Tab background, when hovered
        NGUI_DECLARE_STYLE_COL(Tab);                   // Tab background, when tab-bar is focused & tab is unselected
        NGUI_DECLARE_STYLE_COL(TabSelected);           // Tab background, when tab-bar is focused & tab is selected
        NGUI_DECLARE_STYLE_COL(TabSelectedOverline);   // Tab horizontal overline, when tab-bar is focused & tab is selected
        NGUI_DECLARE_STYLE_COL(TabDimmed);             // Tab background, when tab-bar is unfocused & tab is unselected
        NGUI_DECLARE_STYLE_COL(TabDimmedSelected);     // Tab background, when tab-bar is unfocused & tab is selected
        NGUI_DECLARE_STYLE_COL(TabDimmedSelectedOverline);//..horizontal overline, when tab-bar is unfocused & tab is selected
        NGUI_DECLARE_STYLE_COL(DockingPreview);        // Preview overlay color when about to docking something
        NGUI_DECLARE_STYLE_COL(DockingEmptyBg);        // Background color for empty node (e.g. CentralNode with no window docked into it)
        NGUI_DECLARE_STYLE_COL(PlotLines);
        NGUI_DECLARE_STYLE_COL(PlotLinesHovered);
        NGUI_DECLARE_STYLE_COL(PlotHistogram);
        NGUI_DECLARE_STYLE_COL(PlotHistogramHovered);
        NGUI_DECLARE_STYLE_COL(TableHeaderBg);         // Table header background
        NGUI_DECLARE_STYLE_COL(TableBorderStrong);     // Table outer and header borders (prefer using Alpha=1.0 here)
        NGUI_DECLARE_STYLE_COL(TableBorderLight);      // Table inner borders (prefer using Alpha=1.0 here)
        NGUI_DECLARE_STYLE_COL(TableRowBg);            // Table row background (even rows)
        NGUI_DECLARE_STYLE_COL(TableRowBgAlt);         // Table row background (odd rows)
        NGUI_DECLARE_STYLE_COL(TextLink);              // Hyperlink color
        NGUI_DECLARE_STYLE_COL(TextSelectedBg);
        NGUI_DECLARE_STYLE_COL(DragDropTarget);        // Rectangle highlighting a drop target
        NGUI_DECLARE_STYLE_COL(NavHighlight);          // Gamepad/keyboard: current highlighted item
        NGUI_DECLARE_STYLE_COL(NavWindowingHighlight); // Highlight window when using CTRL+TAB
        NGUI_DECLARE_STYLE_COL(NavWindowingDimBg);     // Darken/colorize entire screen behind the CTRL+TAB window list, when active
        NGUI_DECLARE_STYLE_COL(ModalWindowDimBg);      // Darken/colorize entire screen behind a modal window, when one is active

#undef NGUI_DECLARE_STYLE_COL
        static_assert(ImGuiCol_COUNT == 58);
    };
    
    static constexpr class Style : protected Detail::InvokeBase
    {
    private:

        template<typename T, ImGuiStyleVar_ V>
        struct BaseVar
        {
            T val;

            BaseVar(float val) : val(val) {}

        private:
            static constexpr ImGuiStyleVar_ NGuiStyleVar = V;
            friend class Style;
        };

        template<ImGuiStyleVar_ V>
        struct BaseVar<ImVec2, V>
        {
            ImVec2 val;

            BaseVar(const ImVec2& val) : val(val) {}
            BaseVar(float x, float y) : val(x, y) {}

            struct X : BaseVar<float, V> { using BaseVar<float, V>::BaseVar; private: static constexpr bool IsX = true; friend class Style; };
            struct Y : BaseVar<float, V> { using BaseVar<float, V>::BaseVar; private: static constexpr bool IsY = true; friend class Style; };

        private:
            static constexpr ImGuiStyleVar_ NGuiStyleVar = V;
            friend class Style;
        };

    public:
#define NGUI_DECLARE_STYLE_VAR(Name_) struct Name_ : BaseVar<decltype(ImGuiStyle::Name_), ImGuiStyleVar_##Name_> { using BaseVar::BaseVar; }

        NGUI_DECLARE_STYLE_VAR(Alpha);
        NGUI_DECLARE_STYLE_VAR(DisabledAlpha);
        NGUI_DECLARE_STYLE_VAR(WindowPadding);
        NGUI_DECLARE_STYLE_VAR(WindowRounding);
        NGUI_DECLARE_STYLE_VAR(WindowBorderSize);
        NGUI_DECLARE_STYLE_VAR(WindowMinSize);
        NGUI_DECLARE_STYLE_VAR(WindowTitleAlign);
        NGUI_DECLARE_STYLE_VAR(ChildRounding);
        NGUI_DECLARE_STYLE_VAR(ChildBorderSize);
        NGUI_DECLARE_STYLE_VAR(PopupRounding);
        NGUI_DECLARE_STYLE_VAR(PopupBorderSize);
        NGUI_DECLARE_STYLE_VAR(FramePadding);
        NGUI_DECLARE_STYLE_VAR(FrameRounding);
        NGUI_DECLARE_STYLE_VAR(FrameBorderSize);
        NGUI_DECLARE_STYLE_VAR(ItemSpacing);
        NGUI_DECLARE_STYLE_VAR(ItemInnerSpacing);
        NGUI_DECLARE_STYLE_VAR(IndentSpacing);
        NGUI_DECLARE_STYLE_VAR(CellPadding);
        NGUI_DECLARE_STYLE_VAR(ScrollbarSize);
        NGUI_DECLARE_STYLE_VAR(ScrollbarRounding);
        NGUI_DECLARE_STYLE_VAR(GrabMinSize);
        NGUI_DECLARE_STYLE_VAR(GrabRounding);
        NGUI_DECLARE_STYLE_VAR(TabRounding);
        NGUI_DECLARE_STYLE_VAR(TabBorderSize);
        NGUI_DECLARE_STYLE_VAR(TabBarBorderSize);
        NGUI_DECLARE_STYLE_VAR(TabBarOverlineSize);
        NGUI_DECLARE_STYLE_VAR(TableAngledHeadersAngle);
        NGUI_DECLARE_STYLE_VAR(TableAngledHeadersTextAlign);
        NGUI_DECLARE_STYLE_VAR(ButtonTextAlign);
        NGUI_DECLARE_STYLE_VAR(SelectableTextAlign);
        NGUI_DECLARE_STYLE_VAR(SeparatorTextBorderSize);
        NGUI_DECLARE_STYLE_VAR(SeparatorTextAlign);
        NGUI_DECLARE_STYLE_VAR(SeparatorTextPadding);
        NGUI_DECLARE_STYLE_VAR(DockingSeparatorSize);

#undef NGUI_DECLARE_STYLE_VAR
        static_assert(ImGuiStyleVar_COUNT == 34);

    private:
        struct State
        {
            short v = 0;
            short c = 0;
            short f = 0;
        };

        template<typename Var> requires requires() { { Var::NGuiStyleVar } -> std::convertible_to<ImGuiStyleVar_>; }
        void Push(const Var& v, State& s) const
        {
            ++s.v;

            if constexpr (requires() { Var::IsX; })
                ImGui::PushStyleVarX(v.NGuiStyleVar, v.val);
            else if constexpr (requires() { Var::IsY; })
                ImGui::PushStyleVarY(v.NGuiStyleVar, v.val);
            else
                ImGui::PushStyleVar(v.NGuiStyleVar, v.val);
        }

        template<typename Col> requires requires() { { Col::NGuiCol } -> std::convertible_to<ImGuiCol_>; }
        void Push(const Col& c, State& s) const
        {
            ++s.c;
            ImGui::PushStyleColor(c.NGuiCol, c.col);
        }

        void Push(ImFont* f, State& s) const
        {
            ++s.f;
            ImGui::PushFont(f);
        }

        template<typename ...Types>
        void Push(const std::tuple<Types...>& tup, State& s) const
        {
            [&]<std::size_t ...I>(std::index_sequence<I...>) {
                (Push(std::get<I>(tup), s), ...);
            }(std::make_index_sequence<std::tuple_size_v<std::tuple<Types...>>>());
        }

        void Push(std::invocable auto&& body, const State&) const
        {
            body();
        }

        void Pop(State& s) const
        {
            if (s.v > 0) ImGui::PopStyleVar(s.v);
            if (s.c > 0) ImGui::PopStyleColor(s.c);
            while (s.f-- > 0) ImGui::PopFont();
        }

    public:
        template<typename... Args> requires (sizeof...(Args) > 1)
            void operator()(Args&& ...args) const
        {
            State s;
            (Push(args, s), ...);
            Pop(s);
        }
    } Style;

    static constexpr class TabStopT : protected Detail::InvokeBase
    {
    public:
        void operator()(bool tabStop, auto&& body) const
        {
            InvokeStack<ImGui::PushTabStop, ImGui::PopTabStop>(std::forward<decltype(body)>(body), tabStop);
        }
    } TabStop;

    static constexpr class ButtonRepeatT : protected Detail::InvokeBase
    {
    public:
        void operator()(bool repeat, auto&& body) const
        {
            InvokeStack<ImGui::PushButtonRepeat, ImGui::PopButtonRepeat>(std::forward<decltype(body)>(body), repeat);
        }
    } ButtonRepeat;

    static constexpr class ItemWidthT : protected Detail::InvokeBase
    {
    public:
        void operator()(float itemWidth, auto&& body) const
        {
            InvokeStack<ImGui::PushItemWidth, ImGui::PopItemWidth>(std::forward<decltype(body)>(body), itemWidth);
        }
    } ItemWidth;

    static constexpr class ItemFlagsT : protected Detail::InvokeBase
    {
    public:
        void operator()(ImGuiItemFlags_ flags, bool active, auto&& body) const
        {
            InvokeStack<ImGui::PushItemFlag, ImGui::PopItemFlag>(std::forward<decltype(body)>(body), flags, active);
        }
    } ItemFlags;

    static constexpr class TextWrapPosT : protected Detail::InvokeBase
    {
    public:
        void operator()(float wrapLocalPosX, auto&& body) const
        {
            InvokeStack<ImGui::PushTextWrapPos, ImGui::PopTextWrapPos>(std::forward<decltype(body)>(body), wrapLocalPosX);
        }

        void Disable(auto&& body) const
        {
            operator()(-1.f, std::forward<decltype(body)>(body));
        }

        void ToEnd(auto&& body) const
        {
            operator()(0.f, std::forward<decltype(body)>(body));
        }

    } TextWrapPos;

    static constexpr class GroupT : protected Detail::InvokeBase
    {
    public:
        void operator()(auto&& body) const
        {
            InvokeBlock<ImGui::BeginGroup, ImGui::EndGroup, false>(std::forward<decltype(body)>(body));
        }
    } Group;

    static constexpr struct TextT
    {
        void operator()(FormatArgsWithEnd fmt) const
        {
            ImGui::TextUnformatted(fmt.GetValue(), fmt.GetValueEnd());
        }

        void Colored(const ImVec4& col, FormatArgsWithEnd fmt)
        {
            //Style(Col{ ImGuiCol_Text, col },
            //    [&] { operator()(std::move(fmt)); });
        }

        void Disabled(FormatArgsWithEnd fmt)
        {
            //Style(Col{ ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled] },
            //    [&] { operator()(std::move(fmt)); });
        }

        void Wrapped(FormatArgsWithEnd fmt)
        {
            TextWrapPos(0.f, [&] { operator()(std::move(fmt)); });
        }

        void Bullet(FormatArgsWithEnd fmt)
        {
            ImGui::Bullet();
            operator()(std::move(fmt));
        }

        // TODO: LabelText, SeparatorText
    } Text;

    static constexpr struct ButtonT
    {
        bool operator()(FormatArgs fmt, const ImVec2& size = {}) const
        {
            return ImGui::Button(fmt.GetValue(), size);
        }

        bool Small(FormatArgs fmt) const
        {
            return ImGui::SmallButton(fmt.GetValue());
        }

        bool Invisible(FormatArgs fmtID, const ImVec2& size = {}, ImGuiButtonFlags flags = ImGuiButtonFlags_None) const
        {
            return ImGui::InvisibleButton(fmtID.GetValue(), size, flags);
        }

        bool Arrow(FormatArgs fmt, ImGuiDir dir) const
        {
            return ImGui::ArrowButton(fmt.GetValue(), dir);
        }

    } Button;

    static constexpr struct CheckboxT
    {
        bool operator()(FormatArgs fmt, bool& v) const
        {
            return ImGui::Checkbox(fmt.GetValue(), &v);
        }

        template<std::integral T>
        bool Flags(FormatArgs fmt, T& value, T fullMask) const
        {
            if constexpr (std::signed_integral<T>)
                return ImGui::CheckboxFlags(fmt.GetValue(), reinterpret_cast<int*>(&value), static_cast<int>(fullMask));
            else
                return ImGui::CheckboxFlags(fmt.GetValue(), reinterpret_cast<unsigned int*>(&value), static_cast<unsigned int>(fullMask));
        }

    } Checkbox;

    static constexpr struct RadioButtonT
    {
        bool operator()(FormatArgs fmt, bool& active) const
        {
            return ImGui::RadioButton(fmt.GetValue(), &active);
        }

        bool operator()(FormatArgs fmt, int& selectedIndex, int thisIndex) const
        {
            return ImGui::RadioButton(fmt.GetValue(), &selectedIndex, thisIndex);
        }

        std::optional<int> operator()(std::initializer_list<std::pair<FormatArgs, int>> radioButtons, int& selectedIndex, bool sameLine = false) const
        {
            std::optional<int> modifiedIndex = std::nullopt;

            for (auto it = radioButtons.begin(); it != radioButtons.end(); )
            {
                if (ImGui::RadioButton(it->first.GetValue(), &selectedIndex, it->second))
                    modifiedIndex = it->second;

                ++it;

                if (sameLine && it != radioButtons.end())
                    ImGui::SameLine();
            }

            return modifiedIndex;
        }

        std::optional<int> operator()(std::initializer_list<FormatArgs> radioButtons, int& selectedIndex, bool sameLine = false) const
        {
            std::optional<int> modifiedIndex = std::nullopt;

            int index = 0;
            for (auto it = radioButtons.begin(); it != radioButtons.end(); )
            {
                if (ImGui::RadioButton(it->GetValue(), &selectedIndex, index))
                    modifiedIndex = index;

                ++index;
                ++it;

                if (sameLine && it != radioButtons.end())
                    ImGui::SameLine();
            }

            return modifiedIndex;
        }

    } RadioButton;

    static constexpr struct SliderT : public Detail::BaseItem<SliderT>
    {
        struct Params
        {
            FormatArgs format = nullptr;
            ImGuiSliderFlags_ flags = ImGuiSliderFlags_None;
        };

        template<Detail::Numeric T>
        bool operator()(FormatArgs fmt, T& value, const T& min, const T& max) const
        {
            return ImGui::SliderScalar(fmt.GetValue(), Detail::GetDataType<T>(), &value, &min, &max);
        }

        template<Detail::IsValidatedNumber T>
        bool operator()(FormatArgs fmt, T& value, const typename T::Type& min, const typename T::Type& max) const
        {
            return value.update(operator()(fmt, value.ref(), min, max));
        }

        template<typename T>
        bool operator()(FormatArgs fmt, T& value, const T& min, const T& max, Params&& params) const
        {
            return ImGui::SliderScalar(fmt.GetValue(), Detail::GetDataType<T>(), &value, &min, &max, params.format.GetValue(), Detail::Enum(params.flags));
        }

        template<Detail::IsValidatedNumber T>
        bool operator()(FormatArgs fmt, T& value, const typename T::Type& min, const typename T::Type& max, Params&& params) const
        {
            return value.update(operator()(fmt, value.ref(), min, max, std::move(params)));
        }

        bool Angle(FormatArgs fmt, float& value) const
        {
            return ImGui::SliderAngle(fmt.GetValue(), &value);
        }

        template<typename F>
        bool Angle(FormatArgs fmt, Validated<float, F>& value) const
        {
            return value.update(Angle(fmt, value.ref()));
        }

        struct AngleParams : Params
        {
            float min = -360.f;
            float max = 360.f;
        };

        bool Angle(FormatArgs fmt, float& value, AngleParams&& params)
        {
            return ImGui::SliderAngle(fmt.GetValue(), &value, params.min, params.max, params.format.GetValue(), Detail::Enum(params.flags));
        }
    } Slider;

    static constexpr struct DragT : public Detail::BaseItem<DragT>
    {
        template<typename T>
        struct Params
        {
            float speed = 1.f;
            std::optional<T> min = std::nullopt;
            std::optional<T> max = std::nullopt;
            FormatArgs format = nullptr;
            ImGuiSliderFlags_ flags = ImGuiSliderFlags_None;
        };

        template<Detail::Numeric T>
        bool operator()(FormatArgs fmt, T& value) const
        {
            return ImGui::DragScalar(fmt.GetValue(), Detail::GetDataType<T>(), &value);
        }

        template<Detail::IsValidatedNumber T>
        bool operator()(FormatArgs fmt, T& value) const
        {
            return value.update(operator()(fmt, value.ref()));
        }

        template<Detail::Numeric T>
        bool operator()(FormatArgs fmt, T& value, Params<T>&& params) const
        {
            return ImGui::DragScalar(fmt.GetValue(), Detail::GetDataType<T>(), &value, params.speed,
                params.min ? &*params.min : nullptr,
                params.max ? &*params.max : nullptr,
                params.format.GetValue(), params.flags);
        }

        template<Detail::IsValidatedNumber T>
        bool operator()(FormatArgs fmt, T& value, Params<typename T::Type>&& params) const
        {
            return value.update(operator()(fmt, value.ref(), std::move(params)));
        }

        template<typename T>
        struct ParamsRange : Params<T>
        {
            FormatArgs formatMax = nullptr;
        };

        template<typename T>
        bool operator()(FormatArgs fmt, T& minValue, T& maxValue, ParamsRange<T>&& params) const
        {
            using T2 = std::conditional_t<std::is_integral_v<T>, int, float>;
            constexpr auto f = [](auto&& ...args) {
                if constexpr (std::integral<T>)
                    return ImGui::DragIntRange2(std::forward<decltype(args)>(args)...);
                else if constexpr (std::floating_point<T>)
                    return ImGui::DragFloatRange2(std::forward<decltype(args)>(args)...);
                else
                    return false;
            };
            T2 v1 = static_cast<T2>(minValue), v2 = static_cast<T2>(maxValue);
            bool r = f(fmt.GetValue(), &v1, &v2, params.speed,
                params.min ? *params.min : 0.f,
                params.max ? *params.max : 0.f,
                params.format.GetValue(),
                params.formatMax.GetValue(), params.flags);
            minValue = static_cast<T>(v1);
            maxValue = static_cast<T>(v2);

            return r;
        }

        template<typename T>
        bool operator()(FormatArgs fmt, T& minValue, T& maxValue) const
        {
            return operator()(fmt, minValue, maxValue, ParamsRange<T>{});
        }
    } Drag;

    static constexpr struct TextBoxT : public Detail::BaseItem<TextBoxT>
    {
    private:
        struct StringResizeCallbackData
        {
            std::string& str;
            ImGuiInputTextCallback chainCallback;
            void* chainCallbackUserData;
        };

    public:
        struct Params {
            ImGuiInputTextFlags_ flags = ImGuiInputTextFlags_None;
            std::optional<ImVec2> size = std::nullopt;
            std::optional<FormatArgs> hint = std::nullopt;
        };

        static int StringResizeCallback(ImGuiInputTextCallbackData* data)
        {
            auto* userData = static_cast<StringResizeCallbackData*>(data->UserData);
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
            {
                // Resize string callback
                // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
                std::string& str = userData->str;
                IM_ASSERT(data->Buf == str.c_str());
                str.resize(data->BufTextLen);
                data->Buf = str.data();
            }
            if (userData && userData->chainCallback)
            {
                // Forward to user callback, if any
                data->UserData = userData->chainCallbackUserData;
                return userData->chainCallback(data);
            }
            return 0;
        }

        bool operator()(FormatArgs fmt, std::span<char> buf, Params&& params = {}) const
        {
            if (params.size)
                params.flags |= ImGuiInputTextFlags_Multiline;
            return ImGui::InputTextEx(fmt.GetValue(), params.hint ? params.hint->GetValue() : nullptr, buf.data(), static_cast<int>(buf.size()), params.size ? *params.size : ImVec2(0, 0), params.flags);
        }

        bool operator()(FormatArgs fmt, std::span<char> buf, std::invocable<ImGuiInputTextCallbackData*> auto&& callback, Params&& params = {}) const
        {
            if (params.size)
                params.flags |= ImGuiInputTextFlags_Multiline;
            auto&& [cb, data] = Detail::ThunkCallback<ImGuiInputTextCallback, false>(std::move(callback));
            return ImGui::InputTextEx(fmt.GetValue(), params.hint ? params.hint->GetValue() : nullptr, buf.data(), static_cast<int>(buf.size()), params.size ? *params.size : ImVec2(0, 0), params.flags, cb, data.get());
        }

        bool operator()(FormatArgs fmt, std::string& str, Params&& params = {}) const
        {
            params.flags |= ImGuiInputTextFlags_CallbackResize;
            if (params.size)
                params.flags |= ImGuiInputTextFlags_Multiline;

            StringResizeCallbackData wrapData{ str, nullptr, nullptr };
            return ImGui::InputTextEx(fmt.GetValue(), params.hint ? params.hint->GetValue() : nullptr, str.data(), static_cast<int>(str.capacity() + 1), params.size ? *params.size : ImVec2(0, 0), params.flags, StringResizeCallback, &wrapData);
        }

        template<typename F>
        bool operator()(FormatArgs fmt, Validated<std::string, F>& str, Params&& params = {}) const
        {
            return str.update(operator()(fmt, str.ref(), std::move(params)));
        }

        bool operator()(FormatArgs fmt, std::string& str, std::invocable<ImGuiInputTextCallbackData*> auto&& callback, Params&& params = {}) const
        {
            params.flags |= ImGuiInputTextFlags_CallbackResize;
            if (params.size)
                params.flags |= ImGuiInputTextFlags_Multiline;
            auto&& [cb, data] = Detail::ThunkCallback<ImGuiInputTextCallback, false>(std::move(callback));
            StringResizeCallbackData wrapData{ str, cb, data.get() };
            return ImGui::InputTextEx(fmt.GetValue(), params.hint ? params.hint->GetValue() : nullptr, str.data(), static_cast<int>(str.capacity() + 1), params.size ? *params.size : ImVec2(0, 0), params.flags, StringResizeCallback, &wrapData);
        }

        template<typename F>
        bool operator()(FormatArgs fmt, Validated<std::string, F>& str, std::invocable<ImGuiInputTextCallbackData*> auto&& callback, Params&& params = {}) const
        {
            return str.update(operator()(fmt, str.ref(), std::move(callback), std::move(params)));
        }

    } TextBox;

    static constexpr struct InputT : public Detail::BaseItem<InputT>
    {
    private:
        template<typename T>
        using SpanConvertibleValueType = typename decltype(std::span{ std::declval<T>() })::value_type;

    public:
        struct Params {
        };

        template<typename T>
        struct TypedParams : Params
        {
            std::optional<T> step = std::nullopt;
            std::optional<T> stepFast = std::nullopt;
            FormatArgs format = nullptr;
            ImGuiInputTextFlags_ flags = ImGuiInputTextFlags_None;
        };

        template<Detail::Numeric T>
        bool operator()(FormatArgs fmt, T& value, TypedParams<T>&& params = {}) const
        {
            return ImGui::InputScalar(fmt.GetValue(), Detail::GetDataType<T>(), &value,
                params.step ? &*params.step : nullptr,
                params.stepFast ? &*params.stepFast : nullptr,
                params.format.GetValue(),
                params.flags);
        }

        template<Detail::IsValidatedNumber T>
        bool operator()(FormatArgs fmt, T& value, TypedParams<typename T::Type>&& params = {}) const
        {
            return value.update(operator()(fmt, value.ref(), std::move(params)));
        }

        template<typename T> requires Detail::Numeric<SpanConvertibleValueType<T>>
        bool operator()(FormatArgs fmt, T&& value, TypedParams<SpanConvertibleValueType<T>>&& params = {}) const
        {
            std::span s(value);
            return ImGui::InputScalarN(fmt.GetValue(), Detail::GetDataType<SpanConvertibleValueType<T>>(), s.data(), static_cast<int>(s.size()),
                params.step ? &*params.step : nullptr,
                params.stepFast ? &*params.stepFast : nullptr,
                params.format.GetValue(),
                params.flags);
        }

    } Input;

    static constexpr class TreeNodeT : protected Detail::InvokeBase
    {
    public:
        template<bool Open>
        class Opener
        {
            const TreeNodeT& parent;

            constexpr Opener(const TreeNodeT& p) : parent(p) {}

            friend class TreeNodeT;

        public:
            const TreeNodeT& operator[](ImGuiCond_ cond) const
            {
                ImGui::SetNextItemOpen(Open, cond);
                return parent;
            }

            const TreeNodeT& operator[](bool open) const
            {
                ImGui::SetNextItemOpen(open == Open, ImGuiCond_None);
                return parent;
            }

            const TreeNodeT& operator[](std::pair<bool, ImGuiCond_> state) const
            {
                ImGui::SetNextItemOpen(state.first == Open, state.second);
                return parent;
            }

            void operator()(auto&& ...args) const
            {
                ImGui::SetNextItemOpen(Open, ImGuiCond_None);
                parent(std::forward<decltype(args)>(args)...);
            }
        };

        void operator()(FormatArgs label, auto&& body) const
        {
            InvokeBlock<static_cast<bool(*)(const char*)>(ImGui::TreeNode), ImGui::TreePop, false>(label, std::forward<decltype(body)>(body));
        }

        void operator()(FormatArgs id, FormatArgs label, auto&& body) const
        {
            InvokeBlock<static_cast<bool(*)(const char*, const char*, ...)>(ImGui::TreeNode), ImGui::TreePop, false>(id, std::forward<decltype(body)>(body), label.GetValue());
        }

        const Opener<true> Opened{ *this };
        const Opener<false> Closed{ *this };

    } TreeNode;

    static constexpr class CollapsingHeaderT : public Detail::CallableBlock<CollapsingHeaderT>
    {
    public:
        struct Params {
            ImGuiTreeNodeFlags_ flags = ImGuiTreeNodeFlags_None;
        };

        using Base::operator();
        void operator()(FormatArgs label, Params&& params, auto&& body) const
        {
            InvokeBlock<static_cast<bool(*)(const char*, ImGuiTreeNodeFlags)>(ImGui::CollapsingHeader), [] {}, false>(label, std::forward<decltype(body)>(body), params.flags);
        }
    } CollapsingHeader;
}

template<typename T, typename F>
struct std::formatter<NGui::Validated<T, F>> : std::formatter<T>
{
    template<class FmtContext>
    auto format(NGui::Validated<T, F> v, FmtContext& ctx) const
    {
        return std::formatter<T>::format(v.get(), ctx);
    }
};