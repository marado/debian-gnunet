/*
     This file is part of GNUnet.
     (C) 2011-2014 Christian Grothoff (and other contributing authors)

     GNUnet is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published
     by the Free Software Foundation; either version 3, or (at your
     option) any later version.

     GNUnet is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with GNUnet; see the file COPYING.  If not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330,
     Boston, MA 02111-1307, USA.
*/

/**
 * @file nat/nat_mini.c
 * @brief functions for interaction with miniupnp; tested with miniupnpc 1.5
 * @author Christian Grothoff
 */
#include "platform.h"
#include "gnunet_util_lib.h"
#include "gnunet_nat_lib.h"
#include "nat.h"

#define LOG(kind,...) GNUNET_log_from (kind, "nat", __VA_ARGS__)

/**
 * How long do we give upnpc to create a mapping?
 */
#define MAP_TIMEOUT GNUNET_TIME_relative_multiply (GNUNET_TIME_UNIT_SECONDS, 15)

/**
 * How long do we give upnpc to remove a mapping?
 */
#define UNMAP_TIMEOUT GNUNET_TIME_relative_multiply (GNUNET_TIME_UNIT_SECONDS, 1)

/**
 * How often do we check for changes in the mapping?
 */
#define MAP_REFRESH_FREQ GNUNET_TIME_relative_multiply (GNUNET_TIME_UNIT_MINUTES, 5)



/**
 * Opaque handle to cancel "GNUNET_NAT_mini_get_external_ipv4" operation.
 */
struct GNUNET_NAT_ExternalHandle
{

  /**
   * Function to call with the result.
   */
  GNUNET_NAT_IPCallback cb;

  /**
   * Closure for @e cb.
   */
  void *cb_cls;

  /**
   * Read task.
   */
  GNUNET_SCHEDULER_TaskIdentifier task;

  /**
   * Handle to 'external-ip' process.
   */
  struct GNUNET_OS_Process *eip;

  /**
   * Handle to stdout pipe of 'external-ip'.
   */
  struct GNUNET_DISK_PipeHandle *opipe;

  /**
   * Read handle of @e opipe.
   */
  const struct GNUNET_DISK_FileHandle *r;

  /**
   * When should this operation time out?
   */
  struct GNUNET_TIME_Absolute timeout;

  /**
   * Number of bytes in 'buf' that are valid.
   */
  size_t off;

  /**
   * Destination of our read operation (output of 'external-ip').
   */
  char buf[17];

};


/**
 * Read the output of 'external-ip' into buf.  When complete, parse the
 * address and call our callback.
 *
 * @param cls the `struct GNUNET_NAT_ExternalHandle`
 * @param tc scheduler context
 */
static void
read_external_ipv4 (void *cls,
                    const struct GNUNET_SCHEDULER_TaskContext *tc)
{
  struct GNUNET_NAT_ExternalHandle *eh = cls;
  ssize_t ret;
  struct in_addr addr;
  int iret;

  eh->task = GNUNET_SCHEDULER_NO_TASK;
  if (GNUNET_YES == GNUNET_NETWORK_fdset_handle_isset (tc->read_ready, eh->r))
    ret =
        GNUNET_DISK_file_read (eh->r, &eh->buf[eh->off],
                               sizeof (eh->buf) - eh->off);
  else
    ret = -1;                   /* error reading, timeout, etc. */
  if (ret > 0)
  {
    /* try to read more */
    eh->off += ret;
    eh->task =
        GNUNET_SCHEDULER_add_read_file (GNUNET_TIME_absolute_get_remaining
                                        (eh->timeout), eh->r,
                                        &read_external_ipv4, eh);
    return;
  }
  iret = GNUNET_NO;
  if ((eh->off > 7) && (eh->buf[eh->off - 1] == '\n'))
  {
    eh->buf[eh->off - 1] = '\0';
    if (1 == inet_pton (AF_INET, eh->buf, &addr))
    {
      if (0 == addr.s_addr)
        iret = GNUNET_NO;       /* got 0.0.0.0 */
      else
        iret = GNUNET_OK;
    }
  }
  eh->cb (eh->cb_cls,
          (GNUNET_OK == iret)
          ? &addr :
          NULL,
          (GNUNET_OK == iret)
          ? NULL
          : _("no valid address was returned by `external-ip'"));
  GNUNET_NAT_mini_get_external_ipv4_cancel (eh);
}


