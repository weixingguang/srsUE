/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2015 The srsUE Developers. See the
 * COPYRIGHT file at the top-level directory of this distribution.
 *
 * \section LICENSE
 *
 * This file is part of the srsUE library.
 *
 * srsUE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsUE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#ifndef RLC_H
#define RLC_H

#include <common/log.h>
#include "common/common.h"
#include "common/interfaces.h"
#include "upper/rlc_entity.h"

namespace srsue {

class rlc
    :public rlc_interface_mac
    ,public rlc_interface_pdcp
    ,public rlc_interface_rrc
{
public:
  rlc(srslte::log *rlc_log_);
  void init(pdcp_interface_rlc *pdcp_);

  // PDCP interface
  void write_sdu(uint32_t lcid, uint8_t *payload, uint32_t nof_bytes);

  // MAC interface
  uint32_t get_buffer_state(uint32_t lcid);
  int      read_pdu(uint32_t lcid, uint8_t *payload, uint32_t nof_bytes);
  void     write_pdu(uint32_t lcid, uint8_t *payload, uint32_t nof_bytes);
  void     write_pdu_bcch_bch(uint8_t *payload, uint32_t nof_bytes);
  void     write_pdu_bcch_dlsch(uint8_t *payload, uint32_t nof_bytes);

  // RRC interface
  void add_rlc(RLC_MODE_ENUM mode, uint32_t lcid, LIBLTE_RRC_RLC_CONFIG_STRUCT *cnfg=NULL);

private:
  srslte::log        *rlc_log;
  pdcp_interface_rlc *pdcp;
  rlc_entity          rlc_array[SRSUE_N_RADIO_BEARERS];

  bool valid_lcid(uint32_t lcid);
};

} // namespace srsue


#endif // RLC_H