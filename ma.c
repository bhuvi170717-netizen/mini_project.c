#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif
/* Grid Configuration */
#define WIDTH 50
#define HEIGHT 20
#define MAX_SHAPES 100
/* ANSI Escape Codes for UI Styling */
#define COLOR_RESET   "\033[0m"
#define COLOR_BORDER  "\033[1;36m" // Cyan
#define COLOR_CANVAS  "\033[90m"   // Dark Gray
#define COLOR_SHAPE   "\033[1;33m" // Yellow
#define COLOR_TITLE   "\033[1;35m" // Magenta
#ifdef COLOR_MENU
#undef COLOR_MENU
#endif
#define COLOR_MENU    "\033[1;32m" // Green
#define COLOR_ERROR   "\033[1;31m" // Red
/* Shape Definitions */
typedef enum {
    SHAPE_LINE,
    SHAPE_RECTANGLE,
    SHAPE_CIRCLE,
    SHAPE_TRIANGLE
} ShapeType;
typedef struct {
    int x1, y1;
    int x2, y2;
} LineProps;
typedef struct {
    int x, y;
    int w, h;
} RectProps;
typedef struct {
    int cx, cy;
    int r;
} CircleProps;
typedef struct {
    int x1, y1;
    int x2, y2;
    int x3, y3;
} TriangleProps;
typedef struct {
    int id;
    ShapeType type;
    union {
        LineProps line;
        RectProps rect;
        CircleProps circle;
        TriangleProps tri;
    } data;
} Shape;
/* Global State */
char canvas[HEIGHT][WIDTH];
Shape shapes[MAX_SHAPES];
int num_shapes = 0;
int next_id = 1;
/* Enable Windows ANSI Escape Processing */
void enable_ansi() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif
}
/* Clear Canvas Array with Underscores */
void clear_canvas() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            canvas[y][x] = '_';
        }
    }
}
/* Safe Plotter with Boundary Clipping */
void plot(int x, int y) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
        canvas[y][x] = '*';
    }
}
/* Bresenham's Line Algorithm */
void draw_line(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = -abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;
    
    while (1) {
        plot(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x1 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y1 += sy;
        }
    }
}
/* Rectangle Outline Drawing */
void draw_rect(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;
    // Top and Bottom lines
    for (int i = 0; i < w; i++) {
        plot(x + i, y);
        plot(x + i, y + h - 1);
    }
    // Left and Right lines
    for (int i = 0; i < h; i++) {
        plot(x, y + i);
        plot(x + w - 1, y + i);
    }
}
/* Midpoint/Bresenham's Circle Drawing */
void draw_circle(int xc, int yc, int r) {
    if (r < 0) return;
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;
    
    while (y >= x) {
        plot(xc + x, yc + y);
        plot(xc - x, yc + y);
        plot(xc + x, yc - y);
        plot(xc - x, yc - y);
        plot(xc + y, yc + x);
        plot(xc - y, yc + x);
        plot(xc + y, yc - x);
        plot(xc - y, yc - x);
        
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}
/* Triangle Outline Drawing (via three lines) */
void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3) {
    draw_line(x1, y1, x2, y2);
    draw_line(x2, y2, x3, y3);
    draw_line(x3, y3, x1, y1);
}
/* Render Active Shapes list into Canvas */
void render_shapes() {
    clear_canvas();
    for (int i = 0; i < num_shapes; i++) {
        Shape s = shapes[i];
        switch (s.type) {
            case SHAPE_LINE:
                draw_line(s.data.line.x1, s.data.line.y1, s.data.line.x2, s.data.line.y2);
                break;
            case SHAPE_RECTANGLE:
                draw_rect(s.data.rect.x, s.data.rect.y, s.data.rect.w, s.data.rect.h);
                break;
            case SHAPE_CIRCLE:
                draw_circle(s.data.circle.cx, s.data.circle.cy, s.data.circle.r);
                break;
            case SHAPE_TRIANGLE:
                draw_triangle(s.data.tri.x1, s.data.tri.y1, s.data.tri.x2, s.data.tri.y2, s.data.tri.x3, s.data.tri.y3);
                break;
        }
    }
}
/* Display Canvas with Headers, Borders, and Grid Coordinates */
void display_canvas() {
    // Column header tens-digit
    printf("     ");
    for (int x = 0; x < WIDTH; x++) {
        if (x % 10 == 0) {
            printf("%d", x / 10);
        } else {
            printf(" ");
        }
    }
    printf("\n");
    // Column header units-digit
    printf("     ");
    for (int x = 0; x < WIDTH; x++) {
        printf("%d", x % 10);
    }
    printf("\n");
    // Top border
    printf("    " COLOR_BORDER "+");
    for (int x = 0; x < WIDTH; x++) printf("-");
    printf("+" COLOR_RESET "\n");
    // Grid rows
    for (int y = 0; y < HEIGHT; y++) {
        printf(COLOR_BORDER "%2d  |" COLOR_RESET, y);
        for (int x = 0; x < WIDTH; x++) {
            char cell = canvas[y][x];
            if (cell == '*') {
                printf(COLOR_SHAPE "*" COLOR_RESET);
            } else {
                printf(COLOR_CANVAS "_" COLOR_RESET);
            }
        }
        printf(COLOR_BORDER "|" COLOR_RESET "\n");
    }
    // Bottom border
    printf("    " COLOR_BORDER "+");
    for (int x = 0; x < WIDTH; x++) printf("-");
    printf("+" COLOR_RESET "\n");
}
/* Print detailed list of active shapes */
void list_shapes() {
    printf("\nActive Shapes:\n");
    if (num_shapes == 0) {
        printf("  (None)\n");
        return;
    }
    for (int i = 0; i < num_shapes; i++) {
        Shape s = shapes[i];
        printf("  [%d] ", s.id);
        switch (s.type) {
            case SHAPE_LINE:
                printf("Line: (%d, %d) to (%d, %d)\n", s.data.line.x1, s.data.line.y1, s.data.line.x2, s.data.line.y2);
                break;
            case SHAPE_RECTANGLE:
                printf("Rectangle: Top-Left (%d, %d), Width %d, Height %d\n", s.data.rect.x, s.data.rect.y, s.data.rect.w, s.data.rect.h);
                break;
            case SHAPE_CIRCLE:
                printf("Circle: Center (%d, %d), Radius %d\n", s.data.circle.cx, s.data.circle.cy, s.data.circle.r);
                break;
            case SHAPE_TRIANGLE:
                printf("Triangle: P1(%d, %d), P2(%d, %d), P3(%d, %d)\n", s.data.tri.x1, s.data.tri.y1, s.data.tri.x2, s.data.tri.y2, s.data.tri.x3, s.data.tri.y3);
                break;
        }
    }
}
/* Safe Integer Reading (prevents standard scanf input stream pollution) */
int read_int(const char* prompt) {
    char buf[128];
    int val;
    while (1) {
        printf("%s", prompt);
        if (!fgets(buf, sizeof(buf), stdin)) {
            continue;
        }
        // Remove trailing newline chars
        buf[strcspn(buf, "\r\n")] = '\0';
        if (sscanf(buf, "%d", &val) == 1) {
            return val;
        }
        printf(COLOR_ERROR "Invalid input. Please enter a valid integer.\n" COLOR_RESET);
    }
}
/* Interactive shape adding menu */
void add_shape_menu() {
    printf("\n" COLOR_TITLE "--- Add a Shape ---" COLOR_RESET "\n");
    printf("1. Line\n");
    printf("2. Rectangle\n");
    printf("3. Circle\n");
    printf("4. Triangle\n");
    printf("5. Back to Main Menu\n");
    
    int choice = read_int("Select shape type (1-5): ");
    if (choice < 1 || choice > 5) {
        printf(COLOR_ERROR "Invalid selection.\n" COLOR_RESET);
        return;
    }
    if (choice == 5) return;
    
    if (num_shapes >= MAX_SHAPES) {
        printf(COLOR_ERROR "Cannot add more shapes. Canvas is full!\n" COLOR_RESET);
        return;
    }
    
    Shape s;
    s.id = next_id++;
    
    if (choice == 1) {
        s.type = SHAPE_LINE;
        printf("Entering Line coordinates:\n");
        s.data.line.x1 = read_int("  Start X (0-49): ");
        s.data.line.y1 = read_int("  Start Y (0-19): ");
        s.data.line.x2 = read_int("  End X (0-49): ");
        s.data.line.y2 = read_int("  End Y (0-19): ");
    } else if (choice == 2) {
        s.type = SHAPE_RECTANGLE;
        printf("Entering Rectangle coordinates:\n");
        s.data.rect.x = read_int("  Top-Left X (0-49): ");
        s.data.rect.y = read_int("  Top-Left Y (0-19): ");
        s.data.rect.w = read_int("  Width (>0): ");
        s.data.rect.h = read_int("  Height (>0): ");
        if (s.data.rect.w <= 0 || s.data.rect.h <= 0) {
            printf(COLOR_ERROR "Width and Height must be positive integers.\n" COLOR_RESET);
            return;
        }
    } else if (choice == 3) {
        s.type = SHAPE_CIRCLE;
        printf("Entering Circle coordinates:\n");
        s.data.circle.cx = read_int("  Center X (0-49): ");
        s.data.circle.cy = read_int("  Center Y (0-19): ");
        s.data.circle.r = read_int("  Radius (>=0): ");
        if (s.data.circle.r < 0) {
            printf(COLOR_ERROR "Radius cannot be negative.\n" COLOR_RESET);
            return;
        }
    } else if (choice == 4) {
        s.type = SHAPE_TRIANGLE;
        printf("Entering Triangle coordinates:\n");
        s.data.tri.x1 = read_int("  Vertex 1 X (0-49): ");
        s.data.tri.y1 = read_int("  Vertex 1 Y (0-19): ");
        s.data.tri.x2 = read_int("  Vertex 2 X (0-49): ");
        s.data.tri.y2 = read_int("  Vertex 2 Y (0-19): ");
        s.data.tri.x3 = read_int("  Vertex 3 X (0-49): ");
        s.data.tri.y3 = read_int("  Vertex 3 Y (0-19): ");
    }
    
    shapes[num_shapes++] = s;
    printf(COLOR_MENU "Shape added successfully with ID %d.\n" COLOR_RESET, s.id);
}
/* Interactive shape deleting menu */
void delete_shape_menu() {
    printf("\n" COLOR_TITLE "--- Delete a Shape ---" COLOR_RESET "\n");
    list_shapes();
    if (num_shapes == 0) return;
    
    int id = read_int("Enter the ID of the shape to delete: ");
    int found_idx = -1;
    for (int i = 0; i < num_shapes; i++) {
        if (shapes[i].id == id) {
            found_idx = i;
            break;
        }
    }
    
    if (found_idx == -1) {
        printf(COLOR_ERROR "Shape with ID %d not found.\n" COLOR_RESET, id);
        return;
    }
    
    // Shift elements left to overwrite deleted index
    for (int i = found_idx; i < num_shapes - 1; i++) {
        shapes[i] = shapes[i + 1];
    }
    num_shapes--;
    printf(COLOR_MENU "Shape ID %d deleted successfully.\n" COLOR_RESET, id);
}
/* Interactive shape modifying menu */
void modify_shape_menu() {
    printf("\n" COLOR_TITLE "--- Modify a Shape ---" COLOR_RESET "\n");
    list_shapes();
    if (num_shapes == 0) return;
    
    int id = read_int("Enter the ID of the shape to modify: ");
    int found_idx = -1;
    for (int i = 0; i < num_shapes; i++) {
        if (shapes[i].id == id) {
            found_idx = i;
            break;
        }
    }
    
    if (found_idx == -1) {
        printf(COLOR_ERROR "Shape with ID %d not found.\n" COLOR_RESET, id);
        return;
    }
    
    Shape* s = &shapes[found_idx];
    switch (s->type) {
        case SHAPE_LINE:
            printf("Modifying Line ID %d (current: (%d, %d) to (%d, %d))\n", s->id, s->data.line.x1, s->data.line.y1, s->data.line.x2, s->data.line.y2);
            s->data.line.x1 = read_int("  New Start X (0-49): ");
            s->data.line.y1 = read_int("  New Start Y (0-19): ");
            s->data.line.x2 = read_int("  New End X (0-49): ");
            s->data.line.y2 = read_int("  New End Y (0-19): ");
            break;
        case SHAPE_RECTANGLE:
            printf("Modifying Rectangle ID %d (current: Top-Left (%d, %d), Width %d, Height %d)\n", s->id, s->data.rect.x, s->data.rect.y, s->data.rect.w, s->data.rect.h);
            s->data.rect.x = read_int("  New Top-Left X (0-49): ");
            s->data.rect.y = read_int("  New Top-Left Y (0-19): ");
            int w = read_int("  New Width (>0): ");
            int h = read_int("  New Height (>0): ");
            if (w <= 0 || h <= 0) {
                printf(COLOR_ERROR "Width and Height must be positive. Modification aborted.\n" COLOR_RESET);
                return;
            }
            s->data.rect.w = w;
            s->data.rect.h = h;
            break;
        case SHAPE_CIRCLE:
            printf("Modifying Circle ID %d (current: Center (%d, %d), Radius %d)\n", s->id, s->data.circle.cx, s->data.circle.cy, s->data.circle.r);
            s->data.circle.cx = read_int("  New Center X (0-49): ");
            s->data.circle.cy = read_int("  New Center Y (0-19): ");
            int r = read_int("  New Radius (>=0): ");
            if (r < 0) {
                printf(COLOR_ERROR "Radius cannot be negative. Modification aborted.\n" COLOR_RESET);
                return;
            }
            s->data.circle.r = r;
            break;
        case SHAPE_TRIANGLE:
            printf("Modifying Triangle ID %d (current: P1(%d, %d), P2(%d, %d), P3(%d, %d))\n", s->id, s->data.tri.x1, s->data.tri.y1, s->data.tri.x2, s->data.tri.y2, s->data.tri.x3, s->data.tri.y3);
            s->data.tri.x1 = read_int("  New Vertex 1 X (0-49): ");
            s->data.tri.y1 = read_int("  New Vertex 1 Y (0-19): ");
            s->data.tri.x2 = read_int("  New Vertex 2 X (0-49): ");
            s->data.tri.y2 = read_int("  New Vertex 2 Y (0-19): ");
            s->data.tri.x3 = read_int("  New Vertex 3 X (0-49): ");
            s->data.tri.y3 = read_int("  New Vertex 3 Y (0-19): ");
            break;
    }
    printf(COLOR_MENU "Shape ID %d modified successfully.\n" COLOR_RESET, id);
}
/* Main Loop */
int main() {
    enable_ansi();
    
    while (1) {
        // Clear terminal screen using ANSI Escape Sequences
        printf("\033[H\033[J");
        
        printf(COLOR_TITLE "=============================================\n");
        printf("          2D TERMINAL GRAPHICS EDITOR        \n");
        printf("=============================================\n" COLOR_RESET);
        
        render_shapes();
        display_canvas();
        
        list_shapes();
        
        printf("\n" COLOR_TITLE "--- Menu ---" COLOR_RESET "\n");
        printf("1. Add Shape\n");
        printf("2. Delete Shape\n");
        printf("3. Modify Shape\n");
        printf("4. Clear Canvas (Delete All)\n");
        printf("5. Refresh Canvas\n");
        printf("6. Exit\n");
        
        int choice = read_int("Choose an option (1-6): ");
        
        if (choice == 1) {
            add_shape_menu();
        } else if (choice == 2) {
            delete_shape_menu();
        } else if (choice == 3) {
            modify_shape_menu();
        } else if (choice == 4) {
            num_shapes = 0;
            printf(COLOR_MENU "All shapes cleared.\n" COLOR_RESET);
        } else if (choice == 5) {
            // Simply let loop restart to refresh
            continue;
        } else if (choice == 6) {
            printf("Goodbye!\n");
            break;
        } else {
            printf(COLOR_ERROR "Invalid option.\n" COLOR_RESET);
        }
        
        // Pause to let the user see the result before reloading the screen
        printf("\nPress Enter to continue...");
        char ch;
        while ((ch = getchar()) != '\n' && ch != EOF);
    }
    
    return 0;
}