/**
 * (Asynchronously) signal error invoking "external-ip" to client.
 *
 * @param cls the `struct GNUNET_NAT_ExternalHandle` (freed)
 * @param tc scheduler context
 */
static void
signal_external_ip_error (void *cls,
                          const struct GNUNET_SCHEDULER_TaskContext *tc)
{
  struct GNUNET_NAT_ExternalHandle *eh = cls;

  eh->task = GNUNET_SCHEDULER_NO_TASK;
  eh->cb (eh->cb_cls,
          NULL,
          _("`external-ip' command not found"));
  GNUNET_free (eh);
}


/**
 * Try to get the external IPv4 address of this peer.
 *
 * @param timeout when to fail
 * @param cb function to call with result
 * @param cb_cls closure for @a cb
 * @return handle for cancellation (can only be used until @a cb is called), NULL on error
 */
struct GNUNET_NAT_ExternalHandle *
GNUNET_NAT_mini_get_external_ipv4 (struct GNUNET_TIME_Relative timeout,
                                   GNUNET_NAT_IPCallback cb, void *cb_cls)
{
  struct GNUNET_NAT_ExternalHandle *eh;

  eh = GNUNET_new (struct GNUNET_NAT_ExternalHandle);
  eh->cb = cb;
  eh->cb_cls = cb_cls;
  if (GNUNET_SYSERR ==
      GNUNET_OS_check_helper_binary ("external-ip", GNUNET_NO, NULL))
  {
    LOG (GNUNET_ERROR_TYPE_INFO,
	 _("`external-ip' command not found\n"));
    eh->task = GNUNET_SCHEDULER_add_now (&signal_external_ip_error,
                                         eh);
    return eh;
  }
  LOG (GNUNET_ERROR_TYPE_DEBUG,
       "Running `external-ip' to determine our external IP\n");
  eh->opipe = GNUNET_DISK_pipe (GNUNET_YES, GNUNET_YES, GNUNET_NO, GNUNET_YES);
  if (NULL == eh->opipe)
  {
    eh->task = GNUNET_SCHEDULER_add_now (&signal_external_ip_error,
                                         eh);
    return eh;
  }
  eh->eip =
      GNUNET_OS_start_process (GNUNET_NO, 0, NULL, eh->opipe, NULL,
                               "external-ip", "external-ip",
                               NULL);
  if (NULL == eh->eip)
  {
    GNUNET_DISK_pipe_close (eh->opipe);
    eh->task = GNUNET_SCHEDULER_add_now (&signal_external_ip_error,
                                         eh);
    return eh;
  }
  GNUNET_DISK_pipe_close_end (eh->opipe, GNUNET_DISK_PIPE_END_WRITE);
  eh->timeout = GNUNET_TIME_relative_to_absolute (timeout);
  eh->r = GNUNET_DISK_pipe_handle (eh->opipe, GNUNET_DISK_PIPE_END_READ);
  eh->task =
      GNUNET_SCHEDULER_add_read_file (timeout,
                                      eh->r,
                                      &read_external_ipv4, eh);
  return eh;
}


/**
 * Cancel operation.
 *
 * @param eh operation to cancel
 */
void
GNUNET_NAT_mini_get_external_ipv4_cancel (struct GNUNET_NAT_ExternalHandle *eh)
{
  if (NULL != eh->eip)
  {
    (void) GNUNET_OS_process_kill (eh->eip, SIGKILL);
    GNUNET_OS_process_destroy (eh->eip);
  }
  if (NULL != eh->opipe)
    GNUNET_DISK_pipe_close (eh->opipe);
  if (GNUNET_SCHEDULER_NO_TASK != eh->task)
    GNUNET_SCHEDULER_cancel (eh->task);
  GNUNET_free (eh);
}


/**
 * Handle to a mapping created with upnpc.
 */
struct GNUNET_NAT_MiniHandle
{

  /**
   * Function to call on mapping changes.
   */
  GNUNET_NAT_MiniAddressCallback ac;

  /**
   * Closure for @e ac.
   */
  void *ac_cls;

  /**
   * Command used to install the map.
   */
  struct GNUNET_OS_CommandHandle *map_cmd;

  /**
   * Command used to refresh our map information.
   */
  struct GNUNET_OS_CommandHandle *refresh_cmd;

  /**
   * Command used to remove the mapping.
   */
  struct GNUNET_OS_CommandHandle *unmap_cmd;

  /**
   * Our current external mapping (if we have one).
   */
  struct sockaddr_in current_addr;

