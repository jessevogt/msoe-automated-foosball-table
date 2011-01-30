#include <rtl_cpp.h>

pthread_t thread;

class A {
protected:
	int a;
public:
	virtual void print()=0;
	A() { a = 1; rtl_printf("initializing A\n");}
	virtual ~A() { rtl_printf("uninitializing A\n"); }
};

class B : public A
{
public:
	B() { a = 1; rtl_printf("initializing B\n");}
	~B() { rtl_printf("uninitializing B\n");}
	virtual void print() { rtl_printf("B::print: %d\n", a); }
};

void * start_routine(void *arg)
{
	struct sched_param p;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

	pthread_make_periodic_np (pthread_self(), gethrtime(), 500000000);

	while (1) {
		pthread_wait_np ();
		rtl_printf("I'm here; my arg is %x\n", (unsigned) arg);
	}
	return 0;
}

B a;
A *ptr_b;

int init_module(void) {
	__do_global_ctors_aux();
	ptr_b = new B;
	ptr_b -> print();
	return pthread_create (&thread, NULL, start_routine, 0);
}

void cleanup_module(void) {
	pthread_delete_np (thread);
	delete ptr_b;
	__do_global_dtors_aux();
}
