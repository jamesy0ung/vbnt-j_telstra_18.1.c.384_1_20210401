/************* COPYRIGHT AND CONFIDENTIALITY INFORMATION *****************
*
* Copyright (c) 2017 Technicolor
* All Rights Reserved
*
* This is free software, licensed under the GNU General Public License v2.
* See /LICENSE for more information.
*
**************************************************************************/

/*#######################################################################
#                                                                      #
#  HEADER (INCLUDE) SECTION                                            #
#                                                                      #
###################################################################### */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <linux/netfilter.h>
#include <errno.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <libubus.h>
#include <libubox/blobmsg_json.h>
#include <libubox/list.h>
#include <signal.h>
#include <syslog.h>
#include <pthread.h>
#include <semaphore.h>
#include <net/if.h>
#include "uci.h"
#include <ctype.h>

/*#######################################################################
#                                                                      #
#  TYPES                                                               #
#                                                                      #
###################################################################### */
#define LB_TRACELEVEL_ERROR 1
#define LB_TRACELEVEL_CRIT  2
#define LB_TRACELEVEL_DEBUG 3
#define LB_TRACELEVEL_INFO  4

#define LB_ERR(str, args ...) \
    if(trace_level >= LB_TRACELEVEL_ERROR) { syslog(LOG_DEBUG, "[load_balancer]:ERR: %s:%d - " str, \
           __func__, __LINE__, ## args); }
#define LB_CRT(str, args ...) \
    if(trace_level >= LB_TRACELEVEL_CRIT)  { syslog(LOG_DEBUG, "[load_balancer]:CRT: %s:%d - " str, \
           __func__, __LINE__, ## args); }
#define LB_DBG(str, args ...) \
    if(trace_level >= LB_TRACELEVEL_DEBUG) { syslog(LOG_DEBUG, "[load_balancer]:DBG: %s:%d - " str, \
           __func__, __LINE__, ## args); }
#define LB_INF(str, args ...) \
    if(trace_level >= LB_TRACELEVEL_INFO) { syslog(LOG_DEBUG, "[load_balancer]:INF: %s:%d - " str, \
           __func__, __LINE__, ## args); }

#define MAX_PATH_LEN 128
#define CMD_MAX_LEN 128
#define UBUS_TIMEOUT_MILLISECS 300 /* ubus timeout in milliseconds*/
#define PATH_PROCNET_DEV "/proc/net/dev"

/*#######################################################################
#                                                                      #
#  INTERNAL FUNCTIONS PROTOTYPES                                       #
#                                                                      #
###################################################################### */
static void uloop_timeout_cb(struct uloop_timeout *t);

/*#######################################################################
#                                                                      #
#  VARIABLES                                                           #
#                                                                      #
###################################################################### */
/* global */
static int trace_level = LB_TRACELEVEL_ERROR;
static pthread_mutex_t lb_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lb_mutex_reload = PTHREAD_MUTEX_INITIALIZER;
static struct uloop_timeout u_timeout;

/* nfq */
static struct nfq_handle *handler;
static struct nfq_q_handle *q_handler;

/* load balancer */
typedef struct
{
  bool enabled;
  char primary[IF_NAMESIZE];
  char secondary[IF_NAMESIZE];
  unsigned int nfmark;
  unsigned int threshold_max;
  unsigned int threshold_min;
  bool upstream;
  unsigned int interval;
  unsigned long long bandwidth;
  bool failback;
  uint32_t queue;
} load_balancer_cfg;

typedef struct
{
  bool primary_up;
  bool secondary_up;
  char l3_primary[IF_NAMESIZE];
  bool set_mark;
  unsigned long long bandwidth;
  unsigned long long last_bytes;
  unsigned long last_sec;
  unsigned long long used_bandwidth;
} load_balancer_status;

typedef struct
{
  char ifname[IF_NAMESIZE];
  bool status;
  char l3_ifname[IF_NAMESIZE];
} load_balancer_intf_status;

static load_balancer_cfg lb_cfg;
static load_balancer_status lb_status;
static int lb_config_reloaded = 0;

/* ubus */
static struct ubus_context *ctx;
static struct ubus_context *ctx1;

enum
{
  IFACE_EVENT_ATTR_ACTION,
  IFACE_EVENT_ATTR_IFNAME,
  IFACE_EVENT_ATTR_MAX
};

static const struct blobmsg_policy event_attrs[IFACE_EVENT_ATTR_MAX] =
{
  [IFACE_EVENT_ATTR_ACTION] = { .name = "action", .type = BLOBMSG_TYPE_STRING },
  [IFACE_EVENT_ATTR_IFNAME] = { .name = "interface", .type = BLOBMSG_TYPE_STRING },
};

enum
{
  IFACE_STATUS_ATTR_UP,
  IFACE_STATUS_ATTR_L3_DEVICE,
  IFACE_STATUS_ATTR_MAX
};

static const struct blobmsg_policy iface_attrs[IFACE_STATUS_ATTR_MAX] =
{
  [IFACE_STATUS_ATTR_UP] = { .name = "up", .type = BLOBMSG_TYPE_BOOL},
  [IFACE_STATUS_ATTR_L3_DEVICE] = { .name = "l3_device", .type = BLOBMSG_TYPE_STRING },
};

typedef struct
{
  unsigned long long rx_bytes;
  unsigned long long rx_packets;
  unsigned long rx_errs;
  unsigned long rx_drop;
  unsigned long rx_fifo;
  unsigned long rx_frame;
  unsigned long rx_compressed;
  unsigned long rx_multicast;

  unsigned long long tx_bytes;
  unsigned long long tx_packets;
  unsigned long tx_errs;
  unsigned long tx_drop;
  unsigned long tx_fifo;
  unsigned long tx_frame;
  unsigned long tx_compressed;
  unsigned long tx_multicast;
} load_balancer_net_dev_info;

static sem_t sem;

/*#######################################################################
#                                                                      #
#  INTERNAL FUNCTIONS                                                  #
#                                                                      #
###################################################################### */

/****************************************************************************
 * get nfmark of an interface
*/
static unsigned int get_intf_nfmark(char *intf)
{
  unsigned int nfmark = 0;
  struct uci_context *uci_ctx = NULL;
  struct uci_package *pkg = NULL;
  struct uci_element *e = NULL;
  struct uci_section *s = NULL;

  uci_ctx = uci_alloc_context();
  if(uci_ctx == NULL)
  {
    LB_ERR("uci_alloc_context failed");
    return 1;
  }

  uci_set_savedir(uci_ctx, "/var/state");
  if(uci_load(uci_ctx, "mwan", &pkg) != UCI_OK)
  {
    LB_ERR("loading UCI configuration failed");
    uci_free_context(uci_ctx);
    return 1;
  }

  // Iterate over all UCI configuration sections
  uci_foreach_element(&pkg->sections, e)
  {
    s = uci_to_section(e);
    if((!strcmp(s->type, "policy")) && !s->anonymous )
    {
      const char *interface = NULL;
      const char *mark = NULL;

      // find policy name.  e.g. : mwan.voip_only.interface='wwan'
      interface = uci_lookup_option_string(uci_ctx, s, "interface");
      if(interface != NULL && !strcmp(interface, intf))
      {
        // get policy nfmark. e.g. : mwan.voip_only.nfmark='0x10000000'
        mark = uci_lookup_option_string(uci_ctx, s, "nfmark");
        if(mark != NULL)
        {
          nfmark = strtol(mark, NULL, 16); //string 0x10000000 to hex value
          LB_DBG("get_intf_nfmark :  %x\r\n", nfmark);
        }
        goto out;
      }
    }
  }

out:
  uci_unload(uci_ctx, pkg);
  uci_free_context(uci_ctx);
  return nfmark;
}

/****************************************************************************
 * print configuration
*/
static void lb_print(load_balancer_cfg cfg)
{
  LB_DBG("  Load Balancer Configuration:\n");
  LB_DBG("  primary=%s, secondary=%s\n", cfg.primary, cfg.secondary);
  LB_DBG("  nfmark=%x, threshold_max=%d, threshold_min=%d\n", cfg.nfmark, cfg.threshold_max, cfg.threshold_min);
  LB_DBG("  upstream=%d, interval=%d\n", cfg.upstream, cfg.interval);
  LB_DBG("  bandwidth=%llu Kbps,failback=%d\n", cfg.bandwidth, cfg.failback);
  LB_DBG("  queue=%d\n", cfg.queue);
  LB_DBG("  \n");
}

/****************************************************************************
 * get rx/tx packages of given interface via
 * /proc/net/dev
 * return : number of packages in Bytes
*/
static unsigned long long sys_get_layer3_bytes(char * interface, bool tx)
{
  FILE *fp;
  char buf[512];
  load_balancer_net_dev_info dev_info;

  fp = fopen(PATH_PROCNET_DEV, "r");
  if(!fp)
  {
    LB_ERR("open %s failed\n", PATH_PROCNET_DEV);
    return 0;
  }

  fgets(buf, sizeof buf, fp);
  fgets(buf, sizeof buf, fp);
  memset(&dev_info, 0, sizeof(load_balancer_net_dev_info));

  while (fgets(buf, sizeof(buf), fp))
  {
    char ifname[IF_NAMESIZE];

    sscanf(buf, "%[^:]: %llu %llu %lu %lu %lu %lu %lu %lu %llu %llu %lu %lu %lu %lu %lu %lu",
	        ifname,
                &dev_info.rx_bytes,
                &dev_info.rx_packets,
                &dev_info.rx_errs,
                &dev_info.rx_drop,
                &dev_info.rx_fifo,
                &dev_info.rx_frame,
                &dev_info.rx_compressed,
                &dev_info.rx_multicast,
                &dev_info.tx_bytes,
                &dev_info.tx_packets,
                &dev_info.tx_errs,
                &dev_info.tx_drop,
                &dev_info.tx_fifo,
                &dev_info.tx_frame,
                &dev_info.tx_compressed,
                &dev_info.tx_multicast
	  );
    if (interface && !strcmp(interface, ifname))
    {
      break;
    }
    memset(&dev_info, 0, sizeof(load_balancer_net_dev_info));
  }

  fclose(fp);
  return tx ? dev_info.tx_bytes : dev_info.rx_bytes ;
}

/****************************************************************************
 * e.g. primary link is xDSL, secondary link is LTE
 *
 * Monitor used bandwidth (UB) on xDSL interface:
 *
 * UB > TMAX => modify routing policy to prefer LTE link
 *   - Existing traffic flows will remain on xDSL
 *   - New traffic flows will be forwarded over LTE
 * UB < TMIN => modify routing table to prefer xDSL link
 *   - Existing traffic flows on LTE will remain on LTE
 *   - New traffic flows will be forwarded over xDSL
 *
 * TMAX  : Max. Bandwidth Threshold
 * E.g. 80 % of MB
 * TMIN  : Percentage of Max. Bandwidth Threshold
 * E.g. 75% of MB
 *
 * set_mark == true:  secondary interface
 * set_mark == false: primary interface
*/
static void load_balancer_check_bandwidth()
{
  if(lb_status.primary_up && lb_status.secondary_up)
  {
    if(pthread_mutex_lock(&lb_mutex))
    {
      LB_ERR("fail to lock lb_mutex\n");
      return;
    }
    if(lb_status.set_mark)
    {
      if( (lb_status.used_bandwidth < (lb_status.bandwidth * lb_cfg.threshold_min / 100))
        || ( lb_config_reloaded && (lb_status.used_bandwidth < (lb_status.bandwidth * lb_cfg.threshold_max / 100))) )
      {
        lb_status.set_mark = false;
      }
    }
    else
    {
      if(lb_status.used_bandwidth > lb_status.bandwidth * lb_cfg.threshold_max / 100)
      {
        lb_status.set_mark = true;
      }
    }
    if(lb_config_reloaded)
    {
      lb_config_reloaded = 0;
    }

    if(pthread_mutex_unlock(&lb_mutex))
    {
      LB_ERR("fail to unlock lb_mutex\n");
      return;
    }
    LB_INF("Load balancer worked %d, used bandwidth is %llu\n", lb_status.set_mark, lb_status.used_bandwidth);
  }
  return;
}

/****************************************************************************
 * timeout cb:
 * To calculate the UB (used bandwidth) of primary interface in Kbps
 * And re-enable timer.
*/
static void uloop_timeout_cb(struct uloop_timeout *t)
{
  struct timespec ts;
  unsigned long long cur_bytes = 0;
  unsigned int interval = 0;

  if(pthread_mutex_lock(&lb_mutex_reload))
  {
    LB_ERR("fail to lock lb_mutex_reload\n");
    return;
  }

  if(lb_status.primary_up && lb_status.secondary_up)
  {
    cur_bytes = sys_get_layer3_bytes(lb_status.l3_primary, lb_cfg.upstream);
    LB_INF("%s : cur=%llu Bytes, last=%llu Bytes\n", lb_status.l3_primary, cur_bytes, lb_status.last_bytes);

    /* in case load_balancer is blocked */
    clock_gettime(CLOCK_MONOTONIC, &ts);
    interval = ts.tv_sec - lb_status.last_sec;

    if(lb_status.last_sec > 0 && lb_status.last_bytes > 0 && cur_bytes >= lb_status.last_bytes)
    {
      lb_status.used_bandwidth = (cur_bytes - lb_status.last_bytes) * 8 / 1000 / interval; //kbps
    }

    LB_INF("     used bandwidth=%llu Kbps,upstream=%d,bandwidth=%llu.\n", lb_status.used_bandwidth, lb_cfg.upstream, lb_status.bandwidth);
    lb_status.last_bytes = cur_bytes;
    lb_status.last_sec = ts.tv_sec;

    load_balancer_check_bandwidth();
  }
  else
  {
    LB_DBG("primary is %d, secondary is %d\n", lb_status.primary_up, lb_status.secondary_up);
  }

  /* re-enable timer */
  uloop_timeout_set(&u_timeout, lb_cfg.interval * 1000);
  if(pthread_mutex_unlock(&lb_mutex_reload))
  {
    LB_ERR("fail to unlock lb_mutex_reload\n");
  }
}

/****************************************************************************
 * if failback enabled, secondary interface will be used if primary is down
 *
*/
static void fail_over_checker()
{
  if(!lb_status.primary_up && lb_status.secondary_up && lb_cfg.failback)
  {
    if(pthread_mutex_lock(&lb_mutex))
    {
       LB_ERR("fail to lock lb_mutex\n");
       return;
    }
    lb_status.set_mark = true;
    if(pthread_mutex_unlock(&lb_mutex))
    {
      LB_ERR("fail to unlock lb_mutex\n");
      return;
    }
    LB_DBG("Load balancer fail-over working\n");
  }
  else
  {
    if(pthread_mutex_lock(&lb_mutex))
    {
       LB_ERR("fail to lock lb_mutex\n");
       return;
    }
    lb_status.set_mark = false;
    if(pthread_mutex_unlock(&lb_mutex))
    {
      LB_ERR("fail to unlock lb_mutex\n");
      return;
    }
    LB_DBG("Load balancer fail-over not working\n");
  }
  return;
}

/****************************************************************************
 * callback of ubus call
*/
static void ubus_call_cb(struct ubus_request *req, int type, struct blob_attr *msg)
{
  struct blob_attr *tb[IFACE_EVENT_ATTR_MAX];
  struct blob_attr *cur;
  load_balancer_intf_status * intf_status = (load_balancer_intf_status *)req->priv;
  char *ifname = intf_status->ifname;

  blobmsg_parse(iface_attrs, IFACE_STATUS_ATTR_MAX, tb, blob_data(msg), blob_len(msg));
  if((cur = tb[IFACE_STATUS_ATTR_UP]))
  {
    intf_status->status = blobmsg_get_bool(cur);
    LB_DBG("Status of intf %s is %d\n",ifname, intf_status->status);
    if(intf_status->status && (cur = tb[IFACE_STATUS_ATTR_L3_DEVICE]))
    {
      /* to get l3_device of interface. eg : wan -> pppoe-wan */
      strcpy(intf_status->l3_ifname, blobmsg_get_string(cur));
      LB_DBG("L3 Interface is %s \n", intf_status->l3_ifname);
    }
    else
    {
      memset(intf_status->l3_ifname, 0, IF_NAMESIZE);
    }
  }
}

/****************************************************************************
 * execute: ubus call network.interface.<intf> status
 * to get status of <intf>
*/
static int ubus_call_interface_status(struct ubus_context *ctx, load_balancer_intf_status * intf_status)
{
  int ret;
  uint32_t id;
  char intf_path[MAX_PATH_LEN];
  char *interface = intf_status->ifname;

  snprintf(intf_path, sizeof(intf_path), "network.interface.%s", interface);
  ret = ubus_lookup_id(ctx, intf_path, &id);
  if(ret)
  {
    LB_ERR("Failed to lookup id\n");
    return -1;
  }
  ret = ubus_invoke(ctx, id, "status", NULL, ubus_call_cb, intf_status, UBUS_TIMEOUT_MILLISECS);
  if(ret)
  {
    LB_ERR("Error invoking request : network.interface.%s\n", interface);
    return -1;
  }

  return 0;
}

/****************************************************************************
 * ubus event handler
*/
static void ubus_network_event_handler(struct ubus_context *ctx, struct ubus_event_handler  *ev,
               const char *type, struct blob_attr *msg)
{
  struct blob_attr *tb[IFACE_EVENT_ATTR_MAX];
  struct blob_attr *cur;

  blobmsg_parse(event_attrs, IFACE_EVENT_ATTR_MAX, tb, blob_data(msg), blob_len(msg));
  cur = tb[IFACE_EVENT_ATTR_IFNAME];
  if(!cur)
  {
    return;
  }
  if(!(blobmsg_get_string(cur)))
  {
    return;
  }
  cur = tb[IFACE_EVENT_ATTR_ACTION];
  if(cur)
  {
    /* Attention:
    * ubus event is NOT reliable!
    * So check again! */
    load_balancer_intf_status intf_status;

    strcpy(intf_status.ifname, lb_cfg.primary);
    ubus_call_interface_status(ctx, &intf_status);
    lb_status.primary_up = intf_status.status;
    strcpy(lb_status.l3_primary, intf_status.l3_ifname);

    strcpy(intf_status.ifname, lb_cfg.secondary);
    ubus_call_interface_status(ctx, &intf_status);
    lb_status.secondary_up = intf_status.status;

    fail_over_checker();
    LB_DBG("ubus_network_event_handler primary is %d, secondary is %d\n", lb_status.primary_up, lb_status.secondary_up);
  }
}

/****************************************************************************
 * ubus event listen: network.interface
*/
static void ubus_network_event_listen()
{
  struct ubus_event_handler listener;

  memset(&listener, 0, sizeof(listener));
  listener.cb = ubus_network_event_handler;
  if(ubus_register_event_handler(ctx, &listener, "network.interface"))
  {
    LB_ERR("listen event failed \n");
    exit(1);
  }
}

/****************************************************************************
 * nfq callback:
 * check load balance & fail back.
 * work with firewall rule(s).
*/
static int nfq_cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
        struct nfq_data *nfa, void *data)
{
  bool mark = false;
  struct nfqnl_msg_packet_hdr *ph;
  uint32_t id;

  ph = nfq_get_msg_packet_hdr(nfa);
  if(ph)
  {
    id = ntohl(ph->packet_id);
  }
  else
  {
    return -1;
  }

  if(lb_cfg.enabled == false)
  {
    LB_DBG("load balancer is disabled\n");
    return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
  }

  if(pthread_mutex_lock(&lb_mutex))
  {
    LB_ERR("fail to lock lb_mutex\n");
    return -1;
  }
  mark = lb_status.set_mark;
  if(pthread_mutex_unlock(&lb_mutex))
  {
    LB_ERR("fail to unlock lb_mutex\n");
    return -1;
  }
  if(mark)
  {
    LB_DBG("nfq mark enabled\n");
    return nfq_set_verdict2(qh, id, NF_ACCEPT, lb_cfg.nfmark, 0, NULL);
  }
  else
  {
    LB_DBG("nfq mark disabled\n");
    return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
  }
}

/****************************************************************************
 * nfq bind; queue create;
*/
static int nfq_init()
{
  handler = nfq_open();
  if(!handler)
  {
    LB_ERR("error during nfq_open()\n");
    return -1;
  }

  if(nfq_unbind_pf(handler, AF_INET) < 0)
  {
    LB_ERR("error during nfq_unbind_pf()\n");
    return -1;
  }

  if(nfq_bind_pf(handler, AF_INET) < 0)
  {
    LB_ERR("error during nfq_bind_pf()\n");
    return -1;
  }

  LB_CRT("binding this socket to queue '%d'\n", lb_cfg.queue);
  q_handler = nfq_create_queue(handler, lb_cfg.queue, &nfq_cb, NULL);
  if(!q_handler)
  {
    LB_ERR("error during nfq_create_queue()\n");
    return -1;
  }

  if(nfq_set_mode(q_handler, NFQNL_COPY_META, 0xffff) < 0)
  {
    LB_ERR("can't set packet_copy mode\n");
    return -1;
  }
  return nfq_fd(handler);
}

/****************************************************************************
 * nfq destroy; unbind; close
*/
static void nfq_exit()
{
  nfq_destroy_queue(q_handler);
  nfq_unbind_pf(handler, AF_INET);
  nfq_close(handler);
  LB_CRT("nfq exit\n");
}


/****************************************************************************
 * new thread for ubus event and timer
*/
static void* lb_ubus_service(void *lb_service)
{
  /* ubus init */
  ctx = ubus_connect(NULL);
  if(!ctx)
  {
    LB_ERR("Failed to connect to ubus\n");
    exit(1);
  }

  /* get status of primary and secondary interface */
  load_balancer_intf_status intf_status;

  strcpy(intf_status.ifname, lb_cfg.primary);
  if(ubus_call_interface_status(ctx, &intf_status))
  {
    LB_ERR("Invalid primary interface %s\n", lb_cfg.primary);
    exit(1);
  }
  else
  {
    lb_status.primary_up = intf_status.status;
    strcpy(lb_status.l3_primary, intf_status.l3_ifname);
  }

  strcpy(intf_status.ifname, lb_cfg.secondary);
  if(ubus_call_interface_status(ctx, &intf_status))
  {
    LB_ERR("Invalid secondary interface %s\n", lb_cfg.secondary);
    exit(1);
  }
  else
  {
    lb_status.secondary_up = intf_status.status;
  }

  LB_INF("Load Balancer Status:\n");
  LB_INF("  primary-up=%d , secondary-up=%d\n\n", lb_status.primary_up, lb_status.secondary_up);

  /* ubus event */
  if(uloop_init() != 0)
  {
    ubus_free(ctx);
    exit(1);
  }
  ubus_add_uloop(ctx);
  ubus_network_event_listen();

  /* start timer */
  u_timeout.cb = uloop_timeout_cb;
  uloop_timeout_set(&u_timeout, lb_cfg.interval * 1000);

  uloop_run();
  uloop_done();
  ubus_free(ctx);

  return NULL;
}

/****************************************************************************
 * load/reload cofniguration of load balancer
*/
static int lb_config_load(int reload)
{
  struct uci_context *uci_ctx = NULL;
  struct uci_package *pkg = NULL;
  struct uci_element *e = NULL;
  struct uci_section *s = NULL;
  int load_nok = 1;
  load_balancer_cfg load_cfg;

  memset(&load_cfg, 0, sizeof(load_cfg));
  uci_ctx = uci_alloc_context();
  if(uci_ctx == NULL)
  {
    LB_ERR("uci_alloc_context failed");
    return 1;
  }

  uci_set_confdir(uci_ctx, "/etc/config");
  uci_set_savedir(uci_ctx, "/dev/null");

  if(uci_load(uci_ctx, "mwan", &pkg) != UCI_OK)
  {
    LB_ERR("loading UCI configuration failed");
    uci_free_context(uci_ctx);
    return 1;
  }

  // Iterate over all UCI configuration sections
  uci_foreach_element(&pkg->sections, e)
  {
    s = uci_to_section(e);

    if((!strcmp(s->type, "load_balance")) && !s->anonymous && !strcmp(s->e.name, "global"))
    {
      const char *enabled = NULL;
      const char *threshold_max = NULL;
      const char *threshold_min = NULL;
      const char *primary = NULL;
      const char *secondary = NULL;
      const char *interval = NULL;
      const char *upstream = NULL;
      const char *failover = NULL;
      const char *bandwidth = NULL;
      const char *queue = NULL;
      const char *level = NULL;

      enabled = uci_lookup_option_string(uci_ctx, s, "enabled");
      if(enabled == NULL || !strcmp(enabled, "0"))
      {
        LB_DBG("load balancer disabled!\n")
        load_cfg.enabled = false;
      }
      else
      {
        LB_DBG("load balancer enabled.\n")
        load_cfg.enabled = true;
      }

      threshold_max = uci_lookup_option_string(uci_ctx, s, "threshold_max");
      if(threshold_max == NULL)
        goto out;
      load_cfg.threshold_max = atoi(threshold_max);
      if(load_cfg.threshold_max > 100 || load_cfg.threshold_max < 0)
      {
        LB_CRT("threshold_max=%s, should be [<0-100>]\n", threshold_max);
        goto out;
      }

      threshold_min = uci_lookup_option_string(uci_ctx, s, "threshold_min");
      if(threshold_min == NULL)
        goto out;
      load_cfg.threshold_min = atoi(threshold_min);
      if(load_cfg.threshold_min > 100 || load_cfg.threshold_min < 0)
      {
        LB_CRT("threshold_min=%s, should be [<0-100>]\n", threshold_min);
        goto out;
      }

      primary = uci_lookup_option_string(uci_ctx, s, "primary");
      if(primary == NULL)
        goto out;
      strcpy(load_cfg.primary, primary);

      secondary = uci_lookup_option_string(uci_ctx, s, "secondary");
      if(secondary == NULL)
        goto out;
      strcpy(load_cfg.secondary, secondary);

      load_cfg.nfmark = get_intf_nfmark(load_cfg.secondary);
      if(load_cfg.nfmark == 0)
      {
        LB_CRT("nfmark should not be 0\r\n");
        goto out;
      }

      interval = uci_lookup_option_string(uci_ctx, s, "interval");
      if(interval == NULL)
      {
        load_cfg.interval = 5;
      }
      else
      {
        load_cfg.interval = atoi(interval);
        if(load_cfg.interval == 0)
          load_cfg.interval = 5;
      }

      upstream = uci_lookup_option_string(uci_ctx, s, "upstream");
      if(upstream == NULL)
        load_cfg.upstream = 0;
      else
        load_cfg.upstream = atoi(upstream);

      failover = uci_lookup_option_string(uci_ctx, s, "failover");
      if(failover == NULL)
        load_cfg.failback = 1;
      else
        load_cfg.failback = atoi(failover);

      bandwidth = uci_lookup_option_string(uci_ctx, s, "bandwidth");
      if(bandwidth == NULL)
        goto out;
      load_cfg.bandwidth = atoi(bandwidth);
      if(load_cfg.bandwidth)
        lb_status.bandwidth = load_cfg.bandwidth; //Kbps
      else
        goto out;

      level = uci_lookup_option_string(uci_ctx, s, "trace_level");
      if(level == NULL)
        trace_level = LB_TRACELEVEL_CRIT;
      else
        trace_level = atoi(level);

      if(reload)
      {
        load_cfg.queue = lb_cfg.queue;
        LB_DBG("not support queue reload\n");
      }
      else
      {
        queue = uci_lookup_option_string(uci_ctx, s, "queue");
        if(queue == NULL)
        {
          load_cfg.queue = 0;
        }
        else
        {
          load_cfg.queue = atoi(queue);
          if(load_cfg.queue >= 65535)
          {
            LB_CRT("queue number should be [0-65535)\n");
            goto out;
          }
        }
      }

      if(load_cfg.threshold_min >= load_cfg.threshold_max)
      {
        LB_CRT("threshold min should be less than threshold max\n");
        goto out;
      }

      if(reload)
      {
        load_balancer_intf_status intf_status;
        load_balancer_intf_status intf_status1;
        if(memcmp(&load_cfg, &lb_cfg, sizeof(load_cfg)) == 0)
        {
          load_nok = 0;
          LB_DBG("exactly the same config\n");
          goto out;
        }
        if(strcmp(lb_cfg.primary, load_cfg.primary))
        {
          strcpy(intf_status.ifname, load_cfg.primary);
          if(ubus_call_interface_status(ctx1, &intf_status))
          {
            LB_ERR("invalid primary interface %s\n", load_cfg.primary);
           goto out;
         }
        }
        if(strcmp(lb_cfg.secondary, load_cfg.secondary))
        {
          strcpy(intf_status1.ifname, load_cfg.secondary);
          if(ubus_call_interface_status(ctx1, &intf_status1))
          {
           LB_ERR("invalid secondary interface %s\n", load_cfg.secondary);
           goto out;
          }
          lb_status.secondary_up = intf_status1.status;
        }

        if(strcmp(lb_cfg.primary, load_cfg.primary))
        {
          lb_status.primary_up = intf_status.status;
          strcpy(lb_status.l3_primary, intf_status.l3_ifname);
        }
        if(lb_cfg.upstream != load_cfg.upstream || strcmp(lb_cfg.primary, load_cfg.primary))
        {
          lb_status.last_bytes = lb_status.last_sec = lb_status.used_bandwidth = 0;
          LB_DBG("reset bandwidth info as config changes\n");
        }
        lb_config_reloaded = 1;
      }

      memcpy(&lb_cfg, &load_cfg, sizeof(load_cfg));
      LB_INF("lb_config_load : new configuration\n")
      lb_print(lb_cfg);
      load_nok = 0;
    }
  }

out:
  uci_unload(uci_ctx, pkg);
  uci_free_context(uci_ctx);
  return load_nok;
}

/****************************************************************************
 * signal handler for configuration reload
 */
static void sig_handler(int signo)
{
  if(signo == SIGHUP)
  {
    sem_post(&sem);
    LB_DBG("sem post\n");
  }
}

/****************************************************************************
 * new thread for uci reload
*/
static void* lb_reload_service(void *lb_service)
{
  if(sem_init(&sem, 0, 0) < 0)
  {
    LB_ERR("sem init failed\n");
    exit(1);
  }

  /* ubus init */
  ctx1 = ubus_connect(NULL);
  if(!ctx1)
  {
    LB_ERR("Failed to connect to ubus for reload\n");
    exit(1);
  }

  while(true)
  {
    if(sem_wait(&sem) < 0)
    {
      LB_ERR("sem wait failed\n");
      sem_destroy(&sem);
      ubus_free(ctx1);
      exit(1);
    }
    LB_DBG("sem wait\n");
    if(pthread_mutex_lock(&lb_mutex_reload))
    {
      LB_ERR("fail to lock lb_mutex_reload\n");
      continue;
    }
    if(lb_config_load(1))
    {
      LB_ERR("reload config failed !\n");
    }
    else
    {
      fail_over_checker();
      LB_DBG("reload config OK !\n");
    }
    if(pthread_mutex_unlock(&lb_mutex_reload))
    {
      LB_ERR("fail to unlock lb_mutex_reload\n");
    }
  }

  return NULL;
}


/****************************************************************************
 * main
 */
int main(int argc, char **argv)
{
  int rv;
  char buf[4096] __attribute__ ((aligned));
  int fd;
  pthread_t tid, tid1;
  struct sigaction sa_reload;

  memset(&lb_cfg, 0, sizeof(lb_cfg));
  memset(&lb_status, 0, sizeof(lb_status));

  if(lb_config_load(0))
  {
    LB_ERR("load configuration failed\n");
    return 1;
  }

  if(pthread_create(&tid, NULL, lb_ubus_service, NULL))
  {
    LB_ERR("pthread create for ubus failed\n");
    return 1;
  }

  if(pthread_create(&tid1, NULL, lb_reload_service, NULL))
  {
    LB_ERR("pthread create for reload failed\n");
    return 1;
  }

  /* nfq */
  fd = nfq_init();
  if(fd == -1)
  {
    LB_ERR("load balance enable failed\n");
    return 1;
  }
  fail_over_checker();

  /* SIGHUP */
  sa_reload.sa_handler = sig_handler;
  sigemptyset(&sa_reload.sa_mask);
  sa_reload.sa_flags = SA_RESTART;
  if(sigaction(SIGHUP, &sa_reload,NULL))
  {
    LB_ERR("SIGHUP handler install error\n");
    return 1;
  }
  else
  {
    LB_DBG("SIGHUP handler installed\n");
  }

  while(1)
  {
    if((rv = recv(fd, buf, sizeof(buf), 0)) > 0)
    {
      LB_INF("pkt received\n");
      nfq_handle_packet(handler, buf, rv);
      continue;
    }
    if(rv < 0 && errno == ENOBUFS)
    {
      LB_ERR("losing packets\n");
    }
  }

  nfq_exit();
  return 0;
}
