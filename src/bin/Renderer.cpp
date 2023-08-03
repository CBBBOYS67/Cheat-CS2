#include "Renderer.h"

#include <d3d11.h>

#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <dxgi.h>
#include "imgui_internal.h"

#include "dMenu.h"

#include "Utils.h"
#include "menus/Settings.h"

#include "menus/Translator.h"
// stole this from MaxSu's detection meter

namespace stl
{
	using namespace SKSE::stl;

	template <class T>
	void write_thunk_call()
	{
		auto& trampoline = SKSE::GetTrampoline();
		const REL::Relocation<std::uintptr_t> hook{ T::id, T::offset };
		T::func = trampoline.write_call<5>(hook.address(), T::thunk);
	}
}


LRESULT Renderer::WndProcHook::thunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto& io = ImGui::GetIO();
	if (uMsg == WM_KILLFOCUS) {
		io.ClearInputCharacters();
		io.ClearInputKeys();
	}

	return func(hWnd, uMsg, wParam, lParam);
}


void SetupImGuiStyle()
{
	auto& style = ImGui::GetStyle();
	auto& colors = style.Colors;

	// Theme from https://github.com/ArranzCNL/ImprovedCameraSE-NG

	//style.WindowTitleAlign = ImVec2(0.5, 0.5);
	//style.FramePadding = ImVec2(4, 4);

	// Rounded slider grabber
	style.GrabRounding = 12.0f;

	// Window
	colors[ImGuiCol_WindowBg] = ImVec4{ 0.118f, 0.118f, 0.118f, 0.784f };
	colors[ImGuiCol_ResizeGrip] = ImVec4{ 0.2f, 0.2f, 0.2f, 0.5f };
	colors[ImGuiCol_ResizeGripHovered] = ImVec4{ 0.3f, 0.3f, 0.3f, 0.75f };
	colors[ImGuiCol_ResizeGripActive] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };

	// Header
	colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f };
	colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.3f, 0.3f, 1.0f };
	colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };

	// Title
	colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };
	colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };

	// Frame Background
	colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f };
	colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.3f, 0.3f, 1.0f };
	colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };

	// Button
	colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f };
	colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.3f, 0.3f, 1.0f };
	colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };

	// Tab
	colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };
	colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.38f, 0.38f, 1.0f };
	colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.28f, 0.28f, 1.0f };
	colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f };

}

void Renderer::D3DInitHook::thunk()
{
	func();

	INFO("D3DInit Hooked!");
	auto render_manager = RE::BSRenderManager::GetSingleton();
	if (!render_manager) {
		ERROR("Cannot find render manager. Initialization failed!");
		return;
	}

	auto render_data = render_manager->GetRuntimeData();

	INFO("Getting swapchain...");
	auto swapchain = render_data.swapChain;
	if (!swapchain) {
		ERROR("Cannot find swapchain. Initialization failed!");
		return;
	}

	INFO("Getting swapchain desc...");
	DXGI_SWAP_CHAIN_DESC sd{};
	if (swapchain->GetDesc(std::addressof(sd)) < 0) {
		ERROR("IDXGISwapChain::GetDesc failed.");
		return;
	}

	device = render_data.forwarder;
	context = render_data.context;

	INFO("Initializing ImGui...");
	ImGui::CreateContext();
	if (!ImGui_ImplWin32_Init(sd.OutputWindow)) {
		ERROR("ImGui initialization failed (Win32)");
		return;
	}
	if (!ImGui_ImplDX11_Init(device, context)) {
		ERROR("ImGui initialization failed (DX11)");
		return;
	}

	INFO("ImGui initialized!");

	initialized.store(true);

	WndProcHook::func = reinterpret_cast<WNDPROC>(
		SetWindowLongPtrA(
			sd.OutputWindow,
			GWLP_WNDPROC,
			reinterpret_cast<LONG_PTR>(WndProcHook::thunk)));
	if (!WndProcHook::func)
		ERROR("SetWindowLongPtrA failed!");

// initialize font selection here
	INFO("Building font atlas...");
	std::filesystem::path fontPath;
	bool foundCustomFont = false;
	const ImWchar* glyphRanges = 0;
#define FONTSETTING_PATH "Data\\SKSE\\Plugins\\dMenu\\fonts\\FontConfig.ini"
	CSimpleIniA ini;
	ini.LoadFile(FONTSETTING_PATH);
	if (!ini.IsEmpty()) {
		const char* language = ini.GetValue("config", "font", 0);
		if (language) {
			std::string fontDir = R"(Data\SKSE\Plugins\dMenu\fonts\)" + std::string(language);
			// check if folder exists
			if (std::filesystem::exists(fontDir) && std::filesystem::is_directory(fontDir)) {
				for (const auto& entry : std::filesystem::directory_iterator(fontDir)) {
					auto entryPath = entry.path();
					if (entryPath.extension() == ".ttf" || entryPath.extension() == ".ttc") {
						fontPath = entryPath;
						foundCustomFont = true;
						break;
					}
				}
			}
			if (foundCustomFont) {
				std::string languageStr = language;
				INFO("Loading font: {}", fontPath.string().c_str());
				if (languageStr == "Chinese") {
					INFO("Glyph range set to Chinese");
					glyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesChineseFull();
				} else if (languageStr == "Korean") {
					INFO("Glyph range set to Korean");
					glyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesKorean();
				} else if (languageStr == "Japanese") {
					INFO("Glyph range set to Japanese");
					glyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesJapanese();
				} else if (languageStr == "Thai") {
					INFO("Glyph range set to Thai");
					glyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesThai();
				} else if (languageStr == "Vietnamese") {
					INFO("Glyph range set to Vietnamese");
					glyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesVietnamese();
				} else if (languageStr == "Cyrillic") {
					glyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesCyrillic();
					INFO("Glyph range set to Cyrillic");
				}
			} else {
				INFO("No font found for language: {}", language);
			}
		}
	}
#define ENABLE_FREETYPE 0
#if ENABLE_FREETYPE
	ImFontAtlas* atlas = ImGui::GetIO().Fonts;
	atlas->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
	atlas->FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
#else
#endif
	if (foundCustomFont) {
		ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath.string().c_str(), 32.0f, NULL, glyphRanges);
	}
	SetupImGuiStyle();

}

