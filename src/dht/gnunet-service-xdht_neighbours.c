/*
     This file is part of GNUnet.
     (C) 2009-2013 Christian Grothoff (and other contributing authors)

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
 * @file dht/gnunet-service-xdht_neighbours.c
 * @brief GNUnet DHT service's finger and friend table management code
 * @author Supriti Singh
 */

#include "platform.h"
#include "gnunet_util_lib.h"
#include "gnunet_block_lib.h"
#include "gnunet_hello_lib.h"
#include "gnunet_constants.h"
#include "gnunet_protocols.h"
#include "gnunet_nse_service.h"
#include "gnunet_ats_service.h"
#include "gnunet_core_service.h"
#include "gnunet_datacache_lib.h"
#include "gnunet_transport_service.h"
#include "gnunet_hello_lib.h"
#include "gnunet_dht_service.h"
#include "gnunet_statistics_service.h"
#include "gnunet-service-xdht.h"
#include "gnunet-service-xdht_clients.h"
#include "gnunet-service-xdht_datacache.h"
#include "gnunet-service-xdht_hello.h"
#include "gnunet-service-xdht_neighbours.h"
#include "gnunet-service-xdht_nse.h"
#include "gnunet-service-xdht_routing.h"
#include <fenv.h>
#include "dht.h"

/* TODO:
 1. Use a global array of all known peers in find_successor, Only when 
 a new peer is added in finger or friend peer map, then re calculate
 the array. Or else use the old one.
 2. Should we be using const in all the handle for the message we received 
 * and then copy the fields and make changes to the fields instead of sending
 * them as they come.
 * 3. Everywhere you are storing yourself as the first element in the trail.
 * It is obviously taking too much space. Try to remove it and think of something
 * better.
 4. Choose the correct interval to send finger and verify message.
 5. Do we need expiration time for trail setup and all other messages? TTL
 6. In case of trail setup after TTL, we should again send the request but 
 * through a different route. How do we remeber each time which friend we
 * chose last time for the trail setup. We will need a data structure where we
 * add entry in finger table add and when setup is done remove it.
 * 7. I have not added any authentication on messages exachanged between peers.
 * Only when basic put/get is correct I will add it. */

/**
 * Maximum possible fingers of a peer.
 */
#define MAX_FINGERS 64

/**
 * Maximum allowed number of pending messages per friend peer.
 */
#define MAXIMUM_PENDING_PER_FRIEND 64

/**
 * How long at least to wait before sending another find finger trail request.
 */
#define DHT_MINIMUM_FIND_FINGER_TRAIL_INTERVAL GNUNET_TIME_relative_multiply (GNUNET_TIME_UNIT_SECONDS, 30)

/**
 * How long at most to wait before sending another find finger trail request.
 */
#define DHT_MAXIMUM_FIND_FINGER_TRAIL_INTERVAL GNUNET_TIME_relative_multiply (GNUNET_TIME_UNIT_MINUTES, 10)

/**
 * How long at most to wait for transmission of a GET request to another peer?
 */
#define GET_TIMEOUT GNUNET_TIME_relative_multiply(GNUNET_TIME_UNIT_MINUTES, 2)

/**
 * Maximum number of trails allowed to go through a friend.
 */
#define LINK_THRESHOLD 64


GNUNET_NETWORK_STRUCT_BEGIN
  
/**
 * P2P PUT message
 */
struct PeerPutMessage
{
  /**
   * Type: #GNUNET_MESSAGE_TYPE_DHT_P2P_PUT
   */
  struct GNUNET_MessageHeader header;

  /**
   * Processing options
   */
  uint32_t options GNUNET_PACKED;

  /**
   * Content type.
   */
  uint32_t type GNUNET_PACKED;

  /**
   * Hop count
   */
  uint32_t hop_count GNUNET_PACKED;

  /**
   * Replication level for this message
   */
  uint32_t desired_replication_level GNUNET_PACKED;

  /**
   * Length of the PUT path that follows (if tracked).
   */
  uint32_t put_path_length GNUNET_PACKED;
  
  /**
   * Source peer 
   */
  struct GNUNET_PeerIdentity source_peer;
  
  /** 
   * Current destination 
   */
  struct GNUNET_PeerIdentity current_destination;
  
  /** 
   * Current destination type
   */
  enum current_destination_type current_destination_type;

  /**
   * When does the content expire?
   */
  struct GNUNET_TIME_AbsoluteNBO expiration_time;
  
  /**
   * The key to store the value under.
   */
  struct GNUNET_HashCode key GNUNET_PACKED;
 

  /* put path (if tracked) */

  /* Payload */
 
};


/**
 * P2P Result message
 */
struct PeerGetResultMessage
{
  /**
   * Type: #GNUNET_MESSAGE_TYPE_DHT_P2P_GET_RESULT
   */
  struct GNUNET_MessageHeader header;

  /**
   * Peer which is sending get result message.
   */
  struct GNUNET_PeerIdentity source_peer;
  
  /**
   * Peer which will receive the get result message. 
   */
  struct GNUNET_PeerIdentity destination_peer;

  /**
   * Current index in get path. 
   */
  unsigned int current_path_index;
  
  /**
   * Length of the GET path that follows (if tracked).
   */
  uint32_t get_path_length GNUNET_PACKED;

  /**
   * When does the content expire?
   */
  struct GNUNET_TIME_AbsoluteNBO expiration_time;

  /**
   * The key of the corresponding GET request.
   */
  struct GNUNET_HashCode key;
 
  /* put path (if tracked) */

  /* get path (if tracked) */

  /* Payload */

};


/**
 * P2P GET message
 */
struct PeerGetMessage
{
  /**
   * Type: #GNUNET_MESSAGE_TYPE_DHT_P2P_GET
   */
  struct GNUNET_MessageHeader header;
  
  /**
   * Source peer
   */
  struct GNUNET_PeerIdentity source_peer;

  /**
   * Total number of peers in get path. 
   */
  unsigned int get_path_length;
  
  /**
   * 
   */
  struct GNUNET_PeerIdentity current_destination;

  /**
   *
   */
  enum current_destination_type current_dest_type;  
  
  /**
   * Unique identifier of the request. 
   */
  uint64_t request_id;
  /**
   * When does the content expire?
   */
  struct GNUNET_TIME_AbsoluteNBO expiration_time;
  
  /**
   * The key we are looking for.
   */
  struct GNUNET_HashCode key;
  
  /* Get path. */

};


/**
 * P2P Trail setup message
 */
struct PeerTrailSetupMessage
{
  
  /**
   * Type: #GNUNET_MESSAGE_TYPE_DHT_P2P_TRAIL_SETUP
   */
  struct GNUNET_MessageHeader header;

  /**
   * Source peer which wants to setup the trail to one of its finger. 
   */
  struct GNUNET_PeerIdentity source_peer;

  /**
   * Successor of this finger value will be our finger peer.
   */
  uint64_t destination_finger;
  
  /**
   * Peer which gets this message can be either an intermediate finger or friend. 
   */
  enum current_destination_type current_destination_type;
  
  /**
   * Peer to which this packet is forwarded.
   */
  struct GNUNET_PeerIdentity current_destination;
  
  /**
   * Index into finger peer map. 
   */
  unsigned int finger_map_index;
  
  /**
   * Number of entries in trail list.
   */
  uint32_t trail_length GNUNET_PACKED;
  
};


/**
 * P2P Trail Setup Result message
 */
struct PeerTrailSetupResultMessage
{
  
  /**
   * Type: #GNUNET_MESSAGE_TYPE_DHT_P2P_TRAIL_SETUP_RESULT
   */
  struct GNUNET_MessageHeader header;
  
  /**
   * Finger to which we have found the path. 
   */
  struct GNUNET_PeerIdentity finger_identity;

  /**
   * Peer which was looking for the trail to finger. 
   */
  struct GNUNET_PeerIdentity destination_peer;

  /**
   * Trail index which points to next destination to send this message.
   */
  unsigned int current_index;
  
  /**
   * Index into finger peer map
   */
  unsigned int finger_map_index;
  
  /**
   * Number of entries in trail list.
   */
  uint32_t trail_length GNUNET_PACKED;
  
};


/**
 * P2P Verify Successor message. 
 */
struct PeerVerifySuccessorMessage
{
  
  /**
   * Type: #GNUNET_MESSAGE_TYPE_DHT_P2P_VERIFY_SUCCESSOR
   */
  struct GNUNET_MessageHeader header;
  
  /**
   * Source peer which wants to verify its successor. 
   */
  struct GNUNET_PeerIdentity source_peer;
  
  /**
   * My current successor.
   */
  struct GNUNET_PeerIdentity successor;
  
  /**
   * Total number of peers in trail to current successor.
   */
  unsigned int trail_length;
  
  /**
   * Trail index which points to next destination to send this message.
   */
  unsigned int current_trail_index;
  
};


/**
 * P2P Verify Successor Result message. 
 */
struct PeerVerifySuccessorResultMessage
{
  
  /**
   * Type: #GNUNET_MESSAGE_TYPE_DHT_P2P_VERIFY_SUCCESSOR_RESULT
   */
  struct GNUNET_MessageHeader header;
  
  /**
   * Destination peer which sent the request to verify its successor. 
   */
  struct GNUNET_PeerIdentity destination_peer;
  
  /**
   * Successor to which PeerVerifySuccessorMessage was sent.
   */
  struct GNUNET_PeerIdentity source_successor;
  
  /**
   * source_successor's predecessor
   */
  struct GNUNET_PeerIdentity my_predecessor;
  
  /**
   * Total number of peers in trail.
   * If source_successor is not destination peer, then trail is from destination_peer
   * to my_predecessor.
   * If source_successor is destination peer, then trail is from destination_peer
   * to source_successor.
   */
  unsigned int trail_length;
  
  /**
   * Trail index which points to next destination to send this message.
   */
  unsigned int current_index;
  
};

/**
 * P2P Notify New Successor message.
 */
struct PeerNotifyNewSuccessorMessage
{
  /**
   * Type: #GNUNET_MESSAGE_TYPE_DHT_P2P_NOTIFY_NEW_SUCCESSOR
   */
  struct GNUNET_MessageHeader header;
  
  /**
   * Source peer which wants to notify its new successor. 
   */
  struct GNUNET_PeerIdentity source_peer;
  
  /**
   * New successor identity.
   */
  struct GNUNET_PeerIdentity destination_peer;
  
  /**
   * Number of peers in trail from source_peer to new successor.
   */
  unsigned int trail_length;
  
  /**
   * Trail index which points to next destination to send this message.
   */
  unsigned int current_index;
  
};


GNUNET_NETWORK_STRUCT_END


/**
 * Linked list of messages to send to a particular other peer.
 */
struct P2PPendingMessage
{
  /**
   * Pointer to next item in the list
   */
  struct P2PPendingMessage *next;

  /**
   * Pointer to previous item in the list
   */
  struct P2PPendingMessage *prev;

  /**
   * When does this message time out?
   */
  struct GNUNET_TIME_Absolute timeout;

   /**
   * Message importance level.  FIXME: used? useful?
   */
  unsigned int importance;

  /**
   * Actual message to be sent, allocated at the end of the struct:
   * // msg = (cast) &pm[1];
   * // memcpy (&pm[1], data, len);
   */
  const struct GNUNET_MessageHeader *msg;

};


/**
 * Linked List of peers which are part of trail to reach a particular Finger.
 */
struct TrailPeerList
{
   /**
    * Pointer to next item in the list
    */
   struct TrailPeerList *next;

   /**
    * Pointer to previous item in the list
    */
   struct TrailPeerList *prev;
   
   /**
    * An element in this trail list
    */
   struct GNUNET_PeerIdentity peer;
  
};


/** 
 *  Entry in friend_peermap.
 */
struct FriendInfo
{
  /**
   * Friend Identity 
   */
  struct GNUNET_PeerIdentity id;

  /**
   * Number of trail of which this friend is the first hop.
   */
  unsigned int trail_links;
  
  /**
   * Count of outstanding messages for this friend.
   */
  unsigned int pending_count;
  
  /**
   * Head of pending messages to be sent to this friend.
   */
  struct P2PPendingMessage *head;

  /**
   * Tail of pending messages to be sent to this friend.
   */
  struct P2PPendingMessage *tail;
 
  /**
   * Core handle for sending messages to this friend.
   */
  struct GNUNET_CORE_TransmitHandle *th;

};


/**
 * Entry in finger_peermap.
 */
struct FingerInfo
{
  /**
   * Finger identity.
   */
  struct GNUNET_PeerIdentity finger_identity;
  
  /**
   * Index in finger peer map
   */
  unsigned int finger_map_index;
  
  /**
   * Total number of entries in trail from [me,finger] 
   */
  unsigned int trail_length;
  
  /**
   * Head of trail to reach this finger.
   */
  struct TrailPeerList *head;
  
  /**
   * Tail of trail to reach this finger.
   */
  struct TrailPeerList *tail;
  
};


/**
 * FIXME: Think of a better name. 
 * Data structure passed to sorting algorithm in find_successor.
 */
