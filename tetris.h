
enum { TRACE_TETRIS=0 };

#define tetris_printf(...) do { if (TRACE_TETRIS) printf(__VA_ARGS__); } while(0)

enum { TetrisWidth = 10, TetrisHeight = 22 };
typedef enum { PieceL, PieceJ, PieceI, PieceO, PieceT, PieceS, PieceZ } PieceType;

typedef struct
{
	int input_left;
	int input_right;
	int input_down;
	int input_rotate;
	int input_drop;
} Tetris_Input;

typedef struct
{
	PieceType type;
	int rotation;
	int x, y;
} TetrisPiece;

typedef signed long long i64;
typedef struct
{
	int magic_number;
	TetrisPiece current_piece;
	unsigned char board[TetrisWidth * TetrisHeight];
	int lines_cleared;
	int score;
	i64 current_time_us;
	i64 fall_timer;
	int game_over;
} Tetris;

void get_block_offsets(PieceType piece_type, int rotation, int* out_offsets_x, int* out_offsets_y)
{
	//                     PieceL,  PieceJ,  PieceI, PieceO, PieceT,  PieceS,  PieceZ
	int offsets_x[7*3] = {-1,-1,1,  1,-1,1, -1,1,2,  1,0,1,  1,-1,0,  1,0,-1, -1,0,1};
	int offsets_y[7*3] = { 1, 0,0,  1, 0,0,  0,0,0,  0,1,1,  0, 0,1,  0,1, 1,  0,1,1};

	//                       L, J, I, O, T, S, Z
	int rotation_counts[] = {4, 4, 2, 1, 4, 2, 2};

	out_offsets_x[0] = out_offsets_y[0] = 0;
	for (int i = 1; i < 4; ++i)
	{
		out_offsets_x[i] = offsets_x[piece_type * 3 + i-1];
		out_offsets_y[i] = offsets_y[piece_type * 3 + i-1];
	}

	rotation = rotation % rotation_counts[piece_type];
	for (int r = 0; r < rotation; ++r)
	{
		for (int i = 1; i < 4; ++i)
		{
			int temp = out_offsets_x[i];
			out_offsets_x[i] = -out_offsets_y[i];
			out_offsets_y[i] = temp;
		}
	}
}

void get_piece_blocks(TetrisPiece piece, int* out_piece_x, int* out_piece_y)
{
	int piece_offsets_x[4] = {0};
	int piece_offsets_y[4] = {0};
	get_block_offsets(piece.type, piece.rotation, piece_offsets_x, piece_offsets_y);

	for (int i = 0; i < 4; ++i)
	{
		out_piece_x[i] = piece.x + piece_offsets_x[i];
		out_piece_y[i] = piece.y + piece_offsets_y[i];
	}
}

void get_tetris_color(int tile_color, int* r, int* g, int* b)
{
	int x = 170, y = 70, z = 110;
	switch(tile_color)
	{
	case 0: *r = *g = *b = 0; return;
	case 1: *r = x; *g = *b = y; return;
	case 2: *g = x; *r = *b = y; return;
	case 3: *b = x; *r = *g = y; return;
	case 4: *r = *b = x; *g = y; return;
	case 5: *g = *r = x; *b = y; return;
	case 6: *b = *g = x; *r = y; return;
	case 7: *b = *r = *g = z; return;
	}
}

int move_piece_to(TetrisPiece* piece, Tetris* tetris, int x, int y, int r)
{
	int offsets_x[4];
	int offsets_y[4];
	get_block_offsets(piece->type, piece->rotation + r, offsets_x, offsets_y);

	for (int i = 0; i < 4; ++i)
	{
		int xx = piece->x + x + offsets_x[i];
		int yy = piece->y + y + offsets_y[i];
		if (xx < 0 || xx >= TetrisWidth)
			return 0;
		if (yy >= TetrisHeight)
			return 0;
		if (yy < 0)
			continue;

		if (tetris->board[xx + yy * TetrisWidth])
			return 0;
	}

	piece->rotation += r;
	piece->x += x;
	piece->y += y;
	return 1;
}

