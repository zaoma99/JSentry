//package jsentry;

import jsentry.jvas_swi;
import jsentry.jvas_vtrace;
import jsentry.jvas_vtapi_stub;

public class __jvas_stub$$Common
{
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

		// CAUTION, here, DO NOT appears any code
  }
	*/

	// Thread.currentThread().getStackTrace()[1].getMethodName();

  public void jx_svlt_init(Object filterConfig) {
		try{
			long ctx_id=-1;
			System.out.format("entry: %d, init, %s%n", ctx_id, this.getClass().getName());
		}catch(Throwable e){}

		try{
			long ctx_id=-1;
			System.out.format("exit: %d, init, %s%n", ctx_id, this.getClass().getName());
		}catch(Throwable e){}

		try{
			throw null;  // CAUTION, it is PLACEMENT, DO NOT remove it
		}catch(Throwable e){
			throw e;
		}
  }


  public void jx_svlt_destroy() {
		try{
			long ctx_id=-1;
			System.out.format("entry: %d, destroy, %s%n", ctx_id, this.getClass().getName());
		}catch(Throwable e){}

		try{
			long ctx_id=-1;
			System.out.format("entry: %d, destroy, %s%n", ctx_id, this.getClass().getName());
			// CAUTION, DO NOT RETURN explicitly
		}catch(Throwable e){}

		try{
			throw null;  // CAUTION, it is PLACEMENT, DO NOT remove it
		}catch(Throwable e){
			throw e;
		}
  }

  public void jx_svlt_doFilter(Object request, Object response, Object chain) {
		try{
			long ctx_id = -1;  // as TPIX
			
			//jvas_swi.test(ctx_id, "doFilter::ENTRY");//+me.getClass().getCanonicalName());
			//System.out.println(">>>entry: doFilter, "+this.getClass().getName()+", "+Thread.currentThread().getStackTrace()[1].getClassName());
			
			//long r=jvas_vtrace.sot_append_test( ctx_id, 1, 2, 10, request, response, chain, 0x1122334455l, 1<<31, (short)0x8000, (byte)0x80, true, 'C',null );
			//System.out.format( "sot_append_test returns %d%n", r );

			long l = jvas_vtrace.event_counter_set( ctx_id, 1 );
			if( l == 0 )
			{
				// to create an event
				Object[] entity = new Object[9];
				entity[0] = 1;    // type, temporary process
				entity[1] = 0;    // option
				entity[2] = 0;    // id_low
				entity[3] = 0;    // id_high
				entity[4] = 0;    // from_low
				entity[5] = 0;    // from_high
				entity[6] = 0;    // dest_low
				entity[7] = 0;    // dest_high
				entity[8] = null; // reqdat
				l = jvas_vtrace.event_create2( ctx_id, jvas_vtrace.VT_MODE_IAST, null, entity );
				if( l > 0 )
				{
					System.out.format( "doFilter::ENTRY> event_create2 ok, evtId=%d%n", l );
					// to append initiate SOT
					l = jvas_vtapi_stub.sot_append_x( ctx_id, jvas_vtrace.VT_STAGE_ENTER, jvas_vtrace.VT_CAUSE_TRACE_START, 4, this, request, response, chain );
					System.out.format( "doFilter::ENTRY> sot_append_x returns %d%n", l );
				}
				else
					System.out.format( "doFilter::ENTRY> event_create2 error with %d%n", l );
			}
			else if( l < 0 )
			{
				System.out.format( "doFilter::ENTRY> event_counter_set error with %d%n", l );
			}
			else
				System.out.format( "doFilter::ENTRY> event_counter=%d%n", l );
		}catch(Throwable e){}

		// ================================================================================================================================================= //
		try{
			long ctx_id = -1;  // as TPIX
			/*double dd=5.0;
			float ff=1.2f;
			short ss=2;
			boolean bb=true;
			int i2=2;
			int i3=3;
			*/
			//jvas_swi.test(ctx_id, "doFilter::EXIT");//+me.getClass().getCanonicalName());
			//System.out.println(">>>exit: doFilter, "+this.getClass().getName()+", "+Thread.currentThread().getStackTrace()[1].getClassName());
			long l = jvas_vtrace.event_counter_set( ctx_id, -1 );
			if( l == 0 )
			{
				// to destroy the event has been created before
				jvas_vtrace.event_destroy( 0 );
				System.out.println( "doFilter::EXIT> event_destroy ok" );
			}
			else
				System.out.format( "doFilter::EXIT> event_counter=%d%n", l );
			// CAUTION, DO NOT RETURN explicitly
		}catch(Throwable e){}

		// ================================================================================================================================================= //
		try{
			throw null;  // CAUTION, it isPLACEMENT, DO NOT remove it
		}catch(Throwable e){
			long ctx_id = -1;
			long l = jvas_vtrace.event_counter_set( ctx_id, -1 );
			if( l >= 0 )  //if( l == 0 )
			{
				// to destroy the event has been created before
				jvas_vtrace.event_destroy( 0 );
				System.out.println( "doFilter::EXCEPT> event_destroy ok" );
			}
			throw e;//return;
		}
  }
	// END
}
