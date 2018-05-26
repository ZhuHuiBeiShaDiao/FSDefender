#include "CFSDefender.h"

#include "FSDUtils.h"
#include "FSDCommon.h"
#include "FSDRegistrationInfo.h"
#include "stdio.h"

extern CFSDefender* g;

CFSDefender::CFSDefender()	
	: m_pFilter()
	, m_pPort()
	, m_pScanPath()
{}

CFSDefender::~CFSDefender()
{
}

NTSTATUS CFSDefender::Initialize(
	PDRIVER_OBJECT          pDriverObject
){
	NTSTATUS hr = S_OK;
	
	//
	//  Register with FltMgr to tell it our callback routines
	//

	hr = NewInstanceOf<CFilter>(&m_pFilter, pDriverObject, &FilterRegistration);
	RETURN_IF_FAILED(hr);

	hr = NewInstanceOf<CFSDCommunicationPort>(&m_pPort, g_wszFSDPortName, m_pFilter->Handle(), this, OnConnect, OnDisconnect, OnNewMessage);
	RETURN_IF_FAILED(hr);

	//
	//  Start filtering i/o
	//

	hr = m_pFilter->StartFiltering();
	RETURN_IF_FAILED(hr);

	return S_OK;
}

NTSTATUS CFSDefender::ConnectClient(PFLT_PORT pClientPort)
{
	TRACE(TL_INFO, "User connected. 0x%p\n", pClientPort);
	return S_OK;
}

void CFSDefender::DisconnectClient(PFLT_PORT pClientPort)
{
	TRACE(TL_INFO, "User disconnected. 0x%p\n", pClientPort);
	return;
}

NTSTATUS CFSDefender::HandleNewMessage(IN PVOID pvInputBuffer, IN ULONG uInputBufferLength, OUT PVOID pvOutputBuffer, IN ULONG uOutputBufferLength, OUT PULONG puReturnOutputBufferLength)
{
	UNREFERENCED_PARAMETER(uInputBufferLength);

	TRACE(TL_INFO, "NewMessageRecieved: %s\n", pvInputBuffer ? (CHAR*)pvInputBuffer : "Message is empty");

	sprintf_s((char*)pvOutputBuffer, (size_t)uOutputBufferLength, "Message successfully recieved");
	*puReturnOutputBufferLength = (ULONG)strnlen_s((char*)pvOutputBuffer, static_cast<size_t>(uOutputBufferLength)) + 1;

	return S_OK;
}

NTSTATUS
CFSDefender::FSDInstanceSetup(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
	_In_ DEVICE_TYPE VolumeDeviceType,
	_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
)
/*++

Routine Description:

This routine is called whenever a new instance is created on a volume. This
gives us a chance to decide if we need to attach to this volume or not.

If this routine is not defined in the registration structure, automatic
instances are always created.

Arguments:

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance and its associated volume.

Flags - Flags describing the reason for this attach request.

Return Value:

STATUS_SUCCESS - attach
STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(VolumeDeviceType);
	UNREFERENCED_PARAMETER(VolumeFilesystemType);

	PAGED_CODE();

	TRACE(TL_FUNCTION_ENTRY, "FSD!FSDInstanceSetup: Entered\n");

	return S_OK;
}

NTSTATUS
CFSDefender::FSDInstanceQueryTeardown(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
)
/*++

Routine Description:

This is called when an instance is being manually deleted by a
call to FltDetachVolume or FilterDetach thereby giving us a
chance to fail that detach request.

If this routine is not defined in the registration structure, explicit
detach requests via FltDetachVolume or FilterDetach will always be
failed.

Arguments:

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance and its associated volume.

Flags - Indicating where this detach request came from.

Return Value:

Returns the status of this operation.

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	TRACE(TL_FUNCTION_ENTRY, "FSD!FSDInstanceQueryTeardown: Entered\n");

	return S_OK;
}

VOID
CFSDefender::FSDInstanceTeardownStart(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
/*++

Routine Description:

This routine is called at the start of instance teardown.

Arguments:

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance and its associated volume.

Flags - Reason why this instance is being deleted.

Return Value:

None.

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	TRACE(TL_FUNCTION_ENTRY, "FSD!FSDInstanceTeardownStart: Entered\n");
}

VOID
CFSDefender::FSDInstanceTeardownComplete(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
/*++

Routine Description:

This routine is called at the end of instance teardown.

Arguments:

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance and its associated volume.

Flags - Reason why this instance is being deleted.

Return Value:

None.

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	TRACE(TL_FUNCTION_ENTRY, "FSD!FSDInstanceTeardownComplete: Entered\n");
}

NTSTATUS
CFSDefender::FSDUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
/*++

Routine Description:

This is the unload routine for this miniFilter driver. This is called
when the minifilter is about to be unloaded. We can fail this unload
request if this is not a mandatory unload indicated by the Flags
parameter.

Arguments:

Flags - Indicating if this is a mandatory unload.

Return Value:

Returns STATUS_SUCCESS.

--*/
{
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	TRACE(TL_FUNCTION_ENTRY, "FSD!FSDUnload: Entered\n");

	delete g;

	return S_OK;
}

ULONG_PTR CFSDefender::OperationStatusCtx = 1;