int move_to(Tetris* tetris, int x, int y, int r)
{
	return move_piece_to(&tetris->current_piece, tetris, x, y, r);
}

typedef void Drawer;
void tetris_draw(Drawer* drawer, Tetris* tetris)
{
	tetris_printf("DRAW tetris fall timer: %lld\n", tetris->fall_timer);
	tetris_printf("DRAW tetris piece x,y: %d,%d\n", tetris->current_piece.x,tetris->current_piece.y);

	int screen_width, screen_height;
	get_screen_width_and_height(drawer, &screen_width, &screen_height);

	int h = (screen_height - 20) / TetrisHeight;
	int w = h;
	int margin = w/2;
	rect(drawer, 0, 0, w * TetrisWidth + margin*2, w * TetrisHeight + margin*2, 70,10,50);

	TetrisPiece shadow_piece = tetris->current_piece;
	while (move_piece_to(&shadow_piece, tetris, 0, 1, 0)) {}

	int piece_x[4] = {0};
	int piece_y[4] = {0};
	get_block_offsets(tetris->current_piece.type, tetris->current_piece.rotation, piece_x, piece_y);
	for (int i = 0; i < 4; ++i)
	{
		piece_x[i] += tetris->current_piece.x;
		piece_y[i] += tetris->current_piece.y;
	}

	for (int y = 0; y < TetrisHeight; ++y)
	{
		for (int x = 0; x < TetrisWidth; ++x)
		{
			int tile_color = tetris->board[y * TetrisWidth + x];
			int piece_hit = 0;
			int shadow_hit = 0;
			int shadow_piece_hit = 0;

			for (int i = 0; i < 4; ++i)
			{
				if (piece_x[i] != x || piece_y[i] > y)
					continue;

				int shadow_y = piece_y[i] - tetris->current_piece.y + shadow_piece.y;
				if (piece_y[i] == y)
					piece_hit = 1;
				else if (shadow_y == y)
					shadow_piece_hit = 1;
				else if (shadow_y > y)
					shadow_hit = 1;
				else
					continue;

				tile_color = tetris->current_piece.type + 1;
			}

			int r,g,b;
			get_tetris_color(tile_color, &r,&g,&b);
			int divider = 1;
			if (piece_hit)
				divider = 1;
			else if (shadow_piece_hit)
				divider = 2;
			else if (shadow_hit)
				divider = 3;

			r/=divider; g/=divider; b/=divider;

			rect(drawer, x * w + margin, y * h + margin, w, h, r,g,b);
		}
	}

	char score_buffer[32];
	sprintf(score_buffer, "%d", tetris->score);
	text(drawer, screen_width / 2, 30, score_buffer, -1);

	if (tetris->game_over)
		text(drawer, screen_width / 2, 60, "Game Over!", -1);
}

