#include "AMQPcpp.h"

int main (int argc, char** argv) {



	try {
//		AMQP amqp;
//		AMQP amqp(AMQPDEBUG);
printf("-1)\n");	
		AMQP amqp("fmco1234:fmco@127.0.0.1:5672//ems");		// all connect string
printf("-2)\n");
		AMQPExchange * ex = amqp.createExchange("");
		ex->Declare("ems", "direct");
printf("-3)\n");
//		AMQPExchange * ex = amqp.createExchange("");

		AMQPQueue * qu2 = amqp.createQueue("ch_ems");
		qu2->Declare();
//		qu2->Bind( "e", "");		
printf("-4)\n");
		string ss = "message 1 test message 111";
		/* test very long message
		ss = ss+ss+ss+ss+ss+ss+ss;
		ss += ss+ss+ss+ss+ss+ss+ss;
		ss += ss+ss+ss+ss+ss+ss+ss;
		ss += ss+ss+ss+ss+ss+ss+ss;
		ss += ss+ss+ss+ss+ss+ss+ss;
*/

printf("1)\n");
		ex->setHeader("Delivery-mode", 2);
		ex->setHeader("Content-type", "text/plain");
		ex->setHeader("Content-encoding", "euc-kr");

		//ex->Publish(¸Þ½ÃÁö, route-key(queue_name))
		ex->Publish(  ss , ""); // publish very long message
		
		ex->Publish(  "message 2" , "");
		ex->Publish(  "message 3" , "");
		
printf("2)\n");		

		if (argc==2) {
			AMQPQueue * qu = amqp.createQueue();			
			qu->Cancel(   amqp_cstring_bytes(argv[1]) );
		}												
						
	} catch (AMQPException e) {
		std::cout << e.getMessage() << std::endl;
	}

	return 0;

}
