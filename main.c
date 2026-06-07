#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
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
#define COLOR_TITLE   "\033[1;35m" // Magenta
#define COLOR_ERROR   "\033[1;31m" // Red
#define COLOR_SHAPE   "\033[1;33m" // Yellow
#define COLOR_CANVAS  "\033[90m"   // Dark Gray

#ifdef COLOR_MENU
#undef COLOR_MENU
#endif
#define COLOR_MENU    "\033[1;32m" // Green

/* Custom Key codes for Windows console _getch parsing */
#define KEY_UP    1001
#define KEY_DOWN  1002
#define KEY_LEFT  1003
#define KEY_RIGHT 1004
#define KEY_ESC   27
#define KEY_ENTER 13

/* Shape Color Indices */
enum {
    COLOR_IDX_RESET = 0,
    COLOR_IDX_RED,
    COLOR_IDX_GREEN,
    COLOR_IDX_YELLOW,
    COLOR_IDX_BLUE,
    COLOR_IDX_MAGENTA,
    COLOR_IDX_CYAN,
    COLOR_IDX_WHITE,
    COLOR_IDX_CANVAS
};

/* Color Helper Functions */
const char* color_name(int color_idx) {
    switch (color_idx) {
        case COLOR_IDX_RED:     return "Red";
        case COLOR_IDX_GREEN:   return "Green";
        case COLOR_IDX_YELLOW:  return "Yellow";
        case COLOR_IDX_BLUE:    return "Blue";
        case COLOR_IDX_MAGENTA: return "Magenta";
        case COLOR_IDX_CYAN:    return "Cyan";
        case COLOR_IDX_WHITE:   return "White";
        default:                return "Default";
    }
}

void print_color(int color_idx) {
    switch (color_idx) {
        case COLOR_IDX_RED:     printf("\033[1;31m"); break;
        case COLOR_IDX_GREEN:   printf("\033[1;32m"); break;
        case COLOR_IDX_YELLOW:  printf("\033[1;33m"); break;
        case COLOR_IDX_BLUE:    printf("\033[1;34m"); break;
        case COLOR_IDX_MAGENTA: printf("\033[1;35m"); break;
        case COLOR_IDX_CYAN:    printf("\033[1;36m"); break;
        case COLOR_IDX_WHITE:   printf("\033[1;37m"); break;
        case COLOR_IDX_CANVAS:  printf("\033[90m"); break;
        default:                printf("\033[0m"); break;
    }
}

/* Shape Definitions */
typedef enum {
    SHAPE_LINE,
    SHAPE_RECTANGLE,
    SHAPE_CIRCLE,
    SHAPE_TRIANGLE,
    SHAPE_POINT,
    SHAPE_FLOOD_FILL
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
    int x, y;
} PointProps;

typedef struct {
    int x, y;
} FloodFillProps;

typedef struct {
    int id;
    ShapeType type;
    int color;
    union {
        LineProps line;
        RectProps rect;
        CircleProps circle;
        TriangleProps tri;
        PointProps point;
        FloodFillProps fill;
    } data;
} Shape;

/* Grid Cell Definition */
typedef struct {
    char ch;
    int color;
} Cell;

/* Global State */
Cell canvas[HEIGHT][WIDTH];
Shape shapes[MAX_SHAPES];
int num_shapes = 0;
int next_id = 1;

/* Interactive Editor State Enum */
typedef enum {
    STATE_NAVIGATING,
    STATE_LINE_P1,
    STATE_RECT_P1,
    STATE_CIRCLE_CENTER,
    STATE_TRIANGLE_P1,
    STATE_TRIANGLE_P2
} EditState;

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

/* Parse Input Key including Arrows and Special Keys */
int get_key() {
#ifdef _WIN32
    int ch = _getch();
    if (ch == 0 || ch == 224) {
        int next_ch = _getch();
        switch (next_ch) {
            case 72: return KEY_UP;
            case 80: return KEY_DOWN;
            case 75: return KEY_LEFT;
            case 77: return KEY_RIGHT;
        }
    }
    return ch;
#else
    return getchar();
#endif
}

