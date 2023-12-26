#pragma once
#include <imgui.h>
#include <string_view>
#include <format>
#include <optional>
#include <variant>

namespace NGui
{
    namespace Detail
    {
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
        template<typename C, typename F>
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
        template<typename C, typename F>
        requires (!std::convertible_to<F, C>) && IsVoidPointerStyleCallback<C>
        std::pair<C, void*> ThunkCallback(F&& callback)
        {
            auto* cb = static_cast<Callback<F>*>(CacheCallback(std::make_unique<Callback<F>>(std::move(callback))));
            return [=]<typename R, typename... Args>(R(*)(Args..., void*)) {
                return std::make_pair([](Args&& ...args, void* user_data) {
                    return static_cast<Callback<F>*>(user_data)->callback(std::forward<Args>(args)...);
                    }, cb);
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
        template<typename C, typename F>
            requires (!std::convertible_to<F, C>) && IsDataPointerStyleCallback<C>
        std::pair<C, void*> ThunkCallback(F&& callback)
        {
            using Data = std::tuple_element_t<0, typename CallbackTraits<C>::Arguments>;
            auto* cb = static_cast<Callback<F>*>(CacheCallback(std::make_unique<Callback<F>>(std::move(callback))));
            return std::make_pair([](Data data) {
                return static_cast<Callback<F>*>(data->UserData)->callback(data);
                }, cb);
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
            
            return ImGuiDataType_COUNT;
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
            [[nodiscard]] const auto& Position(const ImVec2& pos, ImGuiCond_ cond = ImGuiCond_Always, const ImVec2& pivot = { 0.f, 0.f }) const
            {
                ImGui::SetNextWindowPos(pos, cond, pivot);
                return *this;
            }

            [[nodiscard]] const auto& Size(const ImVec2& size, ImGuiCond_ cond = ImGuiCond_Always) const
            {
                ImGui::SetNextWindowSize(size, cond);
                return *this;
            }

            [[nodiscard]] const auto& SizeConstraints(const ImVec2& sizeMin, const ImVec2& sizeMax) const
            {
                ImGui::SetNextWindowSizeConstraints(sizeMin, sizeMax);
                return *this;
            }

            [[nodiscard]] const auto& SizeConstraints(std::invocable<ImGuiSizeCallbackData*> auto&& callback) const
            {
                auto&& [cb, data] = Detail::ThunkCallback<ImGuiSizeCallback>(std::move(callback));
                ImGui::SetNextWindowSizeConstraints({}, {}, cb, data);

                return *this;
            }

            void Open(FormatArgs name, Params&& params, auto&& body) const;
        } With;

    } Window;

    void WindowT::Builder::Open(FormatArgs name, Params&& params, auto&& body) const
    {
        Window(name, std::move(params), std::forward<decltype(body)>(body));
    }

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
        void operator()(FormatArgsWithEnd fmt) const
        {
            ImGui::TextUnformatted(fmt.GetValue(), fmt.GetValueEnd());
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
            return ImGui::SliderScalar(fmt.GetValue(), Detail::GetDataType<T>(), &value, &min, &max, params.format.GetValue(), Detail::Enum(params.flags));
        }

        bool Angle(FormatArgs fmt, float& value)
        {
            return ImGui::SliderAngle(fmt.GetValue(), &value);
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

    static constexpr struct DragT
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

        template<typename T>
        bool operator()(FormatArgs fmt, T& value) const
        {
            return ImGui::DragScalar(fmt.GetValue(), Detail::GetDataType<T>(), &value);
        }

        template<typename T>
        bool operator()(FormatArgs fmt, T& value, Params<T>&& params) const
        {
            return ImGui::DragScalar(fmt.GetValue(), Detail::GetDataType<T>(), &value, params.speed,
                params.min ? &*params.min : nullptr,
                params.max ? &*params.max : nullptr,
                params.format.GetValue(), params.flags);
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

    static constexpr class StyleT : protected Detail::InvokeBase
    {
    public:
        template<typename... Args> requires (sizeof...(Args) > 1)
        void operator()(Args&& ...args) const
        {
            Item(0, 0, std::forward<Args>(args)...);
        }

    private:
        template<typename I, typename V, typename... Args> requires (sizeof...(Args) > 0
        && ((std::same_as<std::decay_t<I>, ImGuiCol_> && (std::same_as<std::decay_t<V>, ImU32> || std::same_as<std::decay_t<V>, ImVec4>))
        || (std::same_as<std::decay_t<I>, ImGuiStyleVar_> && (std::same_as<std::decay_t<V>, float> || std::same_as<std::decay_t<V>, ImVec2>))))
        void Item(int c, int s, I idx, const V& val, Args&& ...args) const
        {
            constexpr bool isColor = std::same_as<std::decay_t<I>, ImGuiCol_>;
            constexpr bool isVar = std::same_as<std::decay_t<I>, ImGuiStyleVar_>;
            if constexpr (isColor)
            {
                ImGui::PushStyleColor(idx, val);
                ++c;
            }
            else if constexpr (isVar)
            {
                ImGui::PushStyleVar(idx, val);
                ++s;
            }

            Item(c, s, std::forward<Args>(args)...);
        }

        template<typename... Args> requires (sizeof...(Args) > 0)
        void Item(int c, int s, ImFont* font, Args&& ...args) const
        {
            ImGui::PushFont(font);

            Item(c, s, std::forward<Args>(args)...);

            ImGui::PopFont();
        }

        void Item(int c, int s, auto&& body) const
        {
            body();

            if(c > 0) ImGui::PopStyleColor(c);
            if(s > 0) ImGui::PopStyleVar(s);
        }

    } Style;
}