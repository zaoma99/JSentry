package jsentry_test;

import jsentry.jvas_swi;
import jsentry.sabi;

import java.nio.file.*;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;


public class jvas_test {

		public class k1234{
			public long i64;
			public void setval(long l)  throws Exception
			{
				if( l==10000 )
					throw( new Exception() );
				this.i64=l;
			}
		}

		private static int stat = sabi.SABI_STATE_INIT;
		
	  native static int test(long no, String str);

		static {
			//System.loadLibrary("Sentry");
		}

		/*static int test(long no)
		{
				//return test(no, String.format("Hello this is native call, %d", no));
		}
		*/

		public String myname="moana";
		public void empty_func(){
		}

			private static boolean wait_jvas_boot(String arg_wkdir, int tmo) {
				try{
					// try to access the file boot_log
					Path path = FileSystems.getDefault().getPath(arg_wkdir, sabi.SABI_FILENAME_SENTRY_BOOT_LOG);
					InputStream inp = Files.newInputStream( path, StandardOpenOption.READ );

					byte[] buf = new byte[10];
					int len, i;
					StringBuilder txt = new StringBuilder();

					int ms = tmo==-1 ? 10000000 : tmo;
					do{
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
								System.out.format( "find the end mark\n%s\n", txt );
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
					}while(true);

					inp.close();
					System.out.print( "\n" );

					return (ms > 0);
				}catch(Throwable e){
					System.out.println(e.toString());
				}
				return false;
			}			


      private static long wait_jvas_boot2(int tmo, String ss, Object aa[], char cc[][]) {
				try{
          //Path path = FileSystems.getDefault().getPath(arg_wkdir, sabi.SABI_FILENAME_SENTRY_BOOT_LOG);
          int ms = tmo==-1 ? 10000000 : tmo;

          while(stat == sabi.SABI_STATE_INIT) {
            if( false ){  //Files.exists(path) ){
              ms = 1000;
              do{
                System.out.print( ">>" );
                Thread.sleep(100);
                ms -= 100;
              }while(ms > 0); 
              ms = 100;
              return 1; //break;
            }
            else{
              if( ms <= 0 ){
                if( tmo != -1 )
                  return 1; //break;
                ms = 10000000;
              }

              if( (ms % 100) == 0 ) 
                System.out.print( ">>" );
              ms -= 50; 
              Thread.sleep(50);
            }
          }

          System.out.print( "\n" );

          if(ms > 0)
						return 1; //?1:0; 
					int nn=100;
					System.out.print( cc[nn][0] );
					return 0;
        }catch(Throwable e){ 
          System.out.println(e.toString());
        }
        return 0;
      }

    public static void main(String[] args) {
				//wait_jvas_boot("/home/alex/Spectrum", 5000);
				Object[] aa=new Object[2];
				char[][] cc=new char[3][10];
				wait_jvas_boot2(1000, "prompt", aa, cc);

				long n=0;
				while(n<100000){
					test(n, "test test test");
					n += 1;

	        //System.out.println("Hello World");
					try{
						Thread.sleep(1000);
					}catch(InterruptedException e){
						e.printStackTrace();
            break;
					}catch(Throwable e){
						return;//throw(e);
					}finally{
            n += 2;
						System.out.println( "finally" );
					}
				}
				System.exit(0);
    }
}
