#define _GNU_SOURCE
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "initial_profile.h"
#include "geometry.h"

#define SETUP_VA 0x475668u
#define SETUP_GOT_VA 0x56526cu
#define ADD_RESPONSE_STREAM_VA 0x470224u
#define SERVER_SOCKET_OPEN_VA 0x4c95e4u
#define KEY_STREAMS ((void *)0x52a068)
#define KEY_TYPE ((void *)0x52a0c0)
#define KEY_DATAPORT ((void *)0x52a210)
#define CB_ARRAY_VA ((void *)0x55f47c)
#define CB_DICT_KEY_VA ((void *)0x55f464)
#define CB_DICT_VAL_VA ((void *)0x55f450)
#define ALT_SCREEN_TYPE 111

typedef int (*setup_fn)(void *, void *, void **);
typedef int (*add_stream_fn)(void *, void *);
typedef int (*server_socket_open_fn)(int, int, int, int, int *, unsigned, int *);
typedef unsigned long (*type_id_fn)(void);
typedef void *(*dget_typed_fn)(void *, void *, unsigned long, int *);
typedef int64_t (*dget_i64_fn)(void *, void *, int *);
typedef void (*dset_i64_fn)(void *, void *, int64_t);
typedef void (*dset_val_fn)(void *, void *, void *);
typedef void *(*dmk_fn)(void *, long, void *, void *);
typedef void *(*dmkcopy_fn)(void *, long, void *);
typedef void *(*amk_fn)(void *, long, void *);
typedef long (*acount_fn)(void *);
typedef void *(*aat_fn)(void *, long);
typedef void (*aappend_fn)(void *, void *);
typedef void (*release_fn)(void *);

static setup_fn orig_setup = (setup_fn)SETUP_VA;
static add_stream_fn add_response_stream = (add_stream_fn)ADD_RESPONSE_STREAM_VA;
static server_socket_open_fn server_socket_open = (server_socket_open_fn)SERVER_SOCKET_OPEN_VA;
static type_id_fn cf_array_type_id = (type_id_fn)0x48fc3cu;
static dget_typed_fn cf_dict_get_typed = (dget_typed_fn)0x4a2e8cu;
static dget_i64_fn cf_dict_get_int64 = (dget_i64_fn)0x4a2ba4u;
static dset_i64_fn cf_dict_set_int64 = (dset_i64_fn)0x4a4e60u;
static dset_val_fn cf_dict_set_value = (dset_val_fn)0x490b00u;
static dmk_fn cf_dict_create_mutable = (dmk_fn)0x4908dcu;
static dmkcopy_fn cf_dict_create_mutable_copy = (dmkcopy_fn)0x490a54u;
static amk_fn cf_array_create_mutable = (amk_fn)0x48fdd8u;
static acount_fn cf_array_count = (acount_fn)0x48fe50u;
static aat_fn cf_array_at = (aat_fn)0x48fea4u;
static aappend_fn cf_array_append = (aappend_fn)0x490044u;
static release_fn cf_release = (release_fn)0x48fbb8u;
static void *arr_cb = CB_ARRAY_VA, *dict_key_cb = CB_DICT_KEY_VA, *dict_val_cb = CB_DICT_VAL_VA;

#define CLUSTER_STREAM_TYPE 0x6f


#define K_STREAMS ((void *)0x51e41c)
#define K_TYPE ((void *)0x51e42c)
#define K_DATAPORT ((void *)0x51e43c)
#define K_CONNID ((void *)0x51e450)
#define K_UUID ((void *)0x51e46c)
#define K_LATENCYMS ((void *)0x51e47c)
#define S_SETUP ((void *)0x51b488)
#define S_URL ((void *)0x51e074)
#define S_PROTO ((void *)0x51aa38)
#define S_MIME ((void *)0x51aabc)
#define GPEERADDR ((void *)0x568700)
#define GAIRPLAYCLIENT ((void **)0x566f04)
#define GPAIRVERIFY_HK ((int *)0x567080)
#define GSESSION_AESKEY ((void *)0x5670a0)

