#define RELAY_HOST_TEST 1
#include "../src/relay.c"
#include <assert.h>
#include <signal.h>
typedef struct Obj Obj;
struct Obj {int type,n;char text[128];void *key[64];int64_t val[64];Obj *item[64];};
static Obj pool[4096];static unsigned used;static int stock_calls,teardown_calls;
static void *last_request,*last_teardown,*last_td_session,*last_td_ctx,*last_td_a5,*last_td_done,*info_root;
static void *platform_result;
static void *platform(void *s,unsigned flags,void *key,void *q,int *status){(void)s;(void)flags;(void)key;(void)q;if(status)*status=platform_result?0:-6727;return platform_result;}
static Obj *obj(int type){assert(used<4096);Obj *o=&pool[used++];memset(o,0,sizeof *o);o->type=type;return o;}
static const char *key_text(void *k){uintptr_t p=(uintptr_t)k,lo=(uintptr_t)&pool[0],hi=(uintptr_t)(&pool[4096]);return p>=lo&&p<hi?((Obj *)k)->text:0;}
static int equal_key(void *a,void *b){const char *sa=key_text(a),*sb=key_text(b);return a==b||(sa&&sb&&!strcmp(sa,sb));}
static int index_of(Obj *o,void *k){for(int i=0;i<o->n;i++)if(equal_key(o->key[i],k))return i;return -1;}
static void put(void *p,void *k,int64_t v){Obj *o=p;int i=index_of(o,k);if(i<0){assert(o->n<64);i=o->n++;o->key[i]=k;}o->val[i]=v;}
static void put_obj(void *p,void *k,void *v){put(p,k,(intptr_t)v);}
static int64_t get(void *p,void *k,int *e){if(e)*e=0;int i=index_of(p,k);return i<0?0:((Obj *)p)->val[i];}
static unsigned long type(void *p){return p?((Obj *)p)->type:0;}
static void *get_typed(void *p,void *k,unsigned long t,int *e){void *v=(void *)(intptr_t)get(p,k,e);return v&&type(v)==t?v:0;}
static unsigned long array_type(void){return 2;}
static unsigned long dict_type(void){return 1;}
static unsigned long string_type(void){return 3;}
static void *mkdict(void *a,long b,void *c,void *d){(void)a;(void)b;(void)c;(void)d;return obj(1);}
static void *mkarray(void *a,long b,void *c){(void)a;(void)b;(void)c;return obj(2);}
static void *mkstr(void *a,const char *s,unsigned enc){(void)a;(void)enc;Obj *o=obj(3);snprintf(o->text,sizeof o->text,"%s",s);return o;}
static int copystr(void *p,char *dst,long n,unsigned enc){(void)enc;return snprintf(dst,(size_t)n,"%s",((Obj *)p)->text)<n;}
static void *copy(void *a,long b,void *p){(void)a;(void)b;Obj *o=obj(1);*o=*(Obj *)p;return o;}
static long count(void *p){Obj *o=p;return o&&o->type==2?o->n:0;}
static void *at(void *p,long i){return ((Obj *)p)->item[i];}
static void append(void *p,void *v){Obj *o=p;assert(o->n<64);o->item[o->n++]=v;}
static void release(void *p){(void)p;}
static void *keep(void *p){return p;}
static int add(void *r,void *d){append(get_typed(r,KEY_STREAMS,2,0),d);return 0;}
static int stock(void *s,void *r,void **out){(void)s;stock_calls++;last_request=r;Obj *d=obj(1),*a=obj(2),*in=get_typed(r,KEY_STREAMS,2,0);if(in)for(int i=0;i<in->n;i++)append(a,in->item[i]);put_obj(d,KEY_STREAMS,a);*out=d;return 0;}
static int teardown(void *s,void *a,void *c,unsigned char *d,void *e){teardown_calls++;last_teardown=a;last_td_session=s;last_td_ctx=c;last_td_done=d;last_td_a5=e;if(d)*d=0x5a;return 0;}
static void *info(void *s,void *f,int *status){(void)s;(void)f;*status=0;return info_root;}
static Obj *stream(int t){Obj *d=obj(1);put(d,KEY_TYPE,t);put(d,K_CONNID,123);return d;}
static Obj *req(int a,int b){Obj *r=obj(1),*list=obj(2);if(a)append(list,stream(a));if(b)append(list,stream(b));put_obj(r,KEY_STREAMS,list);return r;}
static void set_num(Obj *d,const char *k,int n){put(d,mkstr(0,k,0),n);}
static void set_obj(Obj *d,const char *k,void *v){put_obj(d,mkstr(0,k,0),v);}
typedef struct { Relay *relay;int fd;unsigned char header[128],*body;unsigned capacity;FrameResult result; } RxTask;
static void *rx_task(void *opaque){RxTask *t=opaque;t->result=read_phone_frame(t->relay,t->fd,t->header,&t->body,&t->capacity);return 0;}
static void sleep_ms(long ms){struct timespec ts={ms/1000,(ms%1000)*1000000L};while(nanosleep(&ts,&ts)&&errno==EINTR){}}
static Relay *test_relay(void *session,int fd){Relay *r=calloc(1,sizeof *r);assert(r);assert(!pthread_mutex_init(&r->mu,0));assert(!pthread_cond_init(&r->changed,0));r->refs=1;r->session=session;r->listener=-1;r->phone=fd;r->car=-1;return r;}
static void *publish_task(void *opaque){replace_current((Relay *)opaque);return 0;}
static int legacy_v03_full_classifier(void *a1){int aux,native;void *filtered=without111(a1,&aux,&native,0);if(filtered)cf_release(filtered);return !a1||count(a1)==0;}
static void test_config(unsigned char cfg[64]){memset(cfg,0,64);float v[6]={1280,720,0,0,1280,720};const unsigned off[6]={0x10,0x14,0x20,0x24,0x28,0x2c};for(int i=0;i<6;i++)memcpy(cfg+off[i],&v[i],4);}
int main(void){
  signal(SIGPIPE,SIG_IGN);
  native_info=&info_root;retain_cf=keep;
  stock_platform_copy=platform;
  cf_get_type_id=type;cf_dict_type_id=dict_type;cf_array_type_id=array_type;str_type=string_type;
  cf_dict_create_mutable=mkdict;cf_array_create_mutable=mkarray;str_create=mkstr;str_copy=copystr;
  cf_dict_get_typed=get_typed;cf_dict_get_int64=get;cf_dict_set_int64=put;cf_dict_set_value=put_obj;
  cf_dict_create_mutable_copy=copy;cf_array_count=count;cf_array_at=at;cf_array_append=append;cf_release=release;add_response_stream=add;orig_setup=stock;stock_teardown=teardown;stock_info=info;
  void *out=0;Obj *r=obj(1);assert(!relay_setup((void *)1,r,&out));assert(last_request==r);
  r=req(110,101);assert(!relay_setup((void *)1,r,&out));assert(last_request==r);
  r=req(110,111);assert(!relay_setup((void *)1,r,&out));
  Obj *a=get_typed(last_request,KEY_STREAMS,2,0);assert(a->n==1&&get(a->item[0],KEY_TYPE,0)==110);
  assert(((Obj *)get_typed(r,KEY_STREAMS,2,0))->n==2);
  a=get_typed(out,KEY_STREAMS,2,0);assert(a->n==2&&get(a->item[1],KEY_TYPE,0)==111&&get(a->item[1],KEY_DATAPORT,0)==19831);
  r=req(111,111);int before=stock_calls;assert(relay_setup((void *)1,r,&out)<0&&before==stock_calls);
  unsigned char done=1;Obj *list=0;void *tdctx=(void *)0x3344,*tda5=(void *)0x5566;Obj *meta=obj(1);
  /* Native-only teardown receives and forwards the complete request unchanged. */
  Obj *tdnative=req(110,0);set_obj(tdnative,"vendorMetadata",meta);before=teardown_calls;
  assert(!relay_teardown((void *)1,tdnative,tdctx,&done,tda5));
  assert(teardown_calls==before+1&&last_teardown==tdnative&&last_td_session==(void *)1&&last_td_ctx==tdctx&&last_td_done==&done&&last_td_a5==tda5&&done==0x5a);
  /* Aux-only teardown retires only the relay and never calls stock with empty streams. */
  Obj *tdaux=req(111,0);done=1;before=teardown_calls;assert(!relay_teardown((void *)1,tdaux,tdctx,&done,tda5));
  assert(teardown_calls==before&&!done);
  /* Mixed streams use a copied dictionary, preserve metadata, and filter only Type 111. */
  Obj *tdmixed=req(110,111);set_obj(tdmixed,"vendorMetadata",meta);Obj *original_streams=get_typed(tdmixed,KEY_STREAMS,2,0);done=0;before=teardown_calls;
  assert(legacy_v03_full_classifier(tdmixed)); /* Pinned CFArrayGetCount wrapper returns its initialized zero for a dictionary. */
  Relay *legacy_regression=test_relay((void *)0x7788,-1);replace_current(legacy_regression);
  assert(!relay_teardown((void *)0x7788,tdmixed,tdctx,&done,tda5));
  assert(teardown_calls==before+1&&last_teardown!=tdmixed&&last_td_session==(void *)0x7788&&last_td_ctx==tdctx&&last_td_done==&done&&last_td_a5==tda5&&done==0x5a);
  Obj *filtered_streams=get_typed(last_teardown,KEY_STREAMS,2,0);assert(filtered_streams!=original_streams&&filtered_streams->n==1);
  assert(get(filtered_streams->item[0],KEY_TYPE,0)==110&&get_typed(last_teardown,mkstr(0,"vendorMetadata",0),1,0)==meta&&original_streams->n==2);
  assert(stopped(legacy_regression)&&!legacy_regression->stop_full&&legacy_regression->stop_aux==1&&legacy_regression->stop_native==1&&!strcmp(legacy_regression->stop_reason,"aux_teardown"));drop(legacy_regression);
  /* Stock ABI full semantics: null, missing/wrong-typed key, and empty arrays pass through. */
  done=0;before=teardown_calls;assert(!relay_teardown((void *)1,0,tdctx,&done,tda5));assert(teardown_calls==before+1&&!last_teardown&&done==0x5a&&last_td_ctx==tdctx&&last_td_done==&done&&last_td_a5==tda5);
  Obj *tdmissing=obj(1);done=0;before=teardown_calls;assert(!relay_teardown((void *)1,tdmissing,tdctx,&done,tda5));assert(teardown_calls==before+1&&last_teardown==tdmissing&&done==0x5a);
  Obj *tdwrong=obj(1);put_obj(tdwrong,KEY_STREAMS,mkstr(0,"not-an-array",0));done=0;before=teardown_calls;assert(!relay_teardown((void *)1,tdwrong,tdctx,&done,tda5));assert(teardown_calls==before+1&&last_teardown==tdwrong&&done==0x5a);
  Obj *tdempty=obj(1),*empty_streams=obj(2);put_obj(tdempty,KEY_STREAMS,empty_streams);done=0;before=teardown_calls;assert(!relay_teardown((void *)1,tdempty,tdctx,&done,tda5));assert(teardown_calls==before+1&&last_teardown==tdempty&&done==0x5a);
  /* Unknown malformed shapes fail open: pass original dict and args untouched. */
  Obj *tdbad=obj(1),*bad_streams=obj(2);append(bad_streams,mkstr(0,"malformed-stream",0));put_obj(tdbad,KEY_STREAMS,bad_streams);done=0;before=teardown_calls;
  assert(!relay_teardown((void *)1,tdbad,tdctx,&done,tda5));assert(teardown_calls==before+1&&last_teardown==tdbad&&done==0x5a&&last_td_ctx==tdctx&&last_td_done==&done&&last_td_a5==tda5);
  void *non_dict=mkstr(0,"malformed-request",0);done=0;before=teardown_calls;assert(!relay_teardown((void *)1,non_dict,tdctx,&done,tda5));assert(teardown_calls==before+1&&last_teardown==non_dict&&done==0x5a);
  void *setup=(void *)SETUP_VA,*inf=(void *)INFO_VA,*td=(void *)TEARDOWN_VA,*prop=(void *)PLATFORM_COPY_VA;
  assert(bind_hooks(&setup,&inf,&td,&prop));assert(setup==(void *)&relay_setup&&prop==(void *)&relay_platform_copy);
  setup=(void *)SETUP_VA;inf=(void *)1;td=(void *)TEARDOWN_VA;prop=(void *)PLATFORM_COPY_VA;assert(!bind_hooks(&setup,&inf,&td,&prop)&&setup==(void *)SETUP_VA&&prop==(void *)PLATFORM_COPY_VA);
  assert(uuid_valid("00000000-0000-4000-8000-000000000001"));assert(!uuid_valid("b7e6-bad"));
  Obj *root=obj(1),*displays=obj(2),*display=obj(1);set_num(display,"type",111);set_num(display,"widthPixels",1010);set_num(display,"heightPixels",376);set_obj(display,"uuid",mkstr(0,"00000000-0000-4000-8000-000000000001",0));append(displays,display);set_obj(root,"displays",displays);
  char uuid[128];assert(profile_read(root,uuid)&&!strcmp(uuid,"00000000-0000-4000-8000-000000000001"));
  info_root=root;int status=0;assert(relay_info(0,0,&status)==root&&!strcmp(profile_uuid,uuid));
  Obj *features=dict_get(root,"enabledFeatures",2);assert(features&&features->n==2);relay_info(0,0,&status);assert(((Obj *)dict_get(root,"enabledFeatures",2))->n==2);
  Obj *initial=obj(1);assert(!relay_setup((void *)1,initial,&out));
  /* Model the real stock caller: property lookup overwrites response AFTER Setup. */
  void *feature_property=mkstr(0,"enabledFeatures",0);int feature_status=-1;
  Obj *ack=relay_platform_copy(0,1,feature_property,0,&feature_status);assert(ack&&ack->n==2&&!feature_status);
  set_obj(out,"enabledFeatures",ack);assert(dict_get(out,"enabledFeatures",2)==ack);
  assert(!strcmp(((Obj *)ack->item[0])->text,"altScreen"));
  assert(!strcmp(((Obj *)ack->item[1])->text,"viewAreas"));
  /* Profile can be populated during initial SETUP, before relay_info runs. */
  profile_uuid[0]=0;assert(!relay_setup((void *)1,initial,&out));
  assert(uuid_valid(profile_uuid));
  assert(relay_platform_copy(0,1,feature_property,0,0));
  Obj *response=obj(1),*oem_features=obj(2);append(oem_features,mkstr(0,"uiContext",0));set_obj(response,"enabledFeatures",oem_features);
  assert(advertise(response));Obj *combined=dict_get(response,"enabledFeatures",2);
  assert(combined->n==3&&!strcmp(combined->item[0]->text,"uiContext"));
  assert(advertise(response)&&((Obj *)dict_get(response,"enabledFeatures",2))->n==3);
  platform_result=oem_features;
  ack=relay_platform_copy(0,1,feature_property,0,&feature_status);assert(ack&&ack->n==3&&!strcmp(ack->item[0]->text,"uiContext"));
  void *other=mkstr(0,"unrelatedProperty",0);assert(relay_platform_copy(0,1,other,0,0)==oem_features);
  info_root=0;assert(relay_platform_copy(0,1,feature_property,0,0)==oem_features);info_root=root;platform_result=0;
  set_num(display,"widthPixels",0);assert(!profile_read(root,uuid));relay_info(0,0,&status);assert(!profile_uuid[0]);
  set_num(display,"widthPixels",1010);append(displays,display);assert(!profile_read(root,uuid));
  /* A real five-second receive timeout must not discard a partial frame. */
  int sockets[2];assert(!socketpair(AF_UNIX,SOCK_STREAM,0,sockets));
  Relay io={0};pthread_mutex_init(&io.mu,0);pthread_cond_init(&io.changed,0);io.refs=1;io.listener=io.phone=io.car=-1;
  assert(!socket_timeout(sockets[1]));RxTask task={.relay=&io,.fd=sockets[1]};pthread_t reader;
  assert(!pthread_create(&reader,0,rx_task,&task));
  unsigned char head[128];memset(head,0x31,sizeof head);unsigned payload_len=16;memcpy(head,&payload_len,4);assert(!write_exact(sockets[0],head,sizeof head).error);
  assert(!write_exact(sockets[0],"abc",3).error);sleep_ms(6200);assert(!write_exact(sockets[0],"defghijklmnop",13).error);
  assert(!pthread_join(reader,0));assert(task.result.complete&&task.result.header_rx.state==RX_COMPLETE&&task.result.body_rx.state==RX_COMPLETE);
  assert(task.result.body_rx.idle_timeouts>=1&&!memcmp(task.header,head,sizeof head)&&!memcmp(task.body,"abcdefghijklmnop",16));
  free(task.body);close(sockets[0]);close(sockets[1]);pthread_cond_destroy(&io.changed);pthread_mutex_destroy(&io.mu);
  /* Socket-option failure is surfaced so callers can fail closed. */
  errno=0;assert(socket_timeout(-1)<0&&errno==EBADF);
  /* Peer close reports the exact failed header stage and errno. */
  assert(!socketpair(AF_UNIX,SOCK_STREAM,0,sockets));Relay writer={0};writer.car=sockets[0];close(sockets[1]);
  unsigned char cfg[64],payload[3]={1,2,3};test_config(cfg);ForwardResult fw=forward(&writer,1,payload,sizeof payload,cfg);
  assert(fw.result.error==EPIPE&&fw.result.sent==0&&fw.requested==128&&!strcmp(fw.stage,"config_header"));close(sockets[0]);
  /* A full send buffer times out after a partial payload; exact bytes and errno survive. */
  assert(!socketpair(AF_UNIX,SOCK_STREAM,0,sockets));int small=4096;assert(!setsockopt(sockets[0],SOL_SOCKET,SO_SNDBUF,&small,sizeof small));
  struct timeval send_wait={0,200000};assert(!setsockopt(sockets[0],SOL_SOCKET,SO_SNDTIMEO,&send_wait,sizeof send_wait));
  writer.car=sockets[0];unsigned char *large=(unsigned char *)calloc(1,1024*1024);assert(large);
  fw=forward(&writer,1,large,1024*1024,cfg);assert(fw.result.error==EAGAIN||fw.result.error==EWOULDBLOCK);
  assert(fw.result.sent>0&&fw.result.sent<1024*1024&&fw.requested==1024*1024&&!strcmp(fw.stage,"config_payload"));
  free(large);close(sockets[0]);close(sockets[1]);
  /* EOF at a frame boundary and partial-frame EOF have distinct results. */
  assert(!socketpair(AF_UNIX,SOCK_STREAM,0,sockets));Relay eof={0};pthread_mutex_init(&eof.mu,0);pthread_cond_init(&eof.changed,0);eof.refs=1;eof.listener=eof.phone=eof.car=-1;
  close(sockets[0]);unsigned char eofhdr[128],*eofbody=0;unsigned eofcap=0;FrameResult eof_frame=read_phone_frame(&eof,sockets[1],eofhdr,&eofbody,&eofcap);assert(!eof_frame.complete&&eof_frame.header_rx.state==RX_EOF&&eof_frame.done==0);close(sockets[1]);
  assert(!socketpair(AF_UNIX,SOCK_STREAM,0,sockets));memset(eofhdr,0,sizeof eofhdr);payload_len=8;memcpy(eofhdr,&payload_len,4);assert(!write_exact(sockets[0],eofhdr,sizeof eofhdr).error);assert(!write_exact(sockets[0],"abc",3).error);shutdown(sockets[0],SHUT_WR);eof_frame=read_phone_frame(&eof,sockets[1],eofhdr,&eofbody,&eofcap);assert(!eof_frame.complete&&eof_frame.body_rx.state==RX_PARTIAL_EOF&&eof_frame.done==3);free(eofbody);close(sockets[0]);close(sockets[1]);pthread_cond_destroy(&eof.changed);pthread_mutex_destroy(&eof.mu);
  /* A full teardown wakes a genuinely blocked receive; type-110-only does not. */
  assert(!socketpair(AF_UNIX,SOCK_STREAM,0,sockets));Relay *life=test_relay((void *)0x1234,sockets[1]);replace_current(life);assert(!socket_timeout(sockets[1]));
  RxTask cancel={.relay=life,.fd=sockets[1]};assert(!pthread_create(&reader,0,rx_task,&cancel));
  list=(Obj *)req(110,0);assert(legacy_v03_full_classifier(list)); /* v0.3 would cancel the active aux relay on this native-only request. */
  assert(!relay_teardown(life->session,list,tdctx,&done,tda5));assert(!stopped(life)&&current==life&&done==0x5a&&last_teardown==list&&last_td_ctx==tdctx&&last_td_done==&done&&last_td_a5==tda5);
  assert(!relay_teardown(life->session,0,tdctx,&done,tda5));assert(stopped(life)&&!current&&done==0x5a);
  assert(!pthread_join(reader,0));assert(!cancel.result.complete&&cancel.result.header_rx.state==RX_CANCELLED);
  close(sockets[0]);close(life->phone);life->phone=-1;drop(life);
  /* An auxiliary-only teardown also retires the relay without stock teardown. */
  assert(!socketpair(AF_UNIX,SOCK_STREAM,0,sockets));life=test_relay((void *)0x5678,sockets[1]);replace_current(life);
  list=(Obj *)req(111,0);before=teardown_calls;done=1;assert(!relay_teardown(life->session,list,tdctx,&done,tda5));assert(stopped(life)&&!current&&teardown_calls==before&&!done);
  close(sockets[0]);close(life->phone);life->phone=-1;drop(life);
  /* Old-generation error cleanup must not retire a reused session pointer. */
  Relay *old=test_relay((void *)0x9abc,-1);replace_current(old);
  Relay *newer=test_relay((void *)0x9abc,-1);replace_current(newer);
  unsigned long long old_id=old->relay_id,new_id=newer->relay_id;
  assert(old_id&&new_id>old_id&&stopped(old)&&!stopped(newer));
  retire_relay(old,"late_old_worker_cleanup");assert(current==newer&&!stopped(newer));
  retire_relay(newer,"new_worker_cleanup");assert(!current&&stopped(newer));drop(old);drop(newer);
  /* Concurrent publications are atomic: exactly one current reference survives. */
  Relay *left=test_relay((void *)0x1111,-1),*right=test_relay((void *)0x2222,-1);pthread_t publishers[2];
  assert(!pthread_create(&publishers[0],0,publish_task,left));assert(!pthread_create(&publishers[1],0,publish_task,right));
  assert(!pthread_join(publishers[0],0));assert(!pthread_join(publishers[1],0));
  pthread_mutex_lock(&current_mu);Relay *winner=current;pthread_mutex_unlock(&current_mu);
  assert(winner&&(winner==left||winner==right)&&winner->relay_id&&winner->session_generation);
  Relay *loser=winner==left?right:left;assert(stopped(loser));
  retire_relay(winner,"test_complete");assert(!current&&stopped(winner));drop(left);drop(right);
  for(int i=0;i<200;i++) {
    Relay *r=calloc(1,sizeof *r);pthread_mutex_init(&r->mu,0);pthread_cond_init(&r->changed,0);
    r->refs=1;r->session=(void *)1;r->listener=r->phone=r->car=-1;
    current=r;hold(r);retire((void *)2,"wrong_session",0,0,0);assert(current==r&&!stopped(r));
    retire((void *)1,"full_teardown",1,0,0);assert(!current&&stopped(r));
    /* Late callback/worker references remain usable after current is retired. */
    assert(r->refs==1);hold(r);drop(r);assert(r->refs==1);drop(r);
  }
  puts("INDEPENDENT_HOST=PASS stock_main_audio=1 no_stream_setup=1 split_setup=1 request_dict_teardown=1 legacy_v03_full_misclassification=1 native110_legacy_full_v04_preserve=1 native_request_identity=1 aux_only_skip_stock=1 mixed_dict_filter=1 full_null_missing_empty=1 malformed_passthrough=1 all_abi_args_preserved=1 worker_frame_idle_resume=1 partial_timeout=1 eof=1 blocked_read_cancel=1 native110_full_teardown=1 atomic_registration=1 stale_cleanup_safe=1 peer_close_write_error=1 partial_send_errno_bytes=1 timeout_setup_fail_closed=1 vehicle=UNTESTED");
  return 0;
}
