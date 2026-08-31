#include "gui/menu_window.h"

#include <cstdint>
#include <format>
#include <string>
#include "shellapi.h"

#include "imgui.h"

#include "build_info.h"
#include "core/config/config.h"
#include "core/free_camera.h"
#include "core/features/speedhack.h"
#include "core/game_data_manager.h"
#include "gui/i18n.h"
#include "gui/notification_popup.h"
#include "gui/helpers.h"
#include "hook/hook_manager.h"

namespace Layout {
    constexpr float ITEM_WIDTH = -150.0f;
    constexpr float BUTTON_WIDTH = 150.0f;
    constexpr float SMALL_BUTTON_WIDTH = 110.0f;

    inline void RightAlignNext(float buttonWidth) {
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - buttonWidth);
    }
}

namespace {
    // 分节说明：绿色小字（zh-CN.json 中存在该 ".desc" 键时才显示）
    void SectionDesc(const char* key) {
        if (!I18N::IsChineseReady() || !I18N::Has(key)) return;
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.42f, 0.78f, 0.45f, 1.0f));
        ImGui::SetWindowFontScale(0.85f);
        ImGui::TextWrapped("%s", I18N::TR(key));
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();
    }
}

namespace ImGui {
    static inline void BeginScrollableArea(const char* str_id) {
        ImGui::SetCursorPosX(0);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ImGui::GetStyle().WindowPadding.x, 0));
        ImGui::BeginChild(str_id, ImVec2(ImGui::GetWindowWidth(), 0), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoBackground);
        ImGui::PopStyleVar();
    }

    static inline void EndScrollableArea() {
        ImGui::EndChild();
    }
}

