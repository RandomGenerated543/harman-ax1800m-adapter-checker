/* Independent October vendor-binary relay. No Harman dynamic111 modules. */
#include "vendor_abi.h"
#include <errno.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>

#define INFO_VA 0x4660bcu
#define INFO_GOT 0x56518cu
#define TEARDOWN_VA 0x4726b0u
#define TEARDOWN_GOT 0x56507cu
#define PLATFORM_COPY_VA 0x488180u
#define PLATFORM_COPY_GOT 0x565024u
typedef void *(*info_fn)(void *,void *,int *);
typedef int (*teardown_fn)(void *,void *,void *,unsigned char *,void *);
typedef void *(*platform_copy_fn)(void *,unsigned,void *,void *,int *);
static info_fn stock_info=(info_fn)INFO_VA;
static teardown_fn stock_teardown=(teardown_fn)TEARDOWN_VA;
static platform_copy_fn stock_platform_copy=(platform_copy_fn)PLATFORM_COPY_VA;
static void *(*retain_cf)(void *)=(void *(*)(void *))0x48fba0u;
static void *(*str_create)(void *,const char *,unsigned)=(void *(*)(void *,const char *,unsigned))0x490fb4u;
static int (*str_copy)(void *,char *,long,unsigned)=(int (*)(void *,char *,long,unsigned))0x491268u;
static unsigned long (*str_type)(void)=(unsigned long (*)(void))0x490c68u;
/* Exact exported October CFDictionaryRef gFakeDict: head-unit /info cache. */
static void **native_info=(void **)0x566f64u;
static pthread_mutex_t profile_mu=PTHREAD_MUTEX_INITIALIZER;
static char profile_uuid[128];
static unsigned profile_generation;
static unsigned long long next_relay_id,next_session_generation;

