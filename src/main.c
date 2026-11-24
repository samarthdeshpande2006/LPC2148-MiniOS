/* LPC2148 MiniOS - ARM Compiler 5 / Keil uVision
 * Requires official LPC2148 startup (correct PLL, VPBDIV, IRQ vector).
 * Hardware assumptions: PCLK=60MHz, P0.10 active-high LED,
 * LM35 output -> P0.28/AD0.1 (0-3.3V ONLY), UART0 P0.0/P0.1 9600 8N1.
 * Optional active-high buzzer driver on P0.11. DO NOT drive a buzzer directly
 * unless its load is within GPIO specifications; use a transistor driver.
 */
#include <LPC214x.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PCLK_HZ 60000000UL
#define BAUD 9600UL
#define LED (1UL<<10)
#define BUZZER (1UL<<11)
#define ADC_CH 1UL
#define ADC_REF_MV 3300UL
#define TEMP_ALARM_C10 350UL   /* 35.0 C */
#define TASKS 4U
#define CMD_MAX 48U

typedef enum { WAITING, READY, RUNNING, SUSPENDED } State;
typedef struct {
 unsigned char id, priority;
 void (*run)(void);
 unsigned long period, due, runs;
 State state;
} Task;

static volatile unsigned long tick_ms;
static volatile unsigned long idle_count;
static unsigned int temperature_c10;
static unsigned int adc_valid;
static unsigned char led_state;
static char command_line[CMD_MAX];
static unsigned int command_len;

static void uart_task(void);
static void temperature_task(void);
static void led_task(void);
static void diagnostics_task(void);

static Task task[TASKS] = {
 {1,0,uart_task,10UL,10UL,0UL,WAITING},
 {2,1,temperature_task,500UL,500UL,0UL,WAITING},
 {3,2,led_task,500UL,500UL,0UL,WAITING},
 {4,3,diagnostics_task,2000UL,2000UL,0UL,WAITING}
};

/* ARM7 aligned 32-bit loads/stores are atomic; only the ISR writes tick_ms. */
static unsigned long millis(void) { return tick_ms; }
static int reached(unsigned long now, unsigned long due) {
 return (long)(now-due)>=0L; /* safe for intervals < 2^31 ms */
}
static void putc0(char c) {
 while (!(U0LSR & (1UL<<5))) { }
 U0THR=(unsigned char)c;
}
static void puts0(const char *s) {
 while (*s) { if (*s=='\n') putc0('\r'); putc0(*s++); }
}
static void uart_init(void) {
 unsigned long divisor=(PCLK_HZ+(8UL*BAUD))/(16UL*BAUD);
 PINSEL0=(PINSEL0 & ~0x0FUL)|0x05UL;
 U0LCR=0x83UL;
 U0DLL=(unsigned char)(divisor & 255UL);
 U0DLM=(unsigned char)((divisor>>8)&255UL);
 U0FDR=0x10UL;
 U0LCR=0x03UL;
 U0FCR=0x07UL;
}

/* Keil ARMCC5 IRQ handler; official startup must dispatch VIC IRQs. */
void timer0_irq(void) __irq {
 if (T0IR & 1UL) { ++tick_ms; T0IR=1UL; }
 VICVectAddr=0UL;
}
static void timer_init(void) {
 T0TCR=2UL;
 T0PR=59UL;          /* 60MHz / 60 = 1MHz TC tick */
 T0MR0=999UL;        /* TC counts 0..999: 1ms per match */
 T0MCR=3UL;
 T0IR=0xFFUL;
 VICIntEnClr=(1UL<<4);
 VICIntSelect &= ~(1UL<<4);  /* Timer0 as IRQ, not FIQ */
 VICVectAddr0=(unsigned long)timer0_irq;
 VICVectCntl0=0x20UL|4UL;
 VICIntEnable=(1UL<<4);
 T0TCR=1UL;
}

