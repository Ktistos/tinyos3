#include <stdio.h>
#include "unit_testing.h"
#include "util.h"
#include "kernel_threads.h"

BARE_TEST(my_test, "This is a silly test")
{
  PTCB ptcb_list[10];
  rlnode list;
  rlnode_new(&list);
  for(int i=0; i<10;i++){
    rlnode* ptcb=&(ptcb_list[i].ptcb_list_node);
    rlnode_new(ptcb);
    rlist_push_front(&list,ptcb);
  }
  ASSERT(rlist_len(&list)==10);
  

}

TEST_SUITE(all_my_tests, "These are mine")
{
  &my_test,
  NULL
};

int main(int argc, char** argv)
{
  return register_test(&all_my_tests) ||
    run_program(argc, argv, &all_my_tests);
}

