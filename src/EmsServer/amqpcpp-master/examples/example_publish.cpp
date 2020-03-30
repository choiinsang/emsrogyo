#include "AMQPcpp.h"

int main (int argc, char** argv) {



	try {
//		AMQP amqp;
//		AMQP amqp(AMQPDEBUG);
printf("-1)\n");	
		AMQP amqp("fmco1234:fmco@10.222.223.185:5672//ems");		// all connect string
printf("-2)\n");
		AMQPExchange * ex = amqp.createExchange("CMD");
		ex->Declare("CMD", "direct");
printf("-3)\n");
//		AMQPExchange * ex = amqp.createExchange("");

		AMQPQueue * qu1 = amqp.createQueue();
		qu1->Declare("ch_ems", AMQP_DURABLE);
		qu1->Bind( "CMD", "CMD2");		
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
cout << "(1)-----" << endl;
		//ex->Publish(¸Þ½ÃÁö, route-key(queue_name))
		ex->Publish(  ss , "CMD2"); // publish very long message
		
//cout << "(2)-----" << endl;
//		ex->Publish(  "message 2" , "CMD2");
//cout << "(3)-----" << endl;
//		ex->Publish(  "message 3" , "CMD2");
		
printf("2)\n");		

		if (argc==2) {
			AMQPQueue * qu = amqp.createQueue();			
			qu->Cancel(   amqp_cstring_bytes(argv[1]) );
		}
		else
			cout << "End Publish" << endl;												
						
	} catch (AMQPException e) {
		std::cout << e.getMessage() << std::endl;
	}

	return 0;

}
