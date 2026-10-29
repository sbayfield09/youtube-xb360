/* Host-only transport adapter. No production DNS or lwIP implementation.
 * Runs the actual probe.c state machine against a local TLS fixture server. */
#define _POSIX_C_SOURCE 200809L
#include "native_net.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
struct tcp_pcb {int fd,pending,closed;void *arg;rx_fn recv;error_fn error;connect_fn connected;size_t tx_size;unsigned char tx[65536];};
struct udp_pcb {void *arg;dns_fn recv;struct pbuf *pending;ip_addr_t server;};
static struct tcp_pcb *current;
static struct udp_pcb *dns;
struct pbuf *pbuf_alloc(int layer,u16_t n,int type) {
    struct pbuf *p=calloc(1,sizeof *p);(void)layer;(void)type;assert(p);
    p->payload=malloc(n);assert(p->payload);p->len=p->tot_len=n;return p;
}
void pbuf_free(struct pbuf *p){free(p->payload);free(p);}
err_t pbuf_take(struct pbuf *p,const void *data,u16_t n){memcpy(p->payload,data,n);return 0;}
u16_t pbuf_copy_partial(struct pbuf *p,void *data,u16_t n,u16_t offset){assert(offset+n<=p->len);memcpy(data,(char *)p->payload+offset,n);return n;}
int ipaddr_aton(const char *s,ip_addr_t *a){struct in_addr p;if(inet_pton(AF_INET,s,&p)!=1)return 0;a->addr=ntohl(p.s_addr);return 1;}
struct tcp_pcb *tcp_new(void){assert(!current);current=calloc(1,sizeof *current);current->fd=-1;return current;}
void tcp_arg(struct tcp_pcb *p,void *v){p->arg=v;}
void tcp_recv(struct tcp_pcb *p,rx_fn f){p->recv=f;}
void tcp_err(struct tcp_pcb *p,error_fn f){p->error=f;}
void tcp_sent(struct tcp_pcb *p,void *f){(void)p;(void)f;}
void tcp_abort(struct tcp_pcb *p){if(p->fd>=0)close(p->fd);assert(p==current);current=NULL;free(p);}
unsigned tcp_sndbuf(struct tcp_pcb *p){return (unsigned)(65535-p->tx_size);}
err_t tcp_write(struct tcp_pcb *p,const void *data,u16_t n,int flags){(void)flags;if(n>tcp_sndbuf(p))return ERR_MEM;memcpy(p->tx+p->tx_size,data,n);p->tx_size+=n;return 0;}
err_t tcp_output(struct tcp_pcb *p){
    if(p->tx_size){size_t n=p->tx_size>97?97:p->tx_size;ssize_t sent=send(p->fd,p->tx,n,MSG_NOSIGNAL);
        if(sent>0){memmove(p->tx,p->tx+sent,p->tx_size-(size_t)sent);p->tx_size-=(size_t)sent;}
        else if(sent<0 && errno!=EAGAIN && errno!=EWOULDBLOCK)return ERR_RST;}
    return 0;
}
void tcp_recved(struct tcp_pcb *p,u16_t n){(void)p;(void)n;}
err_t tcp_connect(struct tcp_pcb *p,ip_addr_t *address,u16_t port,connect_fn f){
    struct sockaddr_in s={0};(void)address;assert(port==443);
    s.sin_family=AF_INET;s.sin_port=htons((unsigned short)atoi(getenv("LB_TEST_PORT")));s.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    p->fd=socket(AF_INET,SOCK_STREAM,0);assert(p->fd>=0);
    if(connect(p->fd,(struct sockaddr *)&s,sizeof s))return ERR_RST;
    assert(fcntl(p->fd,F_SETFL,O_NONBLOCK)==0);p->pending=1;p->connected=f;return 0;
}
struct udp_pcb *udp_new(void){assert(!dns);dns=calloc(1,sizeof *dns);return dns;}
void udp_remove(struct udp_pcb *p){assert(p==dns);if(p->pending)pbuf_free(p->pending);free(p);dns=NULL;}
err_t udp_bind(struct udp_pcb *p,ip_addr_t *address,u16_t port){(void)p;(void)address;assert(port>=49152);return 0;}
void udp_recv(struct udp_pcb *p,dns_fn f,void *v){p->recv=f;p->arg=v;}
err_t udp_sendto(struct udp_pcb *p,struct pbuf *q,ip_addr_t *server,u16_t port){
    static const unsigned char answer[]={0xc0,0x0c,0,1,0,1,0,0,0,60,0,4,142,250,0,1};
    unsigned char *data;assert(port==53);assert(!p->pending);
    p->pending=pbuf_alloc(0,(u16_t)(q->len+sizeof answer),0);data=p->pending->payload;
    memcpy(data,q->payload,q->len);data[2]=0x81;data[3]=0x80;data[7]=1;
    memcpy(data+q->len,answer,sizeof answer);p->server=*server;return 0;
}
int xf_network_ready(void){return 1;}
int xf_network_start(uint32_t n){(void)n;return 1;}
int xf_network_available(void){return 1;}
const char *xf_network_status(void){return "Host test fixture";}
void shim_poll(void){
    struct tcp_pcb *p=current;
    if(dns && dns->pending){struct pbuf *a=dns->pending;dns->pending=NULL;dns->recv(dns->arg,dns,a,&dns->server,53);}
    p=current;if(!p)return;
    if(p->pending){p->pending=0;p->connected(p->arg,p,0);}
    if(tcp_output(p)!=0 && p->error){error_fn e=p->error;close(p->fd);current=NULL;free(p);e(NULL,ERR_RST);return;}
    if(p->recv && !p->closed){
        unsigned char data[311];ssize_t n=recv(p->fd,data,sizeof data,0);
        if(n>0){struct pbuf *b=pbuf_alloc(0,(u16_t)n,0);pbuf_take(b,data,(u16_t)n);assert(p->recv(p->arg,p,b,0)==0);}
        else if(!n){p->closed=1;p->recv(p->arg,p,NULL,0);}
    }
}
