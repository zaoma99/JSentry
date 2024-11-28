package jsentry;

import jsentry.sabi;

//import java.io.Serializable;


public class jvas_vtrace {

  // Data type
  public final static int VT_TYPE_NULL       = 0  ;    //  
  public final static int VT_TYPE_SI8        = 1  ;    // signed integer within 1-byte
  public final static int VT_TYPE_SI16       = 2  ;    // signed integer within 2-byte
  public final static int VT_TYPE_SI32       = 3  ;    // signed integer within 4-byte
  public final static int VT_TYPE_SI64       = 4  ;    // signed integer within 8-byte
  public final static int VT_TYPE_SI128      = 5  ;    // signed integer within 16-byte(reserved)
  public final static int VT_TYPE_UI8        = 6  ;    // unsigned integer within 1-byte
  public final static int VT_TYPE_UI16       = 7  ;    // unsigned integer within 2-byte
  public final static int VT_TYPE_UI32       = 8  ;    // unsigned integer within 4-byte
  public final static int VT_TYPE_UI64       = 9  ;    // unsigned integer within 8-byte
  public final static int VT_TYPE_UI128      = 10 ;    // unsigned integer within 16-byte(reserved)
  public final static int VT_TYPE_F32        = 11 ;    // IEEE-754 binary32 floating-point format
  public final static int VT_TYPE_F64        = 12 ;    // IEEE-754 binary64 floating-point format
  public final static int VT_TYPE_F128       = 13 ;    // reserved
  public final static int VT_TYPE_POINTER    = 14 ;    // same as the void*, pointer of address space
  public final static int VT_TYPE_TIME       = 15 ;    // same as the time_t
  public final static int VT_TYPE_CLOCK      = 16 ;    // same as the struct timespec
  public final static int VT_TYPE_IDENTIFIER = 17 ;  // object identifier
  public final static int VT_TYPE_CSTRING    = 18 ;    // visible character string(UTF-8 encoding)
  public final static int VT_TYPE_BSTRING    = 19 ;    // byte string
  public final static int VT_TYPE_CLASS      = 29 ;
  public final static int VT_TYPE_ARRAY      = 30 ;
  public final static int VT_TYPE_BOOL       = VT_TYPE_UI8  ;
  public final static int VT_TYPE_BYTE       = VT_TYPE_UI8  ;
  public final static int VT_TYPE_OCTET      = VT_TYPE_UI8  ;
  public final static int VT_TYPE_CHAR       = VT_TYPE_SI8  ;
  public final static int VT_TYPE_WCHAR      = VT_TYPE_UI16 ;    // wide character(UTF-16 encoding)
  public final static int VT_TYPE_WORD       = VT_TYPE_UI16 ;
  public final static int VT_TYPE_SHORT      = VT_TYPE_SI16 ;
  public final static int VT_TYPE_INT        = VT_TYPE_SI32 ;
  public final static int VT_TYPE_UINT       = VT_TYPE_UI32 ;
  public final static int VT_TYPE_DWORD      = VT_TYPE_UI32 ;
  public final static int VT_TYPE_LONG       = VT_TYPE_SI64 ;
  public final static int VT_TYPE_ULONG      = VT_TYPE_UI64 ;
  public final static int VT_TYPE_QWORD      = VT_TYPE_UI64 ;
  public final static int VT_TYPE_FLOAT      = VT_TYPE_F32  ;
  public final static int VT_TYPE_DOUBLE     = VT_TYPE_F64  ;
  public final static int VT_TYPE_ENUM       = VT_TYPE_INT  ;
  public final static int VT_TYPE_OFFSET     = VT_TYPE_LONG ;
  public final static int VT_TYPE_SSIZE      = VT_TYPE_LONG ;
  public final static int VT_TYPE_SIZE       = VT_TYPE_ULONG;
  //public final static int VT_TYPE_OFFSET     = VT_TYPE_INT ;
  //public final static int VT_TYPE_SSIZE      = VT_TYPE_INT ;
  //public final static int VT_TYPE_SIZE       = VT_TYPE_UINT;