  /**
   * We check the mapping periodically to see if it
   * still works.  This task triggers the check.
   */
  GNUNET_SCHEDULER_TaskIdentifier refresh_task;

  /**
   * Are we mapping TCP or UDP?
   */
  int is_tcp;

  /**
   * Did we succeed with creating a mapping?
   */
  int did_map;

  /**
   * Did we find our mapping during refresh scan?
   */
  int found;

  /**
   * Which port are we mapping?
   */
  uint16_t port;

};


/**
 * Run "upnpc -l" to find out if our mapping changed.
 *
 * @param cls the `struct GNUNET_NAT_MiniHandle`
 * @param tc scheduler context
 */
static void
do_refresh (void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc);


/**
 * Process the output from the "upnpc -r" command.
 *
 * @param cls the `struct GNUNET_NAT_MiniHandle`
 * @param line line of output, NULL at the end
 */
static void
process_map_output (void *cls, const char *line);


/**
 * Run "upnpc -r" to map our internal port.
 *
 * @param mini our handle
 */
static void
run_upnpc_r (struct GNUNET_NAT_MiniHandle *mini)
{
  char pstr[6];

  GNUNET_snprintf (pstr,
                   sizeof (pstr),
                   "%u",
                   (unsigned int) mini->port);
  mini->map_cmd =
    GNUNET_OS_command_run (&process_map_output, mini, MAP_TIMEOUT,
                           "upnpc", "upnpc", "-r", pstr,
                           mini->is_tcp ? "tcp" : "udp", NULL);
  if (NULL == mini->map_cmd)
  {
    mini->ac (mini->ac_cls,
              GNUNET_SYSERR,
              NULL, 0,
              _("Failed to run `upnpc` command"));
    return;
  }
}


/**
 * Process the output from "upnpc -l" to see if our
 * external mapping changed.  If so, do the notifications.
 *
 * @param cls the `struct GNUNET_NAT_MiniHandle`
 * @param line line of output, NULL at the end
 */
static void
process_refresh_output (void *cls, const char *line)
{
  struct GNUNET_NAT_MiniHandle *mini = cls;
  char pstr[9];
  const char *s;
  unsigned int nport;
  struct in_addr exip;

  if (NULL == line)
  {
    GNUNET_OS_command_stop (mini->refresh_cmd);
    mini->refresh_cmd = NULL;
    if (GNUNET_NO == mini->found)
    {
      /* mapping disappeared, try to re-create */
      if (GNUNET_YES == mini->did_map)
      {
        mini->ac (mini->ac_cls,
                  GNUNET_NO,
                  (const struct sockaddr *) &mini->current_addr,
                  sizeof (mini->current_addr),
                  NULL);
        mini->did_map = GNUNET_NO;
      }
      run_upnpc_r (mini);
    }
    return;
  }
  if (!mini->did_map)
    return;                     /* never mapped, won't find our mapping anyway */

  /* we're looking for output of the form:
   * "ExternalIPAddress = 12.134.41.124" */

  s = strstr (line, "ExternalIPAddress = ");
  if (NULL != s)
  {
    s += strlen ("ExternalIPAddress = ");
    if (1 != inet_pton (AF_INET, s, &exip))
      return;                   /* skip */
    if (exip.s_addr == mini->current_addr.sin_addr.s_addr)
      return;                   /* no change */
    /* update mapping */
    mini->ac (mini->ac_cls, GNUNET_NO,
              (const struct sockaddr *) &mini->current_addr,
              sizeof (mini->current_addr),
              NULL);
    mini->current_addr.sin_addr = exip;
    mini->ac (mini->ac_cls, GNUNET_YES,
              (const struct sockaddr *) &mini->current_addr,
              sizeof (mini->current_addr),
              NULL);
    return;
  }
  /*
   * we're looking for output of the form:
   *
   * "0 TCP  3000->192.168.2.150:3000  'libminiupnpc' ''"
   * "1 UDP  3001->192.168.2.150:3001  'libminiupnpc' ''"
   *
   * the pattern we look for is:
   *
   * "%s TCP  PORT->STRING:OURPORT *" or
   * "%s UDP  PORT->STRING:OURPORT *"
   */
  GNUNET_snprintf (pstr, sizeof (pstr), ":%u ", mini->port);
  if (NULL == (s = strstr (line, "->")))
    return;                     /* skip */
  if (NULL == strstr (s, pstr))
    return;                     /* skip */
  if (1 !=
      SSCANF (line,
              (mini->is_tcp) ? "%*u TCP  %u->%*s:%*u %*s" :
              "%*u UDP  %u->%*s:%*u %*s", &nport))
    return;                     /* skip */
  mini->found = GNUNET_YES;
  if (nport == ntohs (mini->current_addr.sin_port))
    return;                     /* no change */

  /* external port changed, update mapping */
  mini->ac (mini->ac_cls, GNUNET_NO,
            (const struct sockaddr *) &mini->current_addr,
            sizeof (mini->current_addr),
            NULL);
  mini->current_addr.sin_port = htons ((uint16_t) nport);
  mini->ac (mini->ac_cls, GNUNET_YES,
            (const struct sockaddr *) &mini->current_addr,
            sizeof (mini->current_addr),
            NULL);
}


