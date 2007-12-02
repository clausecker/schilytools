/* @(#)scsi-mac-iokit.c	1.13 07/04/25 Copyright 1997,2001-2007 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-mac-iokit.c	1.13 07/04/25 Copyright 1997,2001-2007 J. Schilling";
#endif
/*
 *	Interface to the Darwin IOKit SCSI drivers
 *
 *	Notes: Uses the IOKit/scsi-commands/SCSITaskLib interface
 *
 *	As of October 2001, this interface does not support SCSI parallel bus
 *	(old-fashioned SCSI). It does support ATAPI, Firewire, and USB.
 *
 *	First version done by Constantine Sapuntzakis <csapuntz@Stanford.EDU>
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 *
 *	Copyright (c) 1997,2001-2007 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * The following exceptions apply:
 * CDDL §3.6 needs to be replaced by: "You may create a Larger Work by
 * combining Covered Software with other code if all other code is governed by
 * the terms of a license that is OSI approved (see www.opensource.org) and
 * you may distribute the Larger Work as a single product. In such a case,
 * You must make sure the requirements of this License are fulfilled for
 * the Covered Software."
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
LOCAL	char	_scg_trans_version[] = "scsi-mac-iokit.c-1.13";	/* The version for this transport */

#define	MAX_SCG		16	/* Max # of SCSI controllers */
#define	MAX_TGT		16
#define	MAX_LUN		8

#include <schily/stat.h>
#include <mach/mach.h>
#include <Carbon/Carbon.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/scsi-commands/SCSITaskLib.h>
#include <mach/mach_error.h>

struct scg_local {
	MMCDeviceInterface	**mmcDeviceInterface;
	SCSITaskDeviceInterface	**scsiTaskDeviceInterface;
	mach_port_t		masterPort;
};
#define	scglocal(p)	((struct scg_local *)((p)->local))

#define	MAX_DMA_NEXT	(32*1024)
#if 0
#define	MAX_DMA_NEXT	(64*1024)	/* Check if this is not too big */
#endif

/*
 * Return version information for the low level SCSI transport code.
 * This has been introduced to make it easier to trace down problems
 * in applications.
 */
LOCAL char *
scgo_version(scgp, what)
	SCSI	*scgp;
	int	what;
{
	if (scgp != (SCSI *)0) {
		switch (what) {

		case SCG_VERSION:
			return (_scg_trans_version);
		/*
		 * If you changed this source, you are not allowed to
		 * return "schily" for the SCG_AUTHOR request.
		 */
		case SCG_AUTHOR:
			return (_scg_auth_schily);
		case SCG_SCCS_ID:
			return (__sccsid);
		}
	}
	return ((char *)0);
}

LOCAL int
scgo_help(scgp, f)
	SCSI	*scgp;
	FILE	*f;
{
	__scg_help(f, "SCSITaskDeviceInterface", "Apple SCSI",
		"", "Mac Prom device name", "IOCompactDiscServices/0 or IODVDServices/0",
								FALSE, FALSE);
	return (0);
}


/*
 * Valid Device names:
 *    IOCompactDiscServices
 *    IODVDServices
 *    IOSCSIPeripheralDeviceNub
 *
 * Also a / and a number can be appended to refer to something
 * more than the first device (e.g. IOCompactDiscServices/5 for the 5th
 * compact disc attached)
 */
LOCAL int
scgo_open(scgp, device)
	SCSI	*scgp;
	char	*device;
{
	mach_port_t masterPort = 0;
	io_iterator_t scsiObjectIterator = 0;
	IOReturn ioReturnValue = kIOReturnSuccess;
	CFMutableDictionaryRef dict = NULL;
	io_object_t scsiDevice = 0;
	HRESULT plugInResult;
	IOCFPlugInInterface **plugInInterface = NULL;
	MMCDeviceInterface **mmcDeviceInterface = NULL;
	SCSITaskDeviceInterface **scsiTaskDeviceInterface = NULL;
	SInt32 score = 0;
	int err = -1;
	char *realdevice = NULL, *tmp;
	int driveidx = 1, idx = 1;

	if (device == NULL) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
		"Please specify a device name (e.g. IOCompactDiscServices/0)");
		goto out;
	}

	realdevice = tmp = strdup(device);
	tmp = strchr(tmp, '/');
	if (tmp != NULL) {
		*tmp++ = '\0';
		driveidx = atoi(tmp);
	}

	if (scgp->local == NULL) {
		scgp->local = malloc(sizeof (struct scg_local));
		if (scgp->local == NULL)
			goto out;
	}

	ioReturnValue = IOMasterPort(bootstrap_port, &masterPort);

	if (ioReturnValue != kIOReturnSuccess) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "Couldn't get a master IOKit port. Error %d",
			    ioReturnValue);
		goto out;
	}

	dict = IOServiceMatching(realdevice);
	if (dict == NULL) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "Couldn't create dictionary for searching");
		goto out;
	}

	ioReturnValue = IOServiceGetMatchingServices(masterPort, dict,
						    &scsiObjectIterator);
	dict = NULL;

	if (scsiObjectIterator == 0 ||
	    (ioReturnValue != kIOReturnSuccess)) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "No matching device %s found.", device);
		goto out;
	}

	if (driveidx <= 0)
		driveidx = 1;

	idx = 1;
	while ((scsiDevice = IOIteratorNext(scsiObjectIterator)) != 0) {
		if (idx == driveidx)
			break;
		IOObjectRelease(scsiDevice);
		scsiDevice = 0;
		idx++;
	}

	if (scsiDevice == 0) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "No matching device found. Iterator failed.");
		goto out;
	}

	ioReturnValue = IOCreatePlugInInterfaceForService(scsiDevice,
			kIOMMCDeviceUserClientTypeID,
			kIOCFPlugInInterfaceID,
			&plugInInterface, &score);
	if (ioReturnValue != kIOReturnSuccess) {
		goto try_generic;
	}

	plugInResult = (*plugInInterface)->QueryInterface(plugInInterface,
				CFUUIDGetUUIDBytes(kIOMMCDeviceInterfaceID),
				(LPVOID)&mmcDeviceInterface);

	if (plugInResult != KERN_SUCCESS) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "Unable to get MMC Interface: 0x%lX",
			    (long)plugInResult);

		goto out;
	}

	scsiTaskDeviceInterface =
		(*mmcDeviceInterface)->GetSCSITaskDeviceInterface(mmcDeviceInterface);

	if (scsiTaskDeviceInterface == NULL) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "Failed to get taskDeviceInterface");
		goto out;
	}

	goto init;