static void log_event(const char *fmt,...) {
  char line[512]; struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
  int n=snprintf(line,sizeof line,"AX_INDEPENDENT v=4 pid=%ld ms=%llu ",(long)getpid(),
      (unsigned long long)t.tv_sec*1000+t.tv_nsec/1000000);
  va_list ap;va_start(ap,fmt);vsnprintf(line+n,sizeof line-(size_t)n,fmt,ap);va_end(ap);
  size_t len=strnlen(line,sizeof line-2);line[len++]='\n';
  int fd=open("/usr/data/log/altscreen.log",O_WRONLY|O_CREAT|O_APPEND,0644);
  if(fd>=0){(void)write(fd,line,len);close(fd);}
}
static void *dict_get(void *d,const char *key,unsigned long type) {
  void *k=str_create(0,key,0x08000100u);if(!k)return 0;
  void *v=cf_dict_get_typed(d,k,type,0);cf_release(k);return v;
}
static int64_t dict_num(void *d,const char *key) {
  void *k=str_create(0,key,0x08000100u);if(!k)return 0;
  int64_t v=cf_dict_get_int64(d,k,0);cf_release(k);return v;
}
static int uuid_valid(const char *s) {
  if(!s||strlen(s)!=36)return 0;
  for(unsigned i=0;i<36;i++) {
    if(i==8||i==13||i==18||i==23){if(s[i]!='-')return 0;}
    else if(!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f')||(s[i]>='A'&&s[i]<='F')))return 0;
  }return 1;
}
static int profile_read(void *root,char out[128]) {
  if(!root||cf_get_type_id(root)!=cf_dict_type_id())return 0;
  void *arr=dict_get(root,"displays",cf_array_type_id());if(!arr)return 0;
  long count=cf_array_count(arr);if(count<1||count>32)return 0;
  int matches=0;
  for(long i=0;i<count;i++) {
    void *d=cf_array_at(arr,i);if(!d||cf_get_type_id(d)!=cf_dict_type_id())continue;
    if(dict_num(d,"type")!=111)continue;
    int64_t w=dict_num(d,"widthPixels"),h=dict_num(d,"heightPixels");
    void *u=dict_get(d,"uuid",str_type());char tmp[128]={0};
    if(w<16||h<16||w>8192||h>8192||!u||!str_copy(u,tmp,sizeof tmp,0x08000100u)||!uuid_valid(tmp))return 0;
    memcpy(out,tmp,sizeof tmp);matches++;
  }return matches==1;
}
static int advertise(void *root) {
  if(!root||cf_get_type_id(root)!=cf_dict_type_id())return 0;
  void *key=str_create(0,"enabledFeatures",0x08000100u);
  void *a=cf_array_create_mutable(0,0,arr_cb);
  if(!key||!a){if(key)cf_release(key);if(a)cf_release(a);return 0;}
  void *old=cf_dict_get_typed(root,key,cf_array_type_id(),0);
  if(old)for(long i=0;i<cf_array_count(old);i++)cf_array_append(a,cf_array_at(old,i));
  const char *features[]={"altScreen","viewAreas"};
  for(unsigned f=0;f<2;f++) {
    int found=0;
    for(long i=0;i<cf_array_count(a);i++) {
      void *v=cf_array_at(a,i);char text[64]={0};
      if(v&&cf_get_type_id(v)==str_type()&&str_copy(v,text,sizeof text,0x08000100u)&&!strcmp(text,features[f]))found=1;
    }
    if(!found){void *s=str_create(0,features[f],0x08000100u);if(s){cf_array_append(a,s);cf_release(s);}}
  }
  cf_dict_set_value(root,key,a);cf_release(a);cf_release(key);return 1;
}
/* Initial SETUP may arrive before phone-facing /info. Use the same vendor
 * HU descriptor cache that stock airPlayGetPropertyFromNativeHU reads. */
static int refresh_native_profile(void) {
  char uuid[128]={0};void *root=*native_info;
  if(!root)return 0;
  retain_cf(root);int valid=profile_read(root,uuid);cf_release(root);
  pthread_mutex_lock(&profile_mu);
  if(valid)memcpy(profile_uuid,uuid,sizeof profile_uuid);else profile_uuid[0]=0;
  pthread_mutex_unlock(&profile_mu);return valid;
}
/* The stock SETUP caller obtains this owned array AFTER relay_setup returns
 * and writes it over response.enabledFeatures. Hook that final property path. */
void *relay_platform_copy(void *server,unsigned flags,void *property,void *qualifier,int *status) {
  void *original=stock_platform_copy(server,flags,property,qualifier,status);
  char key[64]={0};
  if(!property||cf_get_type_id(property)!=str_type()||
     !str_copy(property,key,sizeof key,0x08000100u)||strcmp(key,"enabledFeatures"))return original;
  if(!refresh_native_profile())return original;
  if(original&&cf_get_type_id(original)!=cf_array_type_id()) {
    log_event("stage=setup_features stock_wrong_type=1 returned=0");return original;
  }
  void *holder=cf_dict_create_mutable(0,0,dict_key_cb,dict_val_cb);
  void *feature_key=str_create(0,"enabledFeatures",0x08000100u);
  if(!holder||!feature_key){if(holder)cf_release(holder);if(feature_key)cf_release(feature_key);return original;}
  if(original)cf_dict_set_value(holder,feature_key,original);
  void *result=0;
  if(advertise(holder)) {
    result=cf_dict_get_typed(holder,feature_key,cf_array_type_id(),0);
    if(result)retain_cf(result);
  }
  cf_release(holder);cf_release(feature_key);
  if(!result)return original;
  long before=original?cf_array_count(original):0;
  if(original)cf_release(original);
  if(status)*status=0;
  log_event("stage=setup_features stock_count=%ld returned_count=%ld altScreen=1 viewAreas=1",before,cf_array_count(result));
  return result;
}
void *relay_info(void *s,void *f,int *status) {
  void *r=stock_info(s,f,status);char uuid[128]={0};
  int valid=(!status||!*status)&&profile_read(r,uuid);
  pthread_mutex_lock(&profile_mu);
  if(valid)memcpy(profile_uuid,uuid,sizeof profile_uuid);else profile_uuid[0]=0;
  profile_generation++;
  pthread_mutex_unlock(&profile_mu);
  if(valid)advertise(r);
  log_event("stage=info profile111=%d native110=unchanged",valid);
  return r;
}

typedef struct Relay Relay;
struct Relay {
  pthread_mutex_t mu;pthread_cond_t changed;
  unsigned refs;int stopped,listener,phone,car,reply,port,homekit;
  unsigned long long relay_id,session_generation;
  const char *stop_reason;int stop_full,stop_aux,stop_native;
  void *session,*client;unsigned long long conn;
  char uuid[128];unsigned char car_key[16],peer[160];
  unsigned char enc[0x200] __attribute__((aligned(8)));
};
static pthread_mutex_t current_mu=PTHREAD_MUTEX_INITIALIZER;
static Relay *current;
static unsigned long long relay_id_for_session(void *session) {
  unsigned long long id=0;pthread_mutex_lock(&current_mu);
  if(current&&(!session||current->session==session))id=current->relay_id;
  pthread_mutex_unlock(&current_mu);return id;
}
static Relay *hold(Relay *r){pthread_mutex_lock(&r->mu);r->refs++;pthread_mutex_unlock(&r->mu);return r;}
static void drop(Relay *r) {
  int last;pthread_mutex_lock(&r->mu);last=!--r->refs;pthread_mutex_unlock(&r->mu);
  if(last){cf_release(r->session);cf_release(r->client);pthread_cond_destroy(&r->changed);pthread_mutex_destroy(&r->mu);free(r);}
}
static void stop_relay(Relay *r,const char *reason,int full,int aux,int native) {
  pthread_mutex_lock(&r->mu);if(!r->stopped){r->stop_reason=reason;r->stop_full=full;r->stop_aux=aux;r->stop_native=native;}r->stopped=1;
  if(r->listener>=0)shutdown(r->listener,SHUT_RDWR);
  if(r->phone>=0)shutdown(r->phone,SHUT_RDWR);
  if(r->car>=0)shutdown(r->car,SHUT_RDWR);
  pthread_cond_broadcast(&r->changed);pthread_mutex_unlock(&r->mu);
}
static void replace_current(Relay *next) {
  Relay *old;
  pthread_mutex_lock(&current_mu);
  old=current;
  if(next){next->relay_id=++next_relay_id;next->session_generation=++next_session_generation;current=hold(next);}
  else current=0;
  pthread_mutex_unlock(&current_mu);
  if(old){stop_relay(old,"superseded_by_new_relay",0,0,0);drop(old);}
}
static int stopped(Relay *r){pthread_mutex_lock(&r->mu);int s=r->stopped;pthread_mutex_unlock(&r->mu);return s;}
/* Shutdown never closes descriptors owned by the worker, avoiding fd reuse. */
static void retire(void *session,const char *reason,int full,int aux,int native) {
  Relay *old=0;pthread_mutex_lock(&current_mu);
  if(current&&(!session||current->session==session)){old=current;current=0;}
  pthread_mutex_unlock(&current_mu);if(old){stop_relay(old,reason,full,aux,native);drop(old);}
}
static void retire_relay(Relay *expected,const char *reason) {
  Relay *old=0;pthread_mutex_lock(&current_mu);
  if(current==expected){old=current;current=0;}
  pthread_mutex_unlock(&current_mu);if(old){stop_relay(old,reason,0,0,0);drop(old);}
}
static int send_command(Relay *r,const char *type,int url) {
  void *req=cf_dict_create_mutable(0,0,dict_key_cb,dict_val_cb);
  void *params=cf_dict_create_mutable(0,0,dict_key_cb,dict_val_cb);
  if(!req||!params){if(req)cf_release(req);if(params)cf_release(params);return -1;}
  cf_dict_set_cstr(req,KEY_TYPE,(void *)type,0xffffffffu);
  cf_dict_set_cstr(params,K_UUID,r->uuid,0xffffffffu);
  if(url)cf_dict_set_cstr(params,K_CMD_URL,(void *)"maps:/car/instrumentcluster",0xffffffffu);
  cf_dict_set_value(req,K_CMD_PARAMS,params);cf_release(params);
  pthread_mutex_lock(&r->mu);
  int rc=r->stopped?-1:session_send_command(r->session,req,0,0);
  pthread_mutex_unlock(&r->mu);cf_release(req);return rc;
}
static void setup_reply(void *msg) {
  Relay *r=*(Relay **)((char *)msg+0x8fc);int port=0,err=0;
  int len=*(int *)((char *)msg+0x4a0);
  void *body=*(void **)((char *)msg+0x49c);
  if(len>0&&len<1048576&&body) {
    void *root=cf_plist_read(body,len,&err);
    if(root&&!err&&cf_get_type_id(root)==cf_dict_type_id()) {
      void *a=cf_dict_get_typed(root,KEY_STREAMS,cf_array_type_id(),0);
      if(a&&cf_array_count(a)<=32)for(long i=0;i<cf_array_count(a);i++) {
        void *d=cf_array_at(a,i);
        if(d&&cf_get_type_id(d)==cf_dict_type_id()&&cf_dict_get_int64(d,KEY_TYPE,0)==111) {
          int64_t p=cf_dict_get_int64(d,KEY_DATAPORT,0);if(p>0&&p<=65535)port=(int)p;
        }
      }
    }if(root)cf_release(root);
  }
  pthread_mutex_lock(&r->mu);r->port=port;r->reply=1;pthread_cond_broadcast(&r->changed);pthread_mutex_unlock(&r->mu);
  log_event("stage=hu_setup_reply port_valid=%d",port>0);drop(r);
}
static int socket_timeout(int fd) {
  struct timeval tv={5,0};
  if(setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof tv)<0)return -1;
  if(setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof tv)<0)return -1;
  return 0;
}
static int car_setup(Relay *r) {
  if(r->homekit){log_event("stage=hu_setup unsupported_homekit=1");return -1;}
  void *msg=0,*req=0,*sd=0,*arr=0,*body=0;int rc=-1,bodylen=0;
  if(http_create(&msg)||!msg)return -1;
  if(http_init((char *)msg+0x10,S_SETUP,S_URL,S_PROTO))goto end;
  http_addf((char *)msg+0x10,(void *)0x51aa58,(void *)"1");
  req=cf_dict_create_mutable(0,0,dict_key_cb,dict_val_cb);sd=cf_dict_create_mutable(0,0,dict_key_cb,dict_val_cb);
  if(!req||!sd)goto end;
  unsigned char random[16];cf_uuidget(random);
  uint64_t conn;memcpy(&conn,random,8);if(!conn)conn=1;
  cf_dict_set_int64(sd,K_CONNID,(int64_t)conn);cf_dict_set_int64(sd,KEY_TYPE,111);
  cf_dict_set_cstr(sd,K_UUID,r->uuid,0xffffffffu);cf_dict_set_int64(sd,K_LATENCYMS,25);
  cf_arr_ensure(&arr,sd);if(!arr)goto end;cf_dict_set_value(req,KEY_STREAMS,arr);
  body=cf_plist_create(req,&bodylen,0);if(!body)goto end;
  if(http_setbody(msg,S_MIME,body,bodylen))goto end;
  body=0; /* HTTPMessageSetBodyPtr takes ownership of the malloc buffer. */
  *(void **)((char *)msg+0x904)=(void *)&setup_reply;
  *(int *)((char *)msg+0x8f4)=0;*(int *)((char *)msg+0x8f8)=0;*(void **)((char *)msg+0x8fc)=hold(r);
  int submitted=http_send(r->client,msg);
  if(submitted){drop(r);log_event("stage=hu_setup_submit error=%d",submitted);goto end;}
  struct timespec until;clock_gettime(CLOCK_REALTIME,&until);until.tv_sec+=5;
  pthread_mutex_lock(&r->mu);
  while(!r->reply&&!r->stopped)if(pthread_cond_timedwait(&r->changed,&r->mu,&until)==ETIMEDOUT)break;
  int port=r->port,dead=r->stopped;pthread_mutex_unlock(&r->mu);
  if(!port||dead){log_event("stage=hu_setup timeout_or_stopped=1");goto end;}
  char sa[160];memcpy(sa,r->peer,sizeof sa);sock_setport(sa,port);
  int fd=socket(*(unsigned short *)sa,SOCK_STREAM,0);if(fd<0)goto end;
  /* Nonblocking connect with a bounded poll; no global socket flags changed. */
  int flags=fcntl(fd,F_GETFL,0);if(flags<0){close(fd);goto end;}
  if(fcntl(fd,F_SETFL,flags|O_NONBLOCK)<0){close(fd);goto end;}
  int connected=connect(fd,(struct sockaddr *)sa,sock_getsize(sa));
  if(connected<0&&errno==EINPROGRESS) {
    fd_set wf;FD_ZERO(&wf);struct timeval tv={3,0};
    if(fd<FD_SETSIZE){FD_SET(fd,&wf);if(select(fd+1,0,&wf,0,&tv)>0){int error=0;socklen_t n=sizeof error;if(!getsockopt(fd,SOL_SOCKET,SO_ERROR,&error,&n)&&!error)connected=0;}}
  }
  if(connected||fcntl(fd,F_SETFL,flags&~O_NONBLOCK)<0){close(fd);log_event("stage=hu_connect failed=1");goto end;}
  if(socket_timeout(fd)){int e=errno;log_event("stage=hu_socket_options id=%llu error=%d",r->relay_id,e);close(fd);errno=e;goto end;}
  char ks[64],iv[64];unsigned char key[16],nonce[16];
  snprintf(ks,sizeof ks,"AirPlayStreamKey%llu",(unsigned long long)conn);
  snprintf(iv,sizeof iv,"AirPlayStreamIV%llu",(unsigned long long)conn);
  aes_derive(r->car_key,16,ks,(int)strlen(ks),iv,(int)strlen(iv),key,nonce);
  aes_ctr_init(r->enc,key,nonce);
  pthread_mutex_lock(&r->mu);if(!r->stopped){r->car=fd;rc=0;}pthread_mutex_unlock(&r->mu);
  if(rc)close(fd);else log_event("stage=hu_video_connected");
end:
  if(body)free(body);if(arr)cf_release(arr);if(sd)cf_release(sd);if(req)cf_release(req);if(msg)cf_release(msg);return rc;
}
enum RxState { RX_COMPLETE,RX_EOF,RX_PARTIAL_EOF,RX_ERROR,RX_CANCELLED };
typedef struct { enum RxState state;size_t received;int error;unsigned idle_timeouts,interrupts; } RxResult;
static RxResult read_exact(Relay *r,int fd,void *dst,size_t n) {
  char *p=dst;RxResult out={RX_COMPLETE,0,0,0,0};
  while(out.received<n) {
    if(stopped(r)){out.state=RX_CANCELLED;return out;}
    ssize_t got=recv(fd,p+out.received,n-out.received,0);
    if(got>0){out.received+=(size_t)got;continue;}
    if(got==0){if(stopped(r))out.state=RX_CANCELLED;else out.state=out.received?RX_PARTIAL_EOF:RX_EOF;return out;}
    int e=errno;
    if(e==EINTR){out.interrupts++;if(stopped(r)){out.state=RX_CANCELLED;return out;}continue;}
    if(e==EAGAIN||e==EWOULDBLOCK){out.idle_timeouts++;if(stopped(r)){out.state=RX_CANCELLED;return out;}continue;}
    out.state=RX_ERROR;out.error=e;return out;
  }
  return out;
}
static const char *rx_reason(enum RxState state) {
  switch(state){case RX_EOF:return "peer_eof";case RX_PARTIAL_EOF:return "partial_eof";case RX_ERROR:return "socket_error";case RX_CANCELLED:return "cancelled";default:return "complete";}
}
typedef struct {
  int complete,invalid_length,allocation_error;
  const char *stage;size_t done,requested;unsigned length;int error;
  RxResult header_rx,body_rx;
} FrameResult;
/* Shared by the worker and host tests so frame assembly and its timeout state
 * are exercised through the same path used by the production receive loop. */
