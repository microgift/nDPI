/*
 * mining.c [Bitcoin, Ethereum, ZCash, Monero]
 *
 * Copyright (C) 2018-22 - ntop.org
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
#define NDPI_CURRENT_PROTO NDPI_PROTOCOL_MINING
#include "ndpi_api.h"


/* ************************************************************************** */

u_int32_t make_mining_key(struct ndpi_flow_struct *flow) {
  u_int32_t key;

  /* network byte order */
  if(flow->is_ipv6)
    key = ndpi_quick_hash(flow->c_address.v6, 16) + ndpi_quick_hash(flow->s_address.v6, 16);
  else
    key = flow->c_address.v4 + flow->s_address.v4;

  return key;
}

/* ************************************************************************** */

static void cacheMiningHostTwins(struct ndpi_detection_module_struct *ndpi_struct,
				 struct ndpi_flow_struct *flow) {
  if(ndpi_struct->mining_cache)
    ndpi_lru_add_to_cache(ndpi_struct->mining_cache, make_mining_key(flow), NDPI_PROTOCOL_MINING, ndpi_get_current_time(flow));
}

/* ************************************************************************** */

static void ndpi_search_mining_udp(struct ndpi_detection_module_struct *ndpi_struct,
				   struct ndpi_flow_struct *flow) {
  struct ndpi_packet_struct *packet = &ndpi_struct->packet;
  u_int16_t source = ntohs(packet->udp->source);
  u_int16_t dest = ntohs(packet->udp->dest);

  NDPI_LOG_DBG(ndpi_struct, "search MINING UDP\n");

  // printf("==> %s()\n", __FUNCTION__);
  /* 
     Ethereum P2P Discovery Protocol
     https://github.com/ConsenSys/ethereum-dissectors/blob/master/packet-ethereum-disc.c
  */
  if((packet->payload_packet_len > 98)
     && (packet->payload_packet_len < 1280)
     && ((source == 30303) || (dest == 30303))
     && (packet->payload[97] <= 0x04 /* NODES */)
     ) {
    if((packet->iph) && ((ntohl(packet->iph->daddr) & 0xFF000000 /* 255.0.0.0 */) == 0xFF000000))
      ;
    else if(packet->iphv6 && ntohl(packet->iphv6->ip6_dst.u6_addr.u6_addr32[0]) == 0xFF020000)
      ;
    else {
      ndpi_snprintf(flow->flow_extra_info, sizeof(flow->flow_extra_info), "%s", "ETH");
      ndpi_set_detected_protocol(ndpi_struct, flow, NDPI_PROTOCOL_MINING, NDPI_PROTOCOL_UNKNOWN, NDPI_CONFIDENCE_DPI);
      cacheMiningHostTwins(ndpi_struct, flow);
      return;
    }
  }
  
  NDPI_EXCLUDE_PROTO(ndpi_struct, flow);
}

/* ************************************************************************** */

static u_int8_t isEthPort(u_int16_t dport) {
  return(((dport >= 30300) && (dport <= 30305)) ? 1 : 0);
}

/* ************************************************************************** */

