#include "AMQPcpp.h"

int main (int argc, char *argv[]) {



	try {
//		AMQP amqp("123123:akalend@localhost/private");
		AMQP amqp("fmco1234:fmco@127.0.0.1:5672//ems");		

		AMQPQueue * qu2 = amqp.createQueue(argv[1]);
		//qu2->Declare();		
		qu2->Declare(argv[1], AMQP_DURABLE);		
		qu2->Bind("CMD", "");		
		
		
		//qu2->Get(AMQP_NOACK);
		while (  1 ) {
			qu2->Get(AMQP_NOACK);

			AMQPMessage * m= qu2->getMessage();
			
			cout << "count: "<<  m->getMessageCount() << endl;											 
			if (m->getMessageCount() > -1) {
			uint32_t j = 0;
			cout << "message\n"<< m->getMessage(&j) << "\nmessage key: "<<  m->getRoutingKey() << endl;
			cout << "exchange: "<<  m->getExchange() << endl;											
			cout << "Content-type: "<< m->getHeader("Content-type") << endl;	
			cout << "Content-encoding: "<< m->getHeader("Content-encoding") << endl;	
			} else 
				break;				
						
							
		}	
		qu2->Delete(argv[1]);
	} catch (AMQPException e) {
		std::cout << e.getMessage() << std::endl;
	}

	return 0;					

}