static FrameResult read_phone_frame(Relay *r,int fd,unsigned char header[128],unsigned char **body,unsigned *capacity) {
  FrameResult out={0,0,0,"phone_header",0,128,0,0,{RX_COMPLETE,0,0,0,0},{RX_COMPLETE,0,0,0,0}};
  out.header_rx=read_exact(r,fd,header,sizeof(unsigned char)*128);out.done=out.header_rx.received;
  if(out.header_rx.state!=RX_COMPLETE){out.error=out.header_rx.error;return out;}
  memcpy(&out.length,header,4);
  if(out.length>4000000){out.invalid_length=1;out.done=128;out.requested=128;return out;}
  if(out.length>*capacity){void *p=realloc(*body,out.length);if(!p){out.allocation_error=1;out.stage="phone_body";out.error=ENOMEM;out.done=0;out.requested=out.length;return out;}*body=p;*capacity=out.length;}
  if(out.length){out.stage="phone_body";out.requested=out.length;out.body_rx=read_exact(r,fd,*body,out.length);out.done=out.body_rx.received;
    if(out.body_rx.state!=RX_COMPLETE){out.error=out.body_rx.error;return out;}}
  out.complete=1;return out;
}
typedef struct { size_t sent;int error; } WriteResult;
typedef struct { WriteResult result;const char *stage;size_t requested; } ForwardResult;
static WriteResult write_exact(int fd,const void *src,size_t n) {
  const char *p=src;WriteResult out={0,0};while(out.sent<n){ssize_t sent=send(fd,p+out.sent,n-out.sent,MSG_NOSIGNAL);if(sent<0&&errno==EINTR)continue;if(sent<0){out.error=errno;return out;}if(sent==0){out.error=EPIPE;return out;}out.sent+=(size_t)sent;}return out;
}
static ForwardResult forward(Relay *r,int type,unsigned char *data,unsigned n,const unsigned char *cfg) {
  unsigned char header[128];ForwardResult out={{0,0},"config_header",128};
  if(ax_make_header(header,type,n,cfg)){out.result.error=EINVAL;out.stage="frame_header";return out;}
  if(!type)aesctr_update(r->enc,data,(int)n,data);
  out.result=write_exact(r->car,header,sizeof header);if(out.result.error)return out;
  out.stage=type?"config_payload":"video_payload";out.requested=n;out.result=write_exact(r->car,data,n);return out;
}
static void *worker(void *arg) {
  Relay *r=arg;unsigned char *body=0,*cfg=0;unsigned cap=0,cfglen=0;unsigned long frames=0,pushed=0;
  const char *stage="phone_accept_wait",*reason="unknown";int terminal_errno=0;size_t done=0,requested=0;
  fd_set rf;FD_ZERO(&rf);struct timeval wait={20,0};
  if(r->listener>=FD_SETSIZE){reason="listener_fd_out_of_range";goto end;}FD_SET(r->listener,&rf);
  int ready=select(r->listener+1,&rf,0,0,&wait);if(ready<=0||stopped(r)){reason=stopped(r)?"cancelled":"phone_accept_timeout";terminal_errno=ready<0?errno:0;goto end;}
  stage="phone_accept";int phone=accept(r->listener,0,0);if(phone<0){reason="phone_accept_error";terminal_errno=errno;goto end;}
  pthread_mutex_lock(&r->mu);r->phone=phone;pthread_mutex_unlock(&r->mu);
  if(socket_timeout(phone)){reason="socket_option_error";stage="phone_socket_options";terminal_errno=errno;goto end;}
  log_event("stage=phone_video_connected id=%llu generation=%llu",r->relay_id,r->session_generation);
  /* Derive before waiting on HU; session is retained for the worker lifetime. */
  unsigned char dec[0x200] __attribute__((aligned(8))),ckey[32],nonce[8]={0};int chacha=0;
  int key_error=0;errno=0;
  pthread_mutex_lock(&r->mu);
  if(r->stopped)key_error=1;
  else if(*(void **)((char *)r->session+0x3ec)) {
    if(get_stream_keys((char *)r->session+0x3ec,0,(unsigned)r->conn,(unsigned)(r->conn>>32),0,0,ckey))key_error=1;
    chacha=1;
  }else {
    unsigned char key[16],iv[16];derive_screen((char *)r->session+0x200,16,(unsigned)r->conn,(unsigned)(r->conn>>32),key,iv);aesctr_init(dec,key,iv);
  }
  pthread_mutex_unlock(&r->mu);
  if(key_error){reason=stopped(r)?"cancelled":"phone_keys_error";stage="phone_keys";terminal_errno=errno;goto end;}
  stage="hu_setup";errno=0;if(car_setup(r)){reason=stopped(r)?"cancelled":"hu_setup_error";terminal_errno=errno;goto end;}
  log_event("stage=showUI id=%llu submit_rc=%d",r->relay_id,send_command(r,"showUI",1));
  int primed=0;unsigned char cfgheader[128];
  while(!stopped(r)) {
    unsigned char hdr[128];FrameResult frame=read_phone_frame(r,phone,hdr,&body,&cap);stage=frame.stage;done=frame.done;requested=frame.requested;
    if(frame.header_rx.idle_timeouts)log_event("stage=phone_rx id=%llu idle_recovered=1 idle_timeouts=%u interrupts=%u part=header",r->relay_id,frame.header_rx.idle_timeouts,frame.header_rx.interrupts);
    if(frame.body_rx.idle_timeouts)log_event("stage=phone_rx id=%llu idle_recovered=1 idle_timeouts=%u interrupts=%u part=body",r->relay_id,frame.body_rx.idle_timeouts,frame.body_rx.interrupts);
    if(!frame.complete){
      if(frame.invalid_length)reason="invalid_frame_length";
      else if(frame.allocation_error)reason="allocation_error";
      else {const RxResult *failed=frame.header_rx.state!=RX_COMPLETE?&frame.header_rx:&frame.body_rx;reason=rx_reason(failed->state);}
      terminal_errno=frame.error;break;
    }
    unsigned len=frame.length;int type=hdr[4];stage="phone_frame_process";done=0;requested=0;
    if(type==1&&len) {
      unsigned char checked[128];if(ax_make_header(checked,1,len,hdr)){reason="invalid_config_geometry";stage="config_validate";done=0;requested=len;break;}
      void *p=realloc(cfg,len);if(!p){reason="allocation_error";stage="config_copy";done=0;requested=len;terminal_errno=ENOMEM;break;}cfg=p;memcpy(cfg,body,len);cfglen=len;memcpy(cfgheader,hdr,128);primed=0;
      ForwardResult sent=forward(r,1,cfg,cfglen,cfgheader);
      if(sent.result.error){reason="forward_error";stage=sent.stage;terminal_errno=sent.result.error;done=sent.result.sent;requested=sent.requested;break;}
      log_event("stage=config_forwarded id=%llu",r->relay_id);continue;
    }
    if(type!=0||!len)continue;
    if(chacha) {
      if(len<16){reason="short_authenticated_frame";stage="video_decrypt";done=len;requested=16;break;}int err=0;cc_init(dec,ckey,nonce);cc_aad(dec,hdr,128);
      int n=cc_dec(dec,body,len-16,body);if(n<0||(unsigned)n>len-16){reason="decrypt_error";stage="video_decrypt";done=len;requested=len;break;}
      n+=cc_ver(dec,body+n,body+len-16,&err);for(int k=0;k<8&&++nonce[k]==0;k++){}
      if(err||n!=(int)len-16){reason="authentication_error";stage="video_auth";done=len;requested=len;break;}len-=16;
    }else aesctr_update(dec,body,(int)len,body);
    frames++;if(!cfglen)continue;
    if(!primed) {
      primed=stream_has_h264_idr(body,len);
      if(!primed){if(frames%120==1)log_event("stage=keyframe id=%llu submit_rc=%d",r->relay_id,send_command(r,"forceKeyFrame",0));continue;}
    }
    ForwardResult sent=forward(r,0,body,len,0);
    if(sent.result.error){reason="forward_error";stage=sent.stage;terminal_errno=sent.result.error;done=sent.result.sent;requested=sent.requested;break;}pushed++;
    if(pushed==1||pushed%300==0)log_event("stage=forwarding id=%llu frames=%lu",r->relay_id,pushed);
  }
  if(!strcmp(reason,"unknown"))reason=stopped(r)?"cancelled":"worker_exit";
end:
  if(!strcmp(reason,"unknown"))reason=stopped(r)?"cancelled":"worker_exit";
  const char *stop_reason;int stop_full,stop_aux,stop_native;
  pthread_mutex_lock(&r->mu);stop_reason=r->stop_reason;stop_full=r->stop_full;stop_aux=r->stop_aux;stop_native=r->stop_native;pthread_mutex_unlock(&r->mu);
  log_event("stage=worker_end id=%llu generation=%llu reason=%s at=%s errno=%d done=%lu requested=%lu frames=%lu forwarded=%lu stop_reason=%s teardown_full=%d teardown_aux=%d teardown_native=%d",
      r->relay_id,r->session_generation,reason,stage,terminal_errno,(unsigned long)done,(unsigned long)requested,frames,pushed,stop_reason?stop_reason:"worker",stop_full,stop_aux,stop_native);
  stop_relay(r,"worker_end",0,0,0);
  pthread_mutex_lock(&r->mu);
  if(r->listener>=0){close(r->listener);r->listener=-1;}
  if(r->phone>=0){close(r->phone);r->phone=-1;}
  if(r->car>=0){close(r->car);r->car=-1;}
  pthread_mutex_unlock(&r->mu);
  free(body);free(cfg);drop(r);return 0;
}static int start_relay(void *session,uint64_t conn) {
#ifdef RELAY_HOST_TEST
  (void)session;(void)conn;return 19831;
#else
  Relay *r=calloc(1,sizeof *r);if(!r)return 0;
  pthread_mutex_init(&r->mu,0);pthread_cond_init(&r->changed,0);r->refs=1;r->listener=r->phone=r->car=-1;
  pthread_mutex_lock(&profile_mu);memcpy(r->uuid,profile_uuid,sizeof r->uuid);pthread_mutex_unlock(&profile_mu);
  if(!uuid_valid(r->uuid)||!conn||!session||!*GAIRPLAYCLIENT){pthread_cond_destroy(&r->changed);pthread_mutex_destroy(&r->mu);free(r);return 0;}
  r->conn=conn;r->session=retain_cf(session);r->client=retain_cf(*GAIRPLAYCLIENT);
  memcpy(r->car_key,GSESSION_AESKEY,16);sock_copy(GPEERADDR,r->peer);r->homekit=*GPAIRVERIFY_HK;
  int family=*(unsigned short *)((char *)session+0x8c),port=0,fd=-1;
  if(server_socket_open(family,2,6,0,&port,0xfffd4000u,&fd)||fd<0){drop(r);return 0;}
  int flags=fcntl(fd,F_GETFL,0);if(flags<0||fcntl(fd,F_SETFL,flags&~O_NONBLOCK)<0){close(fd);drop(r);return 0;}
  r->listener=fd;replace_current(r);
  log_event("stage=relay_start id=%llu generation=%llu",r->relay_id,r->session_generation);
  pthread_t t;if(pthread_create(&t,0,worker,r)){retire_relay(r,"worker_create_error");close(fd);drop(r);return 0;}
  pthread_detach(t);return port;
#endif
}
/* No-stream, centre-only and audio requests pass to the stock function unchanged. */
static void *without111(void *array,int *alt,int *native,uint64_t *conn) {
  *alt=*native=0;if(!array)return 0;
  long n=cf_array_count(array);if(n<0||n>32){*alt=-1;return 0;}
  for(long i=0;i<n;i++) {
    void *d=cf_array_at(array,i);if(!d||cf_get_type_id(d)!=cf_dict_type_id()){*alt=-1;return 0;}
    if(cf_dict_get_int64(d,KEY_TYPE,0)==111){(*alt)++;if(conn)*conn=(uint64_t)cf_dict_get_int64(d,K_CONNID,0);}else (*native)++;
  }
  if(!*alt)return 0;if(*alt!=1){*alt=-1;return 0;}
  void *a=cf_array_create_mutable(0,0,arr_cb);if(!a){*alt=-1;return 0;}
  for(long i=0;i<n;i++){void *d=cf_array_at(array,i);if(cf_dict_get_int64(d,KEY_TYPE,0)!=111)cf_array_append(a,d);}
  return a;
}
int relay_setup(void *session,void *req,void **out) {
  void *array=cf_dict_get_typed(req,KEY_STREAMS,cf_array_type_id(),0);int alt,native;uint64_t conn=0;
  void *filtered=without111(array,&alt,&native,&conn);
  log_event("stage=setup aux=%d native=%d",alt,native);
  if(alt<0)return -1;
  if(!alt) {
    int rc=orig_setup(session,req,out);
    if(!array||cf_array_count(array)==0) {
      int valid=!rc&&refresh_native_profile();
      log_event("stage=initial_setup stock_rc=%d profile111=%d",rc,valid);
    }
    return rc;
  }
  void *copy=cf_dict_create_mutable_copy(0,0,req);if(!copy){cf_release(filtered);return -1;}
  cf_dict_set_value(copy,KEY_STREAMS,filtered);cf_release(filtered);
  int rc=orig_setup(session,copy,out);cf_release(copy);
  if(rc||!out||!*out)return rc;
  void *entry=cf_dict_create_mutable(0,0,dict_key_cb,dict_val_cb);if(!entry)return rc;
  int port=start_relay(session,conn);
  if(port){cf_dict_set_int64(entry,KEY_TYPE,111);cf_dict_set_int64(entry,KEY_DATAPORT,port);add_response_stream(*out,entry);}
  cf_release(entry);log_event("stage=setup_reply aux_listener=%d stock_rc=%d",port>0,rc);return rc;
}
__attribute__((noinline)) int relay_teardown(void *session,void *request,void *ctx,unsigned char *done,void *a5) {
  uintptr_t caller_ra=(uintptr_t)__builtin_return_address(0);
  uintptr_t caller_pc=caller_ra>=8?caller_ra-8:caller_ra;
  /* Stock ABI a1 is a request dictionary, not its Streams array. */
  if(request&&cf_get_type_id(request)!=cf_dict_type_id()) {
    log_event("stage=teardown id=%llu caller_pc=%08lx classification=passthrough request_kind=invalid",relay_id_for_session(session),(unsigned long)caller_pc);
    return stock_teardown(session,request,ctx,done,a5);
  }
  void *array=request?cf_dict_get_typed(request,KEY_STREAMS,cf_array_type_id(),0):0;
  long count=array?cf_array_count(array):0;
  unsigned long long id=relay_id_for_session(session);
  /* Stock treats null/missing/wrong-typed/empty Streams as a full session
   * teardown. Preserve its request and every ABI argument exactly. */
  if(!array||count==0) {
    log_event("stage=teardown id=%llu caller_pc=%08lx classification=full request_kind=%s streams_kind=%s streams=%ld",
        id,(unsigned long)caller_pc,request?"dict":"null",array?"empty":"missing_or_wrong_type",count);
    retire(session,"full_teardown",1,0,0);
    return stock_teardown(session,request,ctx,done,a5);
  }
  int alt,native;void *filtered=without111(array,&alt,&native,0);
  if(alt<0) {
    log_event("stage=teardown id=%llu caller_pc=%08lx classification=passthrough streams_kind=malformed streams=%ld",id,(unsigned long)caller_pc,count);
    return stock_teardown(session,request,ctx,done,a5);
  }
  log_event("stage=teardown id=%llu caller_pc=%08lx classification=streams streams_kind=array aux=%d native=%d streams=%ld",id,(unsigned long)caller_pc,alt,native,count);
  if(!alt)
    return stock_teardown(session,request,ctx,done,a5);
  retire(session,"aux_teardown",0,alt,native);
  if(!native){cf_release(filtered);if(done)*done=0;return 0;}
  void *copy=cf_dict_create_mutable_copy(0,0,request);
  if(!copy){
    cf_release(filtered);
    log_event("stage=teardown id=%llu caller_pc=%08lx filter_copy_failed=1 passthrough=1",id,(unsigned long)caller_pc);
    return stock_teardown(session,request,ctx,done,a5);
  }
  cf_dict_set_value(copy,KEY_STREAMS,filtered);cf_release(filtered);
  int rc=stock_teardown(session,copy,ctx,done,a5);cf_release(copy);return rc;
}
static int bind_hooks(void **setup,void **info,void **teardown,void **property) {
  if(!setup||!info||!teardown||!property||*setup!=(void *)SETUP_VA||*info!=(void *)INFO_VA||*teardown!=(void *)TEARDOWN_VA||*property!=(void *)PLATFORM_COPY_VA)return 0;
  *setup=(void *)&relay_setup;*info=(void *)&relay_info;*teardown=(void *)&relay_teardown;*property=(void *)&relay_platform_copy;return 1;
}
#ifndef RELAY_HOST_TEST
__attribute__((constructor))static void init(void) {
  log_event("stage=constructor");
  int ok=bind_hooks((void **)SETUP_GOT_VA,(void **)INFO_GOT,(void **)TEARDOWN_GOT,(void **)PLATFORM_COPY_GOT);
  log_event("stage=hooks ready=%d expected_stock=october_202610081657",ok);
}
#endif
