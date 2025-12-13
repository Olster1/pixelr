//NOTE: This is whether the app should rest or keep updating at high frame rate 
bool shouldKeepUpdateing(GameState *gameState) {
    bool result = false;
    CanvasTab *tab = getActiveCanvasTab(gameState);
    if(g_toast_count > 0 || ImGui::IsAnyItemActive() || gameState->selectObject.isActive || (tab && fabsf(gameState->scrollDp) > 1e-6f) || (tab && fabsf(float2_magnitude(gameState->canvasMoveDp)) > 1e-6f) || gameState->drawingShape || (tab && getArrayLength(tab->selected) > 0) || (tab && tab->playback.playing)) {
        result = true;
    }
    return result;
}