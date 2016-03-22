#include "MyThread.h"

#include "stdio.h"

int x=0;

void f()
{
    int i = 0;
    while(i<100)
    {
        printf("in f and i = %d\n",i);
        printf("in f and x = %d\n",x);
        i++;
        x++;
    }
}
void g()
{
    int j = 0;
    while(j<100)
    {
        printf("in g and j = %d\n",j);
        printf("in g and x = %d\n",x);
        j++;
    }
}

void h()
{
    int k = 0;
    while(k<100)
    {
        printf("in h and k = %d\n",k);
        printf("in h and x = %d\n",x);
        k++;
    }
 }


int main()
{
	create(f);
	create(g);
	create(h);

	start();

	printf("Exiting");

	return 0;
}
