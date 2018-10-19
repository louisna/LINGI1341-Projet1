#include <stdlib.h>
#include <stdio.h>
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <string.h>

#include "packet_implement.h"

/* test suites */
int init_suite(void){
	return 0;
}

int clean_suite(void){
	return 0;
}

void test_pkt_get_set(){
	pkt = pkt_new();

	CU_ASSERT_PTR_NOT_EQUAL(pkt, NULL);

	/* Test du type */
	CU_ASSERT_EQUAL(pkt_set_type(pkt, 5), E_TYPE);
	CU_ASSERT_EQUAL(pkt_set_type(pkt, PTYPE_DATA), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_type(pkt), PTYPE_DATA);

	/* Test du tr */
	CU_ASSERT_EQUAL(pkt_set_tr(pkt, 5), E_TR);
	CU_ASSERT_EQUAL(pkt_set_tr(pkt, 1), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_tr(pkt), 1);

	/* Test de la window */
	CU_ASSERT_EQUAL(pkt_set_window(pkt, MAX_WINDOW_SIZE+1), E_WINDOW);
	CU_ASSERT_EQUAL(pkt_set_window(pkt, MAX_WINDOW_SIZE/2), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_window(pkt), MAX_WINDOW_SIZE/2);

	/* Test du seqnum */
	CU_ASSERT_EQUAL(pkt_set_seqnum(pkt, 256), E_SEQNUM);
	CU_ASSERT_EQUAL(pkt_set_seqnum(pkt, -1), E_SEQNUM);
	CU_ASSERT_EQUAL(pkt_set_seqnum(pkt, 100), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_seqnum(pkt), 100);

	/* Test de la longueur */
	CU_ASSERT_EQUAL(pkt_set_length(pkt, 513), E_LENGTH);
	CU_ASSERT_EQUAL(pkt_set_length(pkt, 100), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_length(pkt), 100);

	/* Test du timestamp */
	CU_ASSERT_EQUAL(pkt_set_timestamp(pkt, 1000), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_length(pkt), 1000);

	/* Test du payload */
	char* payload = "test du payload"
	int length = strlen(payload);
	CU_ASSERT_EQUAL(pkt_set_payload(pkt, payload), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_length(pkt), length);
	int cmp = memcmp(pkt_get_payload(pkt), payload, length);
	CU_ASSERT_EQUAL(cmp, 0);

	char* payload2 = "test du payload";
	payload = "verifie qu'on a bien un memcpy"
	cmp = memcmp(pkt_get_payload(pkt), payload, length);
	CU_ASSERT_NOT_EQUAL(cmp, 0);
	int cmp2 = memcmp(pkt_get_payload(pkt), payload2);
	CU_ASSERT_EQUAL(cmp2, 0);

	CU_ASSERT_EQUAL(pkt_set_payload(pkt, NULL, 0), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_length(pkt), 0);

	pkt_del(pkt);
}

int main(){
	/* initialisation du catalogue de test */
	if (CUE_SUCCESS != CU_initialize_registry()){
      return CU_get_error();
    }
    CU_pSuite pSuite = NULL;
	
	/* ajout de la suite de test au catalogue */
	pSuite = CU_add_suite( "Tests de la classe packet_implement", init_suite, clean_suite);
	if ( NULL == pSuite ) {
	  CU_cleanup_registry();
    return CU_get_error();
	}
	
	/* ajout du test a la suite de test */
	if ( (NULL == CU_add_test(pSuite, "Tests des get-set", test_pkt_get_set))
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