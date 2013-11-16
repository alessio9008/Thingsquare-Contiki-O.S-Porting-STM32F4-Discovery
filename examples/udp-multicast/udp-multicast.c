#include "mist.h"

#define PORT 12345
#define MESSAGELEN 15

static struct udp_socket s;
static uip_ipaddr_t addr;
char messaggio[MESSAGELEN];
uint32_t counter=0;

#define SEND_INTERVAL		(5 * CLOCK_SECOND)
static struct etimer periodic_timer, send_timer;

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
  printf("%s\n",data);
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
  while(1) {
  
    /* Set up two timers, one for keeping track of the send interval,
       which is periodic, and one for setting up a randomized send time
       within that interval. */
    etimer_set(&periodic_timer, SEND_INTERVAL);
    etimer_set(&send_timer, (SEND_INTERVAL));

    PROCESS_WAIT_UNTIL(etimer_expired(&send_timer));
    sprintf(messaggio,"D%d",counter);
    counter++;
    printf("SM\n");
    udp_socket_sendto(&s,
                      messaggio, MESSAGELEN,
                      &addr, PORT);

    PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
