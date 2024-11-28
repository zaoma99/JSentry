//package jsentry;

import jsentry.jvas_swi;
//import jsentry.sabi;

import java.util.Locale;


public class __jvas_stub$$String {
	/* 
  Example:
  <public/protected/private> static [ReturnType] <MethodName>([<Origin Class> me] [, arguments...]) {
		// CAUTION: code which not be contained in the try-catch is prohibited

		// ENTRY code block
		try{
			// do something, but DO NOT appreas instructions "goto", "jsr"
			// leave any placement if nothing to do(for consistency of SABI), e.g. "throw null"
			// CAUTION, return directly if the original method is bypassed
		}catch(Throwable e){ DO NOT appears any code }


		// EXIT code block
		try{
			// [ReturnType ret_var=<any_value>;]  // CAUTION, NECESSARY PLACEMENT of local variable going to be returned at EXIT;
																		 // MUST put it at the beginnig at EXIT block;
											   						 // It will be set automatically when re-transforming

			// do something, but DO NOT appreas instructions "goto", "jsr"
			// CAUTION, DO NOT RETURN explicitly
		}catch(Throwable e){ DO NOT appears any code}

		// CATCH code block
		try{
			throw null;  // CAUTION, it is PLACEMENT, DO NOT remove it
		}catch(Throwable e){
			// do something, but DO NOT appreas instructions "goto", "jsr"
			// CAUTION, MUST RE-THROW the "e" explicitly after processing
			throw e;
		}
  }
	*/


  public static String toLowerCase(String me, Locale locale) {
		try{
			System.out.println("entry:"+me);
		}catch(Throwable e){}

		try{
			String retval = null;
			System.out.println("exit:"+retval);
		}catch(Throwable e){}

		try{
			throw null;  // CAUTION, it is PLACEMENT, DO NOT remove it
		}catch(Throwable e){
			System.out.println("exception:"+e.toString());
			throw e;
		}
  }


  public static void String(String me, String origin) {
		try{
			jvas_swi.test(0, origin);
		}catch(Throwable e){}

		try{
			jvas_swi.test(1, me );
			// CAUTION, DO NOT RETURN explicitly
		}catch(Throwable e){}

		try{
			throw null;  // CAUTION, it is PLACEMENT, DO NOT remove it
		}catch(Throwable e){
			jvas_swi.test(2, "exception" );
			throw e;
		}
  }


  public static String concat(String me, String str) {
		try{
			jvas_swi.test(100, str);
		}catch(Throwable e){}

		try{
			String retval = null;
			jvas_swi.test(101, retval );
			// CAUTION, DO NOT RETURN explicitly
		}catch(Throwable e){}

		try{
			throw null;  // CAUTION, it isPLACEMENT, DO NOT remove it
		}catch(Throwable e){
			jvas_swi.test(102, "exception" );
			throw e;
		}
  }
	// END
}
