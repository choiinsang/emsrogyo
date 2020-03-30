#include "AMQPcpp.h"
int i=0;

int onCancel(AMQPMessage * message ) {
	cout << "cancel tag="<< message->getDeliveryTag() << endl;
	return 0;
}
//
//int  onMessage( AMQPMessage * message  ) {
//	uint32_t j = 0;
//	char * data = message->getMessage(&j);
//	if (data)
//		  cout << "Data : " << data << " Len : " << j << endl;
//
//	i++;
//
//	cout << "#" << i << " tag="<< message->getDeliveryTag() << " content-type:"<< message->getHeader("Content-type") ;
//	cout << " encoding:"<< message->getHeader("Content-encoding")<< " mode="<<message->getHeader("Delivery-mode")<<endl;
//
////	if (i > 10) {
////		AMQPQueue * q = message->getQueue();
////		printf("consumer tags : %s\n", (char*)message->getConsumerTag().c_str());
////		q->Cancel( message->getConsumerTag() );
////	}
//	AMQPQueue *que = message->getQueue();
//	que->Delete("ch_ems");
//	cout << "delete ch_ems Queue" << endl;
//	return 0;
//};
//

int main (int argc, char *argv[]) {
char * testp = NULL;
printf ("----");
delete testp;
printf ("----");

	try {
		while(true){
	//		AMQP amqp("123123:akalend@localhost/private");
	
			AMQP amqp("fmco1234:fmco@10.222.223.185:5672//ems");
	
			AMQPExchange * ex = amqp.createExchange("CMD");
			ex->Declare("CMD", "direct");
	//		AMQPQueue * qu2 = amqp.createQueue(argv[1]);
			AMQPQueue * qu1 = amqp.createQueue("ch_ems");
	//		AMQPQueue * qu2 = amqp.createQueue("ch_ems");
	
			//qu2->Declare(argv[1], AMQP_EXCLUSIVE|AMQP_AUTODELETE);
	//		qu2->Declare(argv[1]);
			qu1->Declare("ch_emsA", AMQP_DURABLE);
	//		qu2->Declare("ch_ems", AMQP_DURABLE);
	//		qu2->Bind( "CMD", "CMD");
			qu1->Bind( "CMD", "ch_ems");
	//		qu2->Bind( "CMD", "CMD2");
	
	//		qu2->setConsumerTag("tag_123");
	//		qu2->addEvent(AMQP_MESSAGE, onMessage );
	//		qu2->addEvent(AMQP_CANCEL, onCancel );
	
	//		qu2->Consume(AMQP_NOACK);
	
			while (  1 ) {
				cout << "run......"<< endl;
				sleep(1);
		    
		    qu1->Get(AMQP_NOACK);
				
				AMQPMessage * m= qu1->getMessage();
				
				cout << "count: "<<  m->getMessageCount() << endl;                                                                                       
				if (m->getMessageCount() > -1) {
					uint32_t j = 0;
					cout << "message\n"<< m->getMessage(&j) << "\nmessage key: "<<  m->getRoutingKey() << endl;
					cout << "exchange: "<<  m->getExchange() << endl;                                                                                       
					cout << "Content-type: "<< m->getHeader("Content-type") << endl;        
					cout << "Content-encoding: "<< m->getHeader("Content-encoding") << endl;        
					qu1->Delete("ch_emsA");
				}
				else{
					qu1->Delete("ch_emsA");
				}
				break;
			}
	//		int icount = 0; 
	//		while (  1 ) {
	//                        cout << "run2......"<< icount++ << endl;
	//			if(icount >3)
	//				break;
	//                        sleep(1);
	//		}
	//
	
			int cCount = 0;
			while (true){
	
				AMQPQueue * qu3 = amqp.createQueue();
				AMQPQueue * qu4 = amqp.createQueue();
				AMQPQueue * qu5 = amqp.createQueue();
				AMQPQueue * qu6 = amqp.createQueue();
				AMQPQueue * qu7 = amqp.createQueue();

				AMQPExchange * ex3 = amqp.createExchange();
				AMQPExchange * ex4 = amqp.createExchange();
				AMQPExchange * ex5 = amqp.createExchange();
				AMQPExchange * ex6 = amqp.createExchange();
				AMQPExchange * ex7 = amqp.createExchange();
				
				qu3->Declare("ch_ems1", AMQP_DURABLE);
				qu3->Bind( "CMD", "ch_ems1");
				ex3->Declare("CMD", "direct");
				qu4->Declare("ch_ems2", AMQP_DURABLE);
				qu4->Bind( "CMD", "ch_ems2");
				ex4->Declare("CMD", "direct");
				qu5->Declare("ch_ems3", AMQP_DURABLE);
				qu5->Bind( "CMD", "ch_ems3");
				ex5->Declare("CMD", "direct");
				qu6->Declare("ch_ems4", AMQP_DURABLE);
				qu6->Bind( "CMD", "ch_ems4");
				ex6->Declare("CMD", "direct");
				qu7->Declare("ch_ems5", AMQP_DURABLE);
				qu7->Bind( "CMD", "ch_ems5");
				ex7->Declare("CMD", "direct");
				cout << "=>Create ch_ems"<< endl;
				sleep(1);
				cout << "End Sleep 10" << endl;
		
				//qu3->Purge("ch_ems1");
				//cout << "qu3->Purge(ch_ems1)"<< endl;
	//			qu3->unBind( "CMD", "ch_ems");
	//			cout << "qu3->unBind(ch_ems)"<< endl;
				qu3->Delete("ch_ems1");
				cout << "qu3->Delete(ch_ems1)"<< endl;
		
				//qu4->Purge("ch_ems2");
				//cout << "qu4->Purge(ch_ems2)"<< endl;
	//			qu4->unBind( "CMD", "ch_ems");
	//			cout << "qu4->unBind(ch_ems)"<< endl;
				qu4->Delete("ch_ems2");
				cout << "qu4->Delete(ch_ems2)"<< endl;
		
				//qu5->Purge("ch_ems3");
				//cout << "qu5->Purge(ch_ems3)"<< endl;
	//			qu5->unBind( "CMD", "ch_ems");
	//			cout << "qu5->unBind(ch_ems)"<< endl;
				qu5->Delete("ch_ems3");
				cout << "qu5->Delete(ch_ems3)"<< endl;
				
				//qu6->Purge("ch_ems4");
				//cout << "qu6->Purge(ch_ems4)"<< endl;
	//			qu6->unBind( "CMD", "ch_ems");
	//			cout << "qu6->unBind(ch_ems)"<< endl;
				qu6->Delete("ch_ems4");
				cout << "qu6->Delete(ch_ems4)"<< endl;
		
				//qu7->Purge("ch_ems5");
				//cout << "qu7->Purge(ch_ems5)"<< endl;
	//			qu7->unBind( "CMD", "ch_ems");
	//			cout << "qu7->unBind(ch_ems)"<< endl;
				qu7->Delete("ch_ems5");
				cout << "qu7->Delete(ch_ems5)"<< endl;
				cout << "<=Delete ch_ems"<< endl;
	
				cout << "count: "<<  cCount++ << endl;

				if(cCount > 20){
					break;
				}
				sleep(1);
				
				qu3->closeChannel();
				qu4->closeChannel();
				qu5->closeChannel();
				qu6->closeChannel();
				qu7->closeChannel();
				ex3->closeChannel();
				ex4->closeChannel();
				ex5->closeChannel();
				ex6->closeChannel();
				ex7->closeChannel();
				
			}
			amqp.closeChannel();
		} 
	}
	catch (AMQPException e) {
		std::cout << e.getMessage() << std::endl;
	}
	
	cout << "End " << argv[0] << " Process " << endl;
	return 0;

}

