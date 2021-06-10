/******************************************************************************
 * Copyright 2021 ETC Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************
 * This file is a part of EtcPal. For more information, go to:
 * https://github.com/ETCLabs/EtcPal
 ******************************************************************************/

#include "etcpal/acn_rlp.h"

#include <string.h>
#include "etcpal/pack.h"

#define ACN_RLP_HEADER_SIZE 16u
#define RLP_VECTOR_SIZE 4u

#define RLP_EXTENDED_LENGTH(inheritvec, inherithead, data_len) \
  ((data_len + (inheritvec ? 0 : RLP_VECTOR_SIZE) + (inherithead ? 0 : ACN_RLP_HEADER_SIZE)) > 4095)

#define PROT_MANDATES_L_FLAG(vector)                                                                            \
  ((vector == ACN_VECTOR_ROOT_LLRP) || (vector == ACN_VECTOR_ROOT_BROKER) || (vector == ACN_VECTOR_ROOT_RPT) || \
   (vector == ACN_VECTOR_ROOT_EPT))

// Make sure to use memcmp, not strcmp, because of the extra nulls
#define ACN_PACKET_IDENT "ASC-E1.17\0\0\0"
#define ACN_PACKET_IDENT_SIZE 12

/**
 * @brief Parse an ACN TCP Preamble.
 *
 * Determine whether this byte buffer, received over TCP, starts with an ACN TCP Preamble and thus
 * contains an ACN protocol family packet. For best results, wait until you receive at least
 * #ACN_TCP_PREAMBLE_SIZE bytes before calling this function.
 *
 * @param[in]   buf       Received TCP buffer.
 * @param[in]   buflen    Size in bytes of buf.
 * @param[out]  preamble  Struct to contain parsed preamble information.
 * @return true (preamble was parsed successfully) or false (buffer too short or buffer does not
 *         contain a valid TCP preamble).
 */
bool acn_parse_tcp_preamble(const uint8_t* buf, size_t buflen, AcnTcpPreamble* preamble)
{
  if (!preamble || !buf || (buflen < ACN_TCP_PREAMBLE_SIZE))
    return false;

  if (memcmp(buf, ACN_PACKET_IDENT, ACN_PACKET_IDENT_SIZE) == 0)
  {
    preamble->rlp_block = buf + ACN_TCP_PREAMBLE_SIZE;
    preamble->rlp_block_len = etcpal_unpack_u32b(buf + ACN_PACKET_IDENT_SIZE);
    return true;
  }
  return false;
}

/**
 * @brief Parse an ACN UDP Preamble.
 *
 * Determine whether this received UDP datagram contains an ACN protocol family packet by
 * determining whether it starts with the ACN UDP preamble.
 *
 * @param[in] buf Received UDP buffer.
 * @param[in] buflen Size in bytes of buf.
 * @param[out] preamble Struct to contain parsed preamble information.
 * @return true (preamble was parsed successfully) or false (buffer too short or buffer does not
 *         contain a valid UDP preamble).
 */
bool acn_parse_udp_preamble(const uint8_t* buf, size_t buflen, AcnUdpPreamble* preamble)
{
  if (!preamble || !buf || (buflen < ACN_UDP_PREAMBLE_SIZE))
    return false;

  const uint8_t* pcur = buf;
  uint16_t       preamble_len = etcpal_unpack_u16b(pcur);
  pcur += 2;
  uint16_t postamble_len = etcpal_unpack_u16b(pcur);
  pcur += 2;
  if ((preamble_len == ACN_UDP_PREAMBLE_SIZE) && (memcmp(pcur, ACN_PACKET_IDENT, ACN_PACKET_IDENT_SIZE) == 0) &&
      (buflen > (unsigned)(preamble_len + postamble_len)))
  {
    preamble->rlp_block = buf + ACN_UDP_PREAMBLE_SIZE;
    preamble->rlp_block_len = buflen - preamble_len - postamble_len;
    return true;
  }
  return false;
}

/**
 * @brief Parse only the header of an ACN Root Layer PDU.
 *
 * This can be useful when working with stream sockets.
 *
 * @param[in] buf Root Layer Protocol buffer.
 * @param[in] buflen Size in bytes of buf.
 * @param[in] pdu Struct to contain parsed PDU header.
 * @param[in] last_pdu The last PDU structure that was parsed by this function, or NULL for the
 *                     first PDU in the block.
 * @return true (PDU was parsed successfully) or false (parse error or no more PDUs in the block).
 */
