#include "mist.h"
#include "stm32f4xx.h"
//#include "stm32f4_discovery.h"

#define PORT 12345
#define MESSAGELEN 62

static struct udp_socket s;
static uip_ipaddr_t addr;
char messaggio[MESSAGELEN];
uint32_t counter=0;

//#define SEND_INTERVAL		(30 * CLOCK_SECOND)

#define SEND_INTERVAL		3*CLOCK_SECOND
static struct etimer periodic_timer, send_timer;

void prepare_PIN();
void accendi_PIN();
void spegni_PIN();

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
//  printf("%s\n",data);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(multicast_example_process, ev, data)
{
  PROCESS_BEGIN();

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
  for(int i=0;i<(MESSAGELEN-1);i++) messaggio[i]='A';
  messaggio[MESSAGELEN-1]='\0';
  prepare_PIN();
  spegni_PIN();
  
  while(1) {
  
    /* Set up two timers, one for keeping track of the send interval,
       which is periodic, and one for setting up a randomized send time
       within that interval. */
    etimer_set(&periodic_timer, SEND_INTERVAL);
/*    etimer_set(&send_timer, (random_rand() % SEND_INTERVAL));*/
   
    etimer_set(&send_timer, SEND_INTERVAL);

    PROCESS_WAIT_UNTIL(etimer_expired(&send_timer));
    

    //counter++;
//    printf("SM%d\n",counter);
    accendi_PIN();
    
    udp_socket_sendto(&s,messaggio,MESSAGELEN,&addr, PORT);
    
    spegni_PIN();
    
    PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
      
    }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/


void prepare_PIN(){
  

	GPIO_InitTypeDef GPIO_InitStruct;
	//RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5; // we want to configure all LED GPIO pins
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT; 		// we want the pins to be an output
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz; 	// this sets the GPIO modules clock speed
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP; 	// this sets the pin type to push / pull (as opposed to open drain)
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL; 	// this sets the pullup / pulldown resistors to be inactive
	GPIO_Init(GPIOE, &GPIO_InitStruct); 			// this finally passes all the values to the GPIO_Init function which takes care of setting the corresponding bits.

}

void accendi_PIN(){
    GPIOE->BSRRH=GPIO_Pin_5;
}


void spegni_PIN(){
    GPIOE->BSRRL=GPIO_Pin_5;
}