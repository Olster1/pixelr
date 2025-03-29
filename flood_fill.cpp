
struct FloodFillCavnas {
    int x; 
    int y;
    FloodFillCavnas *next;
    FloodFillCavnas *prev;
};

bool inFloodFillCanvasRange(int x, int y, int canvasW, int canvasH) {
	return (x >= 0 && x < canvasW && y >= 0 && y < canvasH);
}

void pushOnFloodFillQueue(FloodFillCavnas *queue, bool *visited, int x, int y, int canvasW, int canvasH) {
	
	if(inFloodFillCanvasRange(x, y, canvasW, canvasH) && !visited[canvasW*y + x]) {
		FloodFillCavnas *node = pushStruct(&globalPerFrameArena, FloodFillCavnas);
		node->x = x;
        node->y = y;

		queue->next->prev = node;
		node->next = queue->next;

		queue->next = node;
		node->prev = queue;

		//push node to say you visited it
        visited[canvasW*y + x] = true;
		
	}
}

FloodFillCavnas *popOffFloodFillQueue(FloodFillCavnas *queue) {
	FloodFillCavnas *result = 0;

	if(queue->prev != queue) { //something on the queue
		result = queue->prev;

		queue->prev = result->prev;
		queue->prev->next = queue;
	} 

	return result;
}


void floodFillWithBucket(GameState *gameState, Canvas *canvas, int startX, int startY, u32 startColor, u32 newColor, bool isSelect) {
    MemoryArenaMark mark = takeMemoryMark(&globalPerFrameArena);

    bool *visited = pushArray(&globalPerFrameArena, canvas->w*canvas->h, bool);
    easyMemory_zeroSize(visited, sizeof(bool)*canvas->w*canvas->h);

    FloodFillCavnas queue = {};
    queue.next = queue.prev = &queue; 
	pushOnFloodFillQueue(&queue, visited, startX, startY, canvas->w, canvas->h);

	CanvasTab *t = getActiveCanvasTab(gameState);
	if(isSelect) {
		
		if(t) {
			clearSelection(t, gameState);
		}
	}

	bool searching = true;
	while(searching) {	
		FloodFillCavnas *node = popOffFloodFillQueue(&queue);
		if(node) {
			int x = node->x;
			int y = node->y;

			if((getCanvasColor(canvas, x, y) != startColor)) {
                //NOTE: Don't add anymore
			} else {
				if(isSelect) {
					CanvasTab *t = getActiveCanvasTab(gameState);
					if(t) {
						float2 f = make_float2(x, y);
            			pushArrayItem(&t->selected, f, float2);
					}
				} else {
					//NOTE: Set the color
					setCanvasColor(t, canvas, x, y, newColor, t->opacity);
				}

				//push on more directions   
				pushOnFloodFillQueue(&queue, visited, x + 1, y, canvas->w, canvas->h);
				pushOnFloodFillQueue(&queue, visited, x - 1, y, canvas->w, canvas->h);
				pushOnFloodFillQueue(&queue, visited, x, y + 1, canvas->w, canvas->h);
				pushOnFloodFillQueue(&queue, visited, x, y - 1, canvas->w, canvas->h);

				//Diagonal movements
                // pushOnFloodFillQueue(&queue, visited, x + 1, y + 1, canvasW, canvasH);
				// pushOnFloodFillQueue(&queue, visited, x + 1, y - 1, canvasW, canvasH);
				// pushOnFloodFillQueue(&queue, visited, x - 1, y - 1, canvasW, canvasH);
				// pushOnFloodFillQueue(&queue, visited, x - 1, y + 1, canvasW, canvasH);
			}
		} else {
			searching = false;
			break;
		}
	}

    releaseMemoryMark(&mark);
}


void updateBucket(GameState *gameState, Canvas *canvas, bool isSelect = false) {
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
		CanvasTab *tab = getActiveCanvasTab(gameState);

		if(tab) {
		
			//NOTE: Use Flood fill algorithm
			float2 canvasP = getCanvasCoordFromMouse(gameState, canvas->w, canvas->h);
			u32 startColor = getCanvasColor(canvas, canvasP.x, canvasP.y);
			
			if(isValidCanvasRange(canvas, canvasP.x, canvasP.y)) {
				floodFillWithBucket(gameState, canvas, canvasP.x, canvasP.y, getCanvasColor(canvas, canvasP.x, canvasP.y), float4_to_u32_color(tab->colorPicked), isSelect);
			}
		}
    }
}