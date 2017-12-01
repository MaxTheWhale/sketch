#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "display.h"

enum opcode { DX, DY, DT, PEN };
typedef enum opcode opcode;

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

int getFileSize(char* fileName)
{
    FILE *f = fopenCheck(fileName, "rb");
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fclose(f);
    return size;
}

int extractOpcode(unsigned char instr)
{
    return (instr & 0xC0) >> 6;
}

int extractArg(unsigned char instr, bool isSigned)
{
    int r = (instr & 0x3F);
    if (!isSigned) return r;
    if (r > 0x1f) r = -0x20 + (r & 0x1f);
    return r;
}

void executeInstruction(unsigned char instr, state *s)
{
    int op = extractOpcode(instr);
    int arg;
    if (op == DT) arg = extractArg(instr, false);
    else arg = extractArg(instr, true);
    printf("op %d arg %d\n", op, arg);
}

unsigned char *fileToByteArray(char* fileName, int size)
{
    FILE *f = fopenCheck(fileName, "rb");
    unsigned char *data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);
    return data;
}

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

int main()
{
    int size = getFileSize("line.sketch");
    unsigned char *data = fileToByteArray("line.sketch", size);
    state *s = newState("Sketch", 200, 200);
    for (int i = 0; i < size; i++)
    {
        executeInstruction(data[i], s);
    }
    key(s->d);
    return 0;
}
