#include "imgui.h"
#include <string.h>

#define MAX_TOASTS 10

typedef struct {
    char message[256];
    float time_remaining;
    float total_time;
} Toast;

static Toast g_toasts[MAX_TOASTS];
static int g_toast_count = 0;

void addIMGUIToast(const char* msg, float duration) {
    if (g_toast_count >= MAX_TOASTS)
        return;

    Toast* t = &g_toasts[g_toast_count++];
    strncpy(t->message, msg, sizeof(t->message) - 1);
    t->message[sizeof(t->message) - 1] = '\0';
    t->time_remaining = duration;
    t->total_time = duration;
}

void renderIMGUIToasts() {
    ImGuiIO* io = &ImGui::GetIO();

    float y_offset = 50.0f;
    for (int i = 0; i < g_toast_count;) {
        Toast* t = &g_toasts[i];
        t->time_remaining -= io->DeltaTime;

        // Compute fade based on time left (fade in/out effect)
        float alpha = 1.0f;
        if (t->time_remaining < 0.5f)
            alpha = t->time_remaining / 0.5f;
        else if (t->time_remaining > t->total_time - 0.5f)
            alpha = (t->total_time - t->time_remaining) / 0.5f;

        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;

        if (t->time_remaining <= 0.0f) {
            // Remove expired toast
            for (int j = i; j < g_toast_count - 1; ++j)
                g_toasts[j] = g_toasts[j + 1];
            g_toast_count--;
            continue;
        }

        ImGui::SetNextWindowBgAlpha(0.8f * alpha);
        // Bottom-center alignment
        float window_width = 300.0f; // approximate width; ImGui will autosize
        ImVec2 window_pos = ImVec2(io->DisplaySize.x * 0.5f, io->DisplaySize.y - y_offset);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.5f, 1.0f));

        char title[32];
        snprintf(title, sizeof(title), "Toast_%d", i);

        ImGui::Begin(title, NULL,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav);

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        ImGui::TextUnformatted(t->message);
        ImGui::PopStyleVar();

        y_offset += ImGui::GetWindowSize().y + 10.0f;
        ImGui::End();

        ++i;
    }
}
