/* NO_SYS single-threaded raw lwIP transport -> Mbed TLS BIO, no proxy/server. */
#include "probe.h"
#include "platform.h"
#include "dns_wire.h"
#include "native_net.h"
#include "driver_guard.h"
#ifdef LB_TEST_CA_HEADER
#include LB_TEST_CA_HEADER
#else
#include "roots.h"
#endif
#include <lwip/udp.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <stdio.h>
#include <string.h>
static const char host[]="www.youtube.com";
static const char request[]="GET /tv HTTP/1.1\r\nHost: www.youtube.com\r\nUser-Agent: Mozilla/5.0 (Xenon) Cobalt/27.lts LeanbackProbe/0.2\r\nAccept: text/html\r\nAccept-Encoding: identity\r\nConnection: close\r\n\r\n";
enum {IDLE,WAIT_NET,DNS,CONNECT,HANDSHAKE,WRITE,READ,DONE};
static int phase,tls_initialized,connected,remote_closed,transport_error;
static struct tcp_pcb *tcp;
static struct udp_pcb *udp;
static ip_addr_t resolver;
static uint16_t query_id;
static unsigned char rx[65536],body[2*1024*1024];
static size_t rx_head,rx_size,request_at;
static uint32_t began,stage_at,progress_at,dns_at;
static unsigned dns_tries;
static xf_response response;
static lb_probe_status status={.stage="Idle",.detail="Press A to start DHCP, DNS and verified HTTPS"};
static mbedtls_ssl_context ssl;
static mbedtls_ssl_config config;
static mbedtls_x509_crt roots;
static mbedtls_ctr_drbg_context rng;
static mbedtls_entropy_context entropy;
const lb_probe_status *lb_probe_state(void) {return &status;}
static void fail(const char *stage,const char *message) {
    status.stage=stage;snprintf(status.detail,sizeof status.detail,"%s",message);status.active=0;phase=DONE;
}
static void tcp_detach(void) {
    if(tcp) {
        struct tcp_pcb *p=tcp;tcp=NULL;
        tcp_arg(p,NULL);tcp_recv(p,NULL);tcp_sent(p,NULL);tcp_err(p,NULL);tcp_abort(p);
    }
}
void lb_probe_cancel(void) {
    int was_active=status.active;
    tcp_detach();if(udp){udp_remove(udp);udp=NULL;}
    if(tls_initialized) {
        mbedtls_ssl_free(&ssl);mbedtls_ssl_config_free(&config);mbedtls_x509_crt_free(&roots);
        mbedtls_ctr_drbg_free(&rng);mbedtls_entropy_free(&entropy);tls_initialized=0;
    }
    rx_head=rx_size=0;status.active=0;phase=IDLE;
    if(was_active){status.stage="Cancelled";snprintf(status.detail,sizeof status.detail,"Request cancelled; A retries");}
}
static int bio_send(void *unused,const unsigned char *data,size_t n) {
    size_t amount;err_t e;(void)unused;
    if(!tcp || transport_error)return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    amount=n;if(amount>tcp_sndbuf(tcp))amount=tcp_sndbuf(tcp);
    if(amount>4096)amount=4096;
    if(!amount)return MBEDTLS_ERR_SSL_WANT_WRITE;
    e=tcp_write(tcp,data,(u16_t)amount,TCP_WRITE_FLAG_COPY);
    if(e==ERR_MEM)return MBEDTLS_ERR_SSL_WANT_WRITE;
    if(e!=ERR_OK)return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    e=tcp_output(tcp);if(e!=ERR_OK && e!=ERR_MEM)return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    progress_at=xf_monotonic_ms();return (int)amount;
}
static int bio_recv(void *unused,unsigned char *data,size_t n) {
    size_t i;(void)unused;
    if(!rx_size) {
        if(transport_error)return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
        return remote_closed?0:MBEDTLS_ERR_SSL_WANT_READ;
    }
    if(n>rx_size)n=rx_size;
    for(i=0;i<n;++i)data[i]=rx[(rx_head+i)%sizeof rx];
    rx_head=(rx_head+n)%sizeof rx;rx_size-=n;return (int)n;
}
static err_t on_connected(void *arg,struct tcp_pcb *p,err_t error) {
    (void)arg;(void)p;if(error!=ERR_OK)transport_error=error;else connected=1;return ERR_OK;
}
static void on_error(void *arg,err_t error) {(void)arg;tcp=NULL;transport_error=error;}
static err_t on_recv(void *arg,struct tcp_pcb *p,struct pbuf *packet,err_t error) {
    struct pbuf *part;size_t i;(void)arg;
    if(error!=ERR_OK)transport_error=error;
    if(!packet){remote_closed=1;return ERR_OK;}
    if(packet->tot_len>sizeof rx-rx_size)return ERR_MEM;
    for(part=packet;part;part=part->next)for(i=0;i<part->len;++i)
        rx[(rx_head+rx_size++)%sizeof rx]=((unsigned char *)part->payload)[i];
    status.received+=packet->tot_len;
    tcp_recved(p,packet->tot_len);pbuf_free(packet);progress_at=xf_monotonic_ms();return ERR_OK;
}
static void dns_receive(void *arg,struct udp_pcb *p,struct pbuf *packet,ip_addr_t *address,u16_t port) {
    unsigned char data[1500],ip[4];int result;size_t n;(void)arg;(void)p;
    if(!packet)return;
    if(phase!=DNS || port!=53 || !ip_addr_cmp(address,&resolver) || packet->tot_len>sizeof data) {pbuf_free(packet);return;}
    n=pbuf_copy_partial(packet,data,packet->tot_len,0);pbuf_free(packet);
    result=lb_dns_answer(data,n,query_id,host,ip);
    if(result<0){fail("DNS failed","Resolver returned error, truncation, no public A, or CNAME limit");return;}
    if(result==1) {
        ip_addr_t target;err_t e;
        IP4_ADDR(&target,ip[0],ip[1],ip[2],ip[3]);
        snprintf(status.address,sizeof status.address,"%u.%u.%u.%u",ip[0],ip[1],ip[2],ip[3]);
        tcp=tcp_new();if(!tcp){fail("TCP failed","No free TCP PCB");return;}
        tcp_arg(tcp,NULL);tcp_recv(tcp,on_recv);tcp_err(tcp,on_error);
        phase=CONNECT;status.stage="TCP :443";stage_at=xf_monotonic_ms();
        e=tcp_connect(tcp,&target,443,on_connected);
        if(e!=ERR_OK){transport_error=e;fail("TCP failed","tcp_connect rejected route/address");}
    }
}
static int dns_send(void) {
    unsigned char data[300];struct pbuf *p;size_t n=lb_dns_query(data,sizeof data,query_id,host);err_t e;
    if(!n)return 0;
    p=pbuf_alloc(PBUF_TRANSPORT,(u16_t)n,PBUF_RAM);if(!p)return 0;
    e=pbuf_take(p,data,(u16_t)n);if(e==ERR_OK)e=udp_sendto(udp,p,&resolver,53);pbuf_free(p);
    return e==ERR_OK;
}
static int body_byte(void *unused,unsigned char c) {
    (void)unused;if(status.body_bytes>=sizeof body)return 0;
    body[status.body_bytes++]=c;return 1;
}
static void save_body(void) {
    char path[80];FILE *f;size_t wrote;int closed;
    if(status.http_status!=200 || !response.complete)return;
    snprintf(path,sizeof path,"%s:/leanback-page.html",lb_volume());
    f=fopen(path,"wb");if(!f){snprintf(status.detail,sizeof status.detail,"HTTP complete; could not save page to USB");return;}
    wrote=fwrite(body,1,status.body_bytes,f);closed=fclose(f);
    snprintf(status.detail,sizeof status.detail,wrote==status.body_bytes && !closed?
        "Verified HTTPS; complete page saved as leanback-page.html":"HTTP complete; USB page write failed");
}
void lb_probe_start(uint32_t now) {
    int r;unsigned char random[4];char resolver_file[80],value[32];FILE *f;
    lb_probe_cancel();memset(&status,0,sizeof status);status.stage="USB seed / clock";
    if(!xf_network_ready()) {
        if(!xf_network_start(now)){fail("DHCP start failed",xf_network_status());return;}
        began=now;phase=WAIT_NET;status.active=1;status.stage="DHCP pending";
        snprintf(status.detail,sizeof status.detail,"%s",xf_network_status());return;
    }
    if(!lb_platform_prepare(status.detail,sizeof status.detail))return;
    mbedtls_ssl_init(&ssl);mbedtls_ssl_config_init(&config);mbedtls_x509_crt_init(&roots);
    mbedtls_ctr_drbg_init(&rng);mbedtls_entropy_init(&entropy);tls_initialized=1;
    r=mbedtls_ctr_drbg_seed(&rng,mbedtls_entropy_func,&entropy,(const unsigned char *)host,sizeof host-1);
    if(r){fail("Entropy failed","NV seed read/update/flush verification failed; TLS refused");return;}
    r=mbedtls_x509_crt_parse(&roots,(const unsigned char *)lb_roots,sizeof lb_roots);
    if(r){fail("CA failed","Embedded GTS trust store parsing failed");return;}
    r=mbedtls_ssl_config_defaults(&config,MBEDTLS_SSL_IS_CLIENT,MBEDTLS_SSL_TRANSPORT_STREAM,MBEDTLS_SSL_PRESET_DEFAULT);
    if(r){fail("TLS config failed","TLS defaults rejected");return;}
    mbedtls_ssl_conf_authmode(&config,MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&config,&roots,NULL);
    mbedtls_ssl_conf_rng(&config,mbedtls_ctr_drbg_random,&rng);
    r=mbedtls_ssl_setup(&ssl,&config);if(!r)r=mbedtls_ssl_set_hostname(&ssl,host);
    if(r){fail("TLS setup failed","Context or SNI/hostname setup failed");return;}
    mbedtls_ssl_set_bio(&ssl,NULL,bio_send,bio_recv,NULL);
    /* Explicit default public resolver; optional file selects the user's DNS.
     * This is not a proxy or static-IP fallback. DHCP is always required. */
    strcpy(value,"8.8.8.8");
    snprintf(resolver_file,sizeof resolver_file,"%s:/leanback-dns.txt",lb_volume());
    f=fopen(resolver_file,"rb");if(f){size_t n=fread(value,1,sizeof value-1,f);value[n]=0;fclose(f);value[strcspn(value,"\r\n ")]=0;}
    if(!ipaddr_aton(value,&resolver)){fail("DNS config failed","Invalid IPv4 in leanback-dns.txt");return;}
    r=mbedtls_ctr_drbg_random(&rng,random,sizeof random);if(r){fail("RNG failed","Could not randomize DNS query");return;}
    query_id=(uint16_t)((unsigned)random[0]*256+random[1]);
    udp=udp_new();if(!udp){fail("DNS failed","No free UDP PCB");return;}
    if(udp_bind(udp,IP_ADDR_ANY,(u16_t)(49152+(((unsigned)random[2]*256+random[3])&16383)))!=ERR_OK) {fail("DNS failed","Random DNS source port unavailable; retry");return;}
    udp_recv(udp,dns_receive,NULL);
    rx_head=rx_size=request_at=0;connected=remote_closed=transport_error=0;
    began=stage_at=progress_at=dns_at=now;dns_tries=1;phase=DNS;status.active=1;status.stage="DNS A lookup";
    snprintf(status.detail,sizeof status.detail,"Resolver %s; query www.youtube.com",value);
    if(!dns_send())fail("DNS failed","UDP query could not be queued");
}
static void tls_failure(int error) {
    char message[100];status.tls_error=error;status.verify_flags=mbedtls_ssl_get_verify_result(&ssl);
    mbedtls_strerror(error,message,sizeof message);fail("TLS failed",message);
}
void lb_probe_tick(uint32_t now) {
    int r=0;
    /* network_tick callbacks timestamp freshly received bytes. The UI's
     * supplied frame timestamp can predate those callbacks by milliseconds. */
    now=xf_monotonic_ms();
    if(!status.active){if(phase==DONE){tcp_detach();if(udp){udp_remove(udp);udp=NULL;}}return;}
    if(phase==WAIT_NET) {
        if(xf_network_ready()){lb_probe_start(now);return;}
        snprintf(status.detail,sizeof status.detail,"%s",xf_network_status());
        if((uint32_t)(now-began)>25000)fail("DHCP timeout",xf_network_status());
        return;
    }
    if(!xf_network_ready()){fail("Network lost",xf_network_status());return;}
    if((uint32_t)(now-began)>90000 || (uint32_t)(now-progress_at)>30000) {fail("Timeout","90s total / 30s idle deadline");return;}
    if(transport_error){snprintf(status.detail,sizeof status.detail,"lwIP TCP error %d",transport_error);status.stage="TCP failed";status.active=0;phase=DONE;return;}
    if(phase==DNS && (uint32_t)(now-dns_at)>3000) {
        if(dns_tries>=3){fail("DNS timeout","No valid DNS answer after 3 queries");return;}
        ++dns_tries;dns_at=now;if(!dns_send())fail("DNS failed","Retry send failed");return;
    }
    if(phase==CONNECT) {
        if(udp){udp_remove(udp);udp=NULL;}
        if((uint32_t)(now-stage_at)>15000){fail("TCP timeout","No connection to resolved address port 443");return;}
        if(!connected)return;
        phase=HANDSHAKE;status.stage="TLS 1.2 handshake";progress_at=now;
    }
    if(phase==HANDSHAKE) {
        r=mbedtls_ssl_handshake(&ssl);
        if(r==0) {
            status.verify_flags=mbedtls_ssl_get_verify_result(&ssl);
            if(status.verify_flags){fail("Certificate failed","Nonzero verification flags; HTTP refused");return;}
            status.verified=1;snprintf(status.cipher,sizeof status.cipher,"%s",mbedtls_ssl_get_ciphersuite(&ssl));
            phase=WRITE;status.stage="HTTPS GET /tv";progress_at=now;
        }
    } else if(phase==WRITE) {
        r=mbedtls_ssl_write(&ssl,(const unsigned char *)request+request_at,sizeof request-1-request_at);
        if(r>0) {request_at+=(size_t)r;progress_at=now;if(request_at==sizeof request-1){phase=READ;status.stage="HTTPS response";xf_response_stream(&response,body_byte,NULL,sizeof body);}}
    } else if(phase==READ) {
        unsigned char data[4096];
        r=mbedtls_ssl_read(&ssl,data,sizeof data);
        if(r>0){xf_response_feed(&response,data,(size_t)r);progress_at=now;}
        else if(r==0 || r==MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
            /* TCP EOF without authenticated close_notify is not a complete
             * unknown-length response. Known framing may already be complete. */
            if(r==MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)xf_response_eof(&response);
            else if(!response.complete){fail("Truncated TLS","TCP EOF before complete HTTP/close_notify");return;}
        }
        status.http_status=response.status;
        if(response.failed){fail("HTTP parser failed",response.error);return;}
        if(response.complete){status.complete=1;status.stage="HTTPS complete";snprintf(status.detail,sizeof status.detail,"HTTP %d; no redirect following or browser execution",response.status);save_body();status.active=0;phase=DONE;return;}
    }
    if(r<0 && r!=MBEDTLS_ERR_SSL_WANT_READ && r!=MBEDTLS_ERR_SSL_WANT_WRITE && r!=MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)tls_failure(r);
}
