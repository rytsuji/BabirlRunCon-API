#define AT_NORM    0x00000

/* attribute */
#define AT_BOLD    0x00001
#define AT_UNDER   0x00002
#define AT_BLINK   0x00004
#define AT_REVERSE 0x00008

/* foreground color */
#define FG_BLACK   0x00010
#define FG_RED     0x00020
#define FG_GREEN   0x00040
#define FG_YELLOW  0x00080
#define FG_BLUE    0x00100
#define FG_MAGENTA 0x00200
#define FG_CYAN    0x00400
#define FG_WHITE   0x00800

/* background color */
#define BG_BLACK   0x01000
#define BG_RED     0x02000
#define BG_GREEN   0x04000
#define BG_YELLOW  0x08000
#define BG_BLUE    0x10000
#define BG_MAGENTA 0x20000
#define BG_CYAN    0x40000
#define BG_WHITE   0x80000

void cl_screen(void);
void cl_line(void);
void cl_cur_end(void);
void nx_line(void);
void line_cur(int n);
void mv_cur(int x,int y);
void up_cur(int n);
void down_cur(int n);
void right_cur(int n);
void left_cur(int n);
void cprintf(int att,char *fmt,...);
void csprintf(char *str, int att,char *fmt,...);
void save_cur(void);
void restore_cur(void);