struct Sorting_List
{
  /**
   * 64 bit value of peer identity
   */
  uint64_t peer_id;
  
  /**
   * Type : MY_ID, FINGER, FINGER, Value 
   */
  enum current_destination_type type;
  
  /**
   * Pointer to original data structure linked to peer id.
   */
  void *data;
};


/**
 * Task that sends FIND FINGER TRAIL requests.
 */
static GNUNET_SCHEDULER_TaskIdentifier find_finger_trail_task;

/**
 * 
 * Task that periodically verifies my successor. 
 */
static GNUNET_SCHEDULER_TaskIdentifier verify_successor;

/**
 * Identity of this peer.
 */
static struct GNUNET_PeerIdentity my_identity;

/**
 * Hash map of all the friends of a peer
 */
static struct GNUNET_CONTAINER_MultiPeerMap *friend_peermap;

/**
 * Hash map of all the fingers of a peer
 */
static struct GNUNET_CONTAINER_MultiPeerMap *finger_peermap;

/**
 * Handle to CORE.
 */
static struct GNUNET_CORE_Handle *core_api;

/**
 * FIXME: Is it safe to assume its initialized to 0 by default.
 * The current finger index that we have found trail to.
 */
static unsigned int current_finger_index;


/**
 * Called when core is ready to send a message we asked for
 * out to the destination.
 *
 * @param cls the 'struct FriendInfo' of the target friend
 * @param size number of bytes available in buf
 * @param buf where the callee should write the message
 * @return number of bytes written to buf
 */
static size_t
core_transmit_notify (void *cls, size_t size, void *buf)
{
  struct FriendInfo *peer = cls;
  char *cbuf = buf;
  struct P2PPendingMessage *pending;
  size_t off;
  size_t msize;
  
  peer->th = NULL;
  while ((NULL != (pending = peer->head)) &&
         (0 == GNUNET_TIME_absolute_get_remaining (pending->timeout).rel_value_us))
  {
    peer->pending_count--;
    GNUNET_CONTAINER_DLL_remove (peer->head, peer->tail, pending);  
    GNUNET_free (pending);
  }
  if (NULL == pending)
  {
    /* no messages pending */
    return 0;
  }
  if (NULL == buf)
  {
    peer->th =
        GNUNET_CORE_notify_transmit_ready (core_api, GNUNET_NO,
                                           GNUNET_CORE_PRIO_BEST_EFFORT,
                                           GNUNET_TIME_absolute_get_remaining
                                           (pending->timeout), &peer->id,
                                           ntohs (pending->msg->size),
                                           &core_transmit_notify, peer);
    GNUNET_break (NULL != peer->th);
    return 0;
  }
  off = 0;
  while ((NULL != (pending = peer->head)) &&
         (size - off >= (msize = ntohs (pending->msg->size))))
  {
    GNUNET_STATISTICS_update (GDS_stats,
                              gettext_noop
                              ("# Bytes transmitted to other peers"), msize,
                              GNUNET_NO);
    memcpy (&cbuf[off], pending->msg, msize);
    off += msize;
    peer->pending_count--;
    GNUNET_CONTAINER_DLL_remove (peer->head, peer->tail, pending);
    GNUNET_free (pending);
  }
  if (peer->head != NULL)
  {
    peer->th =
        GNUNET_CORE_notify_transmit_ready (core_api, GNUNET_NO,
                                           GNUNET_CORE_PRIO_BEST_EFFORT,
                                           GNUNET_TIME_absolute_get_remaining
                                           (pending->timeout), &peer->id, msize,
                                           &core_transmit_notify, peer);
    GNUNET_break (NULL != peer->th);
  }
  return off;
}


/**
 * Transmit all messages in the friend's message queue.
 *
 * @param peer message queue to process
 */
static void
process_friend_queue (struct FriendInfo *peer)
{
  struct P2PPendingMessage *pending;
  
  if (NULL == (pending = peer->head))
    return;
  if (NULL != peer->th)
    return;
  
  GNUNET_STATISTICS_update (GDS_stats,
                            gettext_noop
                            ("# Bytes of bandwidth requested from core"),
                            ntohs (pending->msg->size), GNUNET_NO);
  
  /* FIXME: Are we correctly initializing importance and pending. */
  peer->th =
      GNUNET_CORE_notify_transmit_ready (core_api, GNUNET_NO,
                                         pending->importance,
                                         GNUNET_TIME_absolute_get_remaining
                                         (pending->timeout), &peer->id,
                                         ntohs (pending->msg->size),
                                         &core_transmit_notify, peer);
  GNUNET_break (NULL != peer->th);
}


/**
 * Construct a trail message and forward it to a friend. 
 * @param source_peer Peer which wants to set up the trail to one of its finger.
 * @param destination_finger Value whose successor we are searching the network.
 * @param current_destination Peer which gets this message. 
 * @param target_friend Current friend to which this message should be forwarded.
 * @param trail_length Numbers of peers in the trail.
 * @param trail_peer_list peers this request has traversed so far  
 * @param finger_map_index Index in finger peer map
 * @param type Type of current destination can be either FRIEND or FINGER
 */
void
GDS_NEIGHBOURS_send_trail_setup (struct GNUNET_PeerIdentity *source_peer,
                                 uint64_t destination_finger,
                                 struct GNUNET_PeerIdentity *current_destination,
                                 struct FriendInfo *target_friend,
                                 unsigned int trail_length,
                                 struct GNUNET_PeerIdentity *trail_peer_list,
                                 unsigned int finger_map_index,
                                 enum current_destination_type type)
{
  struct P2PPendingMessage *pending;
  struct PeerTrailSetupMessage *tsm;
  struct GNUNET_PeerIdentity *peer_list;
  size_t msize;
  
  msize = sizeof (struct PeerTrailSetupMessage) + 
          (trail_length * sizeof (struct GNUNET_PeerIdentity));
  
  if (msize >= GNUNET_SERVER_MAX_MESSAGE_SIZE)
  {
    GNUNET_break (0);
    return;
  }
  
  if (target_friend->pending_count >= MAXIMUM_PENDING_PER_FRIEND)
  {  
    GNUNET_STATISTICS_update (GDS_stats, gettext_noop ("# P2P messages dropped due to full queue"),
				1, GNUNET_NO);
  }
  
  pending = GNUNET_malloc (sizeof (struct P2PPendingMessage) + msize); 
  pending->importance = 0;    /* FIXME */
  pending->timeout = GNUNET_TIME_relative_to_absolute (GET_TIMEOUT);
  tsm = (struct PeerTrailSetupMessage *) &pending[1]; 
  pending->msg = &tsm->header;
  tsm->header.size = htons (msize);
  tsm->header.type = htons (GNUNET_MESSAGE_TYPE_DHT_P2P_TRAIL_SETUP);
  memcpy (&(tsm->destination_finger), &destination_finger, sizeof (uint64_t));
  memcpy (&(tsm->source_peer), source_peer, sizeof (struct GNUNET_PeerIdentity));
  memcpy (&(tsm->current_destination), current_destination, sizeof (struct GNUNET_PeerIdentity));
  tsm->current_destination_type = htonl (type); 
  tsm->trail_length = htonl (trail_length); 
  tsm->finger_map_index = htonl (finger_map_index);
  
  if (trail_peer_list != NULL)
  {
    peer_list = (struct GNUNET_PeerIdentity *) &tsm[1];
    memcpy (peer_list, trail_peer_list, trail_length * sizeof(struct GNUNET_PeerIdentity));
  }

  GNUNET_CONTAINER_DLL_insert_tail (target_friend->head, target_friend->tail, pending);
  target_friend->pending_count++;
  process_friend_queue (target_friend);
  
}


/**
 * Construct a trail setup result message and forward it to a friend. 
 * @param destination_peer Peer which will get the trail to one of its finger.
 * @param source_finger Peer to which the trail has been setup to.
 * @param target_friend Friend to which this message should be forwarded.
 * @param trail_length Numbers of peers in the trail.
 * @param trail_peer_list Peers which are part of the trail from source to destination.
 * @param current_trail_index Index at which sender of this message is located.  
 * @param finger_map_index Index in finger peer map 
 */
void
GDS_NEIGHBOURS_send_trail_setup_result (struct GNUNET_PeerIdentity *destination_peer,
                                        struct GNUNET_PeerIdentity *source_finger,
                                        struct FriendInfo *target_friend,
                                        unsigned int trail_length,
                                        struct GNUNET_PeerIdentity *trail_peer_list,
                                        unsigned int current_trail_index,
                                        unsigned int finger_map_index)
{
  struct P2PPendingMessage *pending;
  struct PeerTrailSetupResultMessage *tsrm;
  struct GNUNET_PeerIdentity *peer_list;
  size_t msize;
  
  msize = sizeof (struct PeerTrailSetupResultMessage) + 
          (trail_length * sizeof (struct GNUNET_PeerIdentity));
  
  if (msize >= GNUNET_SERVER_MAX_MESSAGE_SIZE)
  {
    GNUNET_break (0);
    return;
  }
  
  if (target_friend->pending_count >= MAXIMUM_PENDING_PER_FRIEND)
  {  
    GNUNET_STATISTICS_update (GDS_stats, gettext_noop ("# P2P messages dropped due to full queue"),
				1, GNUNET_NO);
  }

  pending = GNUNET_malloc (sizeof (struct P2PPendingMessage) + msize); 
  pending->importance = 0;    
  pending->timeout = GNUNET_TIME_relative_to_absolute (GET_TIMEOUT);
  tsrm = (struct PeerTrailSetupResultMessage *) &pending[1]; 
  pending->msg = &tsrm->header;
  tsrm->header.size = htons (msize);
  tsrm->header.type = htons (GNUNET_MESSAGE_TYPE_DHT_P2P_TRAIL_SETUP_RESULT);
  memcpy (&(tsrm->destination_peer), destination_peer, sizeof (struct GNUNET_PeerIdentity));
  memcpy (&(tsrm->finger_identity), source_finger, sizeof (struct GNUNET_PeerIdentity));
  tsrm->trail_length = htonl (trail_length);
  tsrm->current_index = htonl (current_trail_index);
  tsrm->finger_map_index = htonl (finger_map_index);
  peer_list = (struct GNUNET_PeerIdentity *) &tsrm[1];
  memcpy (peer_list, trail_peer_list, trail_length * sizeof (struct GNUNET_PeerIdentity));
  
  /* Send the message to chosen friend. */
  GNUNET_CONTAINER_DLL_insert_tail (target_friend->head, target_friend->tail, pending);
  target_friend->pending_count++;
  process_friend_queue (target_friend);
}


/**
 * Construct a PeerVerifySuccessor message and send it to friend.
 * @param source_peer Peer which wants to verify its successor
 * @param successor Peer which is our current successor
 * @param target_friend Friend to which this message should be forwarded.
 * @param trail_peer_list Peer which are part of trail from source to destination
 * @param trail_length Number of peers in the trail list.
 * @param current_trail_index Index in the trial list at which receiving peer should
 *                            read the next element.
 */
void GDS_NEIGHBOURS_send_verify_successor(struct GNUNET_PeerIdentity *source_peer,
                                          struct GNUNET_PeerIdentity *successor,
                                          struct FriendInfo *target_friend,
                                          struct GNUNET_PeerIdentity *trail_peer_list,
                                          unsigned int trail_length,
                                          unsigned int current_trail_index)
{
  struct PeerVerifySuccessorMessage *vsm;
  struct P2PPendingMessage *pending;
  struct GNUNET_PeerIdentity *peer_list;
  size_t msize;
  
  msize = sizeof (struct PeerVerifySuccessorMessage) + 
          (trail_length * sizeof (struct GNUNET_PeerIdentity));
  
  if (msize >= GNUNET_SERVER_MAX_MESSAGE_SIZE)
  {
    GNUNET_break (0);
    return;
  }
 
  if (target_friend->pending_count >= MAXIMUM_PENDING_PER_FRIEND)
  {  
    GNUNET_STATISTICS_update (GDS_stats, gettext_noop ("# P2P messages dropped due to full queue"),
				1, GNUNET_NO);
  }
  
  pending = GNUNET_malloc (sizeof (struct P2PPendingMessage) + msize); 
  pending->importance = 0;    /* FIXME */
  pending->timeout = GNUNET_TIME_relative_to_absolute (GET_TIMEOUT);
  vsm = (struct PeerVerifySuccessorMessage *) &pending[1];
  pending->msg = &vsm->header;
  vsm->header.size = htons (msize);
  vsm->header.type = htons (GNUNET_MESSAGE_TYPE_DHT_P2P_VERIFY_SUCCESSOR);
  memcpy (&(vsm->successor), successor, sizeof (struct GNUNET_PeerIdentity));
  memcpy (&(vsm->source_peer), source_peer, sizeof (struct GNUNET_PeerIdentity));
  vsm->trail_length = htonl (trail_length);
  vsm->current_trail_index = htonl (current_trail_index);
  peer_list = (struct GNUNET_PeerIdentity *) &vsm[1];
  memcpy (peer_list, trail_peer_list, trail_length * sizeof (struct GNUNET_PeerIdentity));
  
  /* Send the message to chosen friend. */
  GNUNET_CONTAINER_DLL_insert_tail (target_friend->head, target_friend->tail, pending);
  target_friend->pending_count++;
  process_friend_queue (target_friend);
  
}


