#include <imgui.h>
#include <nearimgui.h>

ImFont* fontBig = nullptr;

void Setup()
{
    ImFontConfig cfg{};
    ImGui::GetIO().Fonts->AddFontDefault(&cfg);
    cfg.SizePixels = 40.f;
    fontBig = ImGui::GetIO().Fonts->AddFontDefault(&cfg);
}

void Demo(size_t frameId)
{
    auto& io = ImGui::GetIO();
    static float f = 0.0f;
    static int i = 1;
    static int counter = 0;
    static bool open = true;
    static float f1 = 1.f, f2 = 2.f;
    static float windowWidth = 300.f;
    static short checkFlags = 1;
    static int checkFlags2 = 1;
    float ww = sin(frameId / 30.f) * 100.f + windowWidth;

    NGui::Window.SizeConstraints([&](ImGuiSizeCallbackData* data) {
        data->DesiredSize.x = ww;
        data->DesiredSize.y = 400.f;
        })("Hello, world!", {&open}, [&] {
        NGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

        NGui::Style(ImGuiCol_Text, 0xFF0000FF, ImGuiStyleVar_Alpha, 0.2f, ImGuiStyleVar_ItemSpacing, ImVec2(30.f, 30.f), fontBig, [&] {
            NGui::Slider("float", f, 0.0f, 1.0f, { .flags = ImGuiSliderFlags_AlwaysClamp });            // Edit 1 float using a slider from 0.0f to 1.0f
            NGui::Drag("int", i, { .min = 1, .max = 10 });
            NGui::Drag("float range", f1, f2);
            NGui::Drag.Width(80.f)("window width", windowWidth);
        });

        if (NGui::Button({ "Button {}", counter }))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        NGui::Text({ "counter = {}; cursorX = {}", counter, NGui::Window.Cursor.GetX()});

        NGui::Text({ "Application average {:.3f} ms/frame ({:.1f} FPS)", 1000.0f / io.Framerate, io.Framerate });

        NGui::Checkbox.Flags("Flags", checkFlags, static_cast<short>(3));
        NGui::Checkbox.Flags("Flags2", checkFlags2, 3);

        NGui::RadioButton("Flags 1", checkFlags2, 1);
        NGui::RadioButton("Flags 2", checkFlags2, 2);
    });
}