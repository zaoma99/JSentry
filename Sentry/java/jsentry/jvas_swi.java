package jsentry;

import jsentry.sabi;

import java.lang.Thread;
import java.lang.Runnable;

//import java.nio.file.*;
//import java.io.InputStream;
//import java.nio.charset.StandardCharsets;
//import java.net.DatagramSocket;
//import java.net.DatagramPacket;



public class jvas_swi implements sabi {

	//
	protected final static String default_libname = "jvas_swi";
	protected static String arg_libpath;
	protected static String arg_wkdir;
	protected static long arg_res;

	// sentry mode(SABI-Specific), reserved
	protected static int mode = SABI_MODE_IDEL;
	protected static int stat = SABI_STATE_PRIMORDIAL;
	protected static int jourtype = SABI_JOURNAL_ALL;
	private static Thread th_init;


	// native function definitions
	native public static int test(long no, String str);


	// to load library libSentry.so automically when the class file being loaded
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

	//static {
		//load_sentry("/usr/local/lib/libSentry.so");
	//}


	// =====================================================================================================
	protected static class jvas_init implements Runnable {
			//
			/*
			private boolean wait_jvas_boot(int tmo) {
				try{
					// try to access the file boot_log
					Path path = FileSystems.getDefault().getPath(arg_wkdir, sabi.SABI_FILENAME_SENTRY_BOOT_LOG);
					InputStream inp = Files.newInputStream( path, StandardOpenOption.READ );
					
					byte[] buf = new byte[256];
					int len, i;
					StringBuilder txt = new StringBuilder();

					int ms = tmo==-1 ? 10000000 : tmo;

					while(stat == SABI_STATE_INIT) {
						if( inp.available() > 0 ){
							len = inp.read(buf);
							if( len == -1 ){
								ms = 0;
								break;
							}

							// match the specified-string "<PULLOUT>"
							txt.append( new String(buf, 0, len, StandardCharsets.UTF_8) );
							i = txt.lastIndexOf( "<" );
							if( i == -1 )
								txt.setLength(0);
							else if( txt.indexOf( "<PULLOUT>", i ) == -1 ){
								txt.delete( 0, i );
								//System.out.format( "trunc string: %d, %s\n", txt.length(), txt );
							}
							else{
								//System.out.format( "find the end mark\n%s\n", txt );
								ms = 1000;
								do{
									System.out.print( ">>" );
									Thread.sleep(100);
									ms -= 100;
								}while(ms > 0);
								ms = 100;
								break;
							}
						}
						else{
							if( ms <= 0 ){
								if( tmo != -1 )
									break;
								ms = 10000000;
							}

							if( (ms % 500) == 0 )
								System.out.print( ">>>" );
							ms -= 50;
							Thread.sleep(50);
						}
					}

					inp.close();
					System.out.print( "\n" );

					return (ms > 0);
				}catch(Throwable e){
					System.out.println(e.toString());
				}
				return false;
			}


			//
			private boolean wait_jvas_boot2(int tmo) {
				try{
					//Path path = FileSystems.getDefault().getPath(arg_wkdir, sabi.SABI_FILENAME_SENTRY_BOOT_LOG);
					int ms = tmo==-1 ? 10000000 : tmo;

					while(stat == SABI_STATE_INIT) {
						if( false ){  //Files.exists(path) ){
							ms = 1000;
							do{
								System.out.print( ">>" );
								Thread.sleep(100);
								ms -= 100;
							}while(ms > 0);
							ms = 100;
							break;
						}
						else{
							if( ms <= 0 ){
								if( tmo != -1 )
									break;
								ms = 10000000;
							}

							if( (ms % 100) == 0 )
								System.out.print( ">>" );
							ms -= 50;
							Thread.sleep(50);
						}
					}

					System.out.print( "\n" );

					return (ms > 0);
				}catch(Throwable e){
					System.out.println(e.toString());
				}
				return false;
			}
			*/

