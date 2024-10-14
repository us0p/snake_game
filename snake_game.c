#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/termios.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_INTERVAL 150 * 1000
#define M_ROWS 20
#define M_COLS 30

typedef enum { SNAKE, APPLE } ItemType;

typedef struct Position {
  int x;
  int y;
} Position;

typedef struct Element {
  ItemType t;
  Position *p;
} Element;

typedef struct Node {
  struct Node *next;
  Element *val;
} Node;

typedef struct Snake {
  int len;
  Node *head;
  Node *tail;
} Snake;

typedef enum {
  UP = 'k',
  RIGHT = 'l',
  DOWN = 'j',
  LEFT = 'h',
} Movement;

struct termios redirect_input() {
  struct termios oldt, newt;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  newt.c_cc[VMIN] = 1;
  newt.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  return oldt;
}

Position *create_position(int x, int y) {
  Position *p = (Position *)malloc(sizeof(Position));
  p->x = x;
  p->y = y;
  return p;
}

Element *create_element(ItemType t, Position *p) {
  Element *el = (Element *)malloc(sizeof(Element));
  el->t = t;
  el->p = p;
  return el;
}

void append_snake_element(Snake *s, Element *el) {
  if (el->t != SNAKE)
    return;

  Node *n = (Node *)malloc(sizeof(Node));
  n->next = NULL;
  n->val = el;

  if (!s->len) {
    s->head = n;
    s->tail = n;
  } else {
    s->tail->next = n;
    s->tail = n;
  }
  s->len++;
}

Snake *create_snake() {
  Snake *s = (Snake *)malloc(sizeof(Snake));
  for (int i = 0; i < 3; i++) {
    Position *p = create_position(14 - i, 10);
    Element *el = create_element(SNAKE, p);
    append_snake_element(s, el);
  }
  return s;
}

void position_snake(Snake *s, Element *f[20][30]) {
  for (Node *n = s->head; n; n = n->next) {
    f[n->val->p->y][n->val->p->x] = n->val;
  }
}

void get_movement(Movement *m) {
  char buffer;
  if (read(STDIN_FILENO, &buffer, 1) > 0) {
    if (buffer == *m) {
      return;
    }
    switch (buffer) {
    case LEFT:
      if (*m != RIGHT) {
        *m = LEFT;
      }
      break;
    case UP:
      if (*m != DOWN) {
        *m = UP;
      }
      break;
    case RIGHT:
      if (*m != LEFT) {
        *m = RIGHT;
      }
      break;
    case DOWN:
      if (*m != UP) {
        *m = DOWN;
      }
      break;
    default:
      return;
    }
  }
}

void update_position(Position *p, Movement m) {
  switch (m) {
  case UP:
    p->y = (p->y - 1) % 20;
    p->y = p->y >= 0 ? p->y : 19;
    break;
  case RIGHT:
    p->x = (p->x + 1) % 30;
    break;
  case DOWN:
    p->y = (p->y + 1) % 20;
    break;
  case LEFT:
    p->x = (p->x - 1) % 30;
    p->x = p->x >= 0 ? p->x : 29;
    break;
  default:
    return;
  }
}

Position *get_new_snake_element_position(Snake *s, Element *m[M_ROWS][M_COLS]) {
  Position *tail = s->tail->val->p;
  Position *prev;
  int xr = (tail->x + 1) % 30;
  int xl = tail->x - 1 < 0 ? 29 : tail->x - 1;
  int yu = tail->y - 1 < 0 ? 19 : tail->y - 1;
  int yd = (tail->y + 1) % 20;
  if (m[tail->y][xr] && m[tail->y][xr]->t == SNAKE) {
    prev = m[tail->y][xr]->p;
  }
  if (m[tail->y][xl] && m[tail->y][xl]->t == SNAKE) {
    prev = m[tail->y][xl]->p;
  }
  if (m[yu][tail->x] && m[yu][tail->x]->t == SNAKE) {
    prev = m[yu][tail->x]->p;
  }
  if (m[yd][tail->x] && m[yd][tail->x]->t == SNAKE) {
    prev = m[yd][tail->x]->p;
  }
  int x, y;
  if (tail->x == prev->x) {
    if (prev->y > tail->y) {
      y = tail->y - 1 < 0 ? 19 : tail->y - 1;
    } else {
      y = (tail->y + 1) % 20;
    }
    x = tail->x;
  } else {
    if (prev->x > tail->x) {
      x = tail->x - 1 < 0 ? 29 : tail->x - 1;
    } else {
      x = (tail->x + 1) % 30;
    }
    y = tail->y;
  }

  return create_position(x, y);
}