/**
 * Run "upnpc -l" to find out if our mapping changed.
 *
 * @param cls the 'struct GNUNET_NAT_MiniHandle'
 * @param tc scheduler context
 */
static void
do_refresh (void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc)
{
  struct GNUNET_NAT_MiniHandle *mini = cls;
  int ac;

  mini->refresh_task =
    GNUNET_SCHEDULER_add_delayed (MAP_REFRESH_FREQ,
                                  &do_refresh, mini);
  LOG (GNUNET_ERROR_TYPE_DEBUG,
       "Running `upnpc' to check if our mapping still exists\n");
  mini->found = GNUNET_NO;
  ac = GNUNET_NO;
  if (NULL != mini->map_cmd)
  {
    /* took way too long, abort it! */
    GNUNET_OS_command_stop (mini->map_cmd);
    mini->map_cmd = NULL;
    ac = GNUNET_YES;
  }
  if (NULL != mini->refresh_cmd)
  {
    /* took way too long, abort it! */
    GNUNET_OS_command_stop (mini->refresh_cmd);
    mini->refresh_cmd = NULL;
    ac = GNUNET_YES;
  }
  mini->refresh_cmd =
      GNUNET_OS_command_run (&process_refresh_output, mini, MAP_TIMEOUT,
                             "upnpc", "upnpc", "-l", NULL);
  if (GNUNET_YES == ac)
    mini->ac (mini->ac_cls,
              GNUNET_SYSERR,
              NULL, 0,
              _("`upnpc' command took too long, process killed"));
}


/**
 * Process the output from the 'upnpc -r' command.
 *
 * @param cls the `struct GNUNET_NAT_MiniHandle`
 * @param line line of output, NULL at the end
 */
static void
process_map_output (void *cls,
                    const char *line)
{
  struct GNUNET_NAT_MiniHandle *mini = cls;
  const char *ipaddr;
  char *ipa;
  const char *pstr;
  unsigned int port;

  if (NULL == line)
  {
    GNUNET_OS_command_stop (mini->map_cmd);
    mini->map_cmd = NULL;
    if (GNUNET_YES != mini->did_map)
      mini->ac (mini->ac_cls,
                GNUNET_SYSERR,
                NULL, 0,
                _("`upnpc' command failed to establish port mapping"));
    if (GNUNET_SCHEDULER_NO_TASK == mini->refresh_task)
      mini->refresh_task =
        GNUNET_SCHEDULER_add_delayed (MAP_REFRESH_FREQ, &do_refresh, mini);
    return;
  }
  /*
   * The upnpc output we're after looks like this:
   *
   * "external 87.123.42.204:3000 TCP is redirected to internal 192.168.2.150:3000"
   */
  if ((NULL == (ipaddr = strstr (line, " "))) ||
      (NULL == (pstr = strstr (ipaddr, ":"))) ||
      (1 != SSCANF (pstr + 1, "%u", &port)))
  {
    return;                     /* skip line */
  }
  ipa = GNUNET_strdup (ipaddr + 1);
  strstr (ipa, ":")[0] = '\0';
  if (1 != inet_pton (AF_INET, ipa, &mini->current_addr.sin_addr))
  {
    GNUNET_free (ipa);
    return;                     /* skip line */
  }
  GNUNET_free (ipa);

  mini->current_addr.sin_port = htons (port);
  mini->current_addr.sin_family = AF_INET;
#if HAVE_SOCKADDR_IN_SIN_LEN
  mini->current_addr.sin_len = sizeof (struct sockaddr_in);
#endif
  mini->did_map = GNUNET_YES;
  mini->ac (mini->ac_cls, GNUNET_YES,
            (const struct sockaddr *) &mini->current_addr,
            sizeof (mini->current_addr),
            NULL);
}


