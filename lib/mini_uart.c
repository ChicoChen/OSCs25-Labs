#include "utils.h"
#include "mini_uart.h"

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
    *AUX_MU_IER_REG = 0x0u;
    *AUX_MU_LCR_REG = 0x03u;
    *AUX_MU_MCR_REG = 0x0u;
    *AUX_MU_BAUD_REG = 270u;
    *AUX_MU_IIR_REG = 6u;
    *AUX_MU_CNTL_REG = 3u;
}

char read_data(){
    int receive_ready = 0;
    while(!receive_ready){
        receive_ready = (*AUX_MU_LSR_REG) & 0x1u;
    }
    return (char)*AUX_MU_IO_REG;
}

int echo_read_line(char *inputline){
    //TODO: length checking
    int writehead = 0;
    while(1){
        char input = read_data();
        if(input == '\r'){ //enter pressed
            inputline[writehead++] = '\0';
            send_string("\r\n");
            break;
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
        transmit_ready = ((*AUX_MU_LSR_REG) >> 5) & (0x1u);
    }
    *AUX_MU_IO_REG = (unsigned int)c;
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