void Renderer::DXGIPresentHook::thunk(std::uint32_t a_p1)
{
	func(a_p1);

	if (!D3DInitHook::initialized.load())
		return;

	// prologue
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// do stuff
	Renderer::draw();

	// epilogue
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

struct ImageSet
{
	std::int32_t my_image_width = 0;
	std::int32_t my_image_height = 0;
	ID3D11ShaderResourceView* my_texture = nullptr;
};


void Renderer::MessageCallback(SKSE::MessagingInterface::Message* msg)  //CallBack & LoadTextureFromFile should called after resource loaded.
{
	if (msg->type == SKSE::MessagingInterface::kDataLoaded && D3DInitHook::initialized) {
		// Read Texture only after game engine finished load all it renderer resource.
		auto& io = ImGui::GetIO();
		io.MouseDrawCursor = true;
		io.WantSetMousePos = true;
	}
}

bool Renderer::Install()
{
	auto g_message = SKSE::GetMessagingInterface();
	if (!g_message) {
		ERROR("Messaging Interface Not Found!");
		return false;
	}

	g_message->RegisterListener(MessageCallback);

	SKSE::AllocTrampoline(14 * 2);

	stl::write_thunk_call<D3DInitHook>();
	stl::write_thunk_call<DXGIPresentHook>();

	
	return true;
}

void Renderer::flip() 
{
	enable = !enable;
	ImGui::GetIO().MouseDrawCursor = enable;
}


float Renderer::GetResolutionScaleWidth()
{
	return ImGui::GetIO().DisplaySize.x / 1920.f;
}

float Renderer::GetResolutionScaleHeight()
{
	return ImGui::GetIO().DisplaySize.y / 1080.f;
}


void Renderer::draw()
{
	//static constexpr ImGuiWindowFlags windowFlag = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration;

	
	// resize window
	//ImGui::SetNextWindowPos(ImVec2(0, 0));
	//ImGui::SetNextWindowSize(ImVec2(screenSizeX, screenSizeY));


	// Add UI elements here
	//ImGui::Text("sizeX: %f, sizeYL %f", screenSizeX, screenSizeY);



	if (enable) {
		if (!DMenu::initialized) {
			ImVec2 screenSize = ImGui::GetMainViewport()->Size;
			float screenSizeX = screenSize.x;
			float screenSizeY = screenSize.y;
			DMenu::init(screenSizeX, screenSizeY);
		}

		DMenu::draw();
	}

}