static void adc_init(void) {
 /* P0.28 = AD0.1; PINSEL1 bits 25:24 = 01 */
 PINSEL1=(PINSEL1 & ~(3UL<<24)) | (1UL<<24);
 /* ADC clk = 60MHz/(14+1)=4MHz, within LPC2148 ADC specification */
 AD0CR=(1UL<<ADC_CH)|(14UL<<8)|(1UL<<21);
}
static unsigned int adc_read(unsigned int *out) {
 unsigned long raw;
 unsigned long timeout=100000UL;
 AD0CR=(AD0CR & ~(7UL<<24)) | (1UL<<24);
 do { raw=AD0DR1; if (raw & (1UL<<31)) break; }
 while (--timeout);
 AD0CR &= ~(7UL<<24);
 if (!(raw & (1UL<<31)) || (raw & (1UL<<30))) return 0U;
 *out=(unsigned int)((raw>>6)&0x3FFUL);
 return 1U;
}
static void temperature_task(void) {
 unsigned int sample;
 unsigned long mv;
 if (adc_read(&sample)) {
  /* LM35: 10mV / degree C. c10 = mV for tenths of a degree. */
  mv=((unsigned long)sample*ADC_REF_MV+511UL)/1023UL;
  temperature_c10=(unsigned int)mv;
  adc_valid=1U;
  if (temperature_c10 >= TEMP_ALARM_C10) IO0SET=BUZZER;
  else IO0CLR=BUZZER;
 } else {
  adc_valid=0U;
  IO0CLR=BUZZER;
 }
}
static void led_task(void) {
 led_state^=1U;
 if (led_state) IO0SET=LED; else IO0CLR=LED;
}
static const char *state_str(State s) {
 switch(s) {case WAITING:return "WAIT"; case READY:return "READY";
 case RUNNING:return "RUN"; default:return "SUSP";}
}
static void status(void) {
 unsigned int i;
 char b[100];
 sprintf(b,"\nUptime=%lu ms; idle iterations=%lu\n",millis(),idle_count);
 puts0(b);
 for (i=0;i<TASKS;++i) {
  sprintf(b,"Task %u: period=%lu ms priority=%u state=%s runs=%lu\n",
   (unsigned int)task[i].id,task[i].period,
   (unsigned int)task[i].priority,state_str(task[i].state),task[i].runs);
  puts0(b);
 }
 if (adc_valid) {
  sprintf(b,"LM35=%u.%u C, alarm threshold=35.0 C\n",
   temperature_c10/10U,temperature_c10%10U);
  puts0(b);
 } else puts0("LM35: no valid ADC sample yet\n");
 puts0("Idle iterations are not CPU utilization.\n");
}
static void command(const char *s) {
 unsigned int id=0U,i;
 int resume=0;
 const char *p=0;
 if (!strcmp(s,"help")) {
  puts0("Commands: help, status, suspend N, resume N, temp\n");return;
 }
 if (!strcmp(s,"status")) {status();return;}
 if (!strcmp(s,"temp")) {
  char b[64];
  if (!adc_valid) {puts0("Temperature unavailable\n");return;}
  sprintf(b,"Temperature: %u.%u C\n",temperature_c10/10U,temperature_c10%10U);
  puts0(b);return;
 }
 if (!strncmp(s,"suspend ",8)) p=s+8;
 else if (!strncmp(s,"resume ",7)) {p=s+7;resume=1;}
 else {puts0("Unknown command; type help\n");return;}
 /* Reject invalid formats, not just atoi('abc') -> zero. */
 if (*p<'1'||*p>'4'||p[1]!='\0') {
  puts0("Use a single task number from 1 to 4\n");return;
 }
 id=(unsigned int)(*p-'0');
 if (id==1U && !resume) {
  puts0("UART task cannot be suspended from UART\n");return;
 }
 for(i=0;i<TASKS;++i) if(task[i].id==id) {
  if (resume) {
   task[i].due=millis()+task[i].period;
   task[i].state=WAITING;
   puts0("Resumed\n");
  } else {
   task[i].state=SUSPENDED;
   if(id==2U) IO0CLR=BUZZER;
   puts0("Suspended\n");
  }
  return;
 }
}
static void uart_task(void) {
 unsigned int count=0U;
 while ((U0LSR&1UL) && count<16U) {
  char c=(char)U0RBR; ++count;
  if(c=='\r'||c=='\n') {
   if(command_len) {command_line[command_len]='\0';command(command_line);command_len=0U;}
  } else if(c=='\b'||c==127) {
   if(command_len) --command_len;
  } else if(c>=32 && c<=126) {
   if(command_len<CMD_MAX-1U) command_line[command_len++]=c;
   else {command_len=0U;puts0("Input too long; cleared\n");}
  }
 }
}
static void diagnostics_task(void) {
 char b[96];
 sprintf(b,"Heartbeat %lu ms; LED=%u; sensor=%s\n",millis(),
  (unsigned int)led_state,adc_valid?"OK":"WAIT/ERROR");
 puts0(b);
}
static void schedule_once(void) {
 unsigned int i,choice=TASKS;
 unsigned long now=millis();
 for(i=0;i<TASKS;++i) {
  if(task[i].state==WAITING && reached(now,task[i].due)) task[i].state=READY;
  if(task[i].state==READY &&
   (choice==TASKS || task[i].priority<task[choice].priority)) choice=i;
 }
 if(choice==TASKS) {++idle_count;return;}
 task[choice].state=RUNNING;
 task[choice].run();
 ++task[choice].runs;
 now=millis();
 /* No burst catch-up after a delayed task. */
 do {task[choice].due+=task[choice].period;}
 while(reached(now,task[choice].due));
 if(task[choice].state==RUNNING) task[choice].state=WAITING;
}
int main(void) {
 /* Official startup must set CCLK/PCLK=60MHz and enable IRQs. */
 PINSEL0 &= ~((3UL<<20)|(3UL<<22)); /* P0.10, P0.11 GPIO */
 IO0DIR |= LED|BUZZER;
 IO0CLR=LED|BUZZER;
 uart_init();
 adc_init();
 timer_init();
 puts0("\nLPC2148 MiniOS - 4 task cooperative scheduler\nType help\n");
 while(1) schedule_once();
}
