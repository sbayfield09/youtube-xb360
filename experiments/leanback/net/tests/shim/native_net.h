#ifndef LB_HOST_RAW_SHIM_H
#define LB_HOST_RAW_SHIM_H
#include <stdint.h>
#include <stddef.h>
#include "protocol.h"
typedef int err_t;
typedef uint16_t u16_t;
typedef struct {uint32_t addr;} ip_addr_t;
struct pbuf {struct pbuf *next;void *payload;u16_t len,tot_len;};
struct tcp_pcb;
struct udp_pcb;
typedef err_t (*rx_fn)(void *,struct tcp_pcb *,struct pbuf *,err_t);
typedef err_t (*connect_fn)(void *,struct tcp_pcb *,err_t);
typedef void (*error_fn)(void *,err_t);
typedef void (*dns_fn)(void *,struct udp_pcb *,struct pbuf *,ip_addr_t *,u16_t);
#define ERR_OK 0
#define ERR_MEM -1
#define ERR_RST -14
#define TCP_WRITE_FLAG_COPY 1
#define PBUF_TRANSPORT 0
#define PBUF_RAM 0
#define IP_ADDR_ANY NULL
#define IP4_ADDR(p,a,b,c,d) ((p)->addr=((uint32_t)(a)<<24)|((uint32_t)(b)<<16)|((uint32_t)(c)<<8)|(d))
#define ip_addr_cmp(a,b) ((a)->addr==(b)->addr)
int ipaddr_aton(const char *,ip_addr_t *);
struct tcp_pcb *tcp_new(void);
void tcp_arg(struct tcp_pcb *,void *);
void tcp_recv(struct tcp_pcb *,rx_fn);
void tcp_err(struct tcp_pcb *,error_fn);
void tcp_sent(struct tcp_pcb *,void *);
void tcp_abort(struct tcp_pcb *);
unsigned tcp_sndbuf(struct tcp_pcb *);
err_t tcp_write(struct tcp_pcb *,const void *,u16_t,int);
err_t tcp_output(struct tcp_pcb *);
void tcp_recved(struct tcp_pcb *,u16_t);
err_t tcp_connect(struct tcp_pcb *,ip_addr_t *,u16_t,connect_fn);
struct udp_pcb *udp_new(void);
void udp_remove(struct udp_pcb *);
err_t udp_bind(struct udp_pcb *,ip_addr_t *,u16_t);
void udp_recv(struct udp_pcb *,dns_fn,void *);
err_t udp_sendto(struct udp_pcb *,struct pbuf *,ip_addr_t *,u16_t);
struct pbuf *pbuf_alloc(int,u16_t,int);
err_t pbuf_take(struct pbuf *,const void *,u16_t);
u16_t pbuf_copy_partial(struct pbuf *,void *,u16_t,u16_t);
void pbuf_free(struct pbuf *);
int xf_network_ready(void);
int xf_network_start(uint32_t);
int xf_network_available(void);
const char *xf_network_status(void);
void shim_poll(void);
#endif
