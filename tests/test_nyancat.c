#include <stdlib.h>
#include <stdio.h>
#include <CUnit/CUnit.h>
#include <string.h>

#include "nyancat.h"
#include "packet_implement.h"

/* test suites */
int init_suite(void){
	return 0;
}

int clean_suite(void){
	return 0;
}

void test_queue(){
	list_t* list = list_create();
	CU_ASSERT_PTR_NOT_EQUAL(list, NULL);
	CU_ASSERT_EQUAL(list->size, 0);
	CU_ASSERT_PTR_EQUAL(list->head, NULL);
	CU_ASSERT_PTR_EQUAL(list->tail, NULL);

	pkt_t* pkt0 = pkt_new();
	CU_ASSERT_PTR_NOT_EQUAL(pkt0, NULL);
	CU_ASSERT_EQUAL(pkt_set_seqnum(pkt0, 0), PKT_OK);

	pkt_t* pkt1 = pkt_new();
	CU_ASSERT_PTR_NOT_EQUAL(pkt1, NULL);
	CU_ASSERT_EQUAL(pkt_set_seqnum(pkt1, 1), PKT_OK);

	pkt_t* pkt2 = pkt_new();
	CU_ASSERT_PTR_NOT_EQUAL(pkt2, NULL);
	CU_ASSERT_EQUAL(pkt_set_seqnum(pkt2, 2), PKT_OK);

	pkt_t* pkt3 = pkt_new();
	CU_ASSERT_PTR_NOT_EQUAL(pkt3, NULL);
	CU_ASSERT_EQUAL(pkt_set_seqnum(pkt3, 3), PKT_OK);

	CU_ASSERT_EQUAL(add_element_queue(list, pkt0), 0);
	CU_ASSERT_EQUAL(list->size, 1);
	CU_ASSERT_PTR_EQUAL(list->head, pkt0);
	CU_ASSERT_EQUAL(list->tail, pkt0);

	CU_ASSERT_EQUAL(add_element_queue(list, pkt1), 0);
	CU_ASSERT_EQUAL(list->size, 2);
	CU_ASSERT_PTR_EQUAL(list->head, pkt0);
	CU_ASSERT_EQUAL(list->tail, pkt1);

	CU_ASSERT_EQUAL(add_element_queue(list, pkt3), 0);
	CU_ASSERT_EQUAL(list->size, 3);
	CU_ASSERT_PTR_EQUAL(list->head, pkt0);
	CU_ASSERT_EQUAL(list->tail, pkt3);

	CU_ASSERT_EQUAL(add_specific_queue(list, pkt2), 0);
	CU_ASSERT_EQUAL(list->size, 4);
	CU_ASSERT_PTR_EQUAL(list->head, pkt0);
	CU_ASSERT_EQUAL(list->tail, pkt3);

	pkt_t* pktv0;
	CU_ASSERT_EQUAL(pop_element_queue(list, pktv0), 0);
	CU_ASSERT_EQUAL(list->size, 3);
	CU_ASSERT_PTR_EQUAL(pktv0, pkt0);
	CU_ASSERT_PTR_EQUAL(list->head, pkt1);
	CU_ASSERT_EQUAL(list->tail, pkt3);
	free(pktv0);

	pkt_t* pktv1;
	CU_ASSERT_EQUAL(pop_element_queue(list, pktv1), 0);
	CU_ASSERT_EQUAL(list->size, 2);
	CU_ASSERT_PTR_EQUAL(pktv1, pkt1);
	CU_ASSERT_PTR_EQUAL(list->head, pkt2);
	CU_ASSERT_EQUAL(list->tail, pkt3);
	free(pktv1);

	pkt_t* pktv2;
	CU_ASSERT_EQUAL(pop_element_queue(list, pktv2), 0);
	CU_ASSERT_EQUAL(list->size, 1);
	CU_ASSERT_PTR_EQUAL(pktv2, pkt2);
	CU_ASSERT_PTR_EQUAL(list->head, pkt3);
	CU_ASSERT_EQUAL(list->tail, pkt3);
	free(pktv2);

	pkt_t* pktv3;
	CU_ASSERT_EQUAL(pop_element_queue(list, pktv3), 0);
	CU_ASSERT_EQUAL(list->size, 0);
	CU_ASSERT_PTR_EQUAL(pktv3, pkt3);
	CU_ASSERT_PTR_EQUAL(list->head, NULL);
	CU_ASSERT_EQUAL(list->tail, NULL);
	free(pktv3);

}

int main(){
	/* initialisation du catalogue de test */
	if (CUE_SUCCESS != CU_initialize_registry()){
      return CU_get_error();
    }
    CU_pSuite pSuite = NULL;
	
	/* ajout de la suite de test au catalogue */
	pSuite = CU_add_suite( "Tests des méthodes de queue de la classe nyancat", init_suite, clean_suite);
	if ( NULL == pSuite ) {
	  CU_cleanup_registry();
    return CU_get_error();
	}
	
	/* ajout du test a la suite de test */
	if ( (NULL == CU_add_test(pSuite, "Tests des operations de queue", test_queue))
	   )
	{
		CU_cleanup_registry();
		return CU_get_error();
	}
	
	/* execution des tests */
	CU_basic_run_tests();
	CU_basic_show_failures(CU_get_failure_list());
	
	/* liberation les ressources */
	CU_cleanup_registry();
	
	return CU_get_error();
}