/**
 * Start mapping the given port using (mini)upnpc.  This function
 * should typically not be used directly (it is used within the
 * general-purpose #GNUNET_NAT_register() code).  However, it can be
 * used if specifically UPnP-based NAT traversal is to be used or
 * tested.
 *
 * @param port port to map
 * @param is_tcp #GNUNET_YES to map TCP, #GNUNET_NO for UDP
 * @param ac function to call with mapping result
 * @param ac_cls closure for @a ac
 * @return NULL on error (no 'upnpc' installed)
 */
struct GNUNET_NAT_MiniHandle *
GNUNET_NAT_mini_map_start (uint16_t port,
                           int is_tcp,
                           GNUNET_NAT_MiniAddressCallback ac,
                           void *ac_cls)
{
  struct GNUNET_NAT_MiniHandle *ret;

  if (GNUNET_SYSERR ==
      GNUNET_OS_check_helper_binary ("upnpc", GNUNET_NO, NULL))
  {
    LOG (GNUNET_ERROR_TYPE_INFO,
	 _("`upnpc' command not found\n"));
    ac (ac_cls,
        GNUNET_SYSERR,
        NULL, 0,
        _("`upnpc` command not found"));
    return NULL;
  }
  LOG (GNUNET_ERROR_TYPE_DEBUG,
       "Running `upnpc' to install mapping\n");
  ret = GNUNET_new (struct GNUNET_NAT_MiniHandle);
  ret->ac = ac;
  ret->ac_cls = ac_cls;
  ret->is_tcp = is_tcp;
  ret->port = port;
  ret->refresh_task =
    GNUNET_SCHEDULER_add_delayed (MAP_REFRESH_FREQ, &do_refresh, ret);
  run_upnpc_r (ret);
  return ret;
}


/**
 * Process output from our 'unmap' command.
 *
 * @param cls the `struct GNUNET_NAT_MiniHandle`
 * @param line line of output, NULL at the end
 */
static void
process_unmap_output (void *cls, const char *line)
{
  struct GNUNET_NAT_MiniHandle *mini = cls;

  if (NULL == line)
  {
    LOG (GNUNET_ERROR_TYPE_DEBUG,
         "UPnP unmap done\n");
    GNUNET_OS_command_stop (mini->unmap_cmd);
    mini->unmap_cmd = NULL;
    GNUNET_free (mini);
    return;
  }
  /* we don't really care about the output... */
}


/**
 * Remove a mapping created with (mini)upnpc.  Calling
 * this function will give 'upnpc' 1s to remove tha mapping,
 * so while this function is non-blocking, a task will be
 * left with the scheduler for up to 1s past this call.
 *
 * @param mini the handle
 */
void
GNUNET_NAT_mini_map_stop (struct GNUNET_NAT_MiniHandle *mini)
{
  char pstr[6];

  if (GNUNET_SCHEDULER_NO_TASK != mini->refresh_task)
  {
    GNUNET_SCHEDULER_cancel (mini->refresh_task);
    mini->refresh_task = GNUNET_SCHEDULER_NO_TASK;
  }
  if (NULL != mini->refresh_cmd)
  {
    GNUNET_OS_command_stop (mini->refresh_cmd);
    mini->refresh_cmd = NULL;
  }
  if (NULL != mini->map_cmd)
  {
    GNUNET_OS_command_stop (mini->map_cmd);
    mini->map_cmd = NULL;
  }
  if (GNUNET_NO == mini->did_map)
  {
    GNUNET_free (mini);
    return;
  }
  mini->ac (mini->ac_cls, GNUNET_NO,
            (const struct sockaddr *) &mini->current_addr,
            sizeof (mini->current_addr),
            NULL);
  /* Note: oddly enough, deletion uses the external port whereas
   * addition uses the internal port; this rarely matters since they
   * often are the same, but it might... */
  GNUNET_snprintf (pstr,
                   sizeof (pstr),
                   "%u",
                   (unsigned int) ntohs (mini->current_addr.sin_port));
  LOG (GNUNET_ERROR_TYPE_DEBUG,
       "Unmapping port %u with UPnP\n",
       ntohs (mini->current_addr.sin_port));
  mini->unmap_cmd =
      GNUNET_OS_command_run (&process_unmap_output, mini, UNMAP_TIMEOUT,
                             "upnpc", "upnpc", "-d", pstr,
                             mini->is_tcp ? "tcp" : "udp", NULL);
}


/* end of nat_mini.c */