bool acn_parse_root_layer_header(const uint8_t* buf, size_t buflen, AcnRootLayerPdu* pdu, AcnRootLayerPdu* last_pdu)
{
  if (!buf || !pdu)
    return false;

  uint8_t flags_byte = *buf;
  bool    extlength = ACN_PDU_L_FLAG_SET(flags_byte);
  bool    inheritvect = !ACN_PDU_V_FLAG_SET(flags_byte);
  bool    inherithead = !ACN_PDU_H_FLAG_SET(flags_byte);
  bool    inheritdata = !ACN_PDU_D_FLAG_SET(flags_byte);

  const uint8_t* cur_ptr = buf;
  const uint8_t* buf_end = buf + buflen;
  if (cur_ptr + (extlength ? 3 : 2) >= buf_end)
  {
    // Not even enough room for the length?? Get outta here.
    return false;
  }

  uint32_t pdu_len = ACN_PDU_LENGTH(buf);
  uint32_t min_pdu_len = (uint32_t)((extlength ? 3 : 2) + (inheritvect ? 0 : 4) + (inherithead ? 0 : 16));
  if (((inheritvect || inherithead || inheritdata) && !last_pdu) || (pdu_len < min_pdu_len))
  {
    return false;
  }

  cur_ptr += extlength ? 3 : 2;
  if (inheritvect)
  {
    pdu->vector = last_pdu->vector;
  }
  else
  {
    pdu->vector = etcpal_unpack_u32b(cur_ptr);
    cur_ptr += 4;
    if (PROT_MANDATES_L_FLAG(pdu->vector) && !extlength)
      return false;
  }

  if (inherithead)
  {
    pdu->sender_cid = last_pdu->sender_cid;
  }
  else
  {
    memcpy(pdu->sender_cid.data, cur_ptr, ETCPAL_UUID_BYTES);
    cur_ptr += ETCPAL_UUID_BYTES;
  }

  if (inheritdata)
  {
    pdu->pdata = last_pdu->pdata;
    pdu->data_len = last_pdu->data_len;
  }
  else
  {
    pdu->pdata = cur_ptr;
    pdu->data_len = (size_t)(pdu_len - (size_t)(cur_ptr - buf));
  }

  return true;
}

/**
 * @brief Parse an ACN Root Layer PDU.
 *
 * Parse a Root Layer PDU from a Root Layer PDU block. Fills in the AcnRootLayerPdu structure with
 * the parsed PDU information, and stores state data in a AcnPdu structure for parsing the next
 * PDU in the block, if there is one.
 *
 * Example usage:
 *
 * After parsing an ACN protocol family preamble (perhaps by using acn_parse_udp_preamble() or
 * acn_parse_tcp_preamble()), buf points to the data starting after the preamble and buflen is the
 * length parsed from the preamble...
 *
 * @code
 * AcnPdu pdu = ACN_PDU_INIT; // Must initialize this!!
 * bool res = false;
 * do
 * {
 *   AcnRootLayerPdu cur;
 *   res = acn_parse_root_layer_pdu(buf, buflen, &cur, &pdu);
 *   if (res)
 *   {
 *     // Example application handler function that forwards the pdu appropriately based on
 *     // vector
 *     handle_root_layer_pdu(&cur);
 *   }
 * } while (res);
 * @endcode
 *
 * @param[in] buf Root Layer Protocol buffer.
 * @param[in] buflen Size in bytes of buf.
 * @param[out] pdu Struct to contain parsed PDU.
 * @param[in,out] last_pdu State data for future calls.
 * @return true (PDU was parsed successfully) or false (parse error or no more PDUs in the block).
 */
bool acn_parse_root_layer_pdu(const uint8_t* buf, size_t buflen, AcnRootLayerPdu* pdu, AcnPdu* last_pdu)
{
  if (!buf || !pdu || !last_pdu)
    return false;

  AcnPduConstraints rlp_constraints = {
      4,  // vector size
      16  // header size
  };

  if (acn_parse_pdu(buf, buflen, &rlp_constraints, last_pdu))
  {
    pdu->vector = etcpal_unpack_u32b(last_pdu->pvector);
    memcpy(pdu->sender_cid.data, last_pdu->pheader, ETCPAL_UUID_BYTES);
    pdu->pdata = last_pdu->pdata;
    pdu->data_len = last_pdu->data_len;
    return true;
  }
  return false;
}

/**
 * @brief Pack an ACN UDP Preamble into a buffer.
 *
 * All ACN protocol family packets sent over UDP must start with a UDP Preamble.
 *
 * @param[out] buf Buffer into which to pack the preamble.
 * @param[in] buflen Size in bytes of buf. Buffer should be at least of length
 *                   #ACN_UDP_PREAMBLE_SIZE.
 * @return Number of bytes packed (success) or 0 (failure).
 */