/* Integer Square Root (Newton-Raphson method, avoids linking math.h lib) */
int int_sqrt(int n) {
    if (n <= 0) return 0;
    int x = n;
    int y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

/* Clear Canvas Array with Underscores */
void clear_canvas() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            canvas[y][x].ch = '_';
            canvas[y][x].color = COLOR_IDX_CANVAS;
        }
    }
}

/* Safe Plotter with Boundary Clipping and Color Setting */
void plot(int x, int y, int color) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
        canvas[y][x].ch = '*';
        canvas[y][x].color = color;
    }
}

/* Bresenham's Line Algorithm */
void draw_line(int x1, int y1, int x2, int y2, int color) {
    int dx = abs(x2 - x1);
    int dy = -abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;
    
    while (1) {
        plot(x1, y1, color);
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
void draw_rect(int x, int y, int w, int h, int color) {
    if (w <= 0 || h <= 0) return;
    // Top and Bottom lines
    for (int i = 0; i < w; i++) {
        plot(x + i, y, color);
        plot(x + i, y + h - 1, color);
    }
    // Left and Right lines
    for (int i = 0; i < h; i++) {
        plot(x, y + i, color);
        plot(x + w - 1, y + i, color);
    }
}

/* Midpoint/Bresenham's Circle Drawing */
void draw_circle(int xc, int yc, int r, int color) {
    if (r < 0) return;
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;
    
    while (y >= x) {
        plot(xc + x, yc + y, color);
        plot(xc - x, yc + y, color);
        plot(xc + x, yc - y, color);
        plot(xc - x, yc - y, color);
        plot(xc + y, yc + x, color);
        plot(xc - y, yc + x, color);
        plot(xc + y, yc - x, color);
        plot(xc - y, yc - x, color);
        
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
void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int color) {
    draw_line(x1, y1, x2, y2, color);
    draw_line(x2, y2, x3, y3, color);
    draw_line(x3, y3, x1, y1, color);
}

/* Recursive DFS Flood Fill (recomputes bounds inside canvas) */
void dfs_fill(int x, int y, int tgt_color) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    if (canvas[y][x].ch != '_') return;
    
    canvas[y][x].ch = '*';
    canvas[y][x].color = tgt_color;
    
    dfs_fill(x + 1, y, tgt_color);
    dfs_fill(x - 1, y, tgt_color);
    dfs_fill(x, y + 1, tgt_color);
    dfs_fill(x, y - 1, tgt_color);
}

void flood_fill_render(int start_x, int start_y, int tgt_color) {
    dfs_fill(start_x, start_y, tgt_color);
}

/* Render Active Shapes list into Canvas */
void render_shapes() {
    clear_canvas();
    for (int i = 0; i < num_shapes; i++) {
        Shape s = shapes[i];
        switch (s.type) {
            case SHAPE_LINE:
                draw_line(s.data.line.x1, s.data.line.y1, s.data.line.x2, s.data.line.y2, s.color);
                break;
            case SHAPE_RECTANGLE:
                draw_rect(s.data.rect.x, s.data.rect.y, s.data.rect.w, s.data.rect.h, s.color);
                break;
            case SHAPE_CIRCLE:
                draw_circle(s.data.circle.cx, s.data.circle.cy, s.data.circle.r, s.color);
                break;
            case SHAPE_TRIANGLE:
                draw_triangle(s.data.tri.x1, s.data.tri.y1, s.data.tri.x2, s.data.tri.y2, s.data.tri.x3, s.data.tri.y3, s.color);
                break;
            case SHAPE_POINT:
                plot(s.data.point.x, s.data.point.y, s.color);
                break;
            case SHAPE_FLOOD_FILL:
                flood_fill_render(s.data.fill.x, s.data.fill.y, s.color);
                break;
        }
    }
}

/* Display Canvas with Headers, Borders, Grid Coordinates and Interactive Cursor */
void display_canvas(int cursor_x, int cursor_y, int show_cursor) {
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
            if (show_cursor && x == cursor_x && y == cursor_y) {
                // Highlight cursor with inverse background
                printf("\033[1;31;47m+\033[0m");
            } else {
                Cell cell = canvas[y][x];
                print_color(cell.color);
                printf("%c", cell.ch);
                printf(COLOR_RESET);
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
        print_color(s.color);
        printf("%s Shape: " COLOR_RESET, color_name(s.color));
        switch (s.type) {
            case SHAPE_LINE:
                printf("Line from (%d, %d) to (%d, %d)\n", s.data.line.x1, s.data.line.y1, s.data.line.x2, s.data.line.y2);
                break;
            case SHAPE_RECTANGLE:
                printf("Rectangle at (%d, %d), Width %d, Height %d\n", s.data.rect.x, s.data.rect.y, s.data.rect.w, s.data.rect.h);
                break;
            case SHAPE_CIRCLE:
                printf("Circle at Center (%d, %d), Radius %d\n", s.data.circle.cx, s.data.circle.cy, s.data.circle.r);
                break;
            case SHAPE_TRIANGLE:
                printf("Triangle P1(%d, %d), P2(%d, %d), P3(%d, %d)\n", s.data.tri.x1, s.data.tri.y1, s.data.tri.x2, s.data.tri.y2, s.data.tri.x3, s.data.tri.y3);
                break;
            case SHAPE_POINT:
                printf("Point at (%d, %d)\n", s.data.point.x, s.data.point.y);
                break;
            case SHAPE_FLOOD_FILL:
                printf("Paint Bucket Flood Fill at (%d, %d)\n", s.data.fill.x, s.data.fill.y);
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
        buf[strcspn(buf, "\r\n")] = '\0';
        if (sscanf(buf, "%d", &val) == 1) {
            return val;
        }
        printf(COLOR_ERROR "Invalid input. Please enter a valid integer.\n" COLOR_RESET);
    }
}

/* Scrolling TUI Choice Menu Selector */
int select_menu(const char* title, const char* options[], int num_options) {
    int active_idx = 0;
    while (1) {
        printf("\033[H\033[J");
        printf(COLOR_TITLE "=============================================\n");
        printf("  %s  \n", title);
        printf("=============================================\n" COLOR_RESET);
        printf("Use WASD/Arrows to scroll, Enter to confirm, Esc/Q to Cancel.\n\n");

        for (int i = 0; i < num_options; i++) {
            if (i == active_idx) {
                printf("  " COLOR_MENU "-> [ %s ]" COLOR_RESET "\n", options[i]);
            } else {
                printf("     %s  \n", options[i]);
            }
        }
        
        int key = get_key();
        if (key == KEY_UP || key == 'w' || key == 'W') {
            if (active_idx > 0) active_idx--;
        } else if (key == KEY_DOWN || key == 's' || key == 'S') {
            if (active_idx < num_options - 1) active_idx++;
        } else if (key == KEY_ENTER || key == 13) {
            return active_idx;
        } else if (key == KEY_ESC || key == 'q' || key == 'Q') {
            return -1; // Aborted
        }
    }
}

/* Visual Shape Selector from List */
int select_shape_from_list(const char* title) {
    if (num_shapes == 0) return -1;
    int active_idx = 0;
    
    while (1) {
        printf("\033[H\033[J");
        printf(COLOR_TITLE "=============================================\n");
        printf("  %s  \n", title);
        printf("=============================================\n" COLOR_RESET);
        printf("Use WASD/Arrows to scroll, Enter to confirm, Esc/Q to Cancel.\n\n");
        
        for (int i = 0; i < num_shapes; i++) {
            Shape s = shapes[i];
            char desc[128];
            switch (s.type) {
                case SHAPE_LINE:
                    sprintf(desc, "[ID %d] %s Line: (%d, %d) to (%d, %d)", s.id, color_name(s.color), s.data.line.x1, s.data.line.y1, s.data.line.x2, s.data.line.y2);
                    break;
                case SHAPE_RECTANGLE:
                    sprintf(desc, "[ID %d] %s Rectangle: (%d, %d), W:%d, H:%d", s.id, color_name(s.color), s.data.rect.x, s.data.rect.y, s.data.rect.w, s.data.rect.h);
                    break;
                case SHAPE_CIRCLE:
                    sprintf(desc, "[ID %d] %s Circle: Center (%d, %d), R:%d", s.id, color_name(s.color), s.data.circle.cx, s.data.circle.cy, s.data.circle.r);
                    break;
                case SHAPE_TRIANGLE:
                    sprintf(desc, "[ID %d] %s Triangle: P1(%d,%d), P2(%d,%d), P3(%d,%d)", s.id, color_name(s.color), s.data.tri.x1, s.data.tri.y1, s.data.tri.x2, s.data.tri.y2, s.data.tri.x3, s.data.tri.y3);
                    break;
                case SHAPE_POINT:
                    sprintf(desc, "[ID %d] %s Point: (%d, %d)", s.id, color_name(s.color), s.data.point.x, s.data.point.y);
                    break;
                case SHAPE_FLOOD_FILL:
                    sprintf(desc, "[ID %d] %s Paint Bucket: (%d, %d)", s.id, color_name(s.color), s.data.fill.x, s.data.fill.y);
                    break;
            }
            
            if (i == active_idx) {
                printf("  " COLOR_MENU "-> [ %s ]" COLOR_RESET "\n", desc);
            } else {
                printf("     %s  \n", desc);
            }
        }
        
        int key = get_key();
        if (key == KEY_UP || key == 'w' || key == 'W') {
            if (active_idx > 0) active_idx--;
        } else if (key == KEY_DOWN || key == 's' || key == 'S') {
            if (active_idx < num_shapes - 1) active_idx++;
        } else if (key == KEY_ENTER || key == 13) {
            return active_idx;
        } else if (key == KEY_ESC || key == 'q' || key == 'Q') {
            return -1;
        }
    }
}

/* Interactive Color Selector */
int choose_color() {
    const char* cols[] = {
        "Red",
        "Green",
        "Yellow",
        "Blue",
        "Magenta",
        "Cyan",
        "White"
    };
    int sel = select_menu("Select Shape Color", cols, 7);
    if (sel == -1) return COLOR_IDX_GREEN; // Default to Green if canceled
    return sel + 1;
}

/* Form-based shape adding menu with TUI choice selectors */
void add_shape_menu() {
    const char* types[] = {
        "Line",
        "Rectangle",
        "Circle",
        "Triangle",
        "Back to Main Menu"
    };
    
    int choice = select_menu("Add a Shape (Form Input)", types, 5);
    if (choice == -1 || choice == 4) return;
    
    if (num_shapes >= MAX_SHAPES) {
        printf(COLOR_ERROR "Cannot add more shapes. Canvas is full!\n" COLOR_RESET);
        return;
    }
    
    Shape s;
    s.id = next_id++;
    s.color = choose_color();
    
    // Clear terminal screen for coordinate form fields
    printf("\033[H\033[J");
    printf(COLOR_TITLE "=== Enter Coordinates ===\n" COLOR_RESET);
    
    if (choice == 0) {
        s.type = SHAPE_LINE;
        printf("Entering Line coordinates:\n");
        s.data.line.x1 = read_int("  Start X (0-49): ");
        s.data.line.y1 = read_int("  Start Y (0-19): ");
        s.data.line.x2 = read_int("  End X (0-49): ");
        s.data.line.y2 = read_int("  End Y (0-19): ");
    } else if (choice == 1) {
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
    } else if (choice == 2) {
        s.type = SHAPE_CIRCLE;
        printf("Entering Circle coordinates:\n");
        s.data.circle.cx = read_int("  Center X (0-49): ");
        s.data.circle.cy = read_int("  Center Y (0-19): ");
        s.data.circle.r = read_int("  Radius (>=0): ");
        if (s.data.circle.r < 0) {
            printf(COLOR_ERROR "Radius cannot be negative.\n" COLOR_RESET);
            return;
        }
    } else if (choice == 3) {
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

/* Delete shape menu with visual list picker */
void delete_shape_menu() {
    if (num_shapes == 0) {
        printf("\nNo active shapes to delete.\n");
        return;
    }
    
    int idx = select_shape_from_list("Select a Shape to Delete");
    if (idx == -1) return;
    
    int id = shapes[idx].id;
    
    // Shift elements left to overwrite deleted index
    for (int i = idx; i < num_shapes - 1; i++) {
        shapes[i] = shapes[i + 1];
    }
    num_shapes--;
    printf(COLOR_MENU "Shape ID %d deleted successfully.\n" COLOR_RESET, id);
}

/* Modify shape menu with visual list picker and scrolling prompts */
void modify_shape_menu() {
    if (num_shapes == 0) {
        printf("\nNo active shapes to modify.\n");
        return;
    }
    
    int idx = select_shape_from_list("Select a Shape to Modify");
    if (idx == -1) return;
    
    Shape* s = &shapes[idx];
    
    // Yes/No selector for modifying colors
    const char* col_opts[] = {
        "Keep Current Color",
        "Change Color"
    };
    int change_col = select_menu("Modify Color?", col_opts, 2);
    if (change_col == 1) {
        s->color = choose_color();
    }
    
    // Clear terminal screen for coordinates fields
    printf("\033[H\033[J");
    printf(COLOR_TITLE "=== Modify Coordinates ===\n" COLOR_RESET);

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
        case SHAPE_POINT:
            printf("Modifying Point ID %d (current: (%d, %d))\n", s->id, s->data.point.x, s->data.point.y);
            s->data.point.x = read_int("  New X (0-49): ");
            s->data.point.y = read_int("  New Y (0-19): ");
            break;
        case SHAPE_FLOOD_FILL:
            printf("Modifying Flood Fill ID %d (current: (%d, %d))\n", s->id, s->data.fill.x, s->data.fill.y);
            s->data.fill.x = read_int("  New Start X (0-49): ");
            s->data.fill.y = read_int("  New Start Y (0-19): ");
            break;
    }
    printf(COLOR_MENU "Shape ID %d modified successfully.\n" COLOR_RESET, s->id);
}

/* Interactive Visual Draw Loop */
void interactive_editor() {
    int cursor_x = WIDTH / 2;
    int cursor_y = HEIGHT / 2;
    int active_color = COLOR_IDX_GREEN;
    
    EditState state = STATE_NAVIGATING;
    
    int p1_x = 0, p1_y = 0;
    int p2_x = 0, p2_y = 0;
    
    while (1) {
        printf("\033[H\033[J");
        render_shapes();
        
        // Render current interactive preview overlays
        if (state == STATE_LINE_P1) {
            draw_line(p1_x, p1_y, cursor_x, cursor_y, active_color);
        } else if (state == STATE_RECT_P1) {
            int rx = p1_x < cursor_x ? p1_x : cursor_x;
            int ry = p1_y < cursor_y ? p1_y : cursor_y;
            int rw = abs(cursor_x - p1_x) + 1;
            int rh = abs(cursor_y - p1_y) + 1;
            draw_rect(rx, ry, rw, rh, active_color);
        } else if (state == STATE_CIRCLE_CENTER) {
            int dx = cursor_x - p1_x;
            int dy = cursor_y - p1_y;
            int r = int_sqrt(dx*dx + dy*dy);
            draw_circle(p1_x, p1_y, r, active_color);
        } else if (state == STATE_TRIANGLE_P1) {
            draw_line(p1_x, p1_y, cursor_x, cursor_y, active_color);
        } else if (state == STATE_TRIANGLE_P2) {
            draw_triangle(p1_x, p1_y, p2_x, p2_y, cursor_x, cursor_y, active_color);
        }
        
        printf(COLOR_TITLE "=== INTERACTIVE DRAWING CANVAS ===\n" COLOR_RESET);
        display_canvas(cursor_x, cursor_y, 1);
        
        printf("\n" COLOR_TITLE "Status: " COLOR_RESET);
        printf("Pos: (%d, %d) | ", cursor_x, cursor_y);
        printf("Color: ");
        print_color(active_color);
        printf("[%s]" COLOR_RESET " | ", color_name(active_color));
        printf("Mode: ");
        switch (state) {
            case STATE_NAVIGATING:    printf(COLOR_MENU "NAVIGATION" COLOR_RESET); break;
            case STATE_LINE_P1:       printf(COLOR_SHAPE "LINE DRAFT (Start: %d,%d)" COLOR_RESET, p1_x, p1_y); break;
            case STATE_RECT_P1:       printf(COLOR_SHAPE "RECT DRAFT (Start: %d,%d)" COLOR_RESET, p1_x, p1_y); break;
            case STATE_CIRCLE_CENTER: printf(COLOR_SHAPE "CIRCLE DRAFT (Center: %d,%d)" COLOR_RESET, p1_x, p1_y); break;
            case STATE_TRIANGLE_P1:   printf(COLOR_SHAPE "TRIANGLE DRAFT P2 (P1: %d,%d)" COLOR_RESET, p1_x, p1_y); break;
            case STATE_TRIANGLE_P2:   printf(COLOR_SHAPE "TRIANGLE DRAFT P3 (P1: %d,%d, P2: %d,%d)" COLOR_RESET, p1_x, p1_y, p2_x, p2_y); break;
        }
        printf("\n");
        
        printf(COLOR_CANVAS "Controls: " COLOR_RESET);
        if (state == STATE_NAVIGATING) {
            printf("\n  [WASD / Arrows] Move  |  [C] Cycle Color  |  [Q/Esc] Back to Menu\n");
            printf("  [1] Draw Line         |  [2] Draw Rect     |  [3] Draw Circle\n");
            printf("  [4] Draw Triangle     |  [5] Paint Bucket  |  [6] Toggle Pixel (Point)\n");
        } else {
            printf("\n  [WASD / Arrows] Resize/Move Cursor\n");
            printf("  [Enter / Hotkey] Confirm Shape  |  [Esc / Q] Cancel Drawing\n");
        }
        
        int key = get_key();
        
        if ((key == KEY_ESC || key == 'q' || key == 'Q') && state != STATE_NAVIGATING) {
            state = STATE_NAVIGATING;
            continue;
        }
        
        if (key == KEY_UP || key == 'w' || key == 'W') {
            if (cursor_y > 0) cursor_y--;
        } else if (key == KEY_DOWN || key == 's' || key == 'S') {
            if (cursor_y < HEIGHT - 1) cursor_y++;
        } else if (key == KEY_LEFT || key == 'a' || key == 'A') {
            if (cursor_x > 0) cursor_x--;
        } else if (key == KEY_RIGHT || key == 'd' || key == 'D') {
            if (cursor_x < WIDTH - 1) cursor_x++;
        }
        
        else if (key == 'c' || key == 'C') {
            active_color++;
            if (active_color > COLOR_IDX_WHITE) {
                active_color = COLOR_IDX_RED;
            }
        }
        
        else if (key == 'q' || key == 'Q' || key == KEY_ESC) {
            break;
        }
        
        else if (state == STATE_NAVIGATING) {
            if (key == '1') {
                p1_x = cursor_x;
                p1_y = cursor_y;
                state = STATE_LINE_P1;
            } else if (key == '2') {
                p1_x = cursor_x;
                p1_y = cursor_y;
                state = STATE_RECT_P1;
            } else if (key == '3') {
                p1_x = cursor_x;
                p1_y = cursor_y;
                state = STATE_CIRCLE_CENTER;
            } else if (key == '4') {
                p1_x = cursor_x;
                p1_y = cursor_y;
                state = STATE_TRIANGLE_P1;
            } else if (key == '5') {
                if (num_shapes < MAX_SHAPES) {
                    Shape s;
                    s.id = next_id++;
                    s.type = SHAPE_FLOOD_FILL;
                    s.color = active_color;
                    s.data.fill.x = cursor_x;
                    s.data.fill.y = cursor_y;
                    shapes[num_shapes++] = s;
                }
            } else if (key == '6' || key == KEY_ENTER) {
                int found = -1;
                for (int i = 0; i < num_shapes; i++) {
                    if (shapes[i].type == SHAPE_POINT && shapes[i].data.point.x == cursor_x && shapes[i].data.point.y == cursor_y) {
                        found = i;
                        break;
                    }
                }
                if (found != -1) {
                    for (int i = found; i < num_shapes - 1; i++) {
                        shapes[i] = shapes[i + 1];
                    }
                    num_shapes--;
                } else {
                    if (num_shapes < MAX_SHAPES) {
                        Shape s;
                        s.id = next_id++;
                        s.type = SHAPE_POINT;
                        s.color = active_color;
                        s.data.point.x = cursor_x;
                        s.data.point.y = cursor_y;
                        shapes[num_shapes++] = s;
                    }
                }
            }
        }
        
        else if (state == STATE_LINE_P1) {
            if (key == '1' || key == KEY_ENTER) {
                if (num_shapes < MAX_SHAPES) {
                    Shape s;
                    s.id = next_id++;
                    s.type = SHAPE_LINE;
                    s.color = active_color;
                    s.data.line.x1 = p1_x;
                    s.data.line.y1 = p1_y;
                    s.data.line.x2 = cursor_x;
                    s.data.line.y2 = cursor_y;
                    shapes[num_shapes++] = s;
                }
                state = STATE_NAVIGATING;
            }
        }
        
        else if (state == STATE_RECT_P1) {
            if (key == '2' || key == KEY_ENTER) {
                if (num_shapes < MAX_SHAPES) {
                    Shape s;
                    s.id = next_id++;
                    s.type = SHAPE_RECTANGLE;
                    s.color = active_color;
                    s.data.rect.x = p1_x < cursor_x ? p1_x : cursor_x;
                    s.data.rect.y = p1_y < cursor_y ? p1_y : cursor_y;
                    s.data.rect.w = abs(cursor_x - p1_x) + 1;
                    s.data.rect.h = abs(cursor_y - p1_y) + 1;
                    shapes[num_shapes++] = s;
                }
                state = STATE_NAVIGATING;
            }
        }
        
        else if (state == STATE_CIRCLE_CENTER) {
            if (key == '3' || key == KEY_ENTER) {
                if (num_shapes < MAX_SHAPES) {
                    Shape s;
                    s.id = next_id++;
                    s.type = SHAPE_CIRCLE;
                    s.color = active_color;
                    s.data.circle.cx = p1_x;
                    s.data.circle.cy = p1_y;
                    int dx = cursor_x - p1_x;
                    int dy = cursor_y - p1_y;
                    s.data.circle.r = int_sqrt(dx*dx + dy*dy);
                    shapes[num_shapes++] = s;
                }
                state = STATE_NAVIGATING;
            }
        }
        
        else if (state == STATE_TRIANGLE_P1) {
            if (key == '4' || key == KEY_ENTER) {
                p2_x = cursor_x;
                p2_y = cursor_y;
                state = STATE_TRIANGLE_P2;
            }
        }
        
        else if (state == STATE_TRIANGLE_P2) {
            if (key == '4' || key == KEY_ENTER) {
                if (num_shapes < MAX_SHAPES) {
                    Shape s;
                    s.id = next_id++;
                    s.type = SHAPE_TRIANGLE;
                    s.color = active_color;
                    s.data.tri.x1 = p1_x;
                    s.data.tri.y1 = p1_y;
                    s.data.tri.x2 = p2_x;
                    s.data.tri.y2 = p2_y;
                    s.data.tri.x3 = cursor_x;
                    s.data.tri.y3 = cursor_y;
                    shapes[num_shapes++] = s;
                }
                state = STATE_NAVIGATING;
            }
        }
    }
}

/* Specific Scrolling Main Menu Drawer */
int main_menu_select() {
    const char* options[] = {
        "Enter Interactive Draw Mode (Arrows/WASD)",
        "Add Shape (Form Input)",
        "Delete Shape (Visual Picker)",
        "Modify Shape (Visual Picker)",
        "Clear Canvas (Delete All)",
        "Exit"
    };
    int num_options = 6;
    int active_idx = 0;
    
    while (1) {
        printf("\033[H\033[J");
        
        printf(COLOR_TITLE "=============================================\n");
        printf("          2D TERMINAL GRAPHICS EDITOR        \n");
        printf("=============================================\n" COLOR_RESET);
        
        render_shapes();
        display_canvas(0, 0, 0);
        
        list_shapes();
        
        printf("\n" COLOR_TITLE "--- Menu (Use WASD/Arrows & Enter) ---" COLOR_RESET "\n");
        for (int i = 0; i < num_options; i++) {
            if (i == active_idx) {
                printf("  " COLOR_MENU "-> [ %s ]" COLOR_RESET "\n", options[i]);
            } else {
                printf("     %s  \n", options[i]);
            }
        }
        
        int key = get_key();
        if (key == KEY_UP || key == 'w' || key == 'W') {
            if (active_idx > 0) active_idx--;
        } else if (key == KEY_DOWN || key == 's' || key == 'S') {
            if (active_idx < num_options - 1) active_idx++;
        } else if (key == KEY_ENTER || key == 13) {
            return active_idx;
        }
    }
}

/* Main TUI Loop */
int main() {
    enable_ansi();
    
    while (1) {
        int choice = main_menu_select();
        
        if (choice == 0) {
            interactive_editor();
        } else if (choice == 1) {
            add_shape_menu();
            printf("\nPress Enter to continue...");
            char ch;
            while ((ch = getchar()) != '\n' && ch != EOF);
        } else if (choice == 2) {
            delete_shape_menu();
            printf("\nPress Enter to continue...");
            char ch;
            while ((ch = getchar()) != '\n' && ch != EOF);
        } else if (choice == 3) {
            modify_shape_menu();
            printf("\nPress Enter to continue...");
            char ch;
            while ((ch = getchar()) != '\n' && ch != EOF);
        } else if (choice == 4) {
            const char* clear_opts[] = {
                "Keep Canvas (Cancel)",
                "Delete All Shapes (Clear)"
            };
            int clear_choice = select_menu("Clear Canvas?", clear_opts, 2);
            if (clear_choice == 1) {
                num_shapes = 0;
                printf(COLOR_MENU "All shapes cleared.\n" COLOR_RESET);
            } else {
                printf("Clear aborted.\n");
            }
            printf("\nPress Enter to continue...");
            char ch;
            while ((ch = getchar()) != '\n' && ch != EOF);
        } else if (choice == 5) {
            printf("Goodbye!\n");
            break;
        }
    }
    
    return 0;
}
