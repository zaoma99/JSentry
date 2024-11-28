package jsentry;

import java.lang.Thread;

import jsentry.sabi;
import jsentry.jvas_vtrace;


public class jvas_vtrace_test {

	//////////////////////////////////////////////////////////////////////////////////////////
	public static void test_event_create() {
		try{
			long[] caps = {20000, 0x1000000, 0x4000000, 12, 3000, 65535, 1, 0};
			Object[] entity = new Object[9];
			Object[] reqdat = new Object[4];

			entity[0] = Integer.valueOf(1);
			entity[1] = Integer.valueOf(0);
			entity[2] = Long.valueOf(0x12345678l);
			entity[3] = Long.valueOf(0x9abcdef0l);
			entity[4] = Long.valueOf(0);
			entity[5] = Long.valueOf(1);
			entity[6] = Long.valueOf(2);
			entity[7] = Long.valueOf(3);
			entity[8] = reqdat;

			long[] uids = new long[4];
			String[] names = new String[2];
			Object[][] datas = new Object[2][2];

			uids[0] = 0x11223344l;
			uids[1] = 0x55667788l;
			uids[2] = 0x99aabbccl;
			uids[3] = 0xddeeff00l;

			names[0] = "data-a0";
			names[1] = "data-a1";

			datas[0][0] = Integer.valueOf(jvas_vtrace.VT_TYPE_CSTRING);
			datas[0][1] = "the first item value";
			datas[1][0] = Integer.valueOf(jvas_vtrace.VT_TYPE_ARRAY|(jvas_vtrace.VT_TYPE_LONG<<8));
			datas[1][1] = uids;

			reqdat[0] = Long.valueOf(2);
			reqdat[1] = uids;
			reqdat[2] = datas;
			reqdat[3] = names;

			Object init_task = Thread.currentThread();
			long[] ref = jvas_vtrace.event_create( init_task, jvas_vtrace.VT_MODE_IAST, caps, entity );
			System.out.format( "%d\n", ref.length );
		}catch( Throwable e ) {
			e.printStackTrace();
		}
	}


	//////////////////////////////////////////////////////////////////////////////////////////
	public static void main( String[] args ) {
		try{
			
			jvas_vtrace.event_destroy( 0 );

			int n = 50;
			while( --n >= 0 ) {
				System.out.print( "==" );
				Thread.sleep( 100 );
			}

			System.out.print( "\ntest: event_create ... " );
			test_event_create();
			System.out.print( "[FINISHED]\n" );

			n = 1;
			while( n!=0 ) {
				//System.out.println( "I'm waiting" );
				Thread.sleep( 1000 );
			}

		}catch( Throwable e ) {
			System.out.print( e.toString() );
		}
	}
}