size_t acn_pack_udp_preamble(uint8_t* buf, size_t buflen)
{
  if (!buf || buflen < ACN_UDP_PREAMBLE_SIZE)
    return 0;

  uint8_t* cur_ptr = buf;
  etcpal_pack_u16b(cur_ptr, ACN_UDP_PREAMBLE_SIZE);
  cur_ptr += 2;
  etcpal_pack_u16b(cur_ptr, 0);
  cur_ptr += 2;
  memcpy(cur_ptr, ACN_PACKET_IDENT, ACN_PACKET_IDENT_SIZE);
  cur_ptr += ACN_PACKET_IDENT_SIZE;
  return (size_t)(cur_ptr - buf);
}

/**
 * @brief Pack an ACN TCP Preamble into a buffer.
 *
 * All ACN protocol family packets sent over TCP must start with a TCP Preamble.
 *
 * @param[out] buf Buffer into which to pack the preamble.
 * @param[in] buflen Size in bytes of buf. Buffer should be at least of length
 *                   #ACN_TCP_PREAMBLE_SIZE.
 * @param[in] rlp_block_len Full length in bytes of the Root Layer PDU block that will come after
 *                          this TCP preamble.
 * @return Number of bytes packed (success) or 0 (failure).
 */
size_t acn_pack_tcp_preamble(uint8_t* buf, size_t buflen, size_t rlp_block_len)
{
  if (!buf || buflen < ACN_TCP_PREAMBLE_SIZE)
    return 0;

  uint8_t* cur_ptr = buf;
  memcpy(cur_ptr, ACN_PACKET_IDENT, ACN_PACKET_IDENT_SIZE);
  cur_ptr += ACN_PACKET_IDENT_SIZE;
  etcpal_pack_u32b(cur_ptr, (uint32_t)rlp_block_len);
  cur_ptr += 4;
  return (size_t)(cur_ptr - buf);
}

/**
 * @brief Get the buffer size to allocate for a Root Layer PDU block.
 *
 * Calculate the buffer size to allocate for a future call to acn_pack_root_layer_block(), given
 * an array of AcnRootLayerPdu.
 *
 * @param[in] pdu_block Array of structs representing a Root Layer PDU block.
 * @param[in] num_pdus Size of the pdu_block array.
 * @return Buffer size to allocate.
 */
size_t acn_root_layer_buf_size(const AcnRootLayerPdu* pdu_block, size_t num_pdus)
{
  size_t                 block_size = 0;

  for (const AcnRootLayerPdu* pdu = pdu_block; pdu < pdu_block + num_pdus; ++pdu)
  {
    block_size += pdu->data_len;
    // We assume no inheritance here, so the largest possible buffer is allocated.
    block_size += (PROT_MANDATES_L_FLAG(pdu->vector) || RLP_EXTENDED_LENGTH(false, false, pdu->data_len))
                      ? ACN_RLP_HEADER_SIZE_EXT_LEN
                      : ACN_RLP_HEADER_SIZE_NORMAL_LEN;
  }
  return block_size;
}

/**
 * @brief Pack only the header of a Root Layer PDU block into a buffer.
 *
 * This can be useful when working with stream sockets.
 *
 * @param[out] buf Buffer into which to pack the Root Layer PDU header.
 * @param[in] buflen Size in bytes of buf. Buffer should be at least of size
 *                   #ACN_RLP_HEADER_SIZE_EXT_LEN.
 * @param[in] pdu PDU for which to pack the header into a buffer.
 * @return Number of bytes packed (success) or 0 (failure).
 */
size_t acn_pack_root_layer_header(uint8_t* buf, size_t buflen, const AcnRootLayerPdu* pdu)
{
  if (!buf || !pdu || buflen < ACN_RLP_HEADER_SIZE_EXT_LEN)
    return 0;

  uint8_t* cur_ptr = buf;
  *cur_ptr = 0x70;

  if (PROT_MANDATES_L_FLAG(pdu->vector) || RLP_EXTENDED_LENGTH(false, false, pdu->data_len))
  {
    ACN_PDU_SET_L_FLAG(*cur_ptr);
    ACN_PDU_PACK_EXT_LEN(cur_ptr, ACN_RLP_HEADER_SIZE_EXT_LEN + pdu->data_len);
    cur_ptr += 3;
  }
  else
  {
    ACN_PDU_PACK_NORMAL_LEN(cur_ptr, ACN_RLP_HEADER_SIZE_NORMAL_LEN + pdu->data_len);
    cur_ptr += 2;
  }

  etcpal_pack_u32b(cur_ptr, pdu->vector);
  cur_ptr += 4;
  memcpy(cur_ptr, pdu->sender_cid.data, ETCPAL_UUID_BYTES);
  cur_ptr += ETCPAL_UUID_BYTES;
  return (size_t)(cur_ptr - buf);
}