void MenuWindow::Render() {
    if (!is_visible) return;

    ImGui::SetNextWindowSize(ImVec2(420, 360), ImGuiCond_FirstUseEver);
    // 标题动态跟随上游版本号；###freecam 固定窗口 ID，避免版本变化导致布局丢失
    // NoDocking：禁止吸附到停靠区，保证窗口可自由拖动移动
    ImGui::Begin((std::string(I18N::TR("Freecam")) + " " + MOD_VERSION + "###freecam").c_str(), &is_visible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDocking);

    if (ImGui::BeginTabBar("##tabs")) {
        infoTab.Render();
        featuresTab.Render();
        sequencerTab.Render();
        configTab.Render();
        keyBindsTab.Render();
        logTab.Render();

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void MenuWindow::InfoTab::Render() {
    if (ImGui::BeginTabItem(I18N::TR("Info"))) {
        ImGui::BeginScrollableArea("##info_content");

        const bool isFreecamEnabled = freeCamera.IsEnabled();
        GameData::Camera* activeCamera = freeCamera.GetCameraState();

        if (!activeCamera) {
            ImGui::Spacing();
            ImGui::TextDisabled("%s", I18N::TR("Info.noCamera"));

            ImGui::Spacing();
            ImGui::SeparatorText(I18N::TR("About"));

            ImGui::BulletText(I18N::TR("Freecam %s, commit: %s"), MOD_VERSION, MOD_GIT_HASH);
            ImGui::BulletText(I18N::TR("Build date: %s"), __DATE__);

            ImGui::BulletText("%s", I18N::TR("Wiki & docs: "));
            ImGui::SameLine(0, 0);
            ImGui::TextLinkOpenURL("https://github.com/Logersnamed/FreecamMod/wiki ");

            ImGui::BulletText("%s", I18N::TR("Report a bug/feature: "));
            ImGui::SameLine(0, 0);
            ImGui::TextLinkOpenURL("https://github.com/Logersnamed/FreecamMod/issues");

            ImGui::Spacing();
            ImGui::SeparatorText(I18N::TR("Quick Start"));

            ImGui::BulletText(I18N::TR("Toggle UI: [%s]"), cfg.GetKeybindString(ActionType::ToggleMenu).c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            ImHelpers::Tooltip(I18N::TR("Rebindable in config.ini"));

            ImGui::BulletText(I18N::TR("Toggle freecam: [%s]"), cfg.GetKeybindString(ActionType::Toggle).c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            ImHelpers::Tooltip(I18N::TR("Rebindable in config.ini"));

            ImGui::BulletText(I18N::TR("Move: [%s %s %s %s]"), cfg.GetKeybindString(ActionType::MoveForward).c_str(), cfg.GetKeybindString(ActionType::MoveLeft).c_str(), cfg.GetKeybindString(ActionType::MoveBackward).c_str(), cfg.GetKeybindString(ActionType::MoveRight).c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            ImHelpers::Tooltip(I18N::TR("Rebindable in config.ini"));

            ImGui::BulletText(I18N::TR("Move up / down: [%s] / [%s]"), cfg.GetKeybindString(ActionType::MoveUp).c_str(), cfg.GetKeybindString(ActionType::MoveDown).c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            ImHelpers::Tooltip(I18N::TR("Rebindable in config.ini"));

            ImGui::BulletText(I18N::TR("Tilt: [%s] / [%s]"), cfg.GetKeybindString(ActionType::TiltLeft).c_str(), cfg.GetKeybindString(ActionType::TiltRight).c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            ImHelpers::Tooltip(I18N::TR("Rebindable in config.ini"));

            ImGui::TextDisabled("%s", I18N::TR("You can disable show_menu_on_startup via config/gui"));

            ImGui::Spacing();
            ImGui::SeparatorText(I18N::TR("Tabs"));

            ImGui::BulletText("%s", I18N::TR("Features"));
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::TextWrapped("%s", I18N::TR("- controls freecam features: speedhack, change weather/daytime, etc."));
            ImGui::EndDisabled();

            ImGui::BulletText("%s", I18N::TR("Timeline"));
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::TextWrapped("%s", I18N::TR("- set keyframes for the camera and play back smooth camera paths."));
            ImGui::EndDisabled();

            ImGui::BulletText("%s", I18N::TR("Config"));
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::TextWrapped("%s", I18N::TR("- adjust mod settings. Changes are saved across restarts."));
            ImGui::EndDisabled();

            ImGui::Spacing();
            if (ImGui::Button(I18N::TR("Open Config Folder"), ImVec2(Layout::BUTTON_WIDTH, 0.0f))) {
                ::ShellExecuteW(NULL, L"open", cfg.GetConfigDirPath().c_str(), NULL, NULL, SW_SHOWNORMAL);
            }

            ImGui::EndScrollableArea();
            ImGui::EndTabItem();
            return;
        }

        {
            float centerOffset = (ImGui::GetFrameHeight() - ImGui::GetFontSize()) * 0.5f;
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + centerOffset);

            ImGui::PushStyleColor(ImGuiCol_Text, isFreecamEnabled
                ? ImVec4(0.4f, 0.9f, 0.4f, 1.0f)
                : ImVec4(0.9f, 0.4f, 0.4f, 1.0f));
            ImGui::Text("%s", isFreecamEnabled ? I18N::TR("Freecam active") : I18N::TR("Freecam inactive"));
            ImGui::PopStyleColor();

            Layout::RightAlignNext(Layout::BUTTON_WIDTH);
            if (ImGui::Button(I18N::TR("Toggle Freecam"), ImVec2(Layout::BUTTON_WIDTH, 0.0f)))
                freeCamera.Toggle();
        }

        ImGui::SeparatorText(I18N::TR("Camera"));
        ImGui::PushItemWidth(Layout::ITEM_WIDTH);
        ImGui::BeginDisabled(!isFreecamEnabled);

        auto pos = activeCamera->matrix.c3.xyz();
        if (ImGui::DragFloat3(I18N::TR("Position"), &pos.x, 0.1f, 0.0f, 0.0f, "%.2f")) {
            activeCamera->matrix.c3.x = pos.x;
            activeCamera->matrix.c3.y = pos.y;
            activeCamera->matrix.c3.z = pos.z;
        }
        ImGui::SliderFloat(I18N::TR("FOV"), &activeCamera->fov, freeCamera.GetMinFov(), freeCamera.GetMaxFov(), "%.2f rad");
        ImGui::DragFloat(I18N::TR("Render Distance"), &activeCamera->renderDistance, 10.0f, 0.0f, 0.0f, "%.0f m");

        float speed = freeCamera.GetSpeed();
        if (ImGui::DragFloat(I18N::TR("Speed"), &speed, 0.1f, 0.0f, 0.0f, "%.2f")) {
            freeCamera.SetSpeed(speed);
        }
        if (ImGui::IsItemHovered()) {
            ImHelpers::TooltipWithShortcut(I18N::TR("Adjust camera fly speed."), std::format("{} + Scroll", cfg.GetKeybindString(ActionType::ScrollCameraSpeedModifier)).c_str());
        }
        ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::SeparatorText(I18N::TR("Rotation (read-only)"));
        constexpr float radToDeg = 57.29577951308232f;
        const auto rot = activeCamera->GetEuler();
        float pitch = rot.pitch * radToDeg;
        float yaw = rot.yaw * radToDeg;
        float roll = rot.roll * radToDeg;

        ImGui::BeginDisabled();
        ImGui::InputFloat(I18N::TR("Pitch"), &pitch, 0.0f, 0.0f, "%.2f°");
        ImGui::InputFloat(I18N::TR("Yaw"), &yaw, 0.0f, 0.0f, "%.2f°");
        ImGui::InputFloat(I18N::TR("Roll"), &roll, 0.0f, 0.0f, "%.2f°");
        ImGui::EndDisabled();

        ImGui::PopItemWidth();
        ImGui::EndScrollableArea();
        ImGui::EndTabItem();
    }
}

MenuWindow::FeaturesTab::FeaturesTab(HookManager& hookManager, FreeCamera& freeCamera, Speedhack& speedhack, Config& cfg)
    : hookManager(hookManager), freeCamera(freeCamera), speedhack(speedhack), cfg(cfg),
    frameStepper(freeCamera.GetFrameStepper()), cameraStateMgr(freeCamera.GetCameraStateManager()),
    pathRecorder(freeCamera.GetPathRecorder()) {
}

void MenuWindow::FeaturesTab::RenderSpeedhack() {
    if (ImGui::CollapsingHeader(I18N::TR("Speedhack"))) {
        ImGui::PushItemWidth(Layout::ITEM_WIDTH);
        bool isFreecamOnly = speedhack.IsFreecamOnly();
        bool isAvailable = !isFreecamOnly || (isFreecamOnly && freeCamera.IsEnabled());
        ImGui::BeginDisabled(!isAvailable);
        if (ImGui::Button(speedhack.IsEnabled() ? I18N::TR("Disable") : I18N::TR("Enable"), ImVec2(Layout::SMALL_BUTTON_WIDTH, 0.0f))) {
            speedhack.IsEnabled() ? speedhack.Disable() : speedhack.Enable();
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            if (!isAvailable) {
                ImGui::SetTooltip("%s",
                    I18N::TR("Disabled by config: features_work_only_in_freecam.speedhack\nEnable this option or turn on freecam mode to unlock."));
            }
            else {
				ImHelpers::TooltipWithShortcut(I18N::TR("Enable/disable speedhack."), cfg.GetKeybindString(ActionType::ToggleSpeedhack).c_str());
            }
        }
        ImGui::EndDisabled();

        float timeScale = speedhack.GetSpeedhackSpeed();
        if (ImGui::DragFloat(I18N::TR("Speedhack speed"), &timeScale, 0.001)) {
            speedhack.SetSpeed(timeScale);
        }
        if (ImGui::IsItemHovered()) {
            ImHelpers::TooltipWithShortcut(I18N::TR("Adjust speedhack speed."), std::format("{} + Scroll", cfg.GetKeybindString(ActionType::ScrollSpeedhackModifier)).c_str());
        }

        float gameSpeed = speedhack.GetGameSpeed();
        ImGui::BeginDisabled();
        ImGui::DragFloat(I18N::TR("Current game speed"), &gameSpeed);
        ImGui::EndDisabled();

        ImGui::PopItemWidth();
        SectionDesc("Speedhack.desc");
        ImGui::Spacing();
    }
}

void MenuWindow::FeaturesTab::RenderFrameStepper() {
    if (ImGui::CollapsingHeader(I18N::TR("Frame stepper"))) {
        ImGui::PushItemWidth(Layout::ITEM_WIDTH);
        int framesToStep = frameStepper.GetFramesToStep();
        int step = frameStepper.GetStep();

        ImGui::BeginDisabled();
        ImGui::DragInt(I18N::TR("Frames to step left"), &framesToStep);
        ImGui::EndDisabled();

        const int MIN_FRAMES_TO_STOP_STEPPING = 15;
        ImGui::BeginDisabled(!freeCamera.IsEnabled());
        bool canStopStepping = framesToStep && step >= MIN_FRAMES_TO_STOP_STEPPING;
        if (ImGui::Button(canStopStepping ? I18N::TR("Stop stepping") : I18N::TR("Step frames"), ImVec2(Layout::SMALL_BUTTON_WIDTH, 0.0f))) {
            canStopStepping ? frameStepper.Reset() : frameStepper.StepFrames();
        }
		ImHelpers::TooltipWithShortcut(I18N::TR("Step frames forward."), cfg.GetKeybindString(ActionType::StepFrames).c_str());
        ImGui::EndDisabled();

        if (ImGui::InputInt(I18N::TR("Step"), &step)) {
            frameStepper.SetStepFromUI(step);
            IConVar::anyChangeByUi = true;
        }
        ImGui::PopItemWidth();
        SectionDesc("Frame stepper.desc");
        ImGui::Spacing();
    }
}

void MenuWindow::FeaturesTab::RenderCycleWeatherTime() {
    if (ImGui::CollapsingHeader(I18N::TR("Cycle Weather Time"))) {
        ImGui::PushItemWidth(Layout::ITEM_WIDTH);
        auto& cave = hookManager.GetDaytimeUpdateCave();
        bool isFreecamOnly = cave.IsFreecamOnly();
        bool isAvailable = !isFreecamOnly || (isFreecamOnly && freeCamera.IsEnabled());
        ImGui::BeginDisabled(!isAvailable);
        if (ImGui::Button(cave.IsCycleWeatherTime() ? I18N::TR("Stop Cycling") : I18N::TR("Cycle"), ImVec2(Layout::SMALL_BUTTON_WIDTH, 0.0f))) {
            cave.ToggleCycleWeatherTime();
            NotificationPopUp::Notify(I18N::TR("notif.Info"), cave.IsCycleWeatherTime() ? I18N::TR("Started cycling") : I18N::TR("Stoped cycling"));
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            if (!isAvailable) {
                ImGui::SetTooltip("%s",
                    I18N::TR("Disabled by config: features_work_only_in_freecam.cycle_weather_time\nEnable this option or turn on freecam mode to unlock."));
            }
            else {
				ImHelpers::TooltipWithShortcut(I18N::TR("Start/stop cycling weather and time."), cfg.GetKeybindString(ActionType::CycleWeatherTime).c_str());
            }
        }
        ImGui::EndDisabled();

        auto* cycleSpeed = cave.GetCycleSpeedPtr();
        if (cycleSpeed) {
            ImGui::DragInt(I18N::TR("Cycle speed"), cycleSpeed, 1000);
        }
        ImGui::PopItemWidth();
        SectionDesc("Cycle Weather Time.desc");
        ImGui::Spacing();
    }

}

void MenuWindow::FeaturesTab::RenderCameraStateManager() {
    if (ImGui::CollapsingHeader(I18N::TR("Camera State Manager"))) {
        ImGui::PushItemWidth(Layout::ITEM_WIDTH);
        ImGui::BeginDisabled(!freeCamera.IsEnabled());
        {
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                if (!freeCamera.IsEnabled()) {
                    ImGui::SetTooltip("%s", I18N::TR("Camera State Manager works only when free camera enabled."));
                }
            }

            float interpTime = cameraStateMgr.GetInterpolationTime();
            if (ImGui::DragFloat(I18N::TR("Interpolation Time"), &interpTime, 0.01, 0.0f)) {
                cameraStateMgr.SetInterpolationTimeFromUI(interpTime);
            }

            ImGui::Text("%s", I18N::TR("States order: "));
            Input::ReleasedNumkeys order = cameraStateMgr.GetSlotOrder();
            for (auto k : order) {
                ImGui::SameLine();
                ImGui::Text("%d", k);
            }

            float time = cameraStateMgr.GetTime();
            ImGui::BeginDisabled();
            ImGui::DragFloat(I18N::TR("Time"), &time);
            ImGui::EndDisabled();

            int interval = cameraStateMgr.GetInterval();
            ImGui::BeginDisabled();
            ImGui::DragInt(I18N::TR("Interval"), &interval);
            ImGui::EndDisabled();

            ImGui::PopItemWidth();
            SectionDesc("Camera State Manager.desc");
            ImGui::Spacing();
        }
        ImGui::EndDisabled();
    }
}

void MenuWindow::FeaturesTab::RenderPathRecorder() {
    if (ImGui::CollapsingHeader(I18N::TR("Path recorder"))) {
        ImGui::PushItemWidth(Layout::ITEM_WIDTH);
        ImGui::BeginDisabled(!freeCamera.IsEnabled());
        {
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                if (!freeCamera.IsEnabled()) {
                    ImGui::SetTooltip("%s", I18N::TR("Path Recorder works only when free camera enabled."));
                }
            }

            int framesRecorded = pathRecorder.GetFramesRecorded();
            ImGui::BeginDisabled();
            ImGui::DragInt(I18N::TR("Frames Recorded"), &framesRecorded);
            ImGui::EndDisabled();

            //ImGui::SameLine();

            if (ImGui::Button(pathRecorder.IsRecording() ? I18N::TR("Stop Recording") : I18N::TR("Start Recording"), ImVec2(Layout::BUTTON_WIDTH, 0.0f))) {
                pathRecorder.IsRecording() ? pathRecorder.EndRecord() : pathRecorder.StartRecord();
            }
			ImHelpers::TooltipWithShortcut(I18N::TR("Start/stop recording camera path."), cfg.GetKeybindString(ActionType::StartEndRecording).c_str());

            int framesPlayed = pathRecorder.GetFramesPlayed();
            ImGui::BeginDisabled();
            ImGui::DragInt(I18N::TR("Frames Played"), &framesPlayed);
            ImGui::EndDisabled();

            //ImGui::SameLine();

            ImGui::BeginDisabled(framesRecorded < 1);
            if (ImGui::Button(pathRecorder.IsPlaying() ? I18N::TR("Stop Playing") : I18N::TR("Start Playing"), ImVec2(Layout::BUTTON_WIDTH, 0.0f))) {
                pathRecorder.IsPlaying() ? pathRecorder.EndPlay() : pathRecorder.StartPlay();
            }
			ImHelpers::TooltipWithShortcut(I18N::TR("Start/stop playing recorded camera path."), cfg.GetKeybindString(ActionType::StartEndPlayingRecording).c_str());
            ImGui::EndDisabled();

            ImGui::PopItemWidth();
            SectionDesc("Path recorder.desc");
            ImGui::Spacing();
        }
        ImGui::EndDisabled();
    }
}

void MenuWindow::FeaturesTab::Render() {
    if (ImGui::BeginTabItem(I18N::TR("Features"))) {
        ImGui::BeginScrollableArea("##features_content");

        SectionDesc("FeaturesTab.desc");

        RenderSpeedhack();
        RenderCycleWeatherTime();
        RenderFrameStepper();
        RenderCameraStateManager();
        RenderPathRecorder();

        ImGui::EndScrollableArea();
        ImGui::EndTabItem();
    }
}

template<typename T>
void MenuWindow::SequencerTab::DrawCombo(const char* label, Track<T>& track) {
    const char* interpolationTypeNames[] = { I18N::TR("Linear"), I18N::TR("Catmull-Rom") };
    const int count = IM_COUNTOF(interpolationTypeNames);
    int current = (int)track.GetInterpolationType();
    ImGui::Combo(label, &current, interpolationTypeNames, count);
    track.SetInterpolationType((InterpolationType)current);
}

void MenuWindow::SequencerTab::Render() {
    if (ImGui::BeginTabItem(I18N::TR("Sequencer"))) {
        ImGui::BeginScrollableArea("##sequencer_content");

        SectionDesc("Sequencer.desc");

        bool isVisible = timelineWindow.IsVisible();
        if (ImGui::Checkbox(I18N::TR("Show timeline"), &isVisible)) {
            timelineWindow.SetVisibility(isVisible);
        }
        ImHelpers::Tooltip(I18N::TR("Show timeline.desc"));

        float max_time = timeline.GetMaxTime();
        if (ImGui::DragFloat(I18N::TR("Timeline lenght"), &max_time, 1, 16.0f, 3600.0f, "%.f sec")) {
            timeline.SetMaxTime(max_time);
        }

        ImGui::Spacing();
        ImGui::SeparatorText(I18N::TR("Interpolation type"));
        SectionDesc("Interpolation type.desc");

        DrawCombo(I18N::TR("FOV##interp"), timeline.GetFovTrack());
        ImHelpers::Tooltip(I18N::TR("Interpolation type.desc"));
        DrawCombo(I18N::TR("Position##interp"), timeline.GetPosTrack());
        ImHelpers::Tooltip(I18N::TR("Interpolation type.desc"));
        DrawCombo(I18N::TR("Rotation##interp"), timeline.GetRotTrack());
        ImHelpers::Tooltip(I18N::TR("Interpolation type.desc"));

        TimelineConfig& timeline_cfg = timelineWindow.GetConfig();

        ImGui::Spacing();
        ImGui::SeparatorText(I18N::TR("Timeline"));
        SectionDesc("Timeline.desc");

        ImGui::DragInt(I18N::TR("Pixels per Second"), &timeline_cfg.pixels_per_second, 1, timeline_cfg.GetMinPixelsPerSecond(max_time), 1000);
        ImGui::DragInt(I18N::TR("Sidebar Width"), &timeline_cfg.sidebar_width, 1, 50, 1000);
        ImGui::DragInt(I18N::TR("Track Height"), &timeline_cfg.track_height, 1, 10, 200);

        ImGui::Checkbox(I18N::TR("Enable Snap"), &timeline_cfg.snap_enabled);
		ImHelpers::TooltipWithShortcut(I18N::TR("Snap playhead/keyframes to grid when dragging"), "Alt");

        ImGui::BeginDisabled(!timeline_cfg.snap_enabled);
        ImGui::DragInt(I18N::TR("Snap Pixels"), &timeline_cfg.snap_pixels, 1, 1, 100);
        ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::SeparatorText(I18N::TR("Controls"));

        // 快捷键串仅在配置重载后重建，避免每帧堆分配
        static std::string playPauseStr, addAllStr, delSelStr, selAllStr;
        static uint32_t cacheGen = 0;
        if (cacheGen != cfg.iniGeneration) {
            playPauseStr = std::format("[{}]", cfg.GetKeybindString(ActionType::TimelinePlayPause));
            addAllStr = std::format("[{}]", cfg.GetKeybindString(ActionType::TimelineAddAllKeyframes));
            delSelStr = std::format("[{}]", cfg.GetKeybindString(ActionType::TimelineDeleteSelectedKeyframes));
            selAllStr = std::format("[{}]", cfg.GetKeybindString(ActionType::TimelineSelectAllKeyframes));
            cacheGen = cfg.iniGeneration;
        }

        ImGui::TextUnformatted(playPauseStr.c_str()); ImGui::SameLine(); ImGui::TextDisabled("%s", I18N::TR("- Play/Pause")); ImGui::SameLine(); ImGui::TextDisabled("(?)"); ImHelpers::Tooltip(I18N::TR("Rebindable in config.ini"));
        ImGui::TextUnformatted(addAllStr.c_str()); ImGui::SameLine(); ImGui::TextDisabled("%s", I18N::TR("- Add all keyframes")); ImGui::SameLine(); ImGui::TextDisabled("(?)"); ImHelpers::Tooltip(I18N::TR("Rebindable in config.ini"));
        ImGui::TextUnformatted(delSelStr.c_str()); ImGui::SameLine(); ImGui::TextDisabled("%s", I18N::TR("- Delete selected keyframes")); ImGui::SameLine(); ImGui::TextDisabled("(?)"); ImHelpers::Tooltip(I18N::TR("Rebindable in config.ini"));
        ImGui::TextUnformatted(selAllStr.c_str()); ImGui::SameLine(); ImGui::TextDisabled("%s", I18N::TR("- Select all keyframes")); ImGui::SameLine(); ImGui::TextDisabled("(?)"); ImHelpers::Tooltip(I18N::TR("Rebindable in config.ini"));
		ImGui::Text("[Ctrl + Scroll]"); ImGui::SameLine(); ImGui::TextDisabled("%s", I18N::TR("- Zoom timeline"));
        ImGui::Text("[Shift + Scroll]"); ImGui::SameLine(); ImGui::TextDisabled("%s", I18N::TR("- Scroll timeline"));
        ImGui::TextDisabled("%s", I18N::TR("Hold")); ImGui::SameLine(); ImGui::Text("[RMB]"); ImGui::SameLine(); ImGui::TextDisabled("%s", I18N::TR("in game area to look around"));
        ImGui::TextDisabled("%s", I18N::TR("Hold")); ImGui::SameLine(); ImGui::Text("[Shift]"); ImGui::SameLine(); ImGui::TextDisabled("%s", I18N::TR("and click to select multiple keyframes"));
		ImGui::TextDisabled("%s", I18N::TR("Hold")); ImGui::SameLine(); ImGui::Text("[Alt]"); ImGui::SameLine(); ImGui::TextDisabled("%s", I18N::TR("to disable playhead/keyframes snap to grid"));

        ImGui::EndScrollableArea();
        ImGui::EndTabItem();
    }
}

void MenuWindow::ConfigTab::SortConVars() {
    if (!areSorted) {
        for (auto* conVar : IConVar::allConVars) {
            sortedConVars[conVar->GetSection()].push_back(conVar);
        }
        areSorted = true;
    }
}

void MenuWindow::ConfigTab::Render() {
    if (ImGui::BeginTabItem(I18N::TR("Config"))) {
        ImGui::BeginScrollableArea("##config_content");

        SortConVars();

        for (auto& [section, conVars] : sortedConVars) {
            ImGui::PushID(section.c_str());

            if (ImGui::CollapsingHeader(I18N::TR(section.c_str()))) {
                ImGui::PushItemWidth(Layout::ITEM_WIDTH);
                for (auto* conVar : conVars) {
                    ImGui::PushID(conVar->GetName());

                    bool isDefault = conVar->IsValueDefault();

                    // temp solution
                    if (std::string(conVar->GetName()) == "min_fov") {
                        auto* floatVar = dynamic_cast<ConVar<float>*>(conVar);
                        if (floatVar && *floatVar < 0.001f) {
                            isDefault = true;
                        }
                    }

                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, !isDefault);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                    if (ImGui::Button("*", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()))) {
                        conVar->ResetFromUI();
                    }
                    ImGui::PopStyleColor();
                    ImGui::PopStyleColor();
                    ImGui::PopStyleColor();
                    ImGui::PopStyleVar();

                    if (!isDefault && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                        ImGui::SetTooltip("%s", I18N::TR("Reset to default"));
                    }

                    ImGui::SameLine();

                    conVar->Render();
                    if (conVar->WasChangedByUI()) IConVar::anyChangeByUi = true;

                    ImGui::PopID();
                }
                ImGui::PopItemWidth();

                // 分节底部：通俗易懂的功能说明（绿色小字）
                const std::string descKey = section + ".desc";
                SectionDesc(descKey.c_str());
            }

            ImGui::PopID();
        }

        ImGui::EndScrollableArea();
        ImGui::EndTabItem();
    }
}

void MenuWindow::KeyBindsTab::Render() {
    if (ImGui::BeginTabItem(I18N::TR("Keybinds"))) {
        ImGui::BeginScrollableArea("##keybinds_content");
        ImGui::PushItemWidth(Layout::ITEM_WIDTH);

        auto& keybinds = cfg.GetKeybinds();

        static const char* capturingKeybind = nullptr;

        for (auto& keybind : keybinds) {
            std::string name = keybind.name;
            std::string keybindStr{};
            // fix: every frame getting string from file
            if (!cfg.GetKeybindString(keybind, &keybindStr)) continue;

            ImGui::PushID(keybind.name);

            ImGui::BeginDisabled();
            {
                bool isCapturing = (capturingKeybind == keybind.name);

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

                std::string capturedKeybindsStr = "";
                if (isCapturing) {
                    for (ImGuiKey key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key = (ImGuiKey)(key + 1)) {
                        if (!ImGui::IsKeyDown(key)) continue;
                        //if (funcs::IsLegacyNativeDupe(key) || !ImGui::IsKeyDown(key)) continue; 
                        std::string keyName = ImGui::GetKeyName(key);
                        capturedKeybindsStr += keyName + " ";
                    }
                }

                std::string label = isCapturing ? capturedKeybindsStr : keybindStr;
                if (ImGui::Button(label.c_str(), ImVec2(Layout::BUTTON_WIDTH, 0.0f))) {
                    capturingKeybind = keybind.name;
                    ImGui::SetNextFrameWantCaptureKeyboard(true);
                }

                ImGui::PopStyleColor();
                ImGui::PopStyleColor();
            }
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("%s", I18N::TR("Changing keybind from gui is currently not implemented"));
            }

            ImGui::SameLine();

            ImGui::TextUnformatted(I18N::TR(name.c_str()));

            ImGui::PopID();
        }

        ImGui::PopItemWidth();
        ImGui::EndScrollableArea();
        ImGui::EndTabItem();
    }
}

void MenuWindow::LogTab::Render() {
    if (ImGui::BeginTabItem(I18N::TR("Log"))) {
        ImGui::BeginChild("##log_scroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        const auto& lines = Logger::GetLogLines();
        ImGuiListClipper clipper;
        clipper.Begin((int)lines.size());

        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                ImGui::TextUnformatted(lines[i].c_str());
            }
        }

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f)
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::EndTabItem();
    }
}