			//
			public void run(){
			try{
				System.out.print( "==================== In Thread jvas_init ====================\n" );
				int old_stat = stat;
				stat = SABI_STATE_INIT;

				// MUST, waiting for jvas_boot exit
				//wait_jvas_boot2(2000);
				if(true ){
					// try to load the native lib
					//System.out.format( "jvas_swi: try to load \"%s\"\n", arg_libpath );
					//load_sentry( arg_libpath );

					// test necessary native method
					//test(arg_res, String.format("jvas_swi initiation complete finished with %x", arg_res));

					// all is done
					stat = SABI_STATE_READY;

					System.out.print( "jvas_swi: initiation finished\n" );
					return;
				}
				else
					System.out.print( "jvas_swi: faild to initiating\n" );

				stat = old_stat;

			}catch(Throwable e){
				System.out.println(e.toString());
			}
		}
	}
	
	// =====================================================================================================

	// Function:
	public static int begin(long res, String libpath, String wkdir)	{
		try{
			//System.out.println("begin: "+libpath);
			if( stat != SABI_STATE_PRIMORDIAL && stat != SABI_STATE_DEAD )
				return SABI_errState;

			//if( th_init.isAlive() )
			//	return SABI_errAgain;

			stat = SABI_STATE_INIT;

			// save initArgs
			arg_res = res;
			arg_libpath = libpath;
			arg_wkdir = wkdir;

			// new thread for initiating
			th_init = new Thread( new ThreadGroup("jvas_swi init threadgroup"),
													  new jvas_init()
													 );

			//System.out.println( tInit.getThreadGroup().getName() );
			th_init.setDaemon( true );			
			th_init.start();
		
			// Note: DO NOT waiting for the init thread
			return SABI_errOk;
		}catch(Throwable e){
			stat = SABI_STATE_PRIMORDIAL;

			System.out.println(e.toString());
			//throw(e);
		}
		return SABI_errFail;
	}


	// Function:
	public static void end() {
		int old_stat = stat;
		int old_mode = mode;
		stat = SABI_STATE_EXIT;
		mode = SABI_MODE_IDEL;

		try{
			if( old_stat == SABI_STATE_INIT && th_init.isAlive() )
				th_init.join(3000);
		}catch(Throwable e){
			System.out.println( e.toString() );
			//throw(e);
		}
		stat = SABI_STATE_DEAD;
	}


	// Function: 
	public static int setmode(int new_mode)	{
 		mode = new_mode & SABI_STATUS_MASK_MODE;
		// TODO:
		return mode;
	}

	//
	public static int getmode()	{
		return mode;
	}

	//
	public static long getstatus() {
		return mode | (stat << SABI_STATUS_SHIFT_STATE) | (jourtype << SABI_STATUS_SHIFT_JOURTYPE);
	}


	//
	public static long _longValue( Object o ) {
		/*if( o instanceof Long )
			return ((Long)o).longValue();
		if( o instanceof Integer )
			return ((Integer)o).longValue();
		if( o instanceof Short )
			return ((Short)o).longValue();
		if( o instanceof Byte )
			return ((Byte)o).longValue();
		if( o instanceof Float )
			return ((Float)o).longValue();
		if( o instanceof Double )
			return ((Double)o).longValue();
		*/
		return ((Number)o).longValue();
	}

	public static int _intValue( Object o ) {
		return ((Number)o).intValue();
	}

	public static short _shortValue( Object o ) {
		return ((Number)o).shortValue();
	}

	public static byte _byteValue( Object o ) {
		return ((Number)o).byteValue();
	}

	public static float _floatValue( Object o ) {
		return ((Number)o).floatValue();
	}

	public static double _doubleValue( Object o ) {
		return ((Number)o).doubleValue();
	}


	//
	public static long longValue( Long o ) {
		return o.longValue();
	}

	public static int intValue( Integer o ) {
		return o.intValue();
	}

	public static short shortValue( Short o ) {
		return o.shortValue();
	}

	public static byte byteValue( Byte o ) {
		return o.byteValue();
	}

	public static float floatValue( Float o ) {
		return o.floatValue();
	}

	public static double doubleValue( Double o ) {
		return o.doubleValue();
	}

  //
  public static int vtrace_ask_feed( long actref, long evtref, Object extarg ) {
    return SABI_errUnsupport; 
  }



	// Function: testing
	public static int test(long no)
	{
		try{
			//if( mode != SABI_MODE_IDEL )
				return test(no, String.format("Hello this is native call, %d", no));
		}catch( Throwable e ){
			if( 0 != (mode & (SABI_MODE_SILENT | SABI_MODE_QUIET)) )
				System.out.println(e.toString());
		}
		return -1;
	}
}

