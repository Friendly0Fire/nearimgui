#include <imgui.h>
#include <nearimgui.h>

void Demo()
{
    auto& io = ImGui::GetIO();
    static float f = 0.0f;
    static int i = 1;
    static int counter = 0;
    static bool open = true;
    static float f1 = 1.f, f2 = 2.f;

    NGui::Window.With.Size({ 300, 300 }).Open("Hello, world!", { &open }, [&] {
        NGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

        NGui::Slider("float", f, 0.0f, 1.0f, { .flags = ImGuiSliderFlags_AlwaysClamp });            // Edit 1 float using a slider from 0.0f to 1.0f
        NGui::Drag("int", i, { .min = 1, .max = 10 });
        NGui::Drag("float range", f1, f2);

        if (NGui::Button({ "Button {}", counter }))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        NGui::Text({ "counter = {}", counter });

        NGui::Text({ "Application average {:.3f} ms/frame ({:.1f} FPS)", 1000.0f / io.Framerate, io.Framerate });
    });
}