int tetris_update(Tetris* tetris, Tetris_Input* tetris_input, i64 time_us)
{
	//tetris_printf("tetris fall timer: %lld\n", tetris->fall_timer);
	//tetris_printf("tetris piece x,y: %d,%d\n", tetris->current_piece.x,tetris->current_piece.y);

	enum { TetrisInitializedMagicNumber = 737215 };

	if (tetris->magic_number != TetrisInitializedMagicNumber
		|| (tetris->game_over && tetris_input->input_drop))
	{
		memset(tetris, 0, sizeof *tetris);
		tetris->magic_number = TetrisInitializedMagicNumber;
		tetris->current_time_us =  time_us;
		tetris->fall_timer = 1000 * 1000; // 1 second
		tetris->current_piece.x = 5;
		return 1;
	}

	if (tetris->game_over)
		return 0;

	tetris_printf("tetris_input_1 %d,%d,%d,%d,%d\n"
	, tetris_input->input_left
	, tetris_input->input_right
	, tetris_input->input_down
	, tetris_input->input_rotate
	, tetris_input->input_drop);

	{
		i64 t = time_us;
		tetris->fall_timer -= t - tetris->current_time_us;
		//tetris_printf("fall: %lld, t: %lld, ct: %lld, -:%lld\n", tetris->fall_timer, t, tetris->current_time_us, t - tetris->current_time_us);
		tetris->current_time_us = t;
	}

	int move_left = tetris_input->input_left;
	int move_right = tetris_input->input_right;
	int move_down = tetris_input->input_down;
	int rotate = tetris_input->input_rotate;
	int drop = tetris_input->input_drop;

	tetris_input->input_left =
	tetris_input->input_right =
	tetris_input->input_down =
	tetris_input->input_rotate =
	tetris_input->input_drop = 0;

	if (0)
	tetris_printf("tetris_input_2 %d,%d,%d,%d,%d\n"
		, tetris_input->input_left
		, tetris_input->input_right
		, tetris_input->input_down
		, tetris_input->input_rotate
		, tetris_input->input_drop);

	int difficulty = 1 + tetris->lines_cleared / 10;
	i64 drop_delay = 1000 * 1000 / difficulty;
	if (drop)
	{
		tetris->fall_timer = drop_delay;
	}
	else if (move_down)
	{
		tetris->fall_timer = drop_delay;
		tetris->score += 1;
	}
	else if (tetris->fall_timer <= 0)
	{
		move_down = 1;
		tetris->fall_timer += drop_delay;
		if (tetris->fall_timer < 0)
			tetris->fall_timer = drop_delay; // No double drops if the game was paused etc.
	}

	if (rotate)
	{
		int i_piece_nudge = tetris->current_piece.type == PieceI && tetris->current_piece.x >= 8;

		move_to(tetris, 0,0,1)
		|| move_to(tetris,  1,0,1)
		|| move_to(tetris, -1,0,1)
		|| (i_piece_nudge && move_to(tetris, -2,0,1))
		|| move_to(tetris, 0,1,1);
	}

	if (move_left)
		move_to(tetris, -1,0,0);

	if (move_right)
		move_to(tetris, 1,0,0);

	if (drop)
	{
		while (move_to(tetris, 0,1,0))
			tetris->score += 1;
		move_down = 1;
	}

	if (move_down)
	{
		if (!move_to(tetris, 0,1,0))
		{
			int offsets_x[4];
			int offsets_y[4];
			get_block_offsets(tetris->current_piece.type, tetris->current_piece.rotation, offsets_x, offsets_y);

			// stick
			for (int i = 0; i < 4; ++i)
			{
				int x = tetris->current_piece.x + offsets_x[i];
				int y = tetris->current_piece.y + offsets_y[i];
				tetris->board[x + y * TetrisWidth] = tetris->current_piece.type + 1;
			}

			// destroy
			int clears = 0;
			for (int y = TetrisHeight; y-- > 0;)
			{
				int block_count = 0;
				for (int x = 0; x < TetrisWidth; ++x)
				{
					int val = tetris->board[x + y * TetrisWidth];
					if (val)
						block_count += 1;

					tetris->board[x + y * TetrisWidth] = 0;
					tetris->board[x + (y + clears) * TetrisWidth] = val;
				}

				if (block_count == 10)
					clears += 1;
			}

			// meta
			tetris->lines_cleared += clears;
			tetris->score += clears * clears * 100 * difficulty;

			// new piece
			tetris->current_piece.type = (tetris->current_piece.type + 1) % 7;
			tetris->current_piece.x = 5;
			tetris->current_piece.y = 0;
			tetris->current_piece.rotation = 0;

			if (!move_to(tetris, 0,0,0))
			{
				tetris->game_over = 1;
				printf("Game Over! Final Score: %d\n", tetris->score);
			}
		}
	}

	return move_down || move_left || move_right || move_down || rotate || drop;
}
