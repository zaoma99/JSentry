package jsentry;


public interface sabi {

	public final static int SABI_ARCH_WORD_LENGHT = 64;

	// Sentry Mode Definition
	public final static int SABI_MODE_IDEL        = 0;		   //
	public final static int SABI_MODE_TRACE       = 0x01;	 // trace execution
	public final static int SABI_MODE_PREVENT     = 0x02;   // prevent threaten, violation, vulnerabilities, etc. meanwhile protect hosts
	public final static int SABI_MODE_OFG         = 0x04;	 // capture snapshot of "Operator Flow Graph"
	public final static int SABI_MODE_DFG         = 0x08;	 // capture snapshot of "Data Flow Graph"
	public final static int SABI_MODE_IAST        = 0x10;   // special scene of "Interact Application Safe Testing"
	public final static int SABI_MODE_SILENT      = 0x100;  // keep minimum functions, and do not send the BASE anything
	public final static int SABI_MODE_QUIET       = 0x200;  // keep journal quiet
	public final static int SABI_MODE_INDEPENDENT = 0x400;  // independent sentry, do all of jobs by self. e.g Vulnerabilities Analyzing
	public final static int SABI_MODE_DISTRIBUTED = 0x800;  // just means run in distributed environment, no specific functional scene.(reserved)
  public final static int SABI_MODE_ASPP        = SABI_MODE_PREVENT;
  public final static int	SABI_MODE_RASP        = SABI_MODE_PREVENT;
  public final static int	SABI_MODE_TEST        = SABI_MODE_IAST;

	// Sentry state
	public final static int SABI_STATE_UNKNOWN    = 0;
	public final static int SABI_STATE_PRIMORDIAL = 1;  // just born
	public final static int SABI_STATE_INIT       = 2;  // on initiating
	public final static int SABI_STATE_READY      = 3;  // ready to run
	public final static int SABI_STATE_RUN        = 4;  // on running
	public final static int SABI_STATE_PAUSE      = 5;  // paused by controller
	public final static int SABI_STATE_WAIT       = 6;  // waiting for something
	public final static int SABI_STATE_FAULT      = 7;  // fault occurred
	public final static int SABI_STATE_EXIT       = 8;  // on exiting
	public final static int SABI_STATE_DEAD       = 9;  // all jobs done

	// Sentry journal type/mask
	public final static int SABI_JOURNAL_DIS     = 0x00;    // disable all journal type
	public final static int SABI_JOURNAL_INFO	   = 0x01;    // information
	public final static int SABI_JOURNAL_WARN    = 0x02;    // warning
	public final static int SABI_JOURNAL_ERROR	 = 0x04;    // general error
	public final static int SABI_JOURNAL_DEBUG   = 0x08;    // debug information
	public final static int SABI_JOURNAL_EVENT   = 0x10;    // general event
	public final static int SABI_JOURNAL_CRIT    = 0x20;    // critical error
	public final static int SABI_JOURNAL_MT      = 0x40;    // maintenance information
	public final static int SABI_JOURNAL_ALL     = 0x7F;    // all

	// Sentry status bit mask
	public final static int SABI_STATUS_MASK_MODE     = 0xFFFF;
	public final static int SABI_STATUS_MASK_STATE    = 0xFF0000;
	public final static int SABI_STATUS_MASK_JOURTYPE = 0xFE000000;
	public final static int SABI_STATUS_MASK_READY    = 0x01000000;
	
	public final static int SABI_STATUS_SHIFT_MODE    = 0;
	public final static int SABI_STATUS_SHIFT_STATE   = 16;
	public final static int SABI_STATUS_SHIFT_JOURTYPE= 25;
	public final static int SABI_STATUS_SHIFT_READY   = 24;
	

	// Error code
	public final static int SABI_errFail				= -1;
	public final static int SABI_errOk					= 0;
	public final static int SABI_erInvVal				= 22;
	public final static int SABI_errInterrupt		= 4;
	public final static int SABI_errWouldblock	= 11;
	public final static int SABI_errAgain				= 11;
	public final static int SABI_errNoMemory		= 12;
	public final static int SABI_errNotEnough		= 7;
	public final static int SABI_errIO					= 5;
	public final static int SABI_errTimeout			= 110;
	public final static int SABI_errOverflow		= 75;
	public final static int SABI_errBusy				= 16;
	public final static int SABI_errOpenFile		= 50000;
	public final static int SABI_errNotExist		= 50002;
	public final static int SABI_errNotReady		= 50003;
	public final static int SABI_errDuplicate		= 50004;
	public final static int SABI_errInvFunc			= 50005;
	public final static int SABI_errInvVer			= 50006;
	public final static int SABI_errException		= 50007;
	public final static int SABI_errOpen				= 50008;
	public final static int SABI_errNull				= 50010;
	public final static int SABI_errInvExpr			= 50012;
	public final static int SABI_errInvProto		= 50013;
	public final static int SABI_errInvFormat		= 50014;
	public final static int SABI_errSema				= 50016;
	public final static int SABI_errInvCoding		= 50017;
	public final static int SABI_errInvData			= 50018;
	public final static int SABI_errOccupy			= 50019;
	public final static int SABI_errReject			= 50020;
	public final static int SABI_errUnsupport		= 50022;
	public final static int SABI_errCancel			= 50023;
	public final static int SABI_errNoCapacity	= 50024;
	public final static int SABI_errIllegalUser = 50025;
	public final static int SABI_errIgnored			= 50026;
	public final static int SABI_errUser				= 60000;
	public final static int SABI_errMode				= 60001;
	public final static int SABI_errState				= 60002;


	// predefined filenames
	public final String SABI_FILENAME_SENTRY_BOOT_LOG    = "sentry_boot_log";


	// Abstract Method
	//public int begin(long res, String libpath);
	//public void end();
	//public int setmode(int mode);
	//public int getmode();
	//public long getstatus();

}
