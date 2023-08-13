#include <3ds.h>
#include <citro3d.h>
#include <tex3ds.h>

#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <vector>

// Enable Npi Assert to write Errors to file
#define IMGUI_IMPL_CITRO3D_USE_NPI_ASSERT
#define IMGUI_IMPL_CIR_USE_NPI_ASSERT

#include "imgui.h"
#include "imgui_impl_citro3d.h"
#include "imgui_impl_ctr.h"

const auto CLEAR_COLOR = 0x1a2529FF;
const auto SCREEN_WIDTH = 400.0f;
const auto SCREEN_HEIGHT = 480.0f;
const auto TRANSFER_SCALING = GX_TRANSFER_SCALE_NO;
const auto FB_SCALE = 1.0f;
const auto FB_WIDTH = SCREEN_WIDTH * FB_SCALE;
const auto FB_HEIGHT = SCREEN_HEIGHT * FB_SCALE;

const auto DISPLAY_TRANSFER_FLAGS =
    GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) |
    GX_TRANSFER_RAW_COPY(0) | GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
    GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
    GX_TRANSFER_SCALING(TRANSFER_SCALING);

C3D_RenderTarget* Top;
C3D_RenderTarget* Bottom;

#define rev_void(x) reinterpret_cast<void*>(x)

struct NpiEasyTex {
  C3D_Tex* tex = NULL;
  Tex3DS_Texture t3x;
};

bool loadet_s = false;

void NpiEasyTexLoad(NpiEasyTex& texture, const std::string& path) {
  if (texture.tex) C3D_TexDelete(texture.tex);
  texture.tex = new C3D_Tex;
  FILE* fp = fopen(path.c_str(), "rb");
  if (!fp) {
    fclose(fp);
    delete texture.tex;
    return;
  }
  texture.t3x = Tex3DS_TextureImportStdio(fp, texture.tex, nullptr, true);
  C3D_TexSetFilter(texture.tex, GPU_LINEAR, GPU_LINEAR);
  fclose(fp);
  loadet_s = true;
}

void NpiImGuiImage(NpiEasyTex texture, size_t index,
                   const ImVec4& tint_col = ImVec4(1, 1, 1, 1),
                   const ImVec4& border_col = ImVec4(0, 0, 0, 0)) {
  const auto sub = Tex3DS_GetSubTexture(texture.t3x, index);
  ImGui::Image(texture.tex, ImVec2(sub->width, sub->height),
               ImVec2(sub->left, sub->top), ImVec2(sub->right, sub->bottom),
               tint_col, border_col);
}

void NpiImGuiImageButton(NpiEasyTex texture, size_t index,
                         int frame_padding = -1,
                         const ImVec4& bg_col = ImVec4(0, 0, 0, 0),
                         const ImVec4& tint_col = ImVec4(1, 1, 1, 1)) {
  const auto sub = Tex3DS_GetSubTexture(texture.t3x, index);
  ImGui::ImageButton(
      texture.tex, ImVec2(sub->width, sub->height), ImVec2(sub->left, sub->top),
      ImVec2(sub->right, sub->bottom), frame_padding, bg_col, tint_col);
}

// clang-format off
std::vector<std::string> styles = {
  "ImGui Light",
  "ImGui Dark",
  "ImGui Classic",
};
// clang-format on

std::string cstyle = styles[1];

void LoadStyle() {
  if (cstyle == styles[0])
    ImGui::StyleColorsLight();
  else if (cstyle == styles[1])
    ImGui::StyleColorsDark();
  else if (cstyle == styles[2])
    ImGui::StyleColorsClassic();
  else
    ImGui::StyleColorsDark();
}

int main() {
  gfxInitDefault();
  romfsInit();
  C3D_Init(2 * C3D_DEFAULT_CMDBUF_SIZE);

  // create top screen render target
  Top = C3D_RenderTargetCreate(FB_HEIGHT * 0.5f, FB_WIDTH, GPU_RB_RGBA8,
                               GPU_RB_DEPTH24_STENCIL8);
  C3D_RenderTargetSetOutput(Top, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

  // create bottom screen render target
  Bottom = C3D_RenderTargetCreate(FB_HEIGHT * 0.5f, FB_WIDTH * 0.8f,
                                  GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
  C3D_RenderTargetSetOutput(Bottom, GFX_BOTTOM, GFX_LEFT,
                            DISPLAY_TRANSFER_FLAGS);

  ImGui::CreateContext();
  LoadStyle();
  auto& io = ImGui::GetIO();
  auto& style = ImGui::GetStyle();
  style.ScaleAllSizes(0.5f);
  io.IniFilename = nullptr;

  ImGui_ImplCtr_Init();
  ImGui_ImplCitro3D_Init();

  NpiEasyTex ntex;
  NpiEasyTexLoad(ntex, "romfs:/gfx/test.t3x");

  bool show_demo_window = false;

  while (aptMainLoop()) {
    hidScanInput();
    if (hidKeysDown() & KEY_START) exit(0);
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C3D_FrameDrawOn(Top);
    C3D_RenderTargetClear(Top, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
    C3D_FrameDrawOn(Bottom);
    C3D_RenderTargetClear(Bottom, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
    // setup display metrics
    io.DisplaySize = ImVec2(SCREEN_WIDTH, SCREEN_HEIGHT);
    io.DisplayFramebufferScale = ImVec2(FB_SCALE, FB_SCALE);
    ImGui_ImplCitro3D_NewFrame();
    ImGui_ImplCtr_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Test");
    ImGui::Text(
        "Hold Y and use CIRCLEPAD to Move\nThe Window f.e. to Bottom Screen!");
    NpiImGuiImage(ntex, 0);
    ImGui::SameLine();
    NpiImGuiImage(ntex, 1);
    ImGui::SameLine();
    NpiImGuiImage(ntex, 2);
    if (ImGui::BeginCombo("##StyleSelect", cstyle.c_str())) {
      for (size_t n = 0; n < styles.size(); n++) {
        bool is_selected = (cstyle.c_str() == styles[n]);
        if (ImGui::Selectable(styles[n].c_str(), is_selected)) {
          cstyle = styles[n];
          LoadStyle();
        }
        if (is_selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    ImGui::Checkbox("Show Demo Window", &show_demo_window);
    ImGui::End();

    if (show_demo_window)  // For some reason the adress is not enough
    {
      ImGui::ShowDemoWindow(&show_demo_window);
    }
    ImGui::Render();

    ImGui_ImplCitro3D_RenderDrawData(ImGui::GetDrawData(), rev_void(Top),
                                     rev_void(Bottom));
    C3D_FrameEnd(0);
  }
  ImGui_ImplCitro3D_Shutdown();
  ImGui_ImplCtr_Shutdown();
  C3D_Fini();
  return 0;
}