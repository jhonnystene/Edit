/*
 * Edit
 * Copyright (C) 2021 Johnny Stene
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <curses.h>
#include <signal.h>

// Defines
#define DEFAULT_BUFFER_SIZE 65536

// Global data
long file_size = DEFAULT_BUFFER_SIZE;
char* buffer;

FILE* loaded_file;

int top_row = 0;
int current_position = 0;

int enable_colors = 1;

// Text colors
void text_normal()   {if(enable_colors) attron(COLOR_PAIR(1));}
void text_reversed() {if(enable_colors) attron(COLOR_PAIR(2));}
void text_selected() {if(enable_colors) attron(COLOR_PAIR(3));}
void text_error()    {if(enable_colors) attron(COLOR_PAIR(3));}

// Should be unused
//int scanline_top = 0;
//int scanline_current = 0;
//int x_current = 0;

void redraw() {
	text_normal();
	clear(); // Empty the screen

	// Draw the top menu
	text_reversed();
	for(int x = 0; x < COLS; x++) {
		mvaddch(0, x, ' ');
	}

	mvaddstr(0, 1, "File");
	mvaddstr(0, 6, "Edit");

	// Draw the fancy shit around the text
	text_normal();
	for(int y = 1; y < LINES; y++) {
		for(int x = 0; x < COLS; x++) {
			if(y == 1 || y == LINES - 1) {
				if(x == 0) mvaddch(y, x, '+');
				else if(x == COLS - 1) mvaddch(y, x, '+');
				else mvaddch(y, x, '-');
			} else {
				if(x == 0) mvaddch(y, x, '|');
				else if(x == COLS - 1) mvaddch(y, x, '|');
				else mvaddch(y, x, ' ');
			}
		}
	}

	// Draw text from file
	int local_row = 0 - top_row;
	int local_col = 0;

	int cursor_x = 0;
	int cursor_y = 0;
	for(int i = 0; i < file_size; i++) {
		if(i == current_position) {
			cursor_x = local_col + 2;
			cursor_y = local_row + 2;
			//move(local_row + 2, local_col + 2);
		}
		if(buffer[i] == '\n') {
			local_row ++;
			local_col = 0;
		} else if(buffer[i] == 0) {
			break;
		} else {
			if(local_row >= 0 && local_row <= top_row + LINES - 1 && local_col < COLS - 4) {
				mvaddch(local_row + 2, local_col + 2, buffer[i]);
				local_col ++;
			}
		}
	}

	move(cursor_y, cursor_x);

	refresh(); // Present the user with the updated screen
}

void add_to_buffer(char ch) {
	for(int i = file_size - 1; i > current_position - 1; i--) {
		buffer[i] = buffer[i - 1];
	}
	buffer[current_position] = ch;
	current_position ++;
}

void remove_from_buffer() {
	if(current_position == 0) return;
	for(int i = current_position - 1; i < file_size; i++) {
		buffer[i] = buffer[i + 1];
		if(buffer[i + 1] == 0) break; // Don't waste time removing things after we hit EOF
	}
	current_position --;
}

void append_newline_to_buffer() {
	for(int i = 0; i < file_size; i ++) {
		if(buffer[i] == 0) {
			if(buffer[i - 1] == '\n') return;
			buffer[i] = '\n';
			return;
		}
	}
}

int get_real_file_size() {
	for(int i = 0; i < file_size; i++) {
		if(buffer[i] == 0) return i;
	}
	return file_size;
}

void save_file() {
	append_newline_to_buffer();
	fwrite(buffer, sizeof(char), get_real_file_size(), loaded_file);
}

void sigint_handler(int signum) {
	endwin();
	save_file();
	fclose(loaded_file);
	exit(0);
}

int main(int argc, char* argv[]) {
	// Register handler for SIGINT (Ctrl+c)
	signal(SIGINT, sigint_handler);

	if(argc < 2) {
		printf("Usage: edit <filename>\n");
		return -1;
	}

	// Load file
	loaded_file = fopen(argv[1], "r");
	if(loaded_file == NULL) {
		printf("Couldn't open file\n");
		return -1;
	}

	// Get file size
	fseek(loaded_file, 0, SEEK_END);
	long real_file_size = ftell(loaded_file);
	fseek(loaded_file, 0, SEEK_SET);
	if(real_file_size > file_size / 2) file_size += real_file_size;

	// Setup global file buffer
	buffer = (char*) malloc((file_size + 1) * sizeof(char));
	fread(buffer, 1, file_size, loaded_file);

	// Close file and reopen for writing
	fclose(loaded_file);
	loaded_file = fopen(argv[1], "w");

	// Set up terminal
	initscr(); // Init ncurses
	cbreak(); // Print one char at a time, don't wait for a whole line
	noecho(); // Don't print entered characters, we'll do that ourselves

	enable_colors = 0;

	// Set up color
	if(has_colors() == 0) {
		enable_colors = 0;
	}
	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLUE); // Normal
	init_pair(2, COLOR_BLUE, COLOR_WHITE); // Reversed
	init_pair(3, COLOR_WHITE, COLOR_CYAN); // Selected
	init_pair(4, COLOR_WHITE, COLOR_RED);  // Error

	// Draw the screen
	redraw();

	for(;;) { // Main loop
		char new_char = getch();
		if(new_char == KEY_BACKSPACE || new_char == KEY_DC || new_char == 127) remove_from_buffer();
		else if(new_char == KEY_ENTER) add_to_buffer('\n');
		else if(new_char == 0x1B) { // Escape sequence
			if(getch() == 0x5B) { // Arrow keys
				int arrow = getch();
				if(arrow == 0x41) { // Up arrow

				} else if(arrow == 0x42) { // Down arrow

				} else if(arrow == 0x43) { // Right arrow
					if(buffer[current_position + 1] != 0) current_position ++;
				} else if(arrow == 0x44) { // Left arrow
					current_position --;
					if(current_position < 0) current_position = 0;
				}
			}
		}
		else add_to_buffer(new_char);
		redraw();
	}

	return 0;
}