  // triggered reason(according to vt_condition)
  public final static int VT_CAUSE_NULL             = 0;
  public final static int	VT_CAUSE_DATA_PROP        = 0x01;
  public final static int	VT_CAUSE_DATA_TRANS       = 0x02;
  public final static int	VT_CAUSE_SAME_ACTION      = 0x04;
  public final static int	VT_CAUSE_SAME_EVENT       = 0x08;
  public final static int	VT_CAUSE_CRIT_POINT       = 0x10;
  public final static int	VT_CAUSE_CRIT_RES         = 0x20;
  public final static int	VT_CAUSE_SPEC_TRAP        = 0x40;
  public final static int	VT_CAUSE_CONSISTENCY      = 0x80;
  public final static int	VT_CAUSE_INTEGRITY        = 0x100;
  public final static int VT_CAUSE_DATA_VERIFY      = 0x200;
  public final static	int VT_CAUSE_AUTHORIZE        = 0x400;
  public final static	int VT_CAUSE_AUTHENTICATE     = 0x800;
  public final static int VT_CAUSE_CREDENTIALS      = 0x1000;
  public final static	int VT_CAUSE_PERMIT           = 0x2000;
  public final static	int VT_CAUSE_PROG_EXECUTE     = 0x4000;
  public final static	int VT_CAUSE_PROG_ABNORMAL    = 0x8000;
  public final static int VT_CAUSE_TRACE_START      = 0x10000;
  public final static int VT_CAUSE_TRACE_DONE       = 0x20000;
  public final static int VT_CAUSE_TRACE_BREAK      = 0x40000;

  // triggered stage
  public final static int VT_STAGE_NULL			= 0;
  public final static	int VT_STAGE_ENTER		= 1;  // enter into trace point
  public final static	int VT_STAGE_LEAVE		= 2;  // return from trace point
  public final static	int VT_STAGE_CATCH		= 4;  // a caught exception
  public final static	int VT_STAGE_TRAP			= 8;	// anywhere trap(reserved)
  public final static int VT_STAGE_BEGIN		= 16;	// start to tracing
  public final static int VT_STAGE_END		  = 32;	// stop tracing
  public final static	int VT_STAGE_CRASH		= 64; // an uncaught exception

  // trace mode
  public final static int VT_MODE_NULL    = sabi.SABI_MODE_IDEL;
  public final static int	VT_MODE_TRACE   = sabi.SABI_MODE_TRACE;
  public final static int	VT_MODE_TEST    = sabi.SABI_MODE_TEST;
  public final static int	VT_MODE_ASPP    = sabi.SABI_MODE_ASPP;
  public final static int	VT_MODE_OFG     = sabi.SABI_MODE_OFG;
  public final static int	VT_MODE_DFG     = sabi.SABI_MODE_DFG;
  public final static int	VT_MODE_IAST    = VT_MODE_TEST;
  public final static int	VT_MODE_RASP    = VT_MODE_ASPP;

 
  //
  public class vt_clock {
    public long tv_sec;
    public long tv_nsec;
  };


  // sequence of tag/type+length+value, general purpose
  public class vt_tlv { //implements Serializable {
		//public int     tag;    /* tag/type of value */
		//public long    len;    /* length(byte number) of value */
		//public byte[]  val;    /* value(consecutive byte stream) */

		public int    tag;    /* tag/type of value
                             default is 0;
                             bit[0~7] : type, see data type
                             bit[8~15]: only primitive type available if bit[0~7] equals VT_TYPE_ARRAY,
                                        and, only BSTRING available if bit[0~7] equals VT_TYPE_CLASS
                             bit[16~31]:reserved
                           */
    public Object value;
	};

