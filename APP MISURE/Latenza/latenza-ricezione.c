#include "mist.h"
#include "stm32f4xx.h"

#define PORT 12345

static struct udp_socket s;
static uip_ipaddr_t addr;
char messaggio[15];
uint32_t counter=0,counter_received=0,t_start=0,t_end=0,t_latency=0;
double t_latencySec=0;
EXTI_InitTypeDef   EXTI_InitStructure;

void EXTILine0_Config(void);

#define SECONDS_STX 1
#define SEND_INTERVAL		(SECONDS_STX * CLOCK_SECOND)
static struct etimer periodic_timer, send_timer, receive_timer;

/*---------------------------------------------------------------------------*/
PROCESS(multicast_example_process, "Link local multicast example process");
AUTOSTART_PROCESSES(&multicast_example_process);
/*---------------------------------------------------------------------------*/
static void
receiver(struct udp_socket *c,
         void *ptr,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  /*printf("Data received on port %d from port %d with length %d, '%s'\n",
         receiver_port, sender_port, datalen, data);
  */
  // printf("%s\n",data);
  //counter_received++;
  t_end=clock_time();
  t_latency=t_end-t_start;
  t_latencySec=((double)t_latency)/CLOCK_SECOND;
  printf("%.10lf ms\n",t_latencySec*1000);
  
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(multicast_example_process, ev, data)
{
  double pckrate=0,datarate=0;
  PROCESS_BEGIN();
  EXTILine0_Config();
  /* Create a linkl-local multicast addresses. */
  uip_ip6addr(&addr, 0xff02, 0, 0, 0, 0, 0, 0x1337, 0x0001);

  /* Join local group. */
  if(uip_ds6_maddr_add(&addr) == NULL) {
    printf("Error: could not join local multicast group.\n");
  }

  /* Register UDP socket callback */
  udp_socket_register(&s, NULL, receiver);

  /* Bind UDP socket to local port */
  udp_socket_bind(&s, PORT);

  /* Connect UDP socket to remote port */
  udp_socket_connect(&s, NULL, PORT);
  printf("Pacchetti_Ricevuti;PckPerSecond;BitRate(kbps)\n");
  while(1) {
  
    /* Set up two timers, one for keeping track of the send interval,
       which is periodic, and one for setting up a randomized send time
       within that interval. */
    etimer_set(&periodic_timer, SEND_INTERVAL);
    //etimer_set(&send_timer, (random_rand() % SEND_INTERVAL));
    etimer_set(&receive_timer, (5*CLOCK_SECOND));
    //PROCESS_WAIT_UNTIL(etimer_expired(&receive_timer));
    PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
    pckrate=((double)counter_received)/SECONDS_STX;
    datarate=pckrate*70*8/1000;
    //printf("%d;%3.3lf;%3.3lf\n",counter_received,pckrate,datarate);
    counter_received=0;
    counter++;
    /*sprintf(messaggio,"D%d",counter);
    counter++;
    printf("SM\n");
    udp_socket_sendto(&s,
                      messaggio, 15,
                      &addr, PORT);
    */

    
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

void EXTILine0_Config(void)
{
  
  GPIO_InitTypeDef   GPIO_InitStructure;
  NVIC_InitTypeDef   NVIC_InitStructure;

  /* Enable GPIOA clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  /* Enable SYSCFG clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  
  /* Configure PA0 pin as input floating */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Connect EXTI Line0 to PA0 pin */
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);

  /* Configure EXTI Line0 */
  EXTI_InitStructure.EXTI_Line = EXTI_Line0;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  /* Enable and set EXTI Line0 Interrupt to the lowest priority */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

void EXTI0_IRQHandler(void)
{
  if(EXTI_GetITStatus(EXTI_Line0) != RESET)
  {
    t_start=clock_time();
    /* Clear the EXTI line 0 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line0);
  }
}