/**
 * Construct a PeerVerifySuccessorResult message and send it to friend.
 * @param destination_peer Peer which sent verify successor message
 * @param source_successor Peer to which verify successor message was sent.
 * @param my_predecessor source_successor's predecessor.
 * @param target_friend Friend to which this message should be forwarded.
 * @param trail_peer_list Peers which are part of trail from source to destination
 * @param trail_length Number of peers in the trail list.
 * @param current_trail_index Index in the trial list at which receiving peer should
 *                            get the next element.
 */
void GDS_NEIGHBOURS_send_verify_successor_result (struct GNUNET_PeerIdentity *destination_peer,
                                                  struct GNUNET_PeerIdentity *source_successor,
                                                  struct GNUNET_PeerIdentity *my_predecessor,
                                                  struct FriendInfo *target_friend,
                                                  struct GNUNET_PeerIdentity *trail_peer_list,
                                                  unsigned int trail_length,
                                                  unsigned int current_trail_index)
{
  struct PeerVerifySuccessorResultMessage *vsmr;
  struct P2PPendingMessage *pending;
  struct GNUNET_PeerIdentity *peer_list;
  size_t msize;
  
  msize = sizeof (struct PeerVerifySuccessorResultMessage) + 
          (trail_length * sizeof(struct GNUNET_PeerIdentity));
  
  if (msize >= GNUNET_SERVER_MAX_MESSAGE_SIZE)
  {
    GNUNET_break (0);
    return;
  }
  
  
  if (target_friend->pending_count >= MAXIMUM_PENDING_PER_FRIEND)
  {  
    GNUNET_STATISTICS_update (GDS_stats, gettext_noop ("# P2P messages dropped due to full queue"),
				1, GNUNET_NO);
  }

  pending = GNUNET_malloc (sizeof (struct P2PPendingMessage) + msize); 
  pending->importance = 0;    /* FIXME */
  pending->timeout = GNUNET_TIME_relative_to_absolute (GET_TIMEOUT);
  vsmr = (struct PeerVerifySuccessorResultMessage *) &pending[1];
  pending->msg = &vsmr->header;
  vsmr->header.size = htons (msize);
  vsmr->header.type = htons (GNUNET_MESSAGE_TYPE_DHT_P2P_VERIFY_SUCCESSOR_RESULT);
  memcpy (&(vsmr->destination_peer), destination_peer, sizeof (struct GNUNET_PeerIdentity));
  memcpy (&(vsmr->source_successor), source_successor, sizeof (struct GNUNET_PeerIdentity));
  memcpy (&(vsmr->my_predecessor), my_predecessor, sizeof (struct GNUNET_PeerIdentity));
  vsmr->trail_length = htonl (trail_length);
  vsmr->current_index = htonl (current_trail_index);
  
  peer_list = (struct GNUNET_PeerIdentity *) &vsmr[1];
  memcpy (peer_list, trail_peer_list, trail_length * sizeof (struct GNUNET_PeerIdentity));
  
   /* Send the message to chosen friend. */
  GNUNET_CONTAINER_DLL_insert_tail (target_friend->head, target_friend->tail, pending);
  target_friend->pending_count++;
  process_friend_queue (target_friend);
}


/**
 * Construct a PeerNotifyNewSuccessor message and send it to friend.
 * @param source_peer Peer which is sending notify message to its new successor.
 * @param destination_peer Peer which is the new destination.
 * @param target_friend Next friend to pass this message to. 
 * @param peer_list List of peers in the trail to reach to destination_peer.
 * @param current_trail_index Index of peer_list for next target friend position. 
 * @param trail_length Total number of peers in peer list 
 */
void 
GDS_NEIGHBOURS_send_notify_new_successor (struct GNUNET_PeerIdentity *source_peer,
                                          struct GNUNET_PeerIdentity *destination_peer,
                                          struct FriendInfo *target_friend,
                                          struct GNUNET_PeerIdentity *trail_peer_list,
                                          unsigned int trail_length,
                                          unsigned int current_trail_index)
{
  struct PeerNotifyNewSuccessorMessage *nsm;
  struct P2PPendingMessage *pending;
  struct GNUNET_PeerIdentity *peer_list;
  size_t msize;
  
  msize = sizeof (struct PeerNotifyNewSuccessorMessage) + 
          (trail_length * sizeof(struct GNUNET_PeerIdentity));
  
  if (msize >= GNUNET_SERVER_MAX_MESSAGE_SIZE)
  {
    GNUNET_break (0);
    return;
  }
  
  if (target_friend->pending_count >= MAXIMUM_PENDING_PER_FRIEND)
  {  
    GNUNET_STATISTICS_update (GDS_stats, gettext_noop ("# P2P messages dropped due to full queue"),
				1, GNUNET_NO);
  }
  
  pending = GNUNET_malloc (sizeof (struct P2PPendingMessage) + msize); 
  pending->importance = 0;    /* FIXME */
  pending->timeout = GNUNET_TIME_relative_to_absolute (GET_TIMEOUT);
  nsm = (struct PeerNotifyNewSuccessorMessage *) &pending[1];
  pending->msg = &nsm->header;
  nsm->header.size = htons (msize);
  nsm->header.type = htons (GNUNET_MESSAGE_TYPE_DHT_P2P_NOTIFY_NEW_SUCCESSOR);
  memcpy (&(nsm->source_peer), source_peer, sizeof (struct GNUNET_PeerIdentity));
  memcpy (&(nsm->destination_peer), destination_peer, sizeof (struct GNUNET_PeerIdentity));
  nsm->trail_length = htonl (trail_length);
  nsm->current_index = htonl (current_trail_index);
  
  peer_list = (struct GNUNET_PeerIdentity *) &nsm[1];
  memcpy (peer_list, trail_peer_list, trail_length * sizeof (struct GNUNET_PeerIdentity));
  
   /* Send the message to chosen friend. */
  GNUNET_CONTAINER_DLL_insert_tail (target_friend->head, target_friend->tail, pending);
  target_friend->pending_count++;
  process_friend_queue (target_friend);
}


/** 
 * FIXME: Optimizaiton Once the basic code is running. Add the optimization
 * where you check if the threshold on number of links that should go through
 * a particular friend has crossed. If yes then again choose a different
 * friend. Important that the new friend chosen should be different. How to 
 * ensure this? This is an important optimization as without this one x-vine
 * is actually not a sybil tolerant DHT.
 * Randomly choose one of your friends from the friends_peer map
 * @return Friend
 */
static struct FriendInfo *
select_random_friend()
{  
  unsigned int current_size;
  unsigned int *index; 
  unsigned int j = 0;
  struct GNUNET_CONTAINER_MultiPeerMapIterator *iter;
  struct GNUNET_PeerIdentity key_ret;
  struct FriendInfo *friend;
  
  current_size = GNUNET_CONTAINER_multipeermap_size (friend_peermap);
  index = GNUNET_CRYPTO_random_permute (GNUNET_CRYPTO_QUALITY_WEAK, current_size);
  iter = GNUNET_CONTAINER_multipeermap_iterator_create (friend_peermap);
 
  while(j < (*index))
  {
    if(GNUNET_YES == GNUNET_CONTAINER_multipeermap_iterator_next (iter,NULL,NULL))
    {
      j++;
    }
    else 
      return NULL;
  }  

  if(GNUNET_YES == GNUNET_CONTAINER_multipeermap_iterator_next (iter,&key_ret,(const void **)&friend))
    return friend;
  else
    return NULL;
}


/**
 * Compute finger_identity to which we want to setup the trail
 * @return finger_identity 
 */
static uint64_t *
compute_finger_identity()
{
  uint64_t my_id64 ;
  uint64_t *finger_identity64;
  
  finger_identity64 = GNUNET_malloc (sizeof (uint64_t));
  memcpy (&my_id64, &my_identity, sizeof (uint64_t));
  /*FIXME: Do we need a mod finger = ((my_id + pow(2, finger_index)) mod (pow (2, MAX_FINGERS))*/
  *finger_identity64 = (my_id64 + (unsigned long) pow (2,current_finger_index));
  
  return finger_identity64;
}


/**
 * Compute immediate predecessor identity in the network.
 * @return peer identity of immediate predecessor.
 */
static uint64_t *
compute_predecessor_identity()
{
  uint64_t my_id ;
  uint64_t *predecessor;

  predecessor = GNUNET_malloc (sizeof (uint64_t));
  memcpy (&my_id, &my_identity, sizeof (uint64_t));
  /* FIXME: Do we need to use mod pow(2, MAX_FINGERS) here? */
  *predecessor = (my_id -1);
          
  return predecessor;
}


/**
 * Periodically ping your successor to ask its current predecessor
 * 
 * @param cls closure for this task
 * @param tc the context under which the task is running
 */
static void
send_verify_successor_message (void *cls,
                               const struct GNUNET_SCHEDULER_TaskContext *tc )
{
  struct GNUNET_TIME_Relative next_send_time;
  struct GNUNET_CONTAINER_MultiPeerMapIterator *finger_iter;
  struct GNUNET_PeerIdentity key_ret;
  struct FriendInfo *target_friend;
  struct GNUNET_PeerIdentity *next_hop;
  struct GNUNET_PeerIdentity *peer_list;
  unsigned int finger_trail_current_index;
  struct FingerInfo *finger;
  unsigned int finger_index;
  unsigned int i;
  int flag = 0;
  
  finger_iter = GNUNET_CONTAINER_multipeermap_iterator_create (finger_peermap);  
  for (finger_index = 0; finger_index < GNUNET_CONTAINER_multipeermap_size (finger_peermap); finger_index++)
  {
    if(GNUNET_YES == GNUNET_CONTAINER_multipeermap_iterator_next (finger_iter, &key_ret,
                                                                 (const void **)&finger)) 
    {
      if (0 == finger->finger_map_index)
      {
        flag = 1;
        break;
      }
    }
  }
  GNUNET_CONTAINER_multipeermap_iterator_destroy (finger_iter);
  
  if( flag == 0)
    goto send_new_request;
  
  peer_list = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity) * finger->trail_length);
 
  struct TrailPeerList *iterate;
  iterate = finger->head;
  i = 0;
  while ( i < (finger->trail_length))
  {
    memcpy (&peer_list[i], &(iterate->peer), sizeof (struct GNUNET_PeerIdentity));
    iterate = iterate->next;
    i++;
  }
 
  next_hop = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
  memcpy (next_hop, &peer_list[0], sizeof (struct GNUNET_PeerIdentity));
  target_friend = GNUNET_CONTAINER_multipeermap_get (friend_peermap, next_hop);
  finger_trail_current_index = 0; 
  
  
  GDS_NEIGHBOURS_send_verify_successor (&my_identity,
                                        &(finger->finger_identity),
                                        target_friend,
                                        peer_list,
                                        finger->trail_length,
                                        finger_trail_current_index);
  
  
  /* FIXME: Use a random value so that this message is send not at the same
   interval as send_find_finger_trail_message. */
  send_new_request:
  next_send_time.rel_value_us =
      DHT_MINIMUM_FIND_FINGER_TRAIL_INTERVAL.rel_value_us +
      GNUNET_CRYPTO_random_u64 (GNUNET_CRYPTO_QUALITY_WEAK,
                                DHT_MAXIMUM_FIND_FINGER_TRAIL_INTERVAL.rel_value_us /
                                (current_finger_index + 50));
 
  verify_successor =
      GNUNET_SCHEDULER_add_delayed (next_send_time, &send_verify_successor_message,
                                    NULL);
}


/**
 * Task to send a find finger trail message. We attempt to find trail
 * to our fingers, successor and predecessor in the network.
 *
 * @param cls closure for this task
 * @param tc the context under which the task is running
 */
static void
send_find_finger_trail_message (void *cls,
                                const struct GNUNET_SCHEDULER_TaskContext *tc)
{
  struct FriendInfo *target_friend;
  struct GNUNET_TIME_Relative next_send_time;
  uint64_t *finger_identity;
  unsigned int finger_map_index;
  
  if (1 == current_finger_index)
  {
    finger_identity = compute_predecessor_identity();  
    goto select_friend;
  }
  else
  {
    finger_identity = compute_finger_identity();
  }
  
  select_friend:
  /* FIXME: Here should we choose randomly or not. */
  target_friend = select_random_friend();
  
  finger_map_index = current_finger_index;
  current_finger_index = ( current_finger_index + 1) % MAX_FINGERS;
  
  if(NULL != target_friend)
  { 
    GDS_NEIGHBOURS_send_trail_setup (&my_identity, *finger_identity, &(target_friend->id),
                                     target_friend, 0, NULL,
                                     finger_map_index, FRIEND);
  }
  
  /* FIXME: How to decide the correct interval? */
  next_send_time.rel_value_us =
      DHT_MINIMUM_FIND_FINGER_TRAIL_INTERVAL.rel_value_us +
      GNUNET_CRYPTO_random_u64 (GNUNET_CRYPTO_QUALITY_WEAK,
                                DHT_MAXIMUM_FIND_FINGER_TRAIL_INTERVAL.rel_value_us /
                                (current_finger_index + 10));
 
  find_finger_trail_task =
      GNUNET_SCHEDULER_add_delayed (next_send_time, &send_find_finger_trail_message,
                                    NULL);
}