  // capacity configure
  public class vt_capacity { //implements Serializable {
    public long	sot_max;		/* maximum of SOT recorded */
    public long	mem_max;		/* maximum of memory occupied */
    public long	dsk_max;		/* maximum of disk space occupied */
    public long	cputm_max;	/* maximum of CPU time consumed(sec) */
    public long	timeout;		/* timeout of executions(msec) */
    public long fd_max;		  /* maximum of files opened */
    public long freq;			  /* trace frequency */
    public long flags;      /* see below */
  };

  // special sample frequency
  public final static int VT_CAPA_FREQ_EVERY   = 1;    /* trace every time */
  public final static int VT_CAPA_FREQ_E2E     = 0;    /* entry, exit and exception */
  public final static int VT_CAPA_FREQ_ENTRY   = -1;   /* at entry only */
  public final static int VT_CAPA_FREQ_EXIT    = -2;   /* at exit/exception only*/

  // flags of vt_capacity
  public final static int VT_CAPA_FLAG_ENABLE  = 0x01;
  public final static int VT_CAPA_FLAG_KEY     = 0x02;
  public final static int VT_CAPA_FLAG_MUST    = 0x04;
  public final static int VT_CAPA_FLAG_LASTn   = 0x08;
  public final static int VT_CAPA_FLAG_SHIFT_INVOKE   = 4;
  public final static int VT_CAPA_FLAG_SHIFT_RETURN   = 6;
  public final static int VT_CAPA_FLAG_SHIFT_CSTACK   = 8;
  public final static int VT_CAPA_FLAG_SHIFT_EXCEPT   = 10;
  public final static int VT_CAPA_FLAG_SHIFT_CATCH    = 12;
  public final static int VT_CAPA_FLAG_SHIFT_WATCH    = 14;
  public final static int VT_CAPA_FLAG_SHIFT_MEMORY   = 16;
  public final static int VT_CAPA_FLAG_SHIFT_REGS     = 18;
  public final static int VT_CAPA_FLAG_SHIFT_SYSCALL  = 20;
  //public final static int VT_CAPA_FLAG_SHIFT_RESERVED   = 22;
  public final static int VT_CAPA_FLAG_SHIFT_JOURNAL  = 24;  /* 8 bits */

  public final static int VT_CAPA_FLAG_TRACE_DIS    = 0;
  public final static int VT_CAPA_FLAG_TRACE_ENTRY  = 1;
  public final static int VT_CAPA_FLAG_TRACE_EXIT   = 2;
  public final static int VT_CAPA_FLAG_TRACE_ALL    = 3;


  // data sequence, general purpose
  public class vt_data_sequence { //implements Serializable {
    public long     ndat;     /* length of dataset */
    public long[]   uids;     /* one uid is packed in two long items */
    public Object[] datas;    /* value of data item, array of array of Object which represents the vt_tlv */
    public String[] names;    /* name of corresponding data item */
  };

  // contents and/or description of an event
  public class vt_event_entity { //implements Serializable {
    public int    type;       /* event type(reserved) */
 	  public int    opt;        /* reserved */
 	  public long   ident_l;    /* event identifier, low 8-bytes */
    public long   ident_h;    /* high 8-bytes */
    public long   from_l;     /* source address of SAP(optional), low 8-bytes */
    public long   from_h;     /* high 8-bytes */
    public long   dest_l;     /* destination address of SAP(optional), low 8-bytes */
    public long   dest_h;     /* high 8-bytes */
    public Object reqdat;     /* description/contents of corresponding request; array of object which represents the vt_data_sequence */
  };

  //
  public class vt_data_response {
    public int    tag;	   /* reserved */
    public int    opt;     /* reserved */
    public long   tv_sec;  /* timestamp, seconds */
    public long   tv_nsec; /* timestamp, nanosecdons */
    public Object respdat; /* Object, description/contents of corresponding response; array of object which represents the vt_data_sequence */
  };

