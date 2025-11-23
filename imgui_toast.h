
typedef struct {
    char message[256];
    float time_remaining;
    float total_time;
} Toast;

Toast *addIMGUIToast(const char* msg, float duration);
void endImguiToast(Toast *toast);