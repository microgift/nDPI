/*
 * sip.c
 *
 * Copyright (C) 2009-11 - ipoque GmbH
 * Copyright (C) 2011-22 - ntop.org
 *
 * This file is part of nDPI, an open source deep packet inspection
 * library based on the OpenDPI and PACE technology by ipoque GmbH
 *
 * nDPI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * nDPI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with nDPI.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "ndpi_protocol_ids.h"

#define NDPI_CURRENT_PROTO NDPI_PROTOCOL_SIP

#include "ndpi_api.h"

static void ndpi_int_sip_add_connection(struct ndpi_detection_module_struct *ndpi_struct,
					struct ndpi_flow_struct *flow,
					u_int8_t due_to_correlation) {
  ndpi_set_detected_protocol(ndpi_struct, flow, NDPI_PROTOCOL_SIP, NDPI_PROTOCOL_UNKNOWN, NDPI_CONFIDENCE_DPI);
}

#if !defined(WIN32)
static inline
#elif defined(MINGW_GCC)
__mingw_forceinline static
#else
__forceinline static
#endif
void ndpi_search_sip_handshake(struct ndpi_detection_module_struct
			       *ndpi_struct, struct ndpi_flow_struct *flow)
{
  struct ndpi_packet_struct *packet = &ndpi_struct->packet;
  const u_int8_t *packet_payload = packet->payload;
  u_int32_t payload_len = packet->payload_packet_len;

  if(payload_len > 4) {
    /* search for STUN Turn ChannelData Prefix */
    u_int16_t message_len = ntohs(get_u_int16_t(packet->payload, 2));

    if(payload_len - 4 == message_len) {
      NDPI_LOG_DBG2(ndpi_struct, "found STUN TURN ChannelData prefix\n");
      payload_len -= 4;
      packet_payload += 4;
    }
  }

  if(payload_len >= 14) {
    if((memcmp(packet_payload, "NOTIFY ", 7) == 0 || memcmp(packet_payload, "notify ", 7) == 0)
       && (memcmp(&packet_payload[7], "SIP:", 4) == 0 || memcmp(&packet_payload[7], "sip:", 4) == 0)) {

      NDPI_LOG_INFO(ndpi_struct, "found sip NOTIFY\n");
      ndpi_int_sip_add_connection(ndpi_struct, flow, 0);
      return;
    }

    if((memcmp(packet_payload, "REGISTER ", 9) == 0 || memcmp(packet_payload, "register ", 9) == 0)
       && (memcmp(&packet_payload[9], "SIP:", 4) == 0 || memcmp(&packet_payload[9], "sip:", 4) == 0)) {

      NDPI_LOG_INFO(ndpi_struct, "found sip REGISTER\n");
      ndpi_int_sip_add_connection(ndpi_struct, flow, 0);
      return;
    }

    if((memcmp(packet_payload, "INVITE ", 7) == 0 || memcmp(packet_payload, "invite ", 7) == 0)
       && (memcmp(&packet_payload[7], "SIP:", 4) == 0 || memcmp(&packet_payload[7], "sip:", 4) == 0)) {
      NDPI_LOG_INFO(ndpi_struct, "found sip INVITE\n");
      ndpi_int_sip_add_connection(ndpi_struct, flow, 0);
      return;
    }

    /* seen this in second direction on the third position,
     * maybe it could be deleted, if somebody sees it in the first direction,
     * please delete this comment.
     */

    /*
      if(memcmp(packet_payload, "SIP/2.0 200 OK", 14) == 0 || memcmp(packet_payload, "sip/2.0 200 OK", 14) == 0) {
      NDPI_LOG_INFO(ndpi_struct, "found sip SIP/2.0 0K\n");
      ndpi_int_sip_add_connection(ndpi_struct, flow, 0);
      return;
      }
    */
    if(memcmp(packet_payload, "SIP/2.0 ", 8) == 0 || memcmp(packet_payload, "sip/2.0 ", 8) == 0) {
      NDPI_LOG_INFO(ndpi_struct, "found sip SIP/2.0 *\n");
      ndpi_int_sip_add_connection(ndpi_struct, flow, 0);
      return;
    }

    if((memcmp(packet_payload, "BYE ", 4) == 0 || memcmp(packet_payload, "bye ", 4) == 0)
       && (memcmp(&packet_payload[4], "SIP:", 4) == 0 || memcmp(&packet_payload[4], "sip:", 4) == 0)) {
      NDPI_LOG_INFO(ndpi_struct, "found sip BYE\n");
      ndpi_int_sip_add_connection(ndpi_struct, flow, 0);
      return;
    }

    if((memcmp(packet_payload, "ACK ", 4) == 0 || memcmp(packet_payload, "ack ", 4) == 0)
       && ((memcmp(&packet_payload[4], "SIP:", 4) == 0 || memcmp(&packet_payload[4], "sip:", 4) == 0) ||
           (memcmp(&packet_payload[4], "TEL:", 4) == 0 || memcmp(&packet_payload[4], "tel:", 4) == 0))) {
      NDPI_LOG_INFO(ndpi_struct, "found sip ACK\n");
      ndpi_int_sip_add_connection(ndpi_struct, flow, 0);
      return;
    }

    if((memcmp(packet_payload, "CANCEL ", 7) == 0 || memcmp(packet_payload, "cancel ", 7) == 0)
       && ((memcmp(&packet_payload[7], "SIP:", 4) == 0 || memcmp(&packet_payload[7], "sip:", 4) == 0) ||
           (memcmp(&packet_payload[7], "TEL:", 4) == 0 || memcmp(&packet_payload[7], "tel:", 4) == 0))) {
      NDPI_LOG_INFO(ndpi_struct, "found sip CANCEL\n");
      ndpi_int_sip_add_connection(ndpi_struct, flow, 0);
      return;
    }

    if((memcmp(packet_payload, "PUBLISH ", 8) == 0 || memcmp(packet_payload, "publish ", 8) == 0)
       && (memcmp(&packet_payload[8], "SIP:", 4) == 0 || memcmp(&packet_payload[8], "sip:", 4) == 0)) {
      NDPI_LOG_INFO(ndpi_struct, "found sip PUBLISH\n");
      ndpi_int_sip_add_connection(ndpi_struct, flow, 0);
      return;
    }

    if((memcmp(packet_payload, "SUBSCRIBE ", 10) == 0 || memcmp(packet_payload, "subscribe ", 10) == 0)
       && (memcmp(&packet_payload[10], "SIP:", 4) == 0 || memcmp(&packet_payload[10], "sip:", 4) == 0)) {
      NDPI_LOG_INFO(ndpi_struct, "found sip SUBSCRIBE\n");
      ndpi_int_sip_add_connection(ndpi_struct, flow, 0);
      return;
    }
        
    /* SIP message extension RFC 3248 */
    if((memcmp(packet_payload, "MESSAGE ", 8) == 0 || memcmp(packet_payload, "message ", 8) == 0)
       && (memcmp(&packet_payload[8], "SIP:", 4) == 0 || memcmp(&packet_payload[8], "sip:", 4) == 0)) {
      NDPI_LOG_INFO(ndpi_struct, "found sip MESSAGE\n");
      ndpi_int_sip_add_connection(ndpi_struct, flow, 0);
      return;
    }

    /* Courtesy of Miguel Quesada <mquesadab@gmail.com> */
    if((memcmp(packet_payload, "OPTIONS ", 8) == 0
	|| memcmp(packet_payload, "options ", 8) == 0)
       && ((memcmp(&packet_payload[8], "SIP:", 4) == 0 || memcmp(&packet_payload[8], "sip:", 4) == 0) ||
           (memcmp(&packet_payload[8], "TEL:", 4) == 0 || memcmp(&packet_payload[8], "tel:", 4) == 0))) {
      NDPI_LOG_INFO(ndpi_struct, "found sip OPTIONS\n");
      ndpi_int_sip_add_connection(ndpi_struct, flow, 0);
      return;
    }

    if((memcmp(packet_payload, "REFER ", 6) == 0 || memcmp(packet_payload, "refer ", 6) == 0)
       && (memcmp(&packet_payload[6], "SIP:", 4) == 0 || memcmp(&packet_payload[6], "sip:", 4) == 0)) {
      NDPI_LOG_INFO(ndpi_struct, "found sip REFER\n");
      ndpi_int_sip_add_connection(ndpi_struct, flow, 0);
      return;
    }

    if((memcmp(packet_payload, "PRACK ", 6) == 0 || memcmp(packet_payload, "prack ", 6) == 0)
       && (memcmp(&packet_payload[6], "SIP:", 4) == 0 || memcmp(&packet_payload[6], "sip:", 4) == 0)) {
      NDPI_LOG_INFO(ndpi_struct, "found sip PRACK\n");
      ndpi_int_sip_add_connection(ndpi_struct, flow, 0);
      return;
    }

    if((memcmp(packet_payload, "INFO ", 5) == 0 || memcmp(packet_payload, "info ", 5) == 0)
       && (memcmp(&packet_payload[5], "SIP:", 4) == 0 || memcmp(&packet_payload[5], "sip:", 4) == 0)) {
      NDPI_LOG_INFO(ndpi_struct, "found sip INFO\n");
      ndpi_int_sip_add_connection(ndpi_struct, flow, 0);
      return;
    }
  }

  /* add bitmask for tcp only, some stupid udp programs
   * send a very few (< 10 ) packets before invite (mostly a 0x0a0x0d, but just search the first 3 payload_packets here */
  if(packet->udp != NULL && flow->packet_counter < 10) {
    NDPI_LOG_DBG2(ndpi_struct, "need next packet\n");
    return;
  }

  if(payload_len == 4 && get_u_int32_t(packet_payload, 0) == 0) {
    NDPI_LOG_DBG2(ndpi_struct, "maybe sip. need next packet\n");
    return;
  }

  NDPI_EXCLUDE_PROTO(ndpi_struct, flow);
}

static void ndpi_search_sip(struct ndpi_detection_module_struct *ndpi_struct, struct ndpi_flow_struct *flow)
{
  NDPI_LOG_DBG(ndpi_struct, "search sip\n");

  ndpi_search_sip_handshake(ndpi_struct, flow);
}

void init_sip_dissector(struct ndpi_detection_module_struct *ndpi_struct, u_int32_t *id)
{
  ndpi_set_bitmask_protocol_detection("SIP", ndpi_struct, *id,
				      NDPI_PROTOCOL_SIP,
				      ndpi_search_sip,
				      NDPI_SELECTION_BITMASK_PROTOCOL_V4_V6_TCP_OR_UDP_WITH_PAYLOAD_WITHOUT_RETRANSMISSION,/* Fix courtesy of Miguel Quesada <mquesadab@gmail.com> */
				      SAVE_DETECTION_BITMASK_AS_UNKNOWN,
				      ADD_TO_DETECTION_BITMASK);

  *id += 1;
}

