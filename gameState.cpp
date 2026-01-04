enum CanvasInteractionMode {
    CANVAS_DRAW_MODE,
    CANVAS_MOVE_MODE,
    CANVAS_FILL_MODE,
    CANVAS_DRAW_CIRCLE_MODE,
    CANVAS_DRAW_LINE_MODE,
    CANVAS_DRAW_RECTANGLE_MODE,
    CANVAS_ERASE_MODE,
    CANVAS_SELECT_RECTANGLE_MODE,
    CANVAS_MOVE_SELECT_MODE,
    CANVAS_SPRAY_CAN,
    CANVAS_COLOR_DROPPER,
};

enum CanvasDrawFlag {
    CANVAS_MIRROR_FLAG = 1 << 0,
    CANVAS_MIRROR_HORIZONTAL_FLAG = 1 << 1,
    CANVAS_MIRROR_VERTICAL_FLAG = 1 << 2,
    
};

#define MAX_CANVAS_DIM 4000

struct CloseCanvasTabInfo {
    bool UIshowUnsavedWindow;
    CanvasTab *canvasTab;
    int arrayIndex;
    bool closeAppAfterwards;
};

typedef enum {
	EASY_PROFILER_DRAW_OVERVIEW,
	EASY_PROFILER_DRAW_ACCUMALTED,
} EasyProfile_DrawType;

typedef enum {
	EASY_PROFILER_DRAW_OPEN,
	EASY_PROFILER_DRAW_CLOSED,
} EasyDrawProfile_OpenState;

typedef struct {
	int hotIndex;

	EasyProfile_DrawType drawType;

	//For navigating the samples in a frame
	float zoomLevel;
	float xOffset; //for the scroll bar
	bool holdingScrollBar;
	float scrollPercent;
	float grabOffset;

	float2 lastMouseP;
	bool firstFrame;

	EasyDrawProfile_OpenState openState;
	float openTimer;

	int level;
} EasyProfile_ProfilerDrawState;

static EasyProfile_ProfilerDrawState *EasyProfiler_initProfilerDrawState() {
	EasyProfile_ProfilerDrawState *result = pushStruct(&globalLongTermArena, EasyProfile_ProfilerDrawState);
		
	result->hotIndex = -1;
	result->level = 0;
	result->drawType = EASY_PROFILER_DRAW_OVERVIEW;

	result->zoomLevel = 1;
	result->holdingScrollBar = false;
	result->xOffset = 0;
	result->lastMouseP =  make_float2(0, 0);
	result->firstFrame = true;
	result->grabOffset = 0;

	result->openTimer = -1;
	result->openState = EASY_PROFILER_DRAW_CLOSED;

	return result;
}

enum InteractionAppState {
    INTERACTION_NONE,
    INTERACTION_IMGUI,
    INTERACTION_DRAWING,
};

struct WindowResizeCommand {
    float2 newSize;
    float2 windowPos;
    bool valid;
};

struct GameState {
    bool inited;
    float dt;
    float screenWidth;
    float aspectRatio_y_over_x;

    CloseCanvasTabInfo closeCanvasTabInfo;

    Interaction currentInteraction;

    float2 mouseP_screenSpace;
    float2 mouseP_01;
    
    char *appDataFolderName;

    int mouseIndexAt;
    int mouseCountAt;
    float2 mouseP_01_array[101];
    float2 lastMouseP;
    
    Font mainFont;

    time_t timeOfLastBackup;

    CanvasInteractionMode interactionMode;
    CanvasInteractionMode lastInteractionMode;
    CanvasInteractionMode interactionModeStartOfFrame;

    EasyProfile_ProfilerDrawState *drawState;

    MouseKeyState mouseBtn[MOUSE_BUTTON_TYPE_COUNT];

    float sprayTimeAt;

    float3 cameraOffset;

    float16 cameraRotation;
    bool inverseZoom;

    float3 startP;
    int grabbedCornerIndex;

    InteractionAppState interactionAppState;

    float scrollDp; //NOTE: velcoity to intergrate for the scroll speed
    bool nearest;
    u32 canvasMirrorFlags;
    u32 currentSelectedMirrorIndexUi;

    ThreadsInfo threadsInfo;

    WindowResizeCommand windowResizeCommand;
    u8 *brushOutlineStencil; //NOTE: Allocateed on the perframe arena to draw the shape of the brush each frame
    int brushOutlineSize;

    Renderer *renderer;

    Camera camera;

    int randomStartUpID;

    KeyStates keys;

    int droppedFileCount;
    char *droppedFilePaths[MAX_DROPPED_FILES];

    bool clearPaletteOnLoad;
    bool checkBackground;

    bool canvasSettingsWindow;
    bool showAboutWindow;
    bool showLayerOptionsWindow;
    bool openSpriteSheetWindow;
    int layerOptionsIndex;
    char *versionString;
    char *exePath;

    int canvasTabsOpened; //NOTE: To see if we need to save the files for reloading on startup

    float4 bgColor;
    char dimStr0[256];
    char dimStr1[256];
    
    bool showNewCanvasWindow;
    bool showExportWindow;
    bool maxColumnsExport;
    float scrollSpeed;
    bool draggingCanvas;
    float2 startDragP;
    bool paintActive;
    float2 lastPaintP;
    bool drawGrid;
    float2 drawShapeStart;
    bool drawingShape;
    bool autoFocus;
    bool selectMode;
    bool blueRedFlippedInUI;
    
    int runningAverageCount;

    EaselSelectObject selectObject;

    CanvasTab *canvasTabs; //NOTE: resize array
    int activeCanvasTab;

    bool showColorPalleteEnter;
    bool showEditPalette;
    int editPaletteIndex;

    char colorsPalleteBuffer[10000]; //NOTE: This is a string for the imgui text box to use

    Clipboard clipboard;
    bool quit;
    bool shouldQuit;
};
