package jsentry;

import java.lang.Thread;

import jsentry.sabi;
import jsentry.jvas_vtrace;

import static jsentry.jvas_vtrace.VT_CAUSE_DATA_PROP;
import static jsentry.jvas_vtrace.VT_STAGE_ENTER;


public class jvas_vtrace_test {

	//////////////////////////////////////////////////////////////////////////////////////////
	public static long[] test_event_create() {
		try{
			long[] caps = {20000, 0x1000000, 0x4000000, 12, 3000, 65535, 1, 0};
			Object[] entity = new Object[9];
			Object[] reqdat = new Object[4];

			entity[0] = 1;//Integer.valueOf(1);
			entity[1] = 0;//Integer.valueOf(0);
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

			names[0] = "data-a0";
			names[1] = "data-a1";

			datas[0][0] = 0;  //Integer.valueOf(jvas_vtrace.VT_TYPE_CSTRING);
			datas[0][1] = "the first item value";
			datas[1][0] = 0;  //Integer.valueOf(jvas_vtrace.VT_TYPE_ARRAY|(jvas_vtrace.VT_TYPE_LONG<<8));
			datas[1][1] = uids;

			reqdat[0] = 0;//Long.valueOf(2);
			reqdat[1] = uids;
			reqdat[2] = datas;
			reqdat[3] = names;

			uids[0] = 0;
			uids[1] = 0;
			uids[2] = 0;
			uids[3] = 0;

			Object init_task = Thread.currentThread();
			long[] ref = jvas_vtrace.event_create( init_task, jvas_vtrace.VT_MODE_IAST, caps, entity );
			System.out.format( "%d\n", ref.length );
			return ref;
		}catch( Throwable e ) {
			e.printStackTrace();
		}
		return null;
	}


	//////////////////////////////////////////////////////////////////////////////////////////
	public static void test_slot_create(long actref) {
		try{
//			jvas_vtrace.vt_sot_entity entity = new jvas_vtrace.vt_sot_entity();
			Object[] entity = new Object[4];

			// 参数
			Object[] reqdat = new Object[4];
			long[] uids = new long[4];
			String[] names = new String[2];
			Object[][] datas = new Object[2][2];

			names[0] = "data-a0";
			names[1] = "data-a1";

			datas[0][0] = 0; //Integer.valueOf(jvas_vtrace.VT_TYPE_CSTRING);
			datas[0][1] = "the first item value";
			datas[1][0] = 0; //Integer.valueOf(jvas_vtrace.VT_TYPE_ARRAY|(jvas_vtrace.VT_TYPE_LONG<<8));
			datas[1][1] = uids;

			reqdat[0] = Long.valueOf(2);
			reqdat[1] = uids;
			reqdat[2] = datas;
			reqdat[3] = names;

			uids[0] = 0x11223344l;
			uids[1] = 0x55667788l;
			uids[2] = 0x99aabbccl;
			uids[3] = 0xddeeff00l;

			// 堆栈
			//vt_call_frame
			Object[] vcf = new Object[4];
			vcf[0] = "package/classname/test(int,int)int";
			vcf[1] = 15;
			vcf[2] = 10;
			vcf[3] = 1L;

//			jvas_vtrace.vt_data_callstack vdc = new jvas_vtrace.vt_data_callstack();
			Object[] vdc = new Object[3];
			vdc[0] = Thread.currentThread();
			vdc[1] = 0; // ask VT to copy stack trace by self
			vdc[2] = new Object[]{vcf, vcf};



			// data_journal
//			jvas_vtrace.vt_data_journal vdj = new jvas_vtrace.vt_data_journal();
			Object[] vdj = new Object[3];
			vdj[0] = 1;
			vdj[1] = "";
			vdj[2] = "";


			Object[] data_watch = new Object[4];
			data_watch[0] = 0;


			entity[0] = reqdat;
			entity[1] = vdc;
			entity[2] = data_watch;
			entity[3] = vdj;

			long[] tpx = jvas_vtrace.tp_idx_copy();
			long ret = jvas_vtrace.sot_append(1L, VT_STAGE_ENTER, VT_CAUSE_DATA_PROP, entity, actref);

			System.out.println("test_slot_create ret:" + ret);
		}catch( Throwable e ) {
			e.printStackTrace();
		}
	}


	//////////////////////////////////////////////////////////////////////////////////////////
	public static void test_report_get(long actref, long evtref) {
		try{
			long[] ret = jvas_vtrace.report_get(actref, evtref);
			System.out.println("test_report_get ret:" + ret);
		}catch( Throwable e ) {
			e.printStackTrace();
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	public static void main( String[] args ) {
		try{
			jvas_vtrace.event_destroy( 0 );

			int n = 10;
			while( --n >= 0 ) {
				System.out.print( "==" );
				Thread.sleep( 100 );
			}

			//jvas_vtrace.tpix_sync(4, "javax/servlet/http/HttpServletRequest", "getParameter(Ljava/lang/String;)" );
			//jvas_vtrace.tpix_sync(5, "javax/servlet/http/HttpServletRequest", "getParameterValues(Ljava/lang/String;)" );
			
			System.out.print( "\ntest: event_create ... " );
			long[] ref = test_event_create();

			if (ref != null && ref.length == 2){
				test_slot_create(ref[1]);
				test_report_get(ref[1], ref[0]);
			}

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


