#include "exception/exception.h"
#include "mini_uart.h"
#include "utils.h"
#include "str_utils.h"

#define ASYNC_BUFFER_SIZE 2048u

typedef struct{
    byte buffer[ASYNC_BUFFER_SIZE];
    size_t head;
    size_t len;
}AsyncBuf;

AsyncBuf async_recv;
AsyncBuf async_tran;

void enable_aux_interrupt() {
    *ENABLE_IRQs1 = (1u << 29);
    get_curr_workload()->uart_mask = false;
}

void disable_aux_interrupt() {
    *DISABLE_IRQs1 = (1u << 29);
    get_curr_workload()->uart_mask = true;
}

void init_uart(){
    //set GPFSEL1
    unsigned int fsel_config = *GPFSEL1;
    unsigned int alt5 = 2u;
    fsel_config &= ~(63u << 12); 
    fsel_config |= (alt5 << 12) | (alt5 << 15);
    *GPFSEL1 = fsel_config;

    //GPPUD &
    unsigned int pud_config = *GPPUD;
    *GPPUD = 0x0u;
    delay(150);
    *GPPUD_CLK0 = 0x11u << 14;
    delay(150);
    *GPPUD = 0x0u;
    *GPPUD_CLK0 = 0x0u;

    *AUXENB = 0x01u;
    *AUX_MU_CNTL_REG = 0x0u;
    *AUX_MU_IER_REG = 0x3u;
    *AUX_MU_LCR_REG = 0x03u;
    *AUX_MU_MCR_REG = 0x0u;
    *AUX_MU_BAUD_REG = 270u;
    *AUX_MU_IIR_REG = 6u;
    *AUX_MU_CNTL_REG = 3u;

    enable_aux_interrupt(); // enable auxiliary interrupt(including mini_uart)

    async_recv.head = 0;
    async_recv.len = 0;
    async_tran.head = 0;
    async_tran.len = 0;
}

char sync_read_data(){
    int receive_ready = 0;
    while(!receive_ready){
        receive_ready = (*AUX_MU_LSR_REG) & 0x1u;
    }
    return (char)*AUX_MU_IO_REG;
}


char async_read_data(){
    while(async_recv.len == 0){}
    async_recv.len--; // len might be modify by rx interrupt if not atomic
    // atomic_add((addr_t) &async_recv.len, -1); //TODO: ldxr cause translation fault
    char data = async_recv.buffer[async_recv.head++];
    async_recv.head %= ASYNC_BUFFER_SIZE;
    if(!(*AUX_MU_IER_REG & 0b01)) *AUX_MU_IER_REG |= 0b01; // enable interrupt again if buffer has space
    return data;
}

int echo_read_line(char *inputline){
    //TODO: length checking
    int writehead = 0;
    while(1){
        char input = async_read_data();
        if(input == '\r'){ //enter pressed
            inputline[writehead++] = '\0';
            send_string("\r\n");
            break;
        }
        else if(input == 127){ // delete signal
            if(writehead == 0) continue;
            inputline[--writehead] = '\0';
            async_send_data('\b');
            async_send_data(' ');
            async_send_data('\b');
        }
        else{
            async_send_data(input);
            inputline[writehead++] = input;
        }
    }
    return writehead;
}

size_t read_to_buf(char *buffer, size_t size){
    size_t total_read = 0;
    while(total_read < size){
        buffer[total_read++] = async_read_data();
        if(async_recv.len == 0) break;
    }
    return total_read;
}

void sync_send_data(char c){
    int transmit_ready = 0;
    while(!transmit_ready){
        transmit_ready = (*AUX_MU_LSR_REG) & (0x1u << 5);
    }
    *AUX_MU_IO_REG = (unsigned int)c;
}

void async_send_data(char c){
    while(async_tran.len >= ASYNC_BUFFER_SIZE){}
    size_t idx = (async_tran.head + async_tran.len) % ASYNC_BUFFER_SIZE; 
    async_tran.buffer[idx] = c;
    async_tran.len++; // len might be modify by rx interrupt if not atomic
    // atomic_add((addr_t) &async_tran.len, 1); //TODO: ldxr cause translation fault
    if(!(*AUX_MU_IER_REG & 0b10)) *AUX_MU_IER_REG |= 0b10; // new data to send, enable interrupt
}

//default async sending
void send_string(char *str){
        _send_string_(str, async_send_data);
}

void _send_string_(char *str, void (*send_func)(char)){
    while(*str != '\0'){
        send_func(*str);
        str++;
    }
}

//default async sending
void send_line(char *line){
    _send_line_(line, async_send_data);
}

size_t send_from_buf(char *buffer, size_t size){
    size_t total_send = 0;
    while(total_send < size){
        sync_send_data(buffer[total_send++]);
        if(async_tran.len == ASYNC_BUFFER_SIZE) break;
    }
    return total_send;
}


void _send_line_(char *line, void (*send_func)(char)){
    _send_string_(line, send_func);
    send_func('\r');
    send_func('\n');
}

// ----- exception handler * async send/recv -----
void uart_except_handler(uint64_t *trap_frame){
    // disable_aux_interrupt(); already disable by exception handler
    uint32_t interrupt_id = (*(AUX_MU_IIR_REG) >> 1) & 0b11;
    if(interrupt_id == 2){
        while((*AUX_MU_LSR_REG) & 0x1u){
            if(async_recv.len >= ASYNC_BUFFER_SIZE){
                *AUX_MU_IER_REG &= ~0b01;
                break;
            }
            size_t idx = (async_recv.head + async_recv.len) % ASYNC_BUFFER_SIZE;
            async_recv.buffer[idx] = (byte)*AUX_MU_IO_REG;
            // // TODO: what if len overflow?
            //it won't since len can't >= ASYNC_BUFFER_SIZE
            async_recv.len++;
        }
    }
    else if(interrupt_id == 1){
        while((*AUX_MU_LSR_REG) & (0x1u << 5)){
            if(async_tran.len <= 0) {
                *AUX_MU_IER_REG &= ~0b10;
                break;
            }

            *AUX_MU_IO_REG = async_tran.buffer[async_tran.head];
            async_tran.head = (async_tran.head + 1) % ASYNC_BUFFER_SIZE;
            async_tran.len--;
        }
    }
    else{
        char temp[16];
        _send_string_("[unknown mini_uart interrupt!]: ", sync_send_data);
        _send_line_(itoa(interrupt_id, temp, DEC), sync_send_data);
    }

    enable_aux_interrupt();
}