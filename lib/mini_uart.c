#include "mini_uart.h"
#include "utils.h"

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

char read_data(){
    int receive_ready = 0;
    while(!receive_ready){
        receive_ready = (*AUX_MU_LSR_REG) & 0x1u;
    }
    return (char)*AUX_MU_IO_REG;
}


char async_read_data(){
    while(async_recv.len == 0){}
    atomic_add((addr_t) &async_recv.len, -1); // len might be modify by rx interrupt
    if(!(*AUX_MU_IER_REG & 0b01)) *AUX_MU_IER_REG |= 0b01; // enable interrupt again if buffer has space
    char data = async_recv.buffer[async_recv.head++];
    async_recv.head %= ASYNC_BUFFER_SIZE;
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
            send_data('\b');
            send_data(' ');
            send_data('\b');
        }
        else{
            send_data(input);
            inputline[writehead++] = input;
        }
    }
    return writehead;
}

void send_data(char c){
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
    atomic_add((addr_t) &async_tran.len, 1);
    if(!(*AUX_MU_IER_REG & 0b10)) *AUX_MU_IER_REG |= 0b10; // enable interrupt has new data to send
}

void send_string(char *str){
    while(*str != '\0'){
        send_data(*str);
        str++;
    }
}

void send_line(char *line){
    while(*line != '\0'){
        send_data(*line);
        line++;
    }
    send_data('\r');
    send_data('\n');
}

void async_send_line(char *line){
    while(*line != '\0'){
        async_send_data(*line);
        line++;
    }
    async_send_data('\r');
    async_send_data('\n');
}


// ----- exception handler * async send/recv -----
void uart_except_handler(){
    disable_aux_interrupt();
    uint32_t interrupt_id = (*(AUX_MU_IIR_REG) >> 1) & 0b11;
    if(interrupt_id == 2){
        while((*AUX_MU_LSR_REG) & 0x1u){
            if(async_recv.len >= ASYNC_BUFFER_SIZE){
                *AUX_MU_IER_REG &= ~0b01;
                break;
            }
            size_t idx = (async_recv.head + async_recv.len) % ASYNC_BUFFER_SIZE;
            async_recv.buffer[idx] = (byte)*AUX_MU_IO_REG;
            async_recv.len++; //TODO: what if len overflow?
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
    else send_line("[unknown mini_uart interrupt!]");
    enable_aux_interrupt();
}