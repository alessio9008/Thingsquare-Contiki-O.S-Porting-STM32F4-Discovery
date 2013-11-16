#include "mist.h"

#define PORT 12345

static struct udp_socket s;
static uip_ipaddr_t addr;
char messaggio[15];
uint32_t counter=0,counter_received=0;

#define SECONDS_STX 30
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
  counter_received++;
  
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(multicast_example_process, ev, data)
{
  double pckrate=0,datarate=0;
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
    printf("%d;%3.3lf;%3.3lf\n",counter_received,pckrate,datarate);
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
