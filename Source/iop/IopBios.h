#ifndef _IOPBIOS_H_
#define _IOPBIOS_H_

#include <list>
#include "../MIPSAssembler.h"
#include "../MIPS.h"
#include "../ELF.h"
#include "../OsStructManager.h"
#include "Iop_BiosBase.h"
#include "Iop_SifMan.h"
#include "Iop_SifCmd.h"
#include "Iop_Ioman.h"
#include "Iop_Cdvdman.h"
#include "Iop_Stdio.h"
#include "Iop_Sysmem.h"
#include "Iop_Modload.h"
#include "Iop_Dynamic.h"
#ifdef _IOP_EMULATE_MODULES
#include "Iop_DbcMan.h"
#include "Iop_PadMan.h"
#include "Iop_Cdvdfsv.h"
#endif

class CIopBios : public Iop::CBiosBase
{
public:
	enum CONTROL_BLOCK
	{
		CONTROL_BLOCK_START	= 0x10,
		CONTROL_BLOCK_END	= 0x10000,
	};

	struct THREADCONTEXT
	{
		uint32		gpr[0x20];
		uint32		epc;
		uint32		delayJump;
	};

	struct THREAD
	{
		uint32			isValid;
		uint32			id;
		uint32			priority;
		THREADCONTEXT	context;
		uint32			status;
		uint32			waitSemaphore;
		uint32			waitEventFlag;
		uint32			waitEventFlagMode;
		uint32			waitEventFlagMask;
		uint32			waitEventFlagResultPtr;
		uint32			wakeupCount;
		uint32			stackBase;
		uint32			stackSize;
		uint32			nextThreadId;
		uint64			nextActivateTime;
	};

	enum THREAD_STATUS
	{
		THREAD_STATUS_CREATED			= 1,
		THREAD_STATUS_RUNNING			= 2,
		THREAD_STATUS_SLEEPING			= 3,
		THREAD_STATUS_ZOMBIE			= 4,
		THREAD_STATUS_WAITING_SEMAPHORE = 5,
		THREAD_STATUS_WAITING_EVENTFLAG = 6,
		THREAD_STATUS_WAIT_VBLANK_START = 7,
		THREAD_STATUS_WAIT_VBLANK_END	= 8,
	};

								CIopBios(CMIPS&, uint8*, uint32);
	virtual						~CIopBios();

	void						LoadAndStartModule(const char*, const char*, unsigned int);
	void						LoadAndStartModule(uint32, const char*, unsigned int);
	void						HandleException();
	void						HandleInterrupt();

	void						Reschedule();

	void						CountTicks(uint32);
	uint64						GetCurrentTime();
	uint64						MilliSecToClock(uint32);
	uint64						MicroSecToClock(uint32);
	uint64						ClockToMicroSec(uint64);

	void						NotifyVBlankStart();
	void						NotifyVBlankEnd();

	void						Reset(Iop::CSifMan*);

	virtual void				SaveState(Framework::CZipArchiveWriter&);
	virtual void				LoadState(Framework::CZipArchiveReader&);

	bool						IsIdle();

#ifdef DEBUGGER_INCLUDED
	void						LoadDebugTags(Framework::Xml::CNode*);
	void						SaveDebugTags(Framework::Xml::CNode*);
#endif

	BiosDebugModuleInfoArray	GetModuleInfos() const;
	BiosDebugThreadInfoArray	GetThreadInfos() const;

	Iop::CIoman*				GetIoman();
	Iop::CCdvdman*				GetCdvdman();
#ifdef _IOP_EMULATE_MODULES
	Iop::CDbcMan*				GetDbcman();
	Iop::CPadMan*				GetPadman();
	Iop::CCdvdfsv*				GetCdvdfsv();
#endif
	void						RegisterDynamicModule(Iop::CDynamic*);

	uint32						CreateThread(uint32, uint32, uint32);
	void						StartThread(uint32, uint32* = NULL);
	void						DeleteThread(uint32);
	void						DelayThread(uint32);
	THREAD*						GetThread(uint32);
	uint32						GetCurrentThreadId() const;
	void						ChangeThreadPriority(uint32, uint32);
	void						SleepThread();
	uint32						WakeupThread(uint32, bool);

	void						SleepThreadTillVBlankStart();
	void						SleepThreadTillVBlankEnd();

	uint32						CreateSemaphore(uint32, uint32);
	uint32						DeleteSemaphore(uint32);
	uint32						SignalSemaphore(uint32, bool);
	uint32						WaitSemaphore(uint32);

	uint32						CreateEventFlag(uint32, uint32, uint32);
	uint32						SetEventFlag(uint32, uint32, bool);
	uint32						ClearEventFlag(uint32, uint32);
	uint32						WaitEventFlag(uint32, uint32, uint32, uint32);
	uint32						ReferEventFlagStatus(uint32, uint32);

