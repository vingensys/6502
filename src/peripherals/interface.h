#ifndef INC_6502_INTERFACE_H
#define INC_6502_INTERFACE_H

#define WIN_ROWS 35
#define WIN_COLS 170

enum ui_mode {
    DEBUGGER_MODE,
    UART_TERMINAL_MODE
};

void interface_display_cpu(void);
void interface_display_mem(void);
void interface_display_header(void);
void interface_display_uart_terminal(void);
void interface_shutdown(void);
void interface_handle_resize(void);
void interface_next_mem_view(void);
void interface_prev_mem_view(void);
void interface_scroll_mem_view(int rows);
void interface_mem_view_home(void);
void interface_mem_view_end(void);
enum ui_mode interface_get_mode(void);
void interface_set_mode(enum ui_mode mode);

#endif