try_generic:
	ioReturnValue = IOCreatePlugInInterfaceForService(scsiDevice,
					kIOSCSITaskDeviceUserClientTypeID,
					kIOCFPlugInInterfaceID,
					&plugInInterface, &score);
	if (ioReturnValue != kIOReturnSuccess) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "Unable to get plugin Interface: %x",
			    ioReturnValue);
		goto out;
	}

	plugInResult = (*plugInInterface)->QueryInterface(plugInInterface,
			    CFUUIDGetUUIDBytes(kIOSCSITaskDeviceInterfaceID),
					(LPVOID)&scsiTaskDeviceInterface);

	if (plugInResult != KERN_SUCCESS) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "Unable to get generic Interface: 0x%lX",
			    (long)plugInResult);

		goto out;
	}

init:
	ioReturnValue =
		(*scsiTaskDeviceInterface)->ObtainExclusiveAccess(scsiTaskDeviceInterface);

	if (ioReturnValue != kIOReturnSuccess) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "Unable to get exclusive access to device");
		goto out;
	}

	if (mmcDeviceInterface) {
		(*mmcDeviceInterface)->AddRef(mmcDeviceInterface);
	}
	(*scsiTaskDeviceInterface)->AddRef(scsiTaskDeviceInterface);
	scglocal(scgp)->mmcDeviceInterface = mmcDeviceInterface;
	scglocal(scgp)->scsiTaskDeviceInterface = scsiTaskDeviceInterface;
	scglocal(scgp)->masterPort = masterPort;
	scg_settarget(scgp, 0, 0, 0);
	err = 1;

out:
	if (scsiTaskDeviceInterface != NULL) {
		(*scsiTaskDeviceInterface)->Release(scsiTaskDeviceInterface);
	}

	if (plugInInterface != NULL) {
		(*plugInInterface)->Release(plugInInterface);
	}

	if (scsiDevice != 0) {
		IOObjectRelease(scsiDevice);
	}

	if (scsiObjectIterator != 0) {
		IOObjectRelease(scsiObjectIterator);
	}

	if (err < 0) {
		if (scgp->local) {
			free(scgp->local);
			scgp->local = NULL;
		}

		if (masterPort) {
			mach_port_deallocate(mach_task_self(), masterPort);
		}
	}

	if (dict != NULL) {
		CFRelease(dict);
	}

	if (realdevice != NULL) {
		free(realdevice);
	}
	return (err);
}

LOCAL int
scgo_close(scgp)
	SCSI	*scgp;
{
	SCSITaskDeviceInterface	**sc;
	MMCDeviceInterface	**mmc;

	if (scgp->local == NULL)
		return (-1);

	sc = scglocal(scgp)->scsiTaskDeviceInterface;
	(*sc)->ReleaseExclusiveAccess(sc);
	(*sc)->Release(sc);
	scglocal(scgp)->scsiTaskDeviceInterface = NULL;

	mmc = scglocal(scgp)->mmcDeviceInterface;
	if (mmc != NULL)
		(*mmc)->Release(mmc);

	mach_port_deallocate(mach_task_self(), scglocal(scgp)->masterPort);

	free(scgp->local);
	scgp->local = NULL;

	return (0);
}

LOCAL long
scgo_maxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	long maxdma = MAX_DMA_NEXT;
#ifdef	SGIOCMAXDMA
	int  m;

	if (ioctl(scglocal(scgp)->scgfile, SGIOCMAXDMA, &m) >= 0) {
		maxdma = m;
		if (scgp->debug > 0) {
			js_fprintf((FILE *)scgp->errfile,
				"maxdma: %d\n", maxdma);
		}
	}