/**
 * @brief Pack a Root Layer PDU block into a buffer.
 *
 * The required buffer size can be calculated in advance using acn_root_layer_buf_size().
 *
 * @param[out] buf Buffer into which to pack the Root Layer PDU block.
 * @param[in] buflen Size in bytes of buf.
 * @param[in] pdu_block Array of AcnRootLayerPdu representing the PDU block to pack.
 * @param[in] num_pdus Number of AcnRootLayerPdu that make up the pdu_block array.
 * @return Number of bytes packed (success) or 0 (failure). Note that this might be less than the
 *         value returned by acn_root_layer_buf_size(); this is because this function performs
 *         more optimizations related to PDU inheritance.
 */
size_t acn_pack_root_layer_block(uint8_t* buf, size_t buflen, const AcnRootLayerPdu* pdu_block, size_t num_pdus)
{
  if (!buf || !pdu_block || (acn_root_layer_buf_size(pdu_block, num_pdus)) > buflen)
    return 0;

  uint8_t*        cur_ptr = buf;
  AcnRootLayerPdu last_pdu = {{{0}}, 0, NULL, 0};
  for (const AcnRootLayerPdu* pdu = pdu_block; pdu < pdu_block + num_pdus; ++pdu)
  {
    bool inheritvec = true;
    bool inherithead = true;
    bool inheritdata = true;
    // Start with no flags set
    *cur_ptr = 0;
    if (pdu == pdu_block)
    {
      // First PDU in the block - no inheritance
      ACN_PDU_SET_V_FLAG(*cur_ptr);
      inheritvec = false;
      last_pdu.vector = pdu->vector;

      ACN_PDU_SET_H_FLAG(*cur_ptr);
      inherithead = false;
      last_pdu.sender_cid = pdu->sender_cid;

      ACN_PDU_SET_D_FLAG(*cur_ptr);
      inheritdata = false;
      last_pdu.pdata = pdu->pdata;
      last_pdu.data_len = pdu->data_len;
    }
    else
    {
      // Check if we can use some inheritance
      if (pdu->vector != last_pdu.vector)
      {
        ACN_PDU_SET_V_FLAG(*cur_ptr);
        inheritvec = false;
        last_pdu.vector = pdu->vector;
      }

      if (0 != ETCPAL_UUID_CMP(&pdu->sender_cid, &last_pdu.sender_cid))
      {
        ACN_PDU_SET_H_FLAG(*cur_ptr);
        inherithead = false;
        last_pdu.sender_cid = pdu->sender_cid;
      }

      if ((pdu->data_len != last_pdu.data_len) || (0 != memcmp(pdu->pdata, last_pdu.pdata, last_pdu.data_len)))
      {
        ACN_PDU_SET_D_FLAG(*cur_ptr);
        inheritdata = false;
        last_pdu.pdata = pdu->pdata;
        last_pdu.data_len = pdu->data_len;
      }
    }

    // Check if we are required to use the 3-byte length field, either by the higher-level protocol
    // or because the length is greater than 4096
    if (PROT_MANDATES_L_FLAG(pdu->vector) ||
        RLP_EXTENDED_LENGTH(inheritvec, inherithead, inheritdata ? 0 : pdu->data_len))
    {
      size_t len = 3u + (inheritvec ? 0u : RLP_VECTOR_SIZE) + (inherithead ? 0u : ACN_RLP_HEADER_SIZE) +
                   (inheritdata ? 0u : pdu->data_len);
      ACN_PDU_SET_L_FLAG(*cur_ptr);
      ACN_PDU_PACK_EXT_LEN(cur_ptr, len);
      cur_ptr += 3;
    }
    else
    {
      size_t len = 2 + (inheritvec ? 0u : RLP_VECTOR_SIZE) + (inherithead ? 0u : ACN_RLP_HEADER_SIZE) +
                   (inheritdata ? 0u : pdu->data_len);
      ACN_PDU_PACK_NORMAL_LEN(cur_ptr, len);
      cur_ptr += 2;
    }

    if (!inheritvec)
    {
      etcpal_pack_u32b(cur_ptr, pdu->vector);
      cur_ptr += 4;
    }

    if (!inherithead)
    {
      memcpy(cur_ptr, pdu->sender_cid.data, ETCPAL_UUID_BYTES);
      cur_ptr += ETCPAL_UUID_BYTES;
    }

    if (!inheritdata)
    {
      memcpy(cur_ptr, pdu->pdata, pdu->data_len);
      cur_ptr += pdu->data_len;
    }
  }
  return (size_t)(cur_ptr - buf);
}