  // description of a call frame
  public class vt_call_frame { //implements Serializable {
    public String full_path;  /* full description of method, be formated as: <package>/<class name>/<method name(parameter list)return_type> */
    public int    name_off;   /* name offset in the full_path */
    public int    decl_off;   /* declaration(parameter list and/or return value) offset in the full_path */
    public long   pos;        /* opcode/instruction position in specified method */
  };

  // description of a call stack
  public class vt_data_callstack { //implements Serializable {
    public Object   thread;   /* thread who makes call */
    public int      nframe;   /* length of frames */
    public Object[] frames;   /* array of array of Object which represents a vt_call_frame */
  };

  // description of an exception
  public class vt_data_exception { //implements Serializable {
    public int    type;    /* exception type, predefined */
    public String message; /* exception message */
    public Object bt;      /* back trace of call stack, array of object which represents a vt_data_callstack */
  };

  // description of journal
  public class vt_data_journal { //implements Serializable {
    public int    type;	    /* see sabi.java */
    //public int    mode;	    /* 1=file only, 2=print only, 3=both */
    public String source;   /* journal source */
    public String message;  /* journal message */
  };

  // system call
  /*
  public class vt_data_syscall { //implements Serializable {
    // TODO:
  };
  */

  // entity of a SOT(sequence of track)
  public class vt_sot_entity { //implements Serializable {
    Object args;      /* arguments of Entry, Exit or Exception; 
                         On the entry or exit, it is an array of object which represents the vt_data_sequence;
                         On the exception point, it is an array of object which represents the vt_data_exception;
                      */
    Object cstk;      /* call stack at this point, array of object which represents the vt_data_callstack */
    Object watch;     /* variables watched, similar as the args(vt_data_sequence) */
    //vt_data_memory    mem;
    //vt_data_regset    regs;
    //vt_data_syscall   sysc;
    Object journal;   /* array of object which represents the vt_data_journal */
  };

  // brief report
  public class vt_brief_report { //implements Serializable {
	  public long num_obj_exposed;
	  public long num_obj_escaped;
	  public long num_obj_lost;
	  public long num_obj_bad;
	  public long num_obj_unused;
	  public long num_obj_unknown;
	  public long num_obj_total;

	  public long num_op_bad;		     /* disabled operator */
	  public long num_op_undefined;  /* unknown operator */
	  public long num_op_missed;     /* missed necessary operator*/

	  public long num_bt_suspicious; /* unexpected back-trace of call stack */
	  public long num_cf_suspicious; /* unexpected control flow(program flow) */

    public long flags;      /* see below, VT_REPORT_FLAG_xxx */
	  public long confidence;	/* final confidence, the high the better */
	};

  //
  public final static int VT_REPORT_FLAG_AUTHORIZE_FAIL        = 0x01;
  public final static int VT_REPORT_FLAG_AUTHENTICATE_ERROR    = 0x02;
  public final static int VT_REPORT_FLAG_CREDENTIALS_BAD       = 0x04;
  public final static int VT_REPORT_FLAG_PERMIT_ERROR          = 0x08;


	//
	protected final static String default_libname = "jvas_vt.so";


  // ===================================================================================================================== //
  // Abbreviations:
  // O:= Optional
  // M:= Must
  // N:= Necessary
  // P:= Proposal

	// ============================================ native function definitions ============================================ //

  // Function:
  // Parameter:
  //   P; tpix:= trace point index
  //   O; mode:= see "trace mode"
  //   O; capacity:= array of long which represents the vt_capacity
  //   N; entity: array of object which represents the vt_event_entity
  // Return:
  //   [0]:= event reference
  //   [1]:= initial action reference
  //   ,otherwise empty array(length=0) will be returned.
	native public static long[] event_create( long   tpix,
                                            int    mode,
                                            Object capacity,
                                            Object entity
                                            );

	// this version rather return only the event reference than an array
	native public static long event_create2( long   tpix,
                                           int    mode,
                                           Object capacity,
                                           Object entity
                                           );