/**
 * Method called whenever a peer connects.
 *
 * @param cls closure
 * @param peer_identity peer identity this notification is about
 */
static void
handle_core_connect (void *cls, const struct GNUNET_PeerIdentity *peer_identity)
{
  struct FriendInfo *friend;

  /* Check for connect to self message */
  if (0 == memcmp (&my_identity, peer_identity, sizeof (struct GNUNET_PeerIdentity)))
    return;
  
  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Connected to %s\n", GNUNET_i2s (peer_identity));
  
  /* If peer already exists in our friend_peermap, then exit. */
  if (GNUNET_YES == GNUNET_CONTAINER_multipeermap_contains (friend_peermap, peer_identity))
  {
    GNUNET_break (0);
    return;
  }

  GNUNET_STATISTICS_update (GDS_stats, gettext_noop ("# peers connected"), 1,
                            GNUNET_NO);

  friend = GNUNET_new (struct FriendInfo);
  friend->id = *peer_identity;
  
  GNUNET_assert (GNUNET_OK ==
                 GNUNET_CONTAINER_multipeermap_put (friend_peermap,
                                                    peer_identity, friend,
                                                    GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_ONLY));

  /* got a first connection, good time to start with FIND FINGER TRAIL requests... */
  if (1 == GNUNET_CONTAINER_multipeermap_size (friend_peermap))
    find_finger_trail_task = GNUNET_SCHEDULER_add_now (&send_find_finger_trail_message, NULL);
}


/**
 * Method called whenever a peer disconnects.
 *
 * @param cls closure
 * @param peer peer identity this notification is about
 */
static void
handle_core_disconnect (void *cls,
			const struct GNUNET_PeerIdentity *peer)
{
  struct FriendInfo *remove_friend;
  struct FingerInfo *remove_finger;
  struct GNUNET_PeerIdentity key_ret;
  struct GNUNET_CONTAINER_MultiPeerMapIterator *finger_iter;
  struct TrailPeerList *iterator;
  struct GNUNET_PeerIdentity *finger_identity;
  int finger_index;
  
  /* Check for self message. */
  if (0 == memcmp (&my_identity, peer, sizeof (struct GNUNET_PeerIdentity)))
    return;
  
   /* Search for peer to remove in your friend_peermap. */
  remove_friend =
      GNUNET_CONTAINER_multipeermap_get (friend_peermap, peer);
  
  if (NULL == remove_friend)
  {
    GNUNET_break (0);
    return;
  }
  
  iterator = GNUNET_malloc (sizeof (struct TrailPeerList));
  finger_identity = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
  finger_iter = GNUNET_CONTAINER_multipeermap_iterator_create (finger_peermap);  
  for (finger_index = 0; finger_index < GNUNET_CONTAINER_multipeermap_size (finger_peermap); finger_index++)
  {
    if(GNUNET_YES == GNUNET_CONTAINER_multipeermap_iterator_next (finger_iter, &key_ret,
                                                                 (const void **)&remove_finger)) 
    {
      iterator = remove_finger->head->next;
      if (0 == GNUNET_CRYPTO_cmp_peer_identity (&(iterator->peer), &(remove_friend->id)))
      {
        memcpy (finger_identity, &(remove_finger->finger_identity), sizeof (struct GNUNET_PeerIdentity));
        GNUNET_assert (GNUNET_YES ==
                       GNUNET_CONTAINER_multipeermap_remove (finger_peermap,
                                                             finger_identity,
                                                             remove_finger));
      }
    }
  }
  
  /* Remove the friend from friend_peermap. */
  GNUNET_assert (GNUNET_YES ==
                 GNUNET_CONTAINER_multipeermap_remove (friend_peermap,
                                                       peer,
                                                       remove_friend));
}


/**
 * To be called on core init/fail.
 *
 * @param cls service closure
 * @param identity the public identity of this peer
 */
static void
core_init (void *cls,
           const struct GNUNET_PeerIdentity *identity)
{
  my_identity = *identity;
  
#if 0
  /* SUPU TEST CODE */
  struct GNUNET_PeerIdentity *print_peer;
  print_peer = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
  memcpy (print_peer, &my_identity, sizeof (struct GNUNET_PeerIdentity));
  FPRINTF (stderr,_("\nSUPU %s, %s, %d,my_identity = %s"),
  __FILE__, __func__,__LINE__, GNUNET_i2s (print_peer));
  /* SUPU TEST CODE ENDS */
#endif
}


/**
 * 
 * @param destination_peer
 * @param existing_trail
 * @param trail_length
 * @return 
 */
static struct GNUNET_PeerIdentity *
invert_trail_list (struct GNUNET_PeerIdentity *destination_peer,
                   struct GNUNET_PeerIdentity *existing_trail, 
                   unsigned int trail_length)
{
  int i;
  int j;
  struct GNUNET_PeerIdentity *new_trail;
  
  j = 0;
  new_trail = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity) * trail_length);
  
  if (trail_length > 1)
  {
    i = trail_length - 2;
    while (i >= 0 )
    {
      memcpy( &new_trail[j], &existing_trail[i], sizeof (struct GNUNET_PeerIdentity));
      i--;
      j++;
    }
  }
  memcpy (&new_trail[j], destination_peer, sizeof(struct GNUNET_PeerIdentity));
 
  return new_trail;
}


/**
 * 
 * @param existing_finger
 * @param new_finger
 * @return 
 */
#if 0
static int
compare_finger_identity (struct GNUNET_PeerIdentity *existing_finger,
                         struct GNUNET_PeerIdentity *new_finger)
{
  int ret;
  ret = (existing_finger > new_finger) ? 1 : 
          (existing_finger == new_finger) ? 0 : -1;
  return ret;
}
#endif

/**
 * FIXME: Not sure of the logic to find the correct predecessor. 
 * Given two finger identities, find the closest predecessor. 
 * @param existing_predecessor
 * @param new_predecessor
 * @return 
 */
#if 0
static int
compare_predecessor(struct GNUNET_PeerIdentity *existing_predecessor,
                    struct GNUNET_PeerIdentity *new_predecessor)
{
  int ret;
  ret = (existing_predecessor < new_predecessor) ? 1 : 
          (existing_predecessor == new_predecessor) ? 0 : -1;
  return ret;
}
#endif

/*
 * Add an entry in finger table. 
 * Add an entry into finger table
 * @param finger_identity Peer identity of finger
 * @param finger_trail Trail to reach the finger
 * @param trail_length Number of peers in the trail. 
 * @param finger_map_index Index in finger peer map.
 */
static
void finger_table_add (struct GNUNET_PeerIdentity *finger_identity,
                       struct GNUNET_PeerIdentity *finger_trail,
                       unsigned int trail_length,
                       unsigned int finger_map_index)
{
  struct FingerInfo *new_finger_entry;
  int i;
#if 0
  struct GNUNET_CONTAINER_MultiPeerMapIterator *finger_iter;
  struct GNUNET_PeerIdentity key_ret;
  struct FingerInfo *existing_finger;
  int finger_index;
  int i;

  
  finger_iter = GNUNET_CONTAINER_multipeermap_iterator_create (finger_peermap); 

  for (finger_index = 0; finger_index < GNUNET_CONTAINER_multipeermap_size (finger_peermap); finger_index++)
  {
    if(GNUNET_YES == GNUNET_CONTAINER_multipeermap_iterator_next (finger_iter, &key_ret,
                                                                 (const void **)&existing_finger)) 
    {
      /* If we already have an entry at the finger map index. */
      if ((finger_map_index == existing_finger->finger_map_index))
      {
        /* Check if the finger entry are same. */
        if (0 == GNUNET_CRYPTO_cmp_peer_identity (&(existing_finger->finger_identity),finger_identity))
        {
          FPRINTF (stderr,_("\nSUPU %s, %s, %d"),    __FILE__, __func__,__LINE__);
          goto add_new_entry;
        }
        else
        {
          /* FIXME: here you should have something better to check which finger
           is closer one. */
          int ret;
          if (finger_map_index == 1)
          {
            FPRINTF (stderr,_("\nSUPU %s, %s, %d"),    __FILE__, __func__,__LINE__);
            ret = compare_predecessor (&(existing_finger->finger_identity),
                                       finger_identity);
          }
          else
          {
            FPRINTF (stderr,_("\nSUPU %s, %s, %d"),    __FILE__, __func__,__LINE__);
            ret = compare_finger_identity (&(existing_finger->finger_identity),
                                          finger_identity);
          }
          if (ret > 0)
          {
            FPRINTF (stderr,_("\nSUPU %s, %s, %d"),    __FILE__, __func__,__LINE__);
            GNUNET_assert (GNUNET_YES ==
                       GNUNET_CONTAINER_multipeermap_remove (finger_peermap,
                                                             &(existing_finger->finger_identity),
                                                             existing_finger));
            goto add_new_entry;
          }
          else
          {
            FPRINTF (stderr,_("\nSUPU %s, %s, %d"),    __FILE__, __func__,__LINE__);
            return;
          }
        }
      }
    }
  }

  add_new_entry:
#endif
  new_finger_entry = GNUNET_malloc (sizeof (struct FingerInfo));
  memcpy (&(new_finger_entry->finger_identity), finger_identity, sizeof (struct GNUNET_PeerIdentity));
  
  i = 0;
  while (i < trail_length)
  {
    struct TrailPeerList *element;
    element = GNUNET_malloc (sizeof (struct TrailPeerList));
    element->next = NULL;
    element->prev = NULL;
    
    memcpy (&(element->peer), &finger_trail[i], sizeof(struct GNUNET_PeerIdentity));
    GNUNET_CONTAINER_DLL_insert_tail(new_finger_entry->head, new_finger_entry->tail, element);
    i++;
  }
  
  new_finger_entry->finger_map_index = finger_map_index;
  new_finger_entry->trail_length = trail_length;
  
  /* FIXME: Here we are keeping multiple hashmap option so that there are
   multiple routes to reach to same finger, redundant routing.
   * Also same peers could be our fingers for different finger map index */
  GNUNET_assert (GNUNET_OK ==
                 GNUNET_CONTAINER_multipeermap_put (finger_peermap,
                                                    &(new_finger_entry->finger_identity),
                                                    new_finger_entry,
                                                    GNUNET_CONTAINER_MULTIHASHMAPOPTION_MULTIPLE)); /* Fixme: change to multiple */
  
  
  if (1 == GNUNET_CONTAINER_multipeermap_size (finger_peermap)
      && (new_finger_entry->finger_map_index!= 1))
  {
    verify_successor = GNUNET_SCHEDULER_add_now (&send_verify_successor_message, NULL);
  }
}
  

/**
 * Compare two peer identities.
 * @param p1 Peer identity
 * @param p2 Peer identity
 * @return 1 if p1 > p2, -1 if p1 < p2 and 0 if p1 == p2. 
 */
  static int
compare_peer_id (const void *p1, const void *p2)
{
  struct Sorting_List *p11;
  struct Sorting_List *p22;
  int ret;
  p11 = GNUNET_malloc (sizeof (struct Sorting_List));
  p22 = GNUNET_malloc (sizeof (struct Sorting_List));
  p11 = (struct Sorting_List *)p1;
  p22 = (struct Sorting_List *)p2;
  ret = ( (p11->peer_id) > (p22->peer_id) ) ? 1 : 
          ( (p11->peer_id) == (p22->peer_id) ) ? 0 : -1;
  return ret;
}

  
/**
 * Return the successor of value in all_known_peers.
 * @param all_known_peers list of all the peers
 * @param value value we have to search in the all_known_peers.
 * @return 
 */
static struct Sorting_List *
find_closest_successor(struct Sorting_List *all_known_peers, uint64_t value,
                       unsigned int size)
{
  int first;
  int last;
  int middle;
  
  first = 0;
  last = size - 1;
  middle = (first + last)/2;
  
  while(first <= last)
  {
    if(all_known_peers[middle].peer_id < value)
    {
      first = middle + 1; 
    }
    else if(all_known_peers[middle].peer_id == value)
    {
      if(middle == (size -1))
      {
        return &all_known_peers[0];
      }
      else
      {
        return &all_known_peers[middle+1];
      }
    }
    else
    {
       last = middle - 1;
    }
  
    middle = (first + last)/2;  
  }
  return NULL;
}


/**
 * Find closest successor for the value.
 * @param value Value for which we are looking for successor
 * @param current_destination NULL if my_identity is successor else finger/friend 
 *                            identity 
 * @param type Next destination type
 * @return Peer identity of next destination i.e. successor of value. 
 */
