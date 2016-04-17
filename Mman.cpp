#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <cstdio>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>
#include <fcntl.h>
#include <cerrno>
#include <ncurses.h>

using namespace std;

void man(int argc, char *args[]);

int main(int argc, char *argv[])
{
    man(argc, argv);
    return 0;
}

void man(int argc, char *args[])
{
    int ch, prev, row, col;
    prev = EOF;
    FILE *fp;
    int y, x;
    /*
    if(argc != 2)
    {
    printf("Usage: %s <a c file name>\n", argv[0]);
    exit(1);
    }
    */
    fp = fopen("/home/magnolias/Github/Mshell/Mshell.cpp", "r");
    if(fp == NULL)
    {
        perror("Cannot open input file");
        exit(1);
    }
    initscr();				/* Start curses mode */
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    mvchgat(0, 0, -1, A_BLINK, 1, NULL);
    getmaxyx(stdscr, row, col);		/* find the boundaries of the screeen */
    while((ch = fgetc(fp)) != EOF)	/* read the file till we reach the end */
    {
        getyx(stdscr, y, x);		/* get the current curser position */
        if(y == (row - 1))			/* are we are at the end of the screen */
        {
            attron(A_STANDOUT);			/* cut bold on */
            printw("<-Press Any Key->");	/* tell the user to press a key */
            attroff(A_STANDOUT);			/* cut bold on */
            getch();
            clear();				/* clear the screen */
            move(0, 0);			/* start at the beginning of the screen */
        }
        if(prev == '/' && ch == '*')    	/* If it is / and * then only
                                         	 * switch bold on */    
        {
            attron(A_STANDOUT);			/* cut bold on */
            getyx(stdscr, y, x);		/* get the current curser position */
            move(y, x - 1);			/* back up one space */
            printw("%c%c", '/', ch); 		/* The actual printing is done here */
        }
        else
            printw("%c", ch);
        refresh();
        if(prev == '*' && ch == '/')
          attroff(A_STANDOUT);        		/* Switch it off once we got *
                                     	 * and then / */
        prev = ch;
    }
    endwin();                       	/* End curses mode */
    fclose(fp);
    return;
}

