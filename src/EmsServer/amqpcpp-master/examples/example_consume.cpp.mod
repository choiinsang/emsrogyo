#include "AMQPcpp.h"
int i=0;

int onCancel(AMQPMessage * message ) {
	cout << "cancel tag="<< message->getDeliveryTag() << endl;
	return 0;
}

int  onMessage( AMQPMessage * message  ) {
	uint32_t j = 0;
	char * data = message->getMessage(&j);
	if (data)
		  cout << "Data : " << data << " Len : " << j << endl;

	i++;

	cout << "#" << i << " tag="<< message->getDeliveryTag() << " content-type:"<< message->getHeader("Content-type") ;
	cout << " encoding:"<< message->getHeader("Content-encoding")<< " mode="<<message->getHeader("Delivery-mode")<<endl;

//	if (i > 10) {
//		AMQPQueue * q = message->getQueue();
//		printf("consumer tags : %s\n", (char*)message->getConsumerTag().c_str());
//		q->Cancel( message->getConsumerTag() );
//	}
	return 0;
};


int main () {


	try {
//		AMQP amqp("123123:akalend@localhost/private");

		AMQP amqp("fmco1234:fmco@127.0.0.1:5672//ems");

		AMQPQueue * qu2 = amqp.createQueue("ch_ems");

		qu2->Declare();
		//qu2->Bind( "", "");

		qu2->setConsumerTag("tag_123");
		qu2->addEvent(AMQP_MESSAGE, onMessage );
		qu2->addEvent(AMQP_CANCEL, onCancel );

		qu2->Consume(AMQP_NOACK);//


	} catch (AMQPException e) {
		std::cout << e.getMessage() << std::endl;
	}

	return 0;

}