static struct GNUNET_PeerIdentity *
find_successor (uint64_t value, struct GNUNET_PeerIdentity *current_destination,
               enum current_destination_type *type)
{
  struct GNUNET_CONTAINER_MultiPeerMapIterator *friend_iter;
  struct GNUNET_CONTAINER_MultiPeerMapIterator *finger_iter;
  struct GNUNET_PeerIdentity key_ret;
  struct FriendInfo *friend;
  struct FingerInfo *finger;
  unsigned int finger_index;
  unsigned int friend_index;
  struct Sorting_List *successor;
  unsigned int size;
  int j;
  
  /* 2 is added in size for my_identity and value which will part of all_known_peers. */
  size = GNUNET_CONTAINER_multipeermap_size (friend_peermap)+
         GNUNET_CONTAINER_multipeermap_size (finger_peermap)+
         2;
  
  struct Sorting_List all_known_peers[size];
  
  int k;
  for (k = 0; k < size; k++)
    all_known_peers[k].peer_id = 0;
  
  /* Copy your identity at 0th index in all_known_peers. */
  j = 0;
  memcpy (&(all_known_peers[j].peer_id), &my_identity, sizeof (uint64_t));
  all_known_peers[j].type = MY_ID;
  all_known_peers[j].data = 0;
  j++;
  
  /* Copy value */
  all_known_peers[j].peer_id = value;
  all_known_peers[j].type = VALUE;
  all_known_peers[j].data = 0;
  j++;
  
  /* Iterate over friend peer map and copy all the elements into array. */
  friend_iter = GNUNET_CONTAINER_multipeermap_iterator_create (friend_peermap); 
  for (friend_index = 0; friend_index < GNUNET_CONTAINER_multipeermap_size (friend_peermap); friend_index++)
  {
    if(GNUNET_YES == GNUNET_CONTAINER_multipeermap_iterator_next(friend_iter,&key_ret,(const void **)&friend)) 
    {
      memcpy (&(all_known_peers[j].peer_id), &(friend->id), sizeof (uint64_t));
      all_known_peers[j].type = FRIEND;
      all_known_peers[j].data = friend;
      j++;
    }
  }
  
  /* Iterate over finger map and copy all the entries into all_known_peers array. */
  finger_iter = GNUNET_CONTAINER_multipeermap_iterator_create (finger_peermap);  
  for (finger_index = 0; finger_index < GNUNET_CONTAINER_multipeermap_size (finger_peermap); finger_index++)
  {
    if(GNUNET_YES == GNUNET_CONTAINER_multipeermap_iterator_next(finger_iter,&key_ret,(const void **)&finger)) 
    {
      memcpy (&(all_known_peers[j].peer_id), &(finger->finger_identity), sizeof (uint64_t));
      all_known_peers[j].type = FINGER;
      all_known_peers[j].data = finger;
      j++;
    }
  }
  
  GNUNET_CONTAINER_multipeermap_iterator_destroy (finger_iter);
  GNUNET_CONTAINER_multipeermap_iterator_destroy (friend_iter);   
  
  qsort (&all_known_peers, size, sizeof (struct Sorting_List), &compare_peer_id);
  
  /* search value in all_known_peers array. */
  successor = find_closest_successor (all_known_peers, value, size);
  
  if (successor->type == MY_ID)
  {
    *type = MY_ID;
    return NULL;
  }
  else if (successor->type == FRIEND)
  {
    *type = FRIEND;
    struct FriendInfo *target_friend;
    target_friend = (struct FriendInfo *)successor->data;
    memcpy (current_destination, &(target_friend->id), sizeof (struct GNUNET_PeerIdentity));
    return current_destination;
  }
  else if (successor->type == FINGER)
  {
    *type = FINGER;
    struct GNUNET_PeerIdentity *next_hop;
    struct FingerInfo *finger;
    struct TrailPeerList *iterator;
    iterator = GNUNET_malloc (sizeof (struct TrailPeerList));
    finger = successor->data;
    iterator = finger->head;
    next_hop = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
    memcpy (next_hop, &(iterator->peer), sizeof (struct GNUNET_PeerIdentity));
    memcpy (current_destination, &(finger->finger_identity), sizeof (struct GNUNET_PeerIdentity));
    return next_hop;
  }
  else
  {
   type = NULL;
   return NULL;
  }
}


/**
 * FIXME:
 * 1. Do we have an expiration time for get request? Yes but I don't know its
 * use case and how to handle it
 * Send a get message to selected target friend. If target friend in NULL,
 * then search for a target friend. 
 * @param request_id Unique ID identifying this request
 * @param source_peer Peer which started get request
 * @param get_peer_path Peer list to reach to final destination which contains the data.
 * @param get_path_length Total numbers of peer in get_path
 * @param key Key key for the content
 * @param target_peer Next peer to forward the message to.
 * @param current_destination Peer which will get this message. 
 * @param current_dest_type Type of current destination can be either FRIEND or FINGER
 */
void
GDS_NEIGHBOURS_handle_get (struct GNUNET_PeerIdentity *source_peer, 
                           struct GNUNET_PeerIdentity *get_peer_path,
                           unsigned int get_path_length,
                           struct GNUNET_HashCode *key,
                           struct GNUNET_PeerIdentity *target_peer,
                           struct GNUNET_PeerIdentity *current_destination,
                           enum current_destination_type *current_dest_type)
{
  struct PeerGetMessage *get_request;
  struct P2PPendingMessage *pending;
  struct GNUNET_PeerIdentity *get_path;
  struct FriendInfo *target_friend;
  uint64_t key_value;
  size_t msize;
  
  msize = sizeof (struct PeerGetMessage) + 
          (get_path_length * sizeof (struct GNUNET_PeerIdentity));
  
  if (msize >= GNUNET_SERVER_MAX_MESSAGE_SIZE)
  {
    GNUNET_break (0);
    return;
  }
  
  memcpy (&key_value, key, sizeof (uint64_t));
  
  /* FIXME: Is this correct comparison? */
  if (NULL == target_peer)
  {
    /* This is the first call made from client file. */
    struct GNUNET_PeerIdentity *next_hop;  
    next_hop = find_successor (key_value, current_destination, current_dest_type);
    
    if (*current_dest_type == MY_ID)
    {
      struct GNUNET_PeerIdentity *destination_peer;
      get_path = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
      memcpy (get_path, &my_identity, sizeof (struct GNUNET_PeerIdentity));
      get_path_length = 1;
      destination_peer = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
      memcpy (destination_peer, source_peer, sizeof (struct GNUNET_PeerIdentity));
      /* FIXME: We enter in this part of code only when this is our first
       call from client file. In client file we already have done datacache_get
       and if we already have the result. So, ideally code should never reach
       here. Remove it after verifying. */
      /* FIXME: Call datacache_get but remove after verified above thing. */
      GDS_NEIGHBOURS_send_get_result (&my_identity,get_path, get_path_length, 
                                      key,destination_peer, 0,
                                      NULL,0,NULL);

      return;
    }
    else
    {
      /* Find the friend corresponding to next_hop */
      target_friend = GNUNET_CONTAINER_multipeermap_get (friend_peermap, next_hop);
    }
  }
  else
    target_friend = GNUNET_CONTAINER_multipeermap_get (friend_peermap, target_peer);
  
  pending = GNUNET_malloc (sizeof (struct P2PPendingMessage) + msize);
  pending->importance = 0;    /* FIXME */
  /* FIXME: Do we have an expiration time for get request */
  get_request = (struct PeerGetMessage *) &pending[1];
  pending->msg = &get_request->header;
  get_request->header.size = htons (msize);
  get_request->header.type = htons (GNUNET_MESSAGE_TYPE_DHT_P2P_GET);
  get_request->get_path_length = htonl (get_path_length);
  get_request->key = *key;
  
  memcpy (&(get_request->source_peer), source_peer, sizeof (struct GNUNET_PeerIdentity));
  memcpy (&(get_request->current_destination), current_destination, sizeof (struct GNUNET_PeerIdentity));
  get_request->current_dest_type = htonl (*current_dest_type);
  get_path = (struct GNUNET_PeerIdentity *) &get_request[1];
  memcpy (get_path, get_peer_path, get_path_length * sizeof (struct GNUNET_PeerIdentity));
  GNUNET_CONTAINER_DLL_insert_tail (target_friend->head, target_friend->tail, pending);
  target_friend->pending_count++;
  process_friend_queue (target_friend);
  
}


/**
 * FIXME: In this function, you just find the target friend and send the message
 * to next peer. In handle_dht_p2p_put, you should check the options and type 
 * and check if you are final destination or not. if not then find the next
 * destination and send the message forward. 
 * @param type type of the block
 * @param options routing options
 * @param desired_replication_level desired replication count
 * @param expiration_time when does the content expire
 * @param hop_count how many hops has this message traversed so far
 * @param key key for the content
 * @param put_path_length number of entries in @a put_path
 * @param put_path peers this request has traversed so far (if tracked)
 * @param data payload to store
 * @param data_size number of bytes in @a data
 * @param current_destination
 * @param dest_type
 * @param target_peer_id
 */
void
GDS_NEIGHBOURS_handle_put (enum GNUNET_BLOCK_Type type,
                           enum GNUNET_DHT_RouteOption options,
                           uint32_t desired_replication_level,
                           struct GNUNET_TIME_Absolute expiration_time,
                           uint32_t hop_count,
                           const struct GNUNET_HashCode *key,
                           unsigned int put_path_length,
                           struct GNUNET_PeerIdentity *put_path,
                           const void *data, size_t data_size,
                           struct GNUNET_PeerIdentity *current_destination,
                           enum current_destination_type dest_type,
                           struct GNUNET_PeerIdentity *target_peer)
{
  struct PeerPutMessage *ppm;
  struct P2PPendingMessage *pending;
  struct FriendInfo *target_friend;
  struct GNUNET_PeerIdentity *pp;
  size_t msize;
  uint64_t key_value;
  
  msize = put_path_length * sizeof (struct GNUNET_PeerIdentity) + data_size +
          sizeof (struct PeerPutMessage);
  
  if (msize >= GNUNET_SERVER_MAX_MESSAGE_SIZE)
  {
    put_path_length = 0;
    msize = data_size + sizeof (struct PeerPutMessage);
  }
  
  if (msize >= GNUNET_SERVER_MAX_MESSAGE_SIZE)
  {
    GNUNET_break (0);
    return;
  }
  
  memcpy (&key_value, key, sizeof (uint64_t)); 
  struct GNUNET_PeerIdentity *current_dest;
  current_dest = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
  if (target_peer == NULL)
  {
     /* This is the first time the call has been made from handle_dht_local_put.
     So, you need to search for the next peer to send this message to. */
    struct GNUNET_PeerIdentity *next_hop;  
   
    next_hop = find_successor (key_value, current_dest, &dest_type);

    
    if (dest_type == MY_ID)
    {
      /* FIXME: How do we handle different block types? */
      /* FIXME: Here depending on the replication level choose 'r' successors
       to this peer and send put to all of these peers. */
      //replicate_put();
      GDS_DATACACHE_handle_put (expiration_time, key, put_path_length, put_path,
                                type, data_size, data);
      return;
    }
    else
    {
      /* Find the friend corresponding to next_hop */
      target_friend = GNUNET_CONTAINER_multipeermap_get (friend_peermap, next_hop);
    }
  }
  else
    target_friend = GNUNET_CONTAINER_multipeermap_get (friend_peermap, target_peer);
  
  pending = GNUNET_malloc (sizeof (struct P2PPendingMessage) + msize);
  pending->importance = 0;    /* FIXME */
  pending->timeout = expiration_time;
  ppm = (struct PeerPutMessage *) &pending[1];
  pending->msg = &ppm->header;
  ppm->header.size = htons (msize);
  ppm->header.type = htons (GNUNET_MESSAGE_TYPE_DHT_P2P_PUT);
  ppm->options = htonl (options);
  ppm->type = htonl (type);
  ppm->hop_count = htonl (hop_count + 1);
  ppm->desired_replication_level = htonl (desired_replication_level);
  ppm->put_path_length = htonl (put_path_length);
  ppm->expiration_time = GNUNET_TIME_absolute_hton (expiration_time);
  if (current_destination != NULL)
    memcpy (&(ppm->current_destination), current_destination, sizeof (struct GNUNET_PeerIdentity));
  else
    memcpy (&(ppm->current_destination), current_dest, sizeof (struct GNUNET_PeerIdentity));
  ppm->current_destination_type = htonl (dest_type);
  ppm->key = *key;
  
  pp = (struct GNUNET_PeerIdentity *) &ppm[1];
  memcpy (pp, put_path,
           sizeof (struct GNUNET_PeerIdentity) * put_path_length);
  memcpy (&pp[put_path_length], data, data_size);
  GNUNET_CONTAINER_DLL_insert_tail (target_friend->head, target_friend->tail, pending);
  target_friend->pending_count++;
  process_friend_queue (target_friend);
}


