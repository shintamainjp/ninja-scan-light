/*
 * Copyright (c) 2013, M.Naruoka (fenrir)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice, 
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 * - Neither the name of the naruoka.org nor the names of its contributors 
 *   may be used to endorse or promote products derived from this software 
 *   without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#ifndef _MSD_H_
#define _MSD_H_

#include "type.h"

typedef struct {
  DWORD_t dCBWSignature;
  DWORD_t dCBWTag;
  DWORD_t dCBWDataTransferLength;
  BYTE  bmCBWFlags;
  BYTE  bCBWLUN;
  BYTE  bCBWCBLength;
  BYTE  CBWCB[16];
} msd_cbw_t;

typedef struct {
  DWORD_t dCSWSignature;
  DWORD_t dCSWTag;
  DWORD_t dCSWDataResidue;
  BYTE  bCSWStatus;
} msd_csw_t;

extern msd_cbw_t __xdata msd_cbw;
extern msd_csw_t __xdata msd_csw;

#define MSD_EP_IN  3
#define MSD_EP_OUT 3

#define CONCAT2_(a,b) (a ## b)
#define CONCAT2(a,b) CONCAT2_(a, b) 

#define MSD_EP_IN_PACKET_SIZE  (CONCAT2(PACKET_SIZE_EP, MSD_EP_IN))
#define MSD_EP_OUT_PACKET_SIZE  (CONCAT2(PACKET_SIZE_EP, MSD_EP_OUT))

#define MSD_RESET           0xFF    // Mass-storage device Reset
#define MSD_GET_MAX_LUN     0xFE    // Mass-storage device Get Max LUN

void msd_polling();

void usb_MSD_init();
void usb_MSD_req();

extern u8 __xdata msd_action;
#define MSD_HOST_NO_DATA      0x01
#define MSD_HOST_TX           0x02
#define MSD_HOST_RX           0x04
#define MSD_DEVICE_NO_DATA    0x10
#define MSD_DEVICE_RX         0x20
#define MSD_DEVICE_TX         0x40

#define MSD_HOST_SIDE(action) (action & 0x0F)
#define MSD_DEVICE_SIDE(action) (action & 0xF0)

#endif
