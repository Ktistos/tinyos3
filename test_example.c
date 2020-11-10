#include <stdio.h>
#include "unit_testing.h"
#include "util.h"
#include "kernel_threads.h"

BARE_TEST(my_test, "This is a silly test")
{
  
  rlnode list ;
  rlnode_init(&list,NULL);
  rlnode nodes[10];
  for(int i=0; i<10;i++){
    rlnode_init(&nodes[i],NULL);
    rlist_push_front(&list,&nodes[i]);
  }
  while (!is_rlist_empty(&list))
  {
    rlist_pop_front(&list);
  }
  
  assert(is_rlist_empty(&list));

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

