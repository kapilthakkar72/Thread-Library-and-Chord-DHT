//// Start Date : 04/09/2013
////second session: 05/09/2013
////third session: 06/09/2013 - 07/09/2013
////author: Nihal
////versions 1,2,3,
///create..run....rr...sleep...sema...prodconsumer..
///completed 10/09/2013

#include <stdio.h>
#include "thread.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Remeber I have used different conventions here. You should refer to Assignment manual for that.////
//Here I have created thread and called go in master thread "to". You can/should do it in Main         ////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void f()
{
  int i=0;
  while(1){
    printf("tid :%d in f: %d\n",getmyid(),i++);
    usleep(90000);

  }
}
void g()
{
  int i=0;
  while(1){
    printf("tid :%d in g: %d\n",getmyid(),i++);
    usleep(90000);
  }
}
void h()
{
  int i=0;
  while(1){
    printf("tid :%d in h: %d\n",getmyid(),i++);
    usleep(90000);
  }
}

void t0(void * dummy)
{
  int i=0;
  createthread(f);
  createthread(g);
  createthread(h);
  go();
  printf("All children created\n");
  while(1)
  {
//    printf("tid :%d in to: %d\n",getmyid(),i++);
//    usleep(90000);

  }
}

int main()
{
    initrootthread(t0, NULL,"root");
}
