
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


void floodFillWithBucket(GameState *gameState, int canvasW, int canvasH, int startX, int startY, u32 startColor, u32 newColor) {
    MemoryArenaMark mark = takeMemoryMark(&globalPerFrameArena);

    bool *visited = pushArray(&globalPerFrameArena, canvasW*canvasH, bool);
    easyMemory_zeroSize(visited, sizeof(bool)*canvasW*canvasH);

    FloodFillCavnas queue = {};
    queue.next = queue.prev = &queue; 
	pushOnFloodFillQueue(&queue, visited, startX, startY, canvasW, canvasH);

	bool searching = true;
	while(searching) {	
		FloodFillCavnas *node = popOffFloodFillQueue(&queue);
		if(node) {
			int x = node->x;
			int y = node->y;

			if((getCanvasColor(gameState, x, y) != startColor)) {
                //NOTE: Don't add anymore
			} else {
				//NOTE: Set the color
				setCanvasColor(gameState, x, y, newColor);

				//push on more directions   
				pushOnFloodFillQueue(&queue, visited, x + 1, y, canvasW, canvasH);
				pushOnFloodFillQueue(&queue, visited, x - 1, y, canvasW, canvasH);
				pushOnFloodFillQueue(&queue, visited, x, y + 1, canvasW, canvasH);
				pushOnFloodFillQueue(&queue, visited, x, y - 1, canvasW, canvasH);

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
