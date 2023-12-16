#pragma once
#include <string_view>

namespace NearImGui
{
    namespace Detail
    {
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
            void Invoke(std::string_view name, auto&& body, Args&& ...args) const
            {
                bool ret;
                if (ret = Begin(ConvertString(name), std::forward<decltype(args)>(args)...))
                    body();

                if (AlwaysCallEnd || ret)
                    End();
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
    }

    static constexpr class WindowT : public Detail::CallableBlock<WindowT>
    {
    public:
        struct Params
        {
            bool* open = nullptr;
            ImGuiWindowFlags_ flags = ImGuiWindowFlags_None;
        };

        using Base::operator();
        void operator()(std::string_view name, Params&& params, auto&& body) const
        {
            if (params.open && !*params.open)
                return;

            Invoke<ImGui::Begin, ImGui::End, true>(name, std::move(body), params.open, Detail::Enum(params.flags));
        }
    } Window;
}