/**
 * Send get result back to requesting client. 
 * @param source_peer
 * @param get_path
 * @param target_friend
 * @param get_path_length
 * @param key
 * @param destination_peer
 * @param current_path_index
 * @param data
 * @param data_size
 */
void 
GDS_NEIGHBOURS_send_get_result (struct GNUNET_PeerIdentity *source_peer,
                                struct GNUNET_PeerIdentity *get_path,
                                unsigned int get_path_length,
                                struct GNUNET_HashCode *key,
                                struct GNUNET_PeerIdentity *destination_peer,
                                unsigned int current_path_index,
                                const void *data, size_t data_size,
                                struct GNUNET_PeerIdentity *next_hop)
{
  /* Add get_result into pending message and send the data to target friend. 
   make a call GDS_CLIENTS_process_get_result() with all the fields. */
  struct PeerGetResultMessage *get_result;
  struct GNUNET_PeerIdentity *get_result_path;
  struct P2PPendingMessage *pending;
  struct FriendInfo *target_friend;
  size_t msize;
  
  msize = get_path_length * sizeof (struct GNUNET_PeerIdentity) + data_size +
          sizeof (struct PeerPutMessage);
 
  if (msize >= GNUNET_SERVER_MAX_MESSAGE_SIZE)
  {
    GNUNET_break (0);
    return;
  }
  
  pending = GNUNET_malloc (sizeof (struct P2PPendingMessage) + msize);
  pending->importance = 0;    /* FIXME */
  /* FIXME: Should we add an expiration time like in put message. */
  get_result = (struct PeerGetResultMessage *)&pending[1];
  pending->msg = &get_result->header;
  get_result->header.size = htons (msize);
  get_result->header.type = htons (GNUNET_MESSAGE_TYPE_DHT_P2P_GET_RESULT);
  memcpy (&(get_result->source_peer), source_peer, sizeof (struct GNUNET_PeerIdentity));
  memcpy (&(get_result->destination_peer), destination_peer, sizeof (struct GNUNET_PeerIdentity));
  get_result->current_path_index = current_path_index;
  get_result->key = *key;
  
  get_result_path = (struct GNUNET_PeerIdentity *)&get_result[1];
  memcpy (get_result_path, get_path,
           sizeof (struct GNUNET_PeerIdentity) * get_path_length);
  memcpy (&get_result_path[get_path_length], data, data_size);
  
  target_friend = GNUNET_CONTAINER_multipeermap_get (friend_peermap, next_hop);
  GNUNET_CONTAINER_DLL_insert_tail (target_friend->head, target_friend->tail, pending);
  target_friend->pending_count++;
  process_friend_queue (target_friend);
}


/**
 * 
 * @param cls
 * @param peer
 * @param message
 * @return 
 */
static int
handle_dht_p2p_get_result (void *cls, const struct GNUNET_PeerIdentity *peer,
                           const struct GNUNET_MessageHeader *message)
{
  /* If you are the source, go back to the client file and there search for
   the requesting client and send back the result. */
  struct PeerGetResultMessage *get_result;
  struct GNUNET_PeerIdentity *get_path;
  void *payload;
  size_t payload_size;
  size_t msize;
  unsigned int getlen;
  int current_path_index;
  
  msize = ntohs (message->size);
  if (msize < sizeof (struct PeerGetResultMessage))
  {
    GNUNET_break_op (0);
    return GNUNET_YES;
  }
  
  get_result = (struct PeerGetResultMessage *)message;
  getlen = ntohl (get_result->get_path_length);
  
  if ((msize <
       sizeof (struct PeerGetResultMessage) +
       getlen * sizeof (struct GNUNET_PeerIdentity)) ||
      (getlen >
       GNUNET_SERVER_MAX_MESSAGE_SIZE / sizeof (struct GNUNET_PeerIdentity)))
  {
    GNUNET_break_op (0);
    return GNUNET_YES;
  }
  
  get_path = (struct GNUNET_PeerIdentity *) &get_result[1];
  payload = &get_path[getlen];
  payload_size = msize - (sizeof (struct PeerGetResultMessage) + 
                          getlen * sizeof (struct GNUNET_PeerIdentity));
  current_path_index = ntohl (get_result->current_path_index);
  
  /* Here you just have to check if you are the destination or not if not 
   then read the next peer and send the message. */
  if (0 == (GNUNET_CRYPTO_cmp_peer_identity (&my_identity, &(get_result->destination_peer))))
  {
    /* I am the destination. Call the function in client file.
     * FIXME: Here I don't know how I communicate the result back to client
       file.  */
    //GDS_CLIENTS_process_get_result();
    return GNUNET_YES;
  }
  else
  {
    /* Read the element from the get path at trail index. set the trail index 
     and call gds_neighbours_send_get_result.*/
    struct GNUNET_PeerIdentity *next_hop;
    
    next_hop = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
    memcpy (next_hop, &get_path[current_path_index], sizeof (struct GNUNET_PeerIdentity));
 
    
    if (current_path_index != 0)
      current_path_index--;
    
    GDS_NEIGHBOURS_send_get_result (&my_identity, get_path,
                                    getlen,&(get_result->key),&(get_result->destination_peer),
                                    current_path_index,
                                    payload, payload_size, next_hop);
    return GNUNET_YES;
  }  
  return GNUNET_SYSERR;
}



/**
 * Core handler for p2p put requests.
 *
 * @param cls closure
 * @param peer sender of the request
 * @param message message
 * @param peer peer identity this notification is about
 * @return #GNUNET_OK to keep the connection open,
 *         #GNUNET_SYSERR to close it (signal serious error)
 */
static int
handle_dht_p2p_put (void *cls, const struct GNUNET_PeerIdentity *peer,
                    const struct GNUNET_MessageHeader *message)
{
  struct PeerPutMessage *put;
  struct GNUNET_PeerIdentity *put_path;
  enum GNUNET_DHT_RouteOption options;
  enum current_destination_type current_dst_type;
  struct GNUNET_PeerIdentity *current_destination;
  struct GNUNET_PeerIdentity *source_peer;
  struct GNUNET_PeerIdentity *next_hop;
  struct GNUNET_HashCode test_key;
  uint64_t key_value;
  void *payload;
  size_t payload_size;
  size_t msize;
  uint32_t putlen;
  
  put = (struct PeerPutMessage *) message;
  putlen = ntohl (put->put_path_length);
  msize = ntohs (message->size);
   
  if ((msize <
       sizeof (struct PeerPutMessage) +
       putlen * sizeof (struct GNUNET_PeerIdentity)) ||
      (putlen >
       GNUNET_SERVER_MAX_MESSAGE_SIZE / sizeof (struct GNUNET_PeerIdentity)))
  {
    GNUNET_break_op (0);
    return GNUNET_YES;
  }
  
  put_path = (struct GNUNET_PeerIdentity *) &put[1];
  payload = &put_path[putlen];
  options = ntohl (put->options);
  payload_size = msize - (sizeof (struct PeerPutMessage) + 
                          putlen * sizeof (struct GNUNET_PeerIdentity));
  
  /* FIXME: I don't understand what exactly are we doing here. */
  switch (GNUNET_BLOCK_get_key
          (GDS_block_context, ntohl (put->type), payload, payload_size,
           &test_key))
  {
  case GNUNET_YES:
    if (0 != memcmp (&test_key, &put->key, sizeof (struct GNUNET_HashCode)))
    {
      char *put_s = GNUNET_strdup (GNUNET_h2s_full (&put->key));
      GNUNET_break_op (0);
      GNUNET_log (GNUNET_ERROR_TYPE_WARNING,
                  "PUT with key `%s' for block with key %s\n",
                  put_s, GNUNET_h2s_full (&test_key));
      GNUNET_free (put_s);
      return GNUNET_YES;
    }
    break;
  case GNUNET_NO:
    GNUNET_break_op (0);
    return GNUNET_YES;
  case GNUNET_SYSERR:
    /* cannot verify, good luck */
    break;
  }
  
  /* FIXME: This part is also not clear to me.*/
  if (ntohl (put->type) == GNUNET_BLOCK_TYPE_REGEX) /* FIXME: do for all tpyes */
  {
    switch (GNUNET_BLOCK_evaluate (GDS_block_context,
                                   ntohl (put->type),
                                   NULL,    /* query */
                                   NULL, 0, /* bloom filer */
                                   NULL, 0, /* xquery */
                                   payload, payload_size))
    {
    case GNUNET_BLOCK_EVALUATION_OK_MORE:
    case GNUNET_BLOCK_EVALUATION_OK_LAST:
      break;

    case GNUNET_BLOCK_EVALUATION_OK_DUPLICATE:
    case GNUNET_BLOCK_EVALUATION_RESULT_INVALID:
    case GNUNET_BLOCK_EVALUATION_RESULT_IRRELEVANT:
    case GNUNET_BLOCK_EVALUATION_REQUEST_VALID:
    case GNUNET_BLOCK_EVALUATION_REQUEST_INVALID:
    case GNUNET_BLOCK_EVALUATION_TYPE_NOT_SUPPORTED:
    default:
      GNUNET_break_op (0);
      return GNUNET_OK;
    }
  }
  
  struct GNUNET_PeerIdentity pp[putlen + 1];

  /* extend 'put path' by sender */
  if (0 != (options & GNUNET_DHT_RO_RECORD_ROUTE))
  {
    memcpy (pp, put_path, putlen * sizeof (struct GNUNET_PeerIdentity));
    pp[putlen] = *peer;
    putlen++;
  }
  else
    putlen = 0;
  
  /* Copy the fields of message, call find successor or gds_routing_search,
   depending on the destination_type and if you are the final destination,
   do a datache put or if option is. else call gds_neighbours_handle_get with
   correct parameters. */
   current_destination = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
   memcpy (current_destination, &(put->current_destination), sizeof (struct GNUNET_PeerIdentity));
   current_dst_type = ntohl (put->current_destination_type);
   memcpy (&key_value, &(put->key), sizeof (uint64_t));
   source_peer = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
   memcpy (source_peer, &(put->source_peer), sizeof (struct GNUNET_PeerIdentity));
   
  if (current_dst_type == FRIEND)
  {
    next_hop = find_successor (key_value, current_destination, &current_dst_type);
  }
  else if (current_dst_type == FINGER)
  {
    next_hop = GDS_ROUTING_search (source_peer, current_destination, peer);
  }
  
  if (current_dst_type == MY_ID) 
  {
    /* Here datacache_put*/ 
    /* FIXME: Here depending on replication, call replicate_put() to do the
     put operation on 'r' successors. */
    GDS_DATACACHE_handle_put (GNUNET_TIME_absolute_ntoh (put->expiration_time),
                              &(put->key),putlen, pp, ntohl (put->type), payload_size,
                              payload);
    return GNUNET_YES;
  }
  else
  {
    /* here call gds_neighbours*/
    GDS_NEIGHBOURS_handle_put (ntohl (put->type),ntohl (put->options),
                               ntohl (put->desired_replication_level),
                               GNUNET_TIME_absolute_ntoh (put->expiration_time),
                               ntohl (put->hop_count),&put->key, putlen,
                               pp, payload, payload_size,
                               current_destination, current_dst_type, next_hop);
    return GNUNET_YES;
  }
  return GNUNET_SYSERR;
}


/**
 * FIXME: Handle expiration, options, block type, replication
 * referring the old code. 
 * Core handler for p2p get requests.
 *
 * @param cls closure
 * @param peer sender of the request
 * @param message message
 * @return #GNUNET_OK to keep the connection open,
 *         #GNUNET_SYSERR to close it (signal serious error)
 */