typedef int (*uuidget_fn)(void *);
typedef int (*httpmk_fn)(void **);
typedef int (*httpinit_fn)(void *, void *, void *, void *);
typedef int (*httpaddf_fn)(void *, void *, void *);
typedef void *(*httpsetbody_fn)(void *, void *, void *, int);
typedef int (*httpsend_fn)(void *, void *);
typedef void (*aensure_fn)(void **, void *);
typedef void *(*plistmk_fn)(void *, int *, int);
typedef void *(*plistrd_fn)(void *, int, int *);
typedef void (*dsetcstr_fn)(void *, void *, void *, unsigned);
typedef void *(*semmk_fn)(int);
typedef void (*semwait_fn)(void *, unsigned long long);
typedef void (*semsig_fn)(void *);
typedef void (*sockcopy_fn)(void *, void *);
typedef void (*sockport_fn)(void *, int);
typedef int (*socksize_fn)(void *);
typedef void (*qos_fn)(int, int);
typedef unsigned long (*typeid_fn)(void);
typedef void *(*aat_typed_fn)(void *, long, unsigned long, int *);
/* Stock routine has no status return; v0 is a scratch loop counter at return. */
typedef void (*derive_fn)(void *, int, void *, int, void *, int, void *, void *);
typedef void (*aesinit_fn)(void *, void *, void *);
typedef void *(*at_fn)(void *, long);

static uuidget_fn cf_uuidget = (uuidget_fn)0x4e8040u;
static httpmk_fn http_create = (httpmk_fn)0x4b6bccu;
static httpinit_fn http_init = (httpinit_fn)0x4b9be0u;
static httpaddf_fn http_addf = (httpaddf_fn)0x4ba280u;
static httpsetbody_fn http_setbody = (httpsetbody_fn)0x4b6cbcu;
static httpsend_fn http_send = (httpsend_fn)0x4b6760u;
static aensure_fn cf_arr_ensure = (aensure_fn)0x4a17d4u;
static plistmk_fn cf_plist_create = (plistmk_fn)0x49a9e4u;
static plistrd_fn cf_plist_read = (plistrd_fn)0x49ba74u;
static dsetcstr_fn cf_dict_set_cstr = (dsetcstr_fn)0x4a206cu;
static semmk_fn d_sem_create = (semmk_fn)0x4b0744u;
static semwait_fn d_sem_wait = (semwait_fn)0x4b0c8cu;
static semsig_fn d_sem_signal = (semsig_fn)0x4b0898u;
static sockcopy_fn sock_copy = (sockcopy_fn)0x4ca98cu;
static sockport_fn sock_setport = (sockport_fn)0x4c9d70u;
static socksize_fn sock_getsize = (socksize_fn)0x4c91c0u;
static qos_fn sock_qos = (qos_fn)0x4c85d0u;
static typeid_fn cf_dict_type_id = (typeid_fn)0x4908c4u;
typedef unsigned long (*gettypeid_fn)(void *);
static gettypeid_fn cf_get_type_id = (gettypeid_fn)0x48fb30u;
static derive_fn aes_derive = (derive_fn)0x42cde8u;
static aesinit_fn aes_ctr_init = (aesinit_fn)0x488e10u;
static at_fn cf_array_at2 = (at_fn)0x48fea4u;

typedef void (*derive_screen_fn)(void *, int, unsigned, unsigned, void *, void *);
typedef void (*aesctrinit_fn)(void *, void *, void *);
typedef void (*aesupd_fn)(void *, void *, int, void *);
static derive_screen_fn derive_screen = (derive_screen_fn)0x47f180u;
static aesctrinit_fn aesctr_init = (aesctrinit_fn)0x488e10u;
static aesupd_fn aesctr_update = (aesupd_fn)0x488e94u;

typedef int (*streamkeys_fn)(void *, int, unsigned, unsigned, int, void *, void *);
typedef void (*ccinit_fn)(void *, void *, void *);
typedef void (*ccaad_fn)(void *, void *, int);
typedef int (*ccdec_fn)(void *, void *, int, void *);
typedef int (*ccver_fn)(void *, void *, void *, int *);
static streamkeys_fn get_stream_keys = (streamkeys_fn)0x4718d4u;
static ccinit_fn cc_init = (ccinit_fn)0x4a7e00u;
static ccaad_fn cc_aad = (ccaad_fn)0x4a83ccu;
static ccdec_fn cc_dec = (ccdec_fn)0x4a8550u;
static ccver_fn cc_ver = (ccver_fn)0x4a87b4u;
typedef int (*sendcmd_fn)(void *, void *, void *, void *);
static sendcmd_fn session_send_command = (sendcmd_fn)0x472650u;
#define K_CMD_PARAMS ((void *)0x52a250)
#define K_CMD_URL ((void *)0x52a318)
#define CLUSTER_MAP_URL INITIAL_CLUSTER_MAP_URL
