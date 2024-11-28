//import jsentry.jvas_swi;

public class __Print {
public static void print() {
		try{
      // 入口位置
			//jvas_swi.test( 9999, "test test test" );
      System.out.println("9999");
    }catch(Throwable e){ 
    }   

    try{
			long lll=10000;
			double ddd=2.56;
      // TODO: 出口位置hook
      System.out.println("8888");
			//jvas_swi.test( 8888, "test test test" );
      // CAUTION, DO NOT RETURN explicitly
    }catch(Throwable e){ 
    }   

    try{
      throw null;  // CAUTION, just a PLACEMENT, DO NOT remove it
    }catch(Throwable e){ 
      // TODO: 异常未被原程序捕猎时的hook
      System.out.println("xxxx: "+e.toString());
      throw e;
    }   
  }


public static long wait_jvas_boot2(int tmo, String ss, Object aa[], char cc[][]) {
		try{
      // 入口位置
			//jvas_swi.test( 9999, "test test test" );
      System.out.format("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXENTRY:%d\n", tmo);
			long t = tmo;
			int d=200;
			while( t > 0 )
			{
				System.out.format( "****%d", t );
				Thread.sleep(250);
				t -= d;
			}
			System.out.println("end");
    }catch(Throwable e){}   

    try{
			long retval=0;
			int nn = 4;
			double ddd=2.56;
      // TODO: 出口位置hook
      System.out.format("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXEXIT:%d, %d, %f\n", retval, nn, ddd);
			//jvas_swi.test( 8888, "test test test" );
			long t = 10000;
			int d=500;
			while( t > 0 )
			{
				System.out.format( "****%d", t );
				Thread.sleep(500);
				t -= d;
			}
			System.out.println("end");
      // CAUTION, DO NOT RETURN explicitly
    }catch(Throwable e){}   

    try{
      throw null;  // CAUTION, just a PLACEMENT, DO NOT remove it
    }catch(Throwable e){ 
      // TODO: 异常未被原程序捕猎时的hook
      System.out.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXCATCH: "+e.toString());
      throw e;
		}

	}



}