	// simplest version
	native public static long event_create3( long   tpix,
                                           int    mode,
																					 int    etype
                                           );

  // Function:
  // Parameter:
  // Return: 
	native public static long event_counter_set( long tpix, int delta );


  // Function:
  // Parameter:
  //   N; evtref:= event reference been returned by the event_create; 
  //               method will treats the last "event" has attached on current thread as the target if it is 0
  // Return: none
	native public static void event_destroy( long evtref );


  // Function:
  // Parameter:
  //   N; new_thread:= a new thread object
  //   O; mode:= see "trace mode"
  //   O; capacity:= array of long which represents the vt_capacity
  //   N; evtref:= event reference been created by the event_create;
  //               method will treats the last "event" has attached on current thread as the target if it is 0
  // Return:
  //   action reference if succeed otherwise -1 will be returned.
	native public static long action_new( Object new_thread,
                                        int    mode,
                                        Object capacity,
                                        long   evtref
                                        );


  // Function:
  // Parameter:
  //   O; actref:= delete all if it is 0, otherwise the specified action will be deleted;
  //   N; evtref:= event reference been created by the event_create;
  //               method will treats the last "event" has attached on current thread as the target if it is 0
  // Return: none
	native public static void action_del( long actref, long evtref );


  // Function:
  // Parameter:
  // Return:
  //   action reference if succeed otherwise -1 will be returned.
	//native public static long action_get( Object thread, long evtref );


  // Function:
  // Parameter:
  //   N; tpidx:= index of trace point(not identifier); can be obtained via the method tp_idx_copy()
  //   N; stage:= see "trace stage"
  //   P; cause:= see "triggered reason"
  //   N; entity:= contents be traced; array of object which represents the vt_sot_entity;
  //   N; actref:= action reference been returned from the event_create or the action_new;
  //               method will treats the last "action" has attached on current thread as the target if it is 0
  // Return:
  //   SOT reference if succeed otherwise -1 will be returned.

	// NOTE: it is low performance
	native public static long sot_append( long   tpidx,
                                        int    stage,
                                        int    cause,
                                        Object entity,
                                        long   actref
                                        );

	// NOTE: obsoleted!!!
	native public static long sot_append2(String name,
                                        String sign,
                                        String klass,
                                        int    stage,
                                        int    cause,
                                        Object entity,
                                        long   actref
                                        );

	// NOTE: general purposed
  native public static long sot_append3(long   tpidx,
                                        int    stage,
                                        int    cause,
                                        Object... datobjs
                                        );

	// NOTE: sot_append_x0 ~ sot_append_x10, are all fast compiled function
  native public static long sot_append_x0(long   tpidx,
                                          int    stage,
                                          int    cause,
                                          int    ndat
                                          );

  native public static long sot_append_x1(long   tpidx,
                                          int    stage,
                                          int    cause,
                                          int    ndat,
                                          Object dat0
                                          );
  native public static long sot_append_x2(long   tpidx,
                                          int    stage,
                                          int    cause,
                                          int    ndat,
                                          Object dat0, Object dat1
                                          );
  native public static long sot_append_x3(long   tpidx,
                                          int    stage,
                                          int    cause,
                                          int    ndat,
                                          Object dat0, Object dat1, Object dat2
                                          );
  native public static long sot_append_x4(long   tpidx,
                                          int    stage,
                                          int    cause,
                                          int    ndat,
                                          Object dat0, Object dat1, Object dat2, Object dat3
                                          );
  native public static long sot_append_x5(long   tpidx,
                                          int    stage,
                                          int    cause,
                                          int    ndat,
                                          Object dat0, Object dat1, Object dat2, Object dat3, Object dat4
                                          );
  native public static long sot_append_x6(long   tpidx,
                                          int    stage,
                                          int    cause,
                                          int    ndat,
                                          Object dat0, Object dat1, Object dat2, Object dat3, Object dat4, Object dat5
                                          );
  native public static long sot_append_x7(long   tpidx,
                                          int    stage,
                                          int    cause,
                                          int    ndat,
                                          Object dat0, Object dat1, Object dat2, Object dat3, Object dat4, Object dat5, Object dat6
                                          );
  native public static long sot_append_x8(long   tpidx,
                                          int    stage,
                                          int    cause,
                                          int    ndat,
                                          Object dat0, Object dat1, Object dat2, Object dat3, Object dat4, Object dat5, Object dat6, Object dat7
                                          );
  native public static long sot_append_x9(long   tpidx,
                                          int    stage,
                                          int    cause,
                                          int    ndat,
                                          Object dat0, Object dat1, Object dat2, Object dat3, Object dat4, Object dat5, Object dat6, Object dat7, Object dat8
                                          );
  native public static long sot_append_x10(long   tpidx,
                                           int    stage,
                                           int    cause,
                                           int    ndat,
                                           Object dat0, Object dat1, Object dat2, Object dat3, Object dat4, Object dat5, Object dat6, Object dat7, Object dat8, Object dat9
                                           );