static int
handle_dht_p2p_get (void *cls, const struct GNUNET_PeerIdentity *peer,
                    const struct GNUNET_MessageHeader *message)
{
  struct PeerGetMessage *get;
  struct GNUNET_PeerIdentity *current_destination;
  uint64_t key_value;
  enum current_destination_type cuurent_dest_type;
  struct GNUNET_PeerIdentity *next_hop;
  struct GNUNET_PeerIdentity *get_path;
  size_t msize;
  unsigned int get_length;
  
  msize = ntohs (message->size);
  if (msize < sizeof (struct PeerGetMessage))
  {
    GNUNET_break_op (0);
    return GNUNET_YES;
  }
  
  get = (struct PeerGetMessage *)message;
  get_length = ntohl (get->get_path_length);
  get_path = (struct GNUNET_PeerIdentity *)&get[1];
  
  if ((msize <
       sizeof (struct PeerGetMessage) +
       get_length * sizeof (struct GNUNET_PeerIdentity)) ||
       (get_length >
        GNUNET_SERVER_MAX_MESSAGE_SIZE / sizeof (struct GNUNET_PeerIdentity)))
  {
    GNUNET_break_op (0);
    return GNUNET_YES; 
  }
  
  get = (struct PeerGetMessage *) message;
  current_destination = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
  memcpy (current_destination, &(get->current_destination), sizeof (struct GNUNET_PeerIdentity));
  memcpy (&key_value, &(get->key), sizeof (uint64_t));
  cuurent_dest_type = ntohl (get->current_dest_type);
 
  if (cuurent_dest_type == FRIEND)
  {
    next_hop = find_successor (key_value, current_destination, &cuurent_dest_type);
  }
  else if (cuurent_dest_type == FINGER)
  {
    next_hop = GDS_ROUTING_search (&(get->source_peer), current_destination, peer);
  }
  
  if (cuurent_dest_type == MY_ID)
  {
    struct GNUNET_PeerIdentity *destination_peer;
    //struct GNUNET_PeerIdentity *next_hop;
    //int current_path_index;
    
    /* Add yourself to the get path, increment the get length. */
    destination_peer = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
    memcpy (destination_peer, &(get->source_peer), sizeof (struct GNUNET_PeerIdentity));
    //current_path_index = get_length - 1;
    
    /* I am the final destination. Call GDS_NEIGHBOURS_send_get_result.
     * FIXME: Last parameters are data and data size. First implement datacache get
     * and get the result and send the data. Also seach for the next friend to pass
     * this message to.  */
#if 0 
    /* FIXME: Here we should call GDS_CLIENT_handle_reply. */
    GDS_NEIGHBOURS_send_get_result (get->request_id, &my_identity, get_path,
                                    get_length,&(get->key), destination_peer, current_path_index,
                                    NULL, 0, next_hop);
#endif 
    return GNUNET_YES;
  }
  else
  {
    /* FIXME: Add your self to the get path and increment the get length. */
    
    /* FIXME: Does it matter if the dest_type is friend or finger. */
    GDS_NEIGHBOURS_handle_get (&(get->source_peer), get_path,
                               get_length, &(get->key),next_hop,
                               current_destination,&cuurent_dest_type);
    
    return GNUNET_YES;
  }
  return GNUNET_SYSERR;
}


/** 
 * Handle a PeerTrailSetupMessage. 
 * @param cls closure
 * @param message message
 * @param peer peer identity this notification is about
 * @return GNUNET_OK on success, GNUNET_SYSERR on error
 */
static int
handle_dht_p2p_trail_setup(void *cls, const struct GNUNET_PeerIdentity *peer,
                           const struct GNUNET_MessageHeader *message)
{
  struct PeerTrailSetupMessage *trail_setup; 
  struct GNUNET_PeerIdentity *next_hop; 
  struct FriendInfo *target_friend;
  struct GNUNET_PeerIdentity *current_destination;
  struct GNUNET_PeerIdentity *trail_peer_list;
  enum current_destination_type current_dest_type;
  struct GNUNET_PeerIdentity *next_peer;
  unsigned int trail_length;
  uint32_t current_trail_index;
  unsigned int finger_map_index;
  uint64_t destination_finger_value;
  size_t msize;
  
  msize = ntohs (message->size);
  if (msize < sizeof (struct PeerTrailSetupMessage))
  {
    GNUNET_break_op (0);
    return GNUNET_YES;
  }
  
  trail_setup = (struct PeerTrailSetupMessage *) message; 
  trail_length = ntohl (trail_setup->trail_length); 
  
  if ((msize < sizeof (struct PeerTrailSetupMessage) +
       trail_length * sizeof (struct GNUNET_PeerIdentity)) ||
       (trail_length >
        GNUNET_SERVER_MAX_MESSAGE_SIZE / sizeof (struct GNUNET_PeerIdentity)))
  {
    GNUNET_break_op (0);
    return GNUNET_YES; 
  }
  
  current_dest_type = ntohl (trail_setup->current_destination_type);
  finger_map_index = ntohl (trail_setup->finger_map_index);
  trail_peer_list = (struct GNUNET_PeerIdentity *)&trail_setup[1];
  destination_finger_value = trail_setup->destination_finger;
  current_destination = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
  memcpy (current_destination, &(trail_setup->current_destination), sizeof (struct GNUNET_PeerIdentity));
  
  /* Find the next hop to send the packet to. */
  if (current_dest_type == FRIEND)
  {
    if (0 == (GNUNET_CRYPTO_cmp_peer_identity (&(trail_setup->current_destination),
                                               &my_identity)))
    {
      next_hop = find_successor (destination_finger_value, current_destination, &(current_dest_type));
    }
    else
      return GNUNET_SYSERR; 
  }
  else if (current_dest_type == FINGER)
  {
    if (0 != (GNUNET_CRYPTO_cmp_peer_identity (&(trail_setup->current_destination),
                                               &my_identity)))
    {
      next_hop = GDS_ROUTING_search (&(trail_setup->source_peer),
                                     &(trail_setup->current_destination), peer);
      /* As an optimization, find the successor from the find successor and
       compare both the ids to find the closest peer. */
    } 
    else
    {
      next_hop = find_successor (destination_finger_value, current_destination, &(current_dest_type)); 
    }
  }
 
  /* Add yourself to the trail list and increment the trail length. */
  struct GNUNET_PeerIdentity *peer_list;
  peer_list = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity) * (trail_length + 1));
  if ( trail_length > 0)
  {
    memcpy (peer_list, trail_peer_list, trail_length * sizeof (struct GNUNET_PeerIdentity));
  }
  memcpy (&peer_list[trail_length], &my_identity, sizeof (struct GNUNET_PeerIdentity));
  trail_length++;
  
  if (current_dest_type == MY_ID || 
     (0 == GNUNET_CRYPTO_cmp_peer_identity(next_hop, &(trail_setup->source_peer))))
  {
    struct GNUNET_PeerIdentity *source_peer;
    source_peer = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
    memcpy (source_peer, &(trail_setup->source_peer), sizeof (struct GNUNET_PeerIdentity));
    
    current_trail_index = trail_length - 1; 
    next_peer=  GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
    if (current_trail_index == 0)
    {
      memcpy (next_peer, &(trail_setup->source_peer), sizeof (struct GNUNET_PeerIdentity));
    }
    else
    {
      memcpy (next_peer, &trail_peer_list[current_trail_index-1], sizeof (struct GNUNET_PeerIdentity));
    }
     
    target_friend = GNUNET_CONTAINER_multipeermap_get (friend_peermap, next_peer);
    GNUNET_free (next_peer);
    
    if (0 == trail_setup->finger_map_index)
    {
      struct GNUNET_PeerIdentity *new_trail_list;
      new_trail_list = invert_trail_list (source_peer, peer_list, trail_length);
      finger_table_add (source_peer, new_trail_list, trail_length, 1);
    }
    
    GDS_NEIGHBOURS_send_trail_setup_result (&(trail_setup->source_peer),
                                            &(my_identity),
                                            target_friend, trail_length,
                                            peer_list, current_trail_index,
                                            finger_map_index);
  
    return GNUNET_YES;
  }
  else if (next_hop == NULL)
  {
    return GNUNET_SYSERR;
  }
  else 
  {
    target_friend = GNUNET_CONTAINER_multipeermap_get (friend_peermap, next_hop);
    if(current_dest_type == FINGER)
    {
    /* FIXME: Here it can even return err if routing entries have crossed the
     threshold. In such a case, you should send back a trail setup fail message
     to node just before your. THen its the responsiblity of node before you
     to find an alternate path to reach to the current_desintation which doesnot
     involve you.*/
      
      if ( GNUNET_SYSERR == GDS_ROUTING_add (&(trail_setup->source_peer), 
                                             &(trail_setup->current_destination),
                                             next_hop, peer))
      {
        //trail_setup_rejection()--> add to a list of fail trails and start 
        // a new search rejecting this peer. 
      }
    }
  
    GDS_NEIGHBOURS_send_trail_setup (&(trail_setup->source_peer),
                                     trail_setup->destination_finger,
                                     current_destination, target_friend, trail_length,
                                     peer_list, finger_map_index, current_dest_type);

  return GNUNET_YES;
  }
  return GNUNET_SYSERR;
}


/**
 * Core handle for p2p trail construction result messages.
 * @param cls closure
 * @param message message
 * @param peer peer identity this notification is about
 * @return GNUNET_OK on success, GNUNET_SYSERR on error
 */
static int
handle_dht_p2p_trail_setup_result(void *cls, const struct GNUNET_PeerIdentity *peer,
                                  const struct GNUNET_MessageHeader *message)
{
  struct PeerTrailSetupResultMessage *trail_result;
  struct GNUNET_PeerIdentity *trail_peer_list;
  struct GNUNET_PeerIdentity *next_hop;
  struct FriendInfo *target_friend;
  unsigned int current_trail_index;
  unsigned int finger_map_index;
  unsigned int trail_length;
  size_t msize;
  
  msize = ntohs (message->size);
  if (msize < sizeof (struct PeerTrailSetupResultMessage))
  {
    GNUNET_break_op (0);
    return GNUNET_YES;
  }
  
  trail_result = (struct PeerTrailSetupResultMessage *) message; 
  trail_length = ntohl (trail_result->trail_length); 
  
  if ((msize <
       sizeof (struct PeerTrailSetupResultMessage) +
       trail_length * sizeof (struct GNUNET_PeerIdentity)) ||
       (trail_length >
        GNUNET_SERVER_MAX_MESSAGE_SIZE / sizeof (struct GNUNET_PeerIdentity)))
  {
    GNUNET_break_op (0);
    return GNUNET_YES;
  }
  
  current_trail_index = ntohl (trail_result->current_index);
  finger_map_index = ntohl (trail_result->finger_map_index);

  trail_peer_list = (struct GNUNET_PeerIdentity *) &trail_result[1];
  
  if ( 0 == (GNUNET_CRYPTO_cmp_peer_identity (&(trail_result->destination_peer),
                                              &my_identity)))
  {
#if 0
    /* I don't remember why I have written this code. */
    if (finger_map_index == 1)
    {
      struct GNUNET_PeerIdentity *new_trail_list;
      new_trail_list = invert_trail_list (&(trail_result->finger_identity),
                                          trail_peer_list, trail_length);
      finger_table_add (&(trail_result->finger_identity), new_trail_list, trail_length, 
                        finger_map_index);
      return GNUNET_YES;
    }
    else 
    {
#endif
      finger_table_add (&(trail_result->finger_identity), trail_peer_list, trail_length, 
                       finger_map_index);
     
      return GNUNET_YES;
    //}
  }
  else
  {
    next_hop = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
    
    current_trail_index = current_trail_index - 1; 
    if (current_trail_index == 0)
    {
      memcpy (next_hop, &(trail_result->destination_peer),sizeof (struct GNUNET_PeerIdentity));
    }
    else
    {   
      memcpy (next_hop, &(trail_peer_list[current_trail_index-1]),sizeof (struct GNUNET_PeerIdentity));
    }
      
    target_friend = GNUNET_CONTAINER_multipeermap_get (friend_peermap, next_hop);
    GNUNET_free (next_hop);
      
    GDS_NEIGHBOURS_send_trail_setup_result (&(trail_result->destination_peer),
                                            &(trail_result->finger_identity),
                                            target_friend, trail_length,
                                            trail_peer_list,current_trail_index,
                                            finger_map_index);
      return GNUNET_YES;
    }
  
  return GNUNET_SYSERR;
}


/**
 * FIXME: Use flag in the case finger peer map does not contain predcessor
 * then its NULL. Ideally it should never happen. 
 * Get my current predecessor from the finger peer map
 * @return Current predecessor.
 */
static struct FingerInfo *
get_predecessor()
{
  struct GNUNET_CONTAINER_MultiPeerMapIterator *finger_iter;
  struct GNUNET_PeerIdentity key_ret;
  unsigned int finger_index;
  struct FingerInfo *my_predecessor;
 
  /* Iterate over finger peer map and extract your predecessor. */
  finger_iter = GNUNET_CONTAINER_multipeermap_iterator_create (finger_peermap);  
  for (finger_index = 0; finger_index < GNUNET_CONTAINER_multipeermap_size (finger_peermap); finger_index++)
  {
    if(GNUNET_YES == GNUNET_CONTAINER_multipeermap_iterator_next 
                       (finger_iter,&key_ret,(const void **)&my_predecessor)) 
    {
      if(1 == my_predecessor->finger_map_index)
      {
        break;
      }
    }
  }
  GNUNET_CONTAINER_multipeermap_iterator_destroy (finger_iter);
  return my_predecessor;
}


/**
 * Core handle for p2p verify successor messages.
 * @param cls closure
 * @param message message
 * @param peer peer identity this notification is about
 * @return GNUNET_OK on success, GNUNET_SYSERR on error
 */
static int
handle_dht_p2p_verify_successor(void *cls, const struct GNUNET_PeerIdentity *peer,
                                const struct GNUNET_MessageHeader *message)
{
  struct PeerVerifySuccessorMessage *vsm;
  struct GNUNET_PeerIdentity *trail_peer_list;
  struct FriendInfo *target_friend;
  struct GNUNET_PeerIdentity *next_hop;
  struct GNUNET_PeerIdentity *source_peer;
  unsigned int trail_length;
  unsigned int current_trail_index;
  size_t msize;
  
  msize = ntohs (message->size);
  if (msize < sizeof (struct PeerVerifySuccessorMessage))
  {
    GNUNET_break_op (0);
    return GNUNET_YES;
  }
  
  vsm = (struct PeerVerifySuccessorMessage *) message;
  trail_length = ntohl (vsm->trail_length);
  
  if ((msize < sizeof (struct PeerVerifySuccessorMessage) +
               trail_length * sizeof (struct GNUNET_PeerIdentity)) ||
     (trail_length > GNUNET_SERVER_MAX_MESSAGE_SIZE / sizeof (struct GNUNET_PeerIdentity)))
  {
    GNUNET_break_op (0);
    return GNUNET_YES;
  }
  
  current_trail_index = ntohl (vsm->current_trail_index);       
  trail_peer_list = (struct GNUNET_PeerIdentity *) &vsm[1];
  source_peer = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
  memcpy (source_peer, &(vsm->source_peer), sizeof (struct GNUNET_PeerIdentity));
  
  next_hop = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
  
  if(0 == (GNUNET_CRYPTO_cmp_peer_identity (&(vsm->successor),&my_identity)))
  {
    struct FingerInfo *my_predecessor;
    current_trail_index = trail_length - 1; 
    if (current_trail_index == 0)
      memcpy (next_hop, source_peer, sizeof (struct GNUNET_PeerIdentity));
    else
      memcpy (next_hop, &trail_peer_list[current_trail_index-1], sizeof (struct GNUNET_PeerIdentity));
    
    target_friend = GNUNET_CONTAINER_multipeermap_get (friend_peermap, next_hop);
    GNUNET_free (next_hop);
    
    my_predecessor = get_predecessor();
    if (0 == (GNUNET_CRYPTO_cmp_peer_identity (source_peer,
                                               &(my_predecessor->finger_identity))))
    {
      GDS_NEIGHBOURS_send_verify_successor_result (source_peer,
                                                   &(my_identity),
                                                   &(my_predecessor->finger_identity),
                                                   target_friend,
                                                   trail_peer_list,
                                                   trail_length,
                                                   current_trail_index);
    }
    else
    {
      struct GNUNET_PeerIdentity *new_successor_trail;
      int new_trail_length;
      int i;
      
      new_trail_length = trail_length + my_predecessor->trail_length;
      new_successor_trail = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity) * new_trail_length);
      memcpy (new_successor_trail, trail_peer_list, (trail_length) * sizeof (struct GNUNET_PeerIdentity));
      struct TrailPeerList *iterator;
      iterator = GNUNET_malloc (sizeof (struct TrailPeerList));
      iterator = my_predecessor->head; 
      i = trail_length;
      while (i < new_trail_length)
      {
        memcpy (&new_successor_trail[i], &(iterator->peer), sizeof (struct GNUNET_PeerIdentity));
        iterator = iterator->next;
        i++;
      }
 
      GDS_NEIGHBOURS_send_verify_successor_result (source_peer,
                                                   &(my_identity),
                                                   &(my_predecessor->finger_identity),
                                                   target_friend,
                                                   new_successor_trail,
                                                   new_trail_length,
                                                   current_trail_index); 
    }      
   
  }
  else
  {
    current_trail_index = current_trail_index + 1;
    memcpy (next_hop, &trail_peer_list[current_trail_index], sizeof (struct GNUNET_PeerIdentity));
    target_friend = GNUNET_CONTAINER_multipeermap_get (friend_peermap, next_hop);
    GNUNET_free (next_hop);
        
    GDS_NEIGHBOURS_send_verify_successor (source_peer, &(vsm->successor),target_friend,
                                          trail_peer_list, trail_length, current_trail_index); 
  }
  return GNUNET_YES;
}