	bool						RegisterIntrHandler(uint32, uint32, uint32, uint32);
	bool						ReleaseIntrHandler(uint32);

private:
	enum DEFAULT_STACKSIZE
	{
		DEFAULT_STACKSIZE = 0x4000,
	};

	enum DEFAULT_PRIORITY
	{
		DEFAULT_PRIORITY = 64,
	};

	enum
	{
		MAX_THREAD = 64,
		MAX_SEMAPHORE = 64,
		MAX_EVENTFLAG = 64,
		MAX_INTRHANDLER = 32,
	};

	enum WEF_FLAGS
	{
		WEF_AND		= 0x00,
		WEF_OR		= 0x01,
		WEF_CLEAR	= 0x10,
	};

	struct SEMAPHORE
	{
		uint32			isValid;
		uint32			id;
		uint32			count;
		uint32			maxCount;
		uint32			waitCount;
	};

	struct EVENTFLAG
	{
		uint32			isValid;
		uint32			id;
		uint32			attributes;
		uint32			options;
		uint32			value;
	};

	struct EVENTFLAGINFO
	{
		uint32			attributes;
		uint32			options;
		uint32			initBits;
		uint32			currBits;
		uint32			numThreads;
	};

	struct INTRHANDLER
	{
		uint32			isValid;
		uint32			line;
		uint32			mode;
		uint32			handler;
		uint32			arg;
	};

	struct IOPMOD
	{
		uint32			module;
		uint32			startAddress;
		uint32			gp;
		uint32			textSectionSize;
		uint32			dataSectionSize;
		uint32			bssSectionSize;
		uint16			moduleStructAttr;
		char			moduleName[256];
	};

	enum
	{
		IOPMOD_SECTION_ID = 0x70000080,
	};

	typedef COsStructManager<THREAD> ThreadList;
	typedef COsStructManager<SEMAPHORE> SemaphoreList;
	typedef COsStructManager<EVENTFLAG> EventFlagList;
	typedef COsStructManager<INTRHANDLER> IntrHandlerList;
	typedef std::map<std::string, Iop::CModule*> IopModuleMapType;
	typedef std::list<Iop::CDynamic*> DynamicIopModuleListType;
	typedef std::pair<uint32, uint32> ExecutableRange;

	void							RegisterModule(Iop::CModule*);
	void							ClearDynamicModules();

	void							ExitCurrentThread();
	void							LoadThreadContext(uint32);
	void							SaveThreadContext(uint32);
	uint32							GetNextReadyThread();
	void							ReturnFromException();

	uint32							FindIntrHandler(uint32);

	void							LinkThread(uint32);
	void							UnlinkThread(uint32);

	uint32&							ThreadLinkHead() const;
	uint32&							CurrentThreadId() const;
	uint64&							CurrentTime() const;

	void							LoadAndStartModule(CELF&, const char*, const char*, unsigned int);
	uint32							LoadExecutable(CELF&, ExecutableRange&);
	unsigned int					GetElfProgramToLoad(CELF&);
	void							RelocateElf(CELF&, uint32);
	std::string						ReadModuleName(uint32);
	std::string						GetModuleNameFromPath(const std::string&);
	BiosDebugModuleInfoIterator		FindModule(uint32, uint32);
#ifdef DEBUGGER_INCLUDED
	void							LoadLoadedModules(Framework::Xml::CNode*);
	void							SaveLoadedModules(Framework::Xml::CNode*);
#endif
	void							DeleteModules();
	uint32							Push(uint32&, const uint8*, uint32);

	uint32							AssembleThreadFinish(CMIPSAssembler&);
	uint32							AssembleReturnFromException(CMIPSAssembler&);
	uint32							AssembleIdleFunction(CMIPSAssembler&);

	CMIPS&							m_cpu;
	uint8*							m_ram;
	uint32							m_ramSize;
	uint32							m_threadFinishAddress;
	uint32							m_returnFromExceptionAddress;
	uint32							m_idleFunctionAddress;

	bool							m_rescheduleNeeded;
	ThreadList						m_threads;
	SemaphoreList					m_semaphores;
	EventFlagList					m_eventFlags;
	IntrHandlerList					m_intrHandlers;
		
	IopModuleMapType				m_modules;
	DynamicIopModuleListType		m_dynamicModules;

	BiosDebugModuleInfoArray		m_moduleTags;
	Iop::CSifMan*					m_sifMan;
	Iop::CSifCmd*					m_sifCmd;
	Iop::CStdio*					m_stdio;
	Iop::CIoman*					m_ioman;
	Iop::CCdvdman*					m_cdvdman;
	Iop::CSysmem*					m_sysmem;
	Iop::CModload*					m_modload;
#ifdef _IOP_EMULATE_MODULES
	Iop::CDbcMan*					m_dbcman;
	Iop::CPadMan*					m_padman;
	Iop::CCdvdfsv*					m_cdvdfsv;
#endif
};

#endif
