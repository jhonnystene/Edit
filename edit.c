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
#define ENABLE_COLORS 1

// Global data
long file_size = DEFAULT_BUFFER_SIZE;
char* buffer;
const char* file_name;

FILE* loaded_file;

int top_row = 0;
int current_position = 0;

int enable_colors = ENABLE_COLORS;

// Text colors
void text_normal()        {if(enable_colors) attron(COLOR_PAIR(1));}
void text_reversed()      {if(enable_colors) attron(COLOR_PAIR(2));}
void text_selected()      {if(enable_colors) attron(COLOR_PAIR(3));}
void text_error()         {if(enable_colors) attron(COLOR_PAIR(4));}
void text_menu_selected() {if(enable_colors) attron(COLOR_PAIR(5));}

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

	mvaddstr(0, 2, "Save");
	mvaddstr(0, 8, "Exit");
	mvaddstr(0, 14, "Exit without saving");

	attron(A_BOLD | A_UNDERLINE);
	mvaddch(0, 2, 'S');
	mvaddch(0, 8, 'E');
	mvaddch(0, 15, 'x');
	attroff(A_BOLD | A_UNDERLINE);

	/*for(int i = 0; i < 2; i++) {
		text_reversed();
		mvaddstr(0, 2 + (6 * i), menu_items[i]);
		//text_menu_selected();
		attron(A_BOLD | A_UNDERLINE);
		mvaddch(0, 2 + (6 * i), menu_items[i][0]);
		attroff(A_BOLD | A_UNDERLINE);
	}*/

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
		}
		if(buffer[i] == '\n') {
			local_row ++;
			local_col = 0;
		} else if(buffer[i] == 0) {
			break;
		} else {
			if(local_row >= 0 && local_row <= top_row + LINES - 2 && local_col < COLS - 4) {
				mvaddch(local_row + 2, local_col + 2, buffer[i]);
				local_col ++;
			}
		}
	}

	move(cursor_y, cursor_x);

	refresh(); // Present the user with the updated screen
}

void add_to_buffer(char ch) { // Add a character at our current buffer position
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

void append_newline_to_buffer() { // We can't edit the last item in the file if we don't have either a newline or a space at the end.
	for(int i = 0; i < file_size; i ++) {
		if(buffer[i] == 0) {
			if(buffer[i - 1] == '\n') return;
			buffer[i] = '\n';
			return;
		}
	}
}

int get_real_file_size() { // This stops a bunch of leftover garbage data from being written to the file.
	for(int i = 0; i < file_size; i++) {
		if(buffer[i] == 0) return i;
	}
	return file_size;
}

void save_file() {
	loaded_file = fopen(file_name, "w");
	append_newline_to_buffer(); // We need to do this to stop issues when reloading the file.
	fwrite(buffer, sizeof(char), get_real_file_size(), loaded_file); // Actually dump the buffer out to the file.
	fclose(loaded_file);
}

void graceful_shutdown(int code, int save) {
	endwin(); // Shut down ncurses
	if(save) save_file(); // Save the file currently in memory. TODO: Ask the user if they want to save
	//fclose(loaded_file); // Close the file
	exit(code); // Exit
}

void do_nothing(int code) {} // Traps CTRL+C

int main(int argc, char* argv[]) {
	// Register handler for SIGINT (Ctrl+c)
	signal(SIGINT, do_nothing);

	if(argc < 2) {
		printf("Usage: edit <filename>\n");
		return -1;
	}

	// Load file
	file_name = argv[1];
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
	//loaded_file = fopen(argv[1], "w");

	// Set up terminal
	initscr(); // Init ncurses
	cbreak(); // Print one char at a time, don't wait for a whole line
	noecho(); // Don't print entered characters, we'll do that ourselves

	// Set up color
	if(has_colors() == 0) {
		enable_colors = 0;
	} else {
		start_color();
		init_pair(1, COLOR_WHITE, COLOR_BLUE);  // Normal
		init_pair(2, COLOR_BLUE, COLOR_WHITE);  // Reversed
		init_pair(3, COLOR_WHITE, COLOR_CYAN);  // Selected
		init_pair(4, COLOR_WHITE, COLOR_RED);   // Error
		init_pair(5, COLOR_GREEN, COLOR_WHITE); // Menu Selected
	}

	// Draw the screen
	redraw();

	for(;;) { // Main loop
		char new_char = getch();
		if(new_char == KEY_BACKSPACE || new_char == KEY_DC || new_char == 127) remove_from_buffer();
		else if(new_char == KEY_ENTER) add_to_buffer('\n');
		else if(new_char == 0x1B) { // Escape sequence
			nodelay(stdscr, TRUE);
			char code = getch();
			nodelay(stdscr, FALSE);
			if(code == ERR) { // ESC code
				code = getch();
				if(code == 's') {
					save_file();
				} else if(code == 'e') {
					graceful_shutdown(0, 1);
				} else if(code == 'x') {
					graceful_shutdown(0, 0);
				}
			} else if(code == 0x5B) { // Arrow keys
				int arrow = getch();
				if(arrow == 0x41) { // Up arrow
					// TODO: Recycle this into home() function
					for(; current_position > 0; current_position--) {
						if(buffer[current_position - 1] == '\n') break;
					}

					if(current_position > 0) {
						current_position --;
						for(; current_position > 0; current_position--) {
							if(buffer[current_position - 1] == '\n') break;
						}
					}
					// TODO: Come back to this
					/*// TODO: Fix bug where this always goes to the end of the line
                                        // First, figure out where we are in the line
                                        int x = 0;
                                        for(int px = current_position; px >= 0; px--) {
                                                if(buffer[px] == '\n') break;
                                                x ++;
                                        }

                                        // Next, seek backwards to the previous line
                                        for(; current_position < file_size; current_position--) {
                                                if(buffer[current_position - 1] == '\n') {
                                                        break;
                                                }
                                        }

                                        // Finally, seek backwards to the closest position to X possible
                                        for(int px = 0; px <= x; px++) {
                                                current_position --;
                                                if(buffer[current_position - 1] == '\n' || current_position == 0) break;
                                        }*/
				} else if(arrow == 0x42) { // Down arrow
					for(; current_position < file_size; current_position ++) {
						if(buffer[current_position] == '\n') {
							current_position ++;
							break;
						} else if(buffer[current_position + 1] == 0) break;
					}
					// TODO: Come back to this
					/*// TODO: Fix bug where this always goes to the end of the line
					// First, figure out where we are in the line
					int x = 0;
					for(int px = current_position; px >= 0; px--) {
						x ++;
						if(buffer[px] == '\n') break;
					}

					// Next, seek forward to the next line
					for(; current_position < file_size; current_position++) {
						if(buffer[current_position - 1] == '\n') {
							break;
						}
					}

					// Finally, seek forward to the closest position to X possible
					for(int px = 0; px <= x; px++) {
						current_position ++;
						if(buffer[current_position + 1] == '\n' || buffer[current_position + 1] == 0) break;
					}*/
				} else if(arrow == 0x43) { // Right arrow
					if(buffer[current_position + 1] != 0) current_position ++;
				} else if(arrow == 0x44) { // Left arrow
					current_position --;
					if(current_position < 0) current_position = 0;
				}
			} else {
				// ALT code
			}
		}
		else add_to_buffer(new_char);
		redraw();
	}

	return 0;
}