/**
 * Core handle for p2p notify new successor messages.
 * @param cls closure
 * @param message message
 * @param peer peer identity this notification is about
 * @return GNUNET_OK on success, GNUNET_SYSERR on error
 */
static int
handle_dht_p2p_notify_new_successor(void *cls, const struct GNUNET_PeerIdentity *peer,
                                    const struct GNUNET_MessageHeader *message)
{
  struct PeerNotifyNewSuccessorMessage *nsm;
  struct GNUNET_PeerIdentity *trail_peer_list;
  unsigned int current_trail_index;
  size_t msize;
  unsigned int trail_length;
  
  msize = ntohs (message->size);
  if (msize < sizeof (struct PeerNotifyNewSuccessorMessage))
  {
    GNUNET_break_op (0);
    return GNUNET_YES;
  }
  
  nsm = (struct PeerNotifyNewSuccessorMessage *) message;
  trail_length = ntohl (nsm->trail_length);
  
  if ((msize < sizeof (struct PeerNotifyNewSuccessorMessage) +
               trail_length * sizeof (struct GNUNET_PeerIdentity)) ||
      (trail_length >
       GNUNET_SERVER_MAX_MESSAGE_SIZE / sizeof (struct GNUNET_PeerIdentity)))
  {
    GNUNET_break_op (0);
    return GNUNET_YES;
  }
  
  current_trail_index = ntohl (nsm->current_index);
  trail_peer_list = (struct GNUNET_PeerIdentity *) &nsm[1];
  
  if(0 == (GNUNET_CRYPTO_cmp_peer_identity (&(nsm->destination_peer), &my_identity)))
  {
    struct GNUNET_PeerIdentity *new_trail;
    new_trail = invert_trail_list (&(nsm->source_peer), trail_peer_list, trail_length);
    finger_table_add (&(nsm->source_peer), new_trail, trail_length, 1); 
    return GNUNET_YES;
  }
  else
  {
    struct FriendInfo *target_friend;
    struct GNUNET_PeerIdentity *next_hop;
    
    target_friend = GNUNET_malloc (sizeof (struct FriendInfo));
    next_hop = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
    
    current_trail_index = current_trail_index + 1;
    memcpy (next_hop, &trail_peer_list[current_trail_index], sizeof (struct GNUNET_PeerIdentity));
    target_friend = GNUNET_CONTAINER_multipeermap_get (friend_peermap, next_hop);
    GNUNET_free (next_hop);
    
    GDS_NEIGHBOURS_send_notify_new_successor (&(nsm->source_peer), 
                                              &(nsm->destination_peer),
                                              target_friend, trail_peer_list,
                                              trail_length, current_trail_index);
  }
  return GNUNET_YES;
}


/**
 * Core handle for p2p verify successor result messages.
 * @param cls closure
 * @param message message
 * @param peer peer identity this notification is about
 * @return GNUNET_OK on success, GNUNET_SYSERR on error
 */
static int
handle_dht_p2p_verify_successor_result(void *cls, const struct GNUNET_PeerIdentity *peer,
                                       const struct GNUNET_MessageHeader *message)
{
  struct PeerVerifySuccessorResultMessage *vsrm;
  struct FriendInfo *target_friend;
  struct GNUNET_PeerIdentity *trail_peer_list;
  struct GNUNET_PeerIdentity *next_hop;
  unsigned int trail_length;
  unsigned int current_trail_index;
  size_t msize;
  
  msize = ntohs (message->size);
  if (msize < sizeof (struct PeerVerifySuccessorResultMessage))
  {
    GNUNET_break_op (0);
    return GNUNET_YES;
  }
  
  vsrm = (struct PeerVerifySuccessorResultMessage *) message;
  trail_length = ntohl (vsrm->trail_length); 
  
  if ((msize <
       sizeof (struct PeerVerifySuccessorResultMessage) +
       trail_length * sizeof (struct GNUNET_PeerIdentity)) ||
       (trail_length >
       GNUNET_SERVER_MAX_MESSAGE_SIZE / sizeof (struct GNUNET_PeerIdentity)))
  {
    GNUNET_break_op (0);
    return GNUNET_YES;
  }
  
  current_trail_index = ntohl (vsrm->current_index);
  trail_peer_list = (struct GNUNET_PeerIdentity *) &vsrm[1];

  if(0 == (GNUNET_CRYPTO_cmp_peer_identity (&(vsrm->destination_peer), &(my_identity))))
  {
    if(0 != (GNUNET_CRYPTO_cmp_peer_identity (&(vsrm->my_predecessor), &(my_identity))))
    {
      finger_table_add (&(vsrm->my_predecessor), trail_peer_list, trail_length, 0);
      next_hop = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
      memcpy (next_hop, &trail_peer_list[0], sizeof (struct GNUNET_PeerIdentity));
      target_friend = GNUNET_CONTAINER_multipeermap_get (friend_peermap, next_hop);
      current_trail_index = 0;
      GNUNET_free (next_hop);
      
      GDS_NEIGHBOURS_send_notify_new_successor (&my_identity, &(vsrm->my_predecessor),
                                                target_friend, trail_peer_list,
                                                trail_length, current_trail_index);
    }
  }
  else
  {
    next_hop = GNUNET_malloc (sizeof (struct GNUNET_PeerIdentity));
    current_trail_index = current_trail_index - 1;
    if (current_trail_index == 0)
      memcpy (next_hop, &(vsrm->destination_peer), sizeof (struct GNUNET_PeerIdentity));
    else
      memcpy (next_hop, &trail_peer_list[current_trail_index-1], sizeof (struct GNUNET_PeerIdentity));
    target_friend = GNUNET_CONTAINER_multipeermap_get (friend_peermap, next_hop);
    GNUNET_free (next_hop);
    
    GDS_NEIGHBOURS_send_verify_successor_result (&(vsrm->destination_peer),
                                                 &(vsrm->source_successor),
                                                 &(vsrm->my_predecessor),
                                                 target_friend,
                                                 trail_peer_list,
                                                 trail_length,
                                                 current_trail_index); 
  }
  return GNUNET_YES;
}


/**
 * Initialize neighbours subsystem.
 * @return GNUNET_OK on success, GNUNET_SYSERR on error
 */
int
GDS_NEIGHBOURS_init()
{
  static struct GNUNET_CORE_MessageHandler core_handlers[] = {
    {&handle_dht_p2p_get, GNUNET_MESSAGE_TYPE_DHT_P2P_GET, 0},
    {&handle_dht_p2p_get_result, GNUNET_MESSAGE_TYPE_DHT_P2P_GET_RESULT, 0},
    {&handle_dht_p2p_put, GNUNET_MESSAGE_TYPE_DHT_P2P_PUT, 0},
    {&handle_dht_p2p_trail_setup, GNUNET_MESSAGE_TYPE_DHT_P2P_TRAIL_SETUP, 0},
    {&handle_dht_p2p_trail_setup_result, GNUNET_MESSAGE_TYPE_DHT_P2P_TRAIL_SETUP_RESULT, 0},
    {&handle_dht_p2p_verify_successor, GNUNET_MESSAGE_TYPE_DHT_P2P_VERIFY_SUCCESSOR, 0},
    {&handle_dht_p2p_verify_successor_result, GNUNET_MESSAGE_TYPE_DHT_P2P_VERIFY_SUCCESSOR_RESULT, 0},
    {&handle_dht_p2p_notify_new_successor, GNUNET_MESSAGE_TYPE_DHT_P2P_NOTIFY_NEW_SUCCESSOR, 0},
    {NULL, 0, 0}
  };
  
  core_api =
    GNUNET_CORE_connect (GDS_cfg, NULL, &core_init, &handle_core_connect,
                         &handle_core_disconnect, NULL, GNUNET_NO, NULL,
                         GNUNET_NO, core_handlers);
  if (NULL == core_api)
    return GNUNET_SYSERR;

  friend_peermap = GNUNET_CONTAINER_multipeermap_create (256, GNUNET_NO);
  finger_peermap = GNUNET_CONTAINER_multipeermap_create (MAX_FINGERS, GNUNET_NO); 
 
  return GNUNET_OK;
}


/**
 * Shutdown neighbours subsystem.
 */
void
GDS_NEIGHBOURS_done ()
{
  if (NULL == core_api)
    return;
  
  GNUNET_CORE_disconnect (core_api);
  core_api = NULL;
  
  GNUNET_assert (0 == GNUNET_CONTAINER_multipeermap_size (friend_peermap));
  GNUNET_CONTAINER_multipeermap_destroy (friend_peermap);
  friend_peermap = NULL;
  
  GNUNET_assert (0 == GNUNET_CONTAINER_multipeermap_size (finger_peermap));
  GNUNET_CONTAINER_multipeermap_destroy (finger_peermap);
  finger_peermap = NULL;

  if (GNUNET_SCHEDULER_NO_TASK != find_finger_trail_task)
  {
    GNUNET_SCHEDULER_cancel (find_finger_trail_task);
    find_finger_trail_task = GNUNET_SCHEDULER_NO_TASK;
  }
  
  if (GNUNET_SCHEDULER_NO_TASK != verify_successor)
  {
    GNUNET_SCHEDULER_cancel (verify_successor);
    verify_successor = GNUNET_SCHEDULER_NO_TASK;
  }
  
}


/**
 * Get the ID of the local node.
 *
 * @return identity of the local node
 */
struct GNUNET_PeerIdentity *
GDS_NEIGHBOURS_get_id ()
{
  return &my_identity;
}


/* end of gnunet-service-xdht_neighbours.c */