static void ndpi_search_mining_tcp(struct ndpi_detection_module_struct *ndpi_struct,
				   struct ndpi_flow_struct *flow) {
  struct ndpi_packet_struct *packet = &ndpi_struct->packet;

  NDPI_LOG_DBG(ndpi_struct, "search MINING TCP\n");

  /* Check connection over TCP */
  if(packet->payload_packet_len > 10) {
    if((packet->payload_packet_len > 300)
       && (packet->payload_packet_len < 600)
       && (packet->payload[2] == 0x04)) {
      if(isEthPort(ntohs(packet->tcp->dest)) /* Ethereum port */) {
       ndpi_snprintf(flow->flow_extra_info, sizeof(flow->flow_extra_info), "%s", "ETH");
	ndpi_set_detected_protocol(ndpi_struct, flow, NDPI_PROTOCOL_MINING, NDPI_PROTOCOL_UNKNOWN, NDPI_CONFIDENCE_DPI);
	cacheMiningHostTwins(ndpi_struct, flow);
	return;
      } 
    } else if(ndpi_strnstr((const char *)packet->payload, "{", packet->payload_packet_len)
	 && (
	   ndpi_strnstr((const char *)packet->payload, "\"eth1.0\"", packet->payload_packet_len)
	   || ndpi_strnstr((const char *)packet->payload, "\"worker\":", packet->payload_packet_len)
	   /* || ndpi_strnstr((const char *)packet->payload, "\"id\":", packet->payload_packet_len) - Removed as too generic */
	   )) {
      /*
	Ethereum
	
	{"worker": "eth1.0", "jsonrpc": "2.0", "params": ["0x0fccfff9e61a230ff380530c6827caf4759337c6.rig2", "x"], "id": 2, "method": "eth_submitLogin"}
	{ "id": 2, "jsonrpc":"2.0","result":true}
	{"worker": "", "jsonrpc": "2.0", "params": [], "id": 3, "method": "eth_getWork"}
      */
      ndpi_snprintf(flow->flow_extra_info, sizeof(flow->flow_extra_info), "%s", "ETH");
      ndpi_set_detected_protocol(ndpi_struct, flow, NDPI_PROTOCOL_MINING, NDPI_PROTOCOL_UNKNOWN, NDPI_CONFIDENCE_DPI);
      cacheMiningHostTwins(ndpi_struct, flow);
      return;
    } else if(ndpi_strnstr((const char *)packet->payload, "{", packet->payload_packet_len)
	      && (ndpi_strnstr((const char *)packet->payload, "\"method\":", packet->payload_packet_len)
		  || ndpi_strnstr((const char *)packet->payload, "\"blob\":", packet->payload_packet_len)
		  /* || ndpi_strnstr((const char *)packet->payload, "\"id\":", packet->payload_packet_len) - Removed as too generic */
		)
      ) {
      /*
	ZCash

	{"method":"login","params":{"login":"4BCeEPhodgPMbPWFN1dPwhWXdRX8q4mhhdZdA1dtSMLTLCEYvAj9QXjXAfF7CugEbmfBhgkqHbdgK9b2wKA6nqRZQCgvCDm.cb2b73415c4faf214035a73b9d947c202342f3bf3bdf632132bd6d7af98cb257.ryzen","pass":"x","agent":"xmr-stak-cpu/1.3.0-1.5.0"},"id":1}
	{"id":1,"jsonrpc":"2.0","error":null,"result":{"id":"479059546883218","job":{"blob":"0606e89883d205a65d8ee78991838a1cf3ec2ebbc5fb1fa43dec5fa1cd2bee4069212a549cd731000000005a88235653097aa3e97ef2ceef4aee610751a828f9be1a0758a78365fb0a4c8c05","job_id":"722134174127131","target":"dc460300"},"status":"OK"}}
	{"method":"submit","params":{"id":"479059546883218","job_id":"722134174127131","nonce":"98024001","result":"c9be9381a68d533c059d614d961e0534d7d8785dd5c339c2f9596eb95f320100"},"id":1}

	Monero
	
	{"method":"login","params":{"login":"4BCeEPhodgPMbPWFN1dPwhWXdRX8q4mhhdZdA1dtSMLTLCEYvAj9QXjXAfF7CugEbmfBhgkqHbdgK9b2wKA6nqRZQCgvCDm.cb2b73415c4faf214035a73b9d947c202342f3bf3bdf632132bd6d7af98cb257.ryzen","pass":"x","agent":"xmr-stak-cpu/1.3.0-1.5.0"},"id":1}
	{"id":1,"jsonrpc":"2.0","error":null,"result":{"id":"479059546883218","job":{"blob":"0606e89883d205a65d8ee78991838a1cf3ec2ebbc5fb1fa43dec5fa1cd2bee4069212a549cd731000000005a88235653097aa3e97ef2ceef4aee610751a828f9be1a0758a78365fb0a4c8c05","job_id":"722134174127131","target":"dc460300"},"status":"OK"}}
	{"method":"submit","params":{"id":"479059546883218","job_id":"722134174127131","nonce":"98024001","result":"c9be9381a68d533c059d614d961e0534d7d8785dd5c339c2f9596eb95f320100"},"id":1}
      */
      ndpi_snprintf(flow->flow_extra_info, sizeof(flow->flow_extra_info), "%s", "ZCash/Monero");
      ndpi_set_detected_protocol(ndpi_struct, flow, NDPI_PROTOCOL_MINING, NDPI_PROTOCOL_UNKNOWN, NDPI_CONFIDENCE_DPI);
      cacheMiningHostTwins(ndpi_struct, flow);
      return;
    }
  }

  NDPI_EXCLUDE_PROTO(ndpi_struct, flow);
}

/* ************************************************************************** */

static void ndpi_search_mining(struct ndpi_detection_module_struct *ndpi_struct,
			       struct ndpi_flow_struct *flow) {
  struct ndpi_packet_struct *packet = &ndpi_struct->packet;

  if(packet->tcp)
    return ndpi_search_mining_tcp(ndpi_struct, flow);
  return ndpi_search_mining_udp(ndpi_struct, flow);
}


/* ************************************************************************** */

void init_mining_dissector(struct ndpi_detection_module_struct *ndpi_struct,
			   u_int32_t *id)
{
  ndpi_set_bitmask_protocol_detection("Mining", ndpi_struct, *id,
				      NDPI_PROTOCOL_MINING,
				      ndpi_search_mining,
				      NDPI_SELECTION_BITMASK_PROTOCOL_V4_V6_TCP_OR_UDP_WITH_PAYLOAD_WITHOUT_RETRANSMISSION,
				      SAVE_DETECTION_BITMASK_AS_UNKNOWN,
				      ADD_TO_DETECTION_BITMASK);

  *id += 1;
}

