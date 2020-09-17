/******************************************************************************
* Copyright (c) 2018 - 2020 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

#include "xpm_common.h"
#include "xpm_pmcdomain.h"
#include "xpm_domain_iso.h"
#include "xpm_prot.h"
#include "xpm_debug.h"

static XStatus (*HandlePowerEvent)(XPm_Node *Node, u32 Event);

static XStatus HandlePmcDomainEvent(XPm_Node *Node, u32 Event)
{
	XStatus Status = XST_FAILURE;
	XPm_Power *Power = (XPm_Power *)Node;

	PmDbg("State=%d, Event=%d\n\r", Node->State, Event);

	switch (Node->State)
	{
		case (u8)XPM_POWER_STATE_ON:
			if ((u32)XPM_POWER_EVENT_PWR_UP == Event) {
				Status = XST_SUCCESS;
				Power->UseCount++;
			} else if ((u32)XPM_POWER_EVENT_PWR_DOWN == Event) {
				Status = XST_SUCCESS;
				Power->UseCount--;
			} else {
				/* Required by MISRA */
			}
			break;
		default:
			Status = XST_FAILURE;
			break;
	}

	return Status;
}

/****************************************************************************/
/**
 * @brief  This function configures xppu for PMC or PMC_NPI
 *
 * @return XST_SUCCESS if successful else XST_FAILURE
 *
 ****************************************************************************/
static XStatus PmcXppuCtrl(u32 *Args, u32 NumOfArgs)
{
	return XPmProt_CommonXppuCtrl(Args, NumOfArgs);
}

/****************************************************************************/
/**
 * @brief  This function configures xmpu for PMC (used for DDR memory)
 *
 * @return XST_SUCCESS if successful else XST_FAILURE
 *
 ****************************************************************************/
static XStatus PmcXmpuCtrl(u32 *Args, u32 NumOfArgs)
{
	return XPmProt_CommonXmpuCtrl(Args, NumOfArgs);
}

static struct XPm_PowerDomainOps PmcOps = {
	.XppuCtrl = PmcXppuCtrl,
	.XmpuCtrl = PmcXmpuCtrl,
};

XStatus XPmPmcDomain_Init(XPm_PmcDomain *PmcDomain, u32 Id, XPm_Power *Parent)
{
	XStatus Status = XST_FAILURE;
	u16 DbgErr = XPM_INT_ERR_UNDEFINED;

	Status = XPmPowerDomain_Init(&PmcDomain->Domain, Id, 0x00000000, Parent,
				     &PmcOps);
	if (XST_SUCCESS != Status) {
		PmErr("Status: 0x%x\r\n", Status);
		goto done;
	}

	PmcDomain->Domain.Power.Node.State = (u8)XPM_POWER_STATE_ON;
	PmcDomain->Domain.Power.UseCount = 1;

	HandlePowerEvent = PmcDomain->Domain.Power.HandleEvent;
	PmcDomain->Domain.Power.HandleEvent = HandlePmcDomainEvent;

	/* For all domain, rail stats are updated with init node finish. For
	pmc domain, init node commands are not received so update here */
	Status = XPmPower_UpdateRailStats(&PmcDomain->Domain,
					  (u8)XPM_POWER_STATE_ON);
	if (XST_SUCCESS != Status) {
		DbgErr = XPM_INT_ERR_PMC_RAIL_CONTROL;
	}

done:
	XPm_PrintDbgErr(Status, DbgErr);
	return Status;
}