void generate_apple(Element *m[M_ROWS][M_COLS]) {
  while (true) {
    int x = rand() % 30;
    int y = rand() % 20;
    if (!m[y][x] || m[y][x]->t != SNAKE) {
      Position *p = create_position(x, y);
      Element *el = create_element(APPLE, p);
      m[y][x] = el;
      break;
    }
  }
}

bool eat_apple(Snake *s, Element *m[M_ROWS][M_COLS]) {
  Position *p = s->head->val->p;
  Element *el = m[p->y][p->x];
  if (el && el->t == APPLE) {
    Position *sp = get_new_snake_element_position(s, m);
    Element *el = create_element(SNAKE, sp);
    append_snake_element(s, el);
    return true;
  }
  return false;
}

bool has_colision(Snake *s, Element *m[M_ROWS][M_COLS]) {
  Position *p = s->head->val->p;
  Element *el = m[p->y][p->x];
  if (el && el->t == SNAKE)
    return true;
  return false;
}

void update_matrix_cell(Element *m[M_ROWS][M_COLS], Position *p, Element *val) {
  m[p->y][p->x] = val;
}

bool move_snake(Snake *s, Movement m, Element *matrix[M_ROWS][M_COLS]) {
  Position p = *s->head->val->p;
  update_matrix_cell(matrix, &p, NULL);
  update_position(s->head->val->p, m);
  bool has_colided = has_colision(s, matrix);
  if (has_colided) {
    return has_colided;
  }
  bool has_eaten_apple = eat_apple(s, matrix);
  update_matrix_cell(matrix, s->head->val->p, s->head->val);
  if (has_eaten_apple) {
    generate_apple(matrix);
  }

  for (Node *n = s->head->next; n; n = n->next) {
    Position tmp = *n->val->p;
    update_matrix_cell(matrix, &tmp, NULL);
    n->val->p->x = p.x;
    n->val->p->y = p.y;
    update_matrix_cell(matrix, n->val->p, n->val);
    p = tmp;
  }
  return 0;
}

void render_field(Element *m[M_ROWS][M_COLS]) {
  for (int i = 0; i < M_ROWS; i++) {
    for (int j = 0; j < M_COLS; j++) {
      Element *el = m[i][j];
      if (el) {
        switch (el->t) {
        case SNAKE:
          printf("o");
          break;
        case APPLE:
          printf("a");
          break;
        }
        continue;
      }
      printf("_");
    }
    printf("\n");
  }
}

void clear_field() {
  for (int i = 0; i < 20; i++) {
    printf("\033[A");
    printf("\r");
    for (int j = 0; j < 30; j++) {
      printf(" ");
    }
    printf("\r");
  }
}

void streamline_matrix(Element *matrix[M_ROWS][M_COLS]) {
  for (int i = 0; i < M_ROWS; i++) {
    for (int j = 0; j < M_COLS; j++) {
      matrix[i][j] = NULL;
    }
  }
}

int main(void) {
  srand(time(NULL));
  struct termios old_config = redirect_input();
  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

  Movement m = RIGHT;
  Element *matrix[M_ROWS][M_COLS];
  streamline_matrix(matrix);
  Snake *s = create_snake();
  position_snake(s, matrix);

  generate_apple(matrix);

  while (true) {
    get_movement(&m);
    bool has_colided = move_snake(s, m, matrix);
    if (has_colided) {
      printf("You scored %d!\n", s->len - 3);
      break;
    }
    eat_apple(s, matrix);
    render_field(matrix);
    clear_field();
    usleep(DEFAULT_INTERVAL);
  }
  printf("\n");
  tcsetattr(STDIN_FILENO, TCSANOW, &old_config);
}
