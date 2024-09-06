#include <imgui.h>
#include <nearimgui.h>
#include <array>

ImFont* fontBig = nullptr;
ImFont* fontSmall = nullptr;

void Setup()
{
    ImFontConfig cfg{};
    ImGui::GetIO().Fonts->AddFontDefault(&cfg);
    cfg.SizePixels = 40.f;
    fontBig = ImGui::GetIO().Fonts->AddFontDefault(&cfg);
    cfg.SizePixels = 10.f;
    fontSmall = ImGui::GetIO().Fonts->AddFontDefault(&cfg);
}

float f = 0.0f;
int i = 1;
int counter = 0;
bool open = true;
float f1 = 1.f, f2 = 2.f;
float windowWidth = 300.f;
short checkFlags = 1;
int checkFlags2 = 1;
int checkFlags3 = 0;
std::array<char, 512> buf;
std::string resizableBuf;
size_t resizableBufSizeCb = 0;
bool animate = true;
int vec[3] = { 1, 2, 3 };
NGui::Validated validated{ 10.f, [](float v) { return v > 1.f; } };
NGui::Validated<std::string> validatedString{ "Test", [](const std::string& v) { return v.starts_with("T"); } };
size_t comboIndex = 0;
std::vector<std::string> comboChoices{ { "A", "B", "C" } };

void Demo(size_t frameId)
{
    auto& io = ImGui::GetIO();
    float ww = animate ? (sin(frameId / 30.f) * 100.f + windowWidth) : 300.f;

    NGui::Window.SizeConstraints([&](ImGuiSizeCallbackData* data) {
        data->DesiredSize.x = ww;
        data->DesiredSize.y = 400.f;
        })("Hello, world!", {&open}, [&] {
        NGui::Checkbox("Animate window size", animate);
        NGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

        NGui::Style(NGui::Style::Alpha{ 0.2f }, NGui::Style::ItemSpacing::Y{ 30.f }, NGui::Style::FramePadding{ 2.f, 2.f }, NGui::Color::Text{0xFF0000FF}, fontBig, [&] {
            NGui::Slider("float", f, 0.0f, 1.0f, { .flags = ImGuiSliderFlags_AlwaysClamp });            // Edit 1 float using a slider from 0.0f to 1.0f
            NGui::Drag("int", i, { .min = 1, .max = 10 });
            NGui::Drag("float range", f1, f2);
            NGui::Drag[80.f]("window width", windowWidth);
        });

        static const auto tup = std::make_tuple(NGui::Style::Alpha{ 0.5f }, NGui::Style::ItemSpacing::Y{ 30.f }, NGui::Style::FramePadding{ 2.f, 2.f }, NGui::Color::Text{ 0xFFFF00FF }, fontSmall);
        NGui::Style(tup, [&] {
            NGui::Text("This is wrapped in a tuple");
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
        NGui::RadioButton({ "A", "B", "C" }, checkFlags3, true);

        NGui::TextBox("Simple box", buf);

        NGui::TextBox("Resizable box", resizableBuf);

        NGui::TextBox("Resizable callback box", resizableBuf, [&](ImGuiInputTextCallbackData*) { resizableBufSizeCb = resizableBuf.capacity(); return 0; });

        NGui::Text({ "Resizable capacity: {}", resizableBufSizeCb });

        NGui::Input("f1 input", f1);
        NGui::Input("f2 input", f2, { .step = 10.f });
        NGui::Input("i input", i, { .step = 10, .stepFast = 100 });
        NGui::Input("vec", vec, { .step = 10, .stepFast = 100 });

        NGui::TreeNode("Root", [] {
            NGui::TreeNode("Child A", [] {
                NGui::Text("Lorem");
                });
            NGui::TreeNode("Child B", [] {
                NGui::TreeNode("Child C", [] {
                    NGui::Text("Ipsum");
                    });
                });
            });


        NGui::TreeNode("treenodeid", "Tree Node ID", [] {
            NGui::Text("whatever");
            });

        NGui::TreeNode.Opened("Always open", [] {
            NGui::Text("Always visible");
            });

        NGui::TreeNode.Closed("Always closed", [] {
            NGui::Text("Always hidden");
            });

        NGui::TreeNode.Opened[ImGuiCond_FirstUseEver]("First open", [] {
            NGui::Text("Initially visible");
            });

        NGui::TreeNode.Opened[{ false, ImGuiCond_FirstUseEver }]("Not first open", [] {
            NGui::Text("Not initially visible");
            });

        NGui::CollapsingHeader("Collapsing", [] {
            NGui::Text("Collapsed");
            });

        NGui::Input("Validated float", validated);
        NGui::Drag("Validated float drag", validated);
        NGui::Slider("Validated float slide", validated, 0.f, 100.f);
        NGui::Slider.Vertical("Vertical slider", ImVec2(20, 100), validated, 0.f, 100.f);

        NGui::Text({ "Validated value = {}", validated });

        NGui::TextBox("Validated string", validatedString);

        NGui::Text({ "Validated string = {}", validatedString });

        NGui::ComboBox("Combo", comboIndex, comboChoices);

        float f[3] = { 1.f, 0.f, 1.f };
        NGui::ColorWidget("Color3", f);
        ImVec4 f2{ 1.f, 0.f, 1.f, 0.5f };
        NGui::ColorWidget.Picker("Color4", f2);

        NGui::ColorWidget.Button("Color button", ImVec4{ 0.5f, 0.f, 0.f, 1.f });
    });

    ImGui::ShowDemoWindow();
}