#endif
	return (maxdma);
}

LOCAL void *
scgo_getbuf(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile,
			"scgo_getbuf: %ld bytes\n", amt);
	}
	scgp->bufbase = malloc((size_t)(amt));
	return (scgp->bufbase);
}

LOCAL void
scgo_freebuf(scgp)
	SCSI	*scgp;
{
	if (scgp->bufbase)
		free(scgp->bufbase);
	scgp->bufbase = NULL;
}

LOCAL int
scgo_numbus(scgp)
	SCSI	*scgp;
{
	return (1);
}

LOCAL BOOL
scgo_havebus(scgp, busno)
	SCSI	*scgp;
	int	busno;
{
	if (busno == 0)
		return (TRUE);
	return (FALSE);
}

LOCAL int
scgo_fileno(scgp, busno, tgt, tlun)
	SCSI	*scgp;
	int	busno;
	int	tgt;
	int	tlun;
{
	return (-1);
}

LOCAL int
scgo_initiator_id(scgp)
	SCSI	*scgp;
{
	return (-1);
}

LOCAL int
scgo_isatapi(scgp)
	SCSI	*scgp;

{
	return (FALSE);
}

LOCAL int
scgo_reset(scgp, what)
	SCSI	*scgp;
	int	what;
{
	if (what == SCG_RESET_NOP)
		return (0);
	if (what != SCG_RESET_BUS) {
		errno = EINVAL;
		return (-1);
	}

	errno = 0;
	return (-1);
}

LOCAL int
scgo_send(scgp)
	SCSI		*scgp;
{
	struct scg_cmd		*sp = scgp->scmd;
	SCSITaskDeviceInterface	**sc = NULL;
	SCSITaskInterface	**cmd = NULL;
	IOVirtualRange		iov;
	SCSI_Sense_Data		senseData;
	SCSITaskStatus		status;
	UInt64			bytesTransferred;
	IOReturn		ioReturnValue;
	int			ret = 0;

	if (scgp->local == NULL) {
		return (-1);
	}

	sc = scglocal(scgp)->scsiTaskDeviceInterface;

	cmd = (*sc)->CreateSCSITask(sc);
	if (cmd == NULL) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "Failed to create SCSI task");
		ret = -1;

		sp->error = SCG_FATAL;
		sp->ux_errno = EIO;
		goto out;
	}


	iov.address = (IOVirtualAddress) sp->addr;
	iov.length = sp->size;

	ioReturnValue = (*cmd)->SetCommandDescriptorBlock(cmd,
						sp->cdb.cmd_cdb, sp->cdb_len);

	if (ioReturnValue != kIOReturnSuccess) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "SetCommandDescriptorBlock failed with status %x",
			    ioReturnValue);
		ret = -1;
		goto out;
	}

	ioReturnValue = (*cmd)->SetScatterGatherEntries(cmd, &iov, 1, sp->size,
				(sp->flags & SCG_RECV_DATA) ?
				kSCSIDataTransfer_FromTargetToInitiator :
				kSCSIDataTransfer_FromInitiatorToTarget);
	if (ioReturnValue != kIOReturnSuccess) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "SetScatterGatherEntries failed with status %x",
			    ioReturnValue);
		ret = -1;
		goto out;
	}

	ioReturnValue = (*cmd)->SetTimeoutDuration(cmd, sp->timeout * 1000);
	if (ioReturnValue != kIOReturnSuccess) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "SetTimeoutDuration failed with status %x",
			    ioReturnValue);
		ret = -1;
		goto out;
	}

	memset(&senseData, 0, sizeof (senseData));

	seterrno(0);
	ioReturnValue = (*cmd)->ExecuteTaskSync(cmd,
				&senseData, &status, &bytesTransferred);

	sp->resid = sp->size - bytesTransferred;
	sp->error = SCG_NO_ERROR;
	sp->ux_errno = geterrno();

	if (ioReturnValue != kIOReturnSuccess) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "Command execution failed with status %x",
			    ioReturnValue);
		sp->error = SCG_RETRYABLE;
		ret = -1;
		goto out;
	}

	memset(&sp->scb, 0, sizeof (sp->scb));
	memset(&sp->u_sense.cmd_sense, 0, sizeof (sp->u_sense.cmd_sense));
	if (senseData.VALID_RESPONSE_CODE != 0 || status == 0x02) {
		/*
		 * There is no sense length - we need to asume that
		 * we always get 18 bytes.
		 */
		sp->sense_count = kSenseDefaultSize;
		memmove(&sp->u_sense.cmd_sense, &senseData, kSenseDefaultSize);
		if (sp->ux_errno == 0)
			sp->ux_errno = EIO;
	}

	sp->u_scb.cmd_scb[0] = status;

	/* ??? */
	if (status == kSCSITaskStatus_No_Status) {
		sp->error = SCG_RETRYABLE;
		ret = -1;
		goto out;
	}
	/*
	 * XXX Is it possible to have other senseful SCSI transport error codes?
	 */

out:
	if (cmd != NULL) {
		(*cmd)->Release(cmd);
	}

	return (ret);
}
