/*
 * edit.c
 * By Johnny "jhonnystene" Stene
 * jhonnystene@protonmail.com
 */

#include <stdlib.h>
#include <stdio.h>
#include <curses.h>

// Text colors
void text_normal()   {attron(COLOR_PAIR(1));}
void text_reversed() {attron(COLOR_PAIR(2));}
void text_selected() {attron(COLOR_PAIR(3));}
void text_error()    {attron(COLOR_PAIR(3));}

// Defines
#define BUFFER_SIZE 65536

// Global data
char* buffer;

int top_row = 0; // Which row is at the top?
int current_position = 0; // Where are we in the buffer?

int scanline_top = 0;
int scanline_current = 0;
int x_current = 0;

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
	for(int i = 0; i < BUFFER_SIZE; i++) {
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
	for(int i = BUFFER_SIZE - 1; i > current_position - 1; i--) {
		buffer[i] = buffer[i - 1];
	}
	buffer[current_position] = ch;
	current_position ++;
}

void remove_from_buffer() {
	if(current_position == 0) return;
	for(int i = current_position - 1; i < BUFFER_SIZE; i++) {
		buffer[i] = buffer[i + 1];
		if(buffer[i + 1] == 0) break; // Don't waste time removing things after we hit EOF
	}
	current_position --;
}

int main(int argc, char** argv) {
	// Init global buffer with 64 KiB of space
	buffer = (char*) malloc(BUFFER_SIZE * sizeof(char));

	// Set up terminal
	initscr(); // Init ncurses
	cbreak(); // Print one char at a time, don't wait for a whole line
	noecho(); // Don't print entered characters, we'll do that ourselves

	// Set up color
	if(has_colors() == 0) {
		endwin();
		printf("Error: Your terminal doesn't seem to support colors!\n");
		exit(-1);
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

	endwin(); // Close ncurses window
	return 0;
}

