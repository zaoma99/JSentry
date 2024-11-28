/* ################################################################################
 * Automatically-generated file. Do not edit!
 * ################################################################################
 */

package jsentry;
import jsentry.jvas_vtrace;

public class jvas_vtapi_stub {

  public static int load_sentry(String libpath) {
    try{
      if( libpath.isEmpty() || libpath.length() == 0 ) 
        System.loadLibrary( "libSentry.so" );
      else
        System.load( libpath );
      return 0;
    }   
    /*catch(UnsatisfiedLinkError e){
 *       e.printStackTrace();
 *           }*/
    catch(Throwable e){ 
      e.printStackTrace();
    }   
    return -1; 
  }

  // to load library libSentry.so automically when the class file being loaded
  static {
     //load_sentry("/usr/local/lib/libSentry.so");
  }


	// mapping to servlet.doFilter(...) 
  native public static long sot_append_x( long   tpidx,
                                          int    stage,
                                          int    cause,
                                          int    ndat,
                                          Object _this, Object request, Object response, Object chian );
	// only for exception 
  native public static long sot_append_x( long   tpidx,
                                          int    stage,
                                          int    cause,
                                          int    ndat,
                                          Object _this, Object request, Object response, Object chian, String expmsg );

}
