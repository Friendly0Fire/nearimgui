#include <imgui.h>
#include <nearimgui.h>

void Demo()
{
    auto& io = ImGui::GetIO();
    static float f = 0.0f;
    static int counter = 0;
    static bool open = true;

    NGui::Window("Hello, world!", { &open }, [&] {
        NGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

        if (NGui::Button({ "Button {}", counter }))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        NGui::Text({ "counter = {}", counter });

        NGui::Text({ "Application average {:.3f} ms/frame ({:.1f} FPS)", 1000.0f / io.Framerate, io.Framerate });
    });
}