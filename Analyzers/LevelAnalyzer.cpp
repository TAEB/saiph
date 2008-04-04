#include "LevelAnalyzer.h"

/* constructors */
LevelAnalyzer::LevelAnalyzer(Saiph *saiph) {
	this->saiph = saiph;
}

/* methods */
void LevelAnalyzer::end() {
	if (saiph->command.priority > LA_DESCEND_PRIORITY)
		return;
	/* time to descend */
	int branch = saiph->current_branch;
	int dungeon = saiph->world->player.status.dungeon;
	if (saiph->branches[branch]->map[dungeon][saiph->world->player.row][saiph->world->player.col] == STAIRS_DOWN) {
		/* standing on downstairs, descend */
		char command[2];
		command[0] = MOVE_DOWN;
		command[1] = '\0';
		saiph->setNextCommand(command, LA_DESCEND_PRIORITY);
		return;
	}
	for (int r = MAP_ROW_START; r <= MAP_ROW_END; ++r) {
		for (int c = 0; c < COLS; ++c) {
			if (saiph->branches[branch]->map[dungeon][r][c] == STAIRS_DOWN) {
				int distance = 0;
				bool direct_line = false;
				char move = saiph->shortestPath(r, c, false, distance, direct_line);
				if (move != -1) {
					char command[2];
					command[0] = move;
					command[1] = '\0';
					saiph->setNextCommand(command, LA_DESCEND_PRIORITY);
					return;
				}
			}
		}
	}
}