#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "display.h"

// Enumerate the opcodes
enum opcode { DX, DY, DT, PEN, CLEAR, KEY, COL };
typedef enum opcode opcode;

// State structure that stores the current and previous position, pen state,
// and a pointer to the display.
struct state
{
    int x, y, xp, yp;
    bool penDown;
    display *d;
};
typedef struct state state;

// Open files safely
FILE *fopenCheck(char *file, char *mode)
{
    FILE *p = fopen(file, mode);
    if (p != NULL) return p;
    fprintf(stderr, "Can't open %s: ", file);
    fflush(stderr);
    perror("");
    exit(1);
}

// Get the size of a file in bytes
int getFileSize(char* fileName)
{
    FILE *f = fopenCheck(fileName, "rb");
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fclose(f);
    return size;
}

// Extract the opcode of a sketch instruction
int extractOpcode(unsigned char instr)
{
    int arg = (instr & 0xc0) >> 6;
    if (arg < 3) return arg;
    return (instr & 0x0f);
}

// Get the length of the operand, returning -1 if it is a shorthand instruction
int extractLength(unsigned char instr)
{
    if (((instr & 0xc0) >> 6) == 3)
    {
        int l = (instr & 0x30) >> 4;
        if (l == 3) return 4;
        return l;
    }
    return -1;
}

// Given the length of the operand, extract it from the byte array, making sure
// it is signed.
int extractArg(unsigned char *instr, int length)
{
    if (length == -1)
    {
        int r = (instr[0] & 0x3f);
        if (r > 0x1f) r = -0x20 + (r & 0x1f);
        return r;
    }
    if (length == 0) return 0;
    if (length == 1)
    {
        signed char r = instr[1];
        return (int)r;
    }
    if (length == 2)
    {
        signed short r = (instr[1] << 8) | instr[2];
        return (int)r;
    }
    if (length == 4)
    {
        int r = (instr[1] << 24) | (instr[2] << 16) | (instr[3] << 8) | instr[4];
        return r;
    }
}

// Execute a sketch instruction, returning the number of bytes the instruction
// was.
int executeInstruction(unsigned char *instr, state *s)
{
    int op = extractOpcode(instr[0]);
    int argLength = extractLength(instr[0]);
    int instrLength = argLength + 1;
    if (instrLength == 0) instrLength++;
    int arg = extractArg(instr, argLength);

    if (op == DX) s->x += arg;
    else if (op == DY)
    {
        s->y += arg;
        if (s->penDown) line(s->d, s->xp, s->yp, s->x, s->y);
        s->xp = s->x;
        s->yp = s->y;
    }
    else if (op == DT)
    {
        if (argLength == -1 && arg < 0) arg += 0x40;
        pause(s->d, arg * 10);
    }
    else if (op == PEN) s->penDown = !s->penDown;
    else if (op == CLEAR) clear(s->d);
    else if (op == KEY) key(s->d);
    else if (op == COL) colour(s->d, arg);
    return instrLength;
}

// Run a sketch program stored in a byte array of length 'size'.
void runSketch(unsigned char *instr, int size, state *s)
{
    int i = 0;
    while (i < size)
    {
        i += executeInstruction(instr + i, s);
    }
    return;
}

// Convert a file to an array of bytes, given the file size.
unsigned char *fileToByteArray(char* fileName, int size)
{
    FILE *f = fopenCheck(fileName, "rb");
    unsigned char *data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);
    return data;
}

// Make a new state structure with a new display, initialising it appropiately.
state *newState(char *windowTitle, int windowXSize, int windowYSize)
{
    state *s = malloc(sizeof(state));
    s->d = newDisplay(windowTitle, windowXSize, windowYSize);
    s->x = 0;
    s->y = 0;
    s->xp = 0;
    s->yp = 0;
    s->penDown = false;
    return s;
}

// Run a sketch file given as a command line argument
int main(int n, char *args[])
{
    if (n != 2)
    {
        printf("Run using: ./sketch file.sketch\n");
    }
    else
    {
        int size = getFileSize(args[1]);
        unsigned char *data = fileToByteArray(args[1], size);
        state *s = newState(args[1], 200, 200);
        runSketch(data, size, s);
    }
    return 0;
}