/*************************************************************************
MiniFilter callback routines.
*************************************************************************/
FLT_PREOP_CALLBACK_STATUS
CFSDefender::FSDPreOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
/*++

Routine Description:

This routine is a pre-operation dispatch routine for this miniFilter.

This is non-pageable because it could be called on the paging path

Arguments:

Data - Pointer to the filter callbackData that is passed to us.

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance, its associated volume and
file object.

CompletionContext - The context for the completion routine for this
operation.

Return Value:

The return value is the status of the operation.

--*/
{
	NTSTATUS hr;

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	TRACE(TL_FUNCTION_ENTRY, "FSD!FSDPreOperation: Entered\n");

	//
	//  See if this is an operation we would like the operation status
	//  for.  If so request it.
	//
	//  NOTE: most filters do NOT need to do this.  You only need to make
	//        this call if, for example, you need to know if the oplock was
	//        actually granted.
	//

	if (FSDDoRequestOperationStatus(Data)) {

		hr = FltRequestOperationStatusCallback(Data,
			FSDOperationStatusCallback,
			(PVOID)(++OperationStatusCtx));
		if (FAILED(hr)) {
			TRACE(TL_ERROR, "FSD!FSDPreOperation: FltRequestOperationStatusCallback Failed, status=%08x\n", hr);
		}
	}

	PFLT_FILE_NAME_INFORMATION lnameInfo;

	if (FltObjects->FileObject != NULL)
	{
		hr = FltGetFileNameInformation(Data, FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP, &lnameInfo);
		if (FAILED(hr) && hr != STATUS_FLT_NAME_CACHE_MISS)
		{
			TRACE(TL_ERROR, "FSD!FSDPreOperation: FltGetFileNameInformation Failed, status=%08x\n", hr);
			return FLT_PREOP_SUCCESS_WITH_CALLBACK;
		}

		if (NT_SUCCESS(hr) && PrintFileName(lnameInfo->Name))
		{
			CHAR szIrpCode[256] = {};
			PrintIrpCode(Data->Iopb->MajorFunction, Data->Iopb->MinorFunction, szIrpCode, sizeof(szIrpCode));

			TRACE(TL_VERBOSE, "PID: %u File: %.*ls %s\n", FltGetRequestorProcessId(Data), lnameInfo->Name.Length, lnameInfo->Name.Buffer, szIrpCode);
		}
	}

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

VOID
CFSDefender::FSDOperationStatusCallback(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
	_In_ NTSTATUS OperationStatus,
	_In_ PVOID RequesterContext
)
/*++

Routine Description:

This routine is called when the given operation returns from the call
to IoCallDriver.  This is useful for operations where STATUS_PENDING
means the operation was successfully queued.  This is useful for OpLocks
and directory change notification operations.

This callback is called in the context of the originating thread and will
never be called at DPC level.  The file object has been correctly
referenced so that you can access it.  It will be automatically
dereferenced upon return.

This is non-pageable because it could be called on the paging path

Arguments:

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance, its associated volume and
file object.

RequesterContext - The context for the completion routine for this
operation.

OperationStatus -

Return Value:

The return value is the status of the operation.

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);

	TRACE(TL_FUNCTION_ENTRY, "FSD!FSDOperationStatusCallback: Entered\n");

	TRACE(TL_VERBOSE, "FSD!FSDOperationStatusCallback: Status=%08x ctx=%p IrpMj=%02x.%02x \"%s\"\n",
		OperationStatus,
		RequesterContext,
		ParameterSnapshot->MajorFunction,
		ParameterSnapshot->MinorFunction,
		FltGetIrpName(ParameterSnapshot->MajorFunction));
}

FLT_POSTOP_CALLBACK_STATUS
CFSDefender::FSDPostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
)
/*++

Routine Description:

This routine is the post-operation completion routine for this
miniFilter.

This is non-pageable because it may be called at DPC level.

Arguments:

Data - Pointer to the filter callbackData that is passed to us.

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance, its associated volume and
file object.

CompletionContext - The completion context set in the pre-operation routine.

Flags - Denotes whether the completion is successful or is being drained.

Return Value:

The return value is the status of the operation.

--*/
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	TRACE(TL_FUNCTION_ENTRY, "FSD!FSDPostOperation: Entered\n");

	return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS
CFSDefender::FSDPreOperationNoPostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
/*++

Routine Description:

This routine is a pre-operation dispatch routine for this miniFilter.

This is non-pageable because it could be called on the paging path

Arguments:

Data - Pointer to the filter callbackData that is passed to us.

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance, its associated volume and
file object.

CompletionContext - The context for the completion routine for this
operation.

Return Value:

The return value is the status of the operation.

--*/
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	TRACE(TL_FUNCTION_ENTRY, "FSD!FSDPreOperationNoPostOperation: Entered\n");

	// This template code does not do anything with the callbackData, but
	// rather returns FLT_PREOP_SUCCESS_NO_CALLBACK.
	// This passes the request down to the next miniFilter in the chain.

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

BOOLEAN
CFSDefender::FSDDoRequestOperationStatus(
	_In_ PFLT_CALLBACK_DATA Data
)
/*++

Routine Description:

This identifies those operations we want the operation status for.  These
are typically operations that return STATUS_PENDING as a normal completion
status.

Arguments:

Return Value:

TRUE - If we want the operation status
FALSE - If we don't

--*/
{
	PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

	//
	//  return boolean state based on which operations we are interested in
	//

	return (BOOLEAN)

		//
		//  Check for oplock operations
		//

		(((iopb->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
		((iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK) ||
			(iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK) ||
			(iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1) ||
			(iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)))

			||

			//
			//    Check for directy change notification
			//

			((iopb->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
			(iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY))
			);
}