  // Function:
  // Parameter:
  //   O; sotref:= SOT reference be created by sot_append(); remove all if it is 0;
  //   N; actref:= action reference;
  //               method will treats the last "action" has attached on current thread as the target if it is 0
  // Return: none
	native public static void sot_remove( long sotref, long actref );


  // Function: (RESERVED; be replaced by the sot_append)
  // Parameter:
  //   N; evtref:= event reference;
  //               method will treats the last "event" has attached on current thread as the target if it is 0
  //   N; response:= contents of response; array of object which represents the vt_data_response
  // Return: 
  //   return 0 if succeed, or -1 if failed.
	native public static int fill_response( long evtref, Object response );


  // Function:
  // Parameter:
  //   O; actref:= action reference; specific action only if it is valid actref, or all actions if it is 0
  //   N; evtref:= event reference;
  //               method will treats the last "event" has attached on current thread as the target if it is 0
  //   P; datseq:= array of object which represents the vt_data_sequence
  // Return:
  //   array of long which represents the vt_brief_report if succeed, otherwise an empty array will be returned.
	native public static long[] calc_confidence( long     actref,
                                               long     evtref,
                                               Object[] datseq
                                               );

  // Function:
  // Parameter: same as the java_calc_confidence
  // Return:
  //   return 0 if succeed, or -1 if failed.
  native public static int feed_data( long     actref,
                                      long     evtref,
                                      Object[] datseq
                                      );

  // Function:
  // Parameter:
  //   O; actref:= action reference; all actions if it is 0
  //   N; evtref:= event reference
  //               method will treats the last "event" has attached on current thread as the target if it is 0
  // Return:
  //   array of long which represents the vt_brief_report if succeed, otherwise an empty array will be returned.
	native public static long[] report_get( long actref,
                                          long evtref
                                          );


  // Function: OBSOLETED
  // Parameter: none
  // Return:
  //   array of long which represents "index of trace point" if succeed, otherwise an empty array will be returned.
	native public static long[] tp_idx_copy();


	// Function:
	// Parameter:
	// N; key:=
  // N; klass:=
	// N: name_sign:=
	// Return:
  //   return 0 if succeed, or -1 if failed.
	native public static int tpix_sync( long key, String klass, String name_sign );


	//
	public static int load_sentry(String libpath)	{
		try{
			if( libpath.isEmpty() || libpath.length() == 0 )
				System.loadLibrary( default_libname );
			else
				System.load( libpath );
			return 0;
		}
		/*catch(UnsatisfiedLinkError e){
			e.printStackTrace();
		}*/
		catch(Throwable e){
			e.printStackTrace();
		}
		return -1;
	}

	// to load library libSentry.so automically when the class file being loaded
	static {
		load_sentry("/usr/local/lib/libSentry.so");
	}

}
