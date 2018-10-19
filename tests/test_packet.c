#include <stdlib.h>
#include <stdio.h>
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <string.h>
#include <>

#include "packet_implement.h"

/* test suites */
int init_suite(void){
	return 0;
}

int clean_suite(void){
	return 0;
}

void test_pkt_get_set(){
	pkt_t* pkt = pkt_new();

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

void test_encode_decode(){
	pkt_t* pkt = pkt_new();
	CU_ASSERT_PTR_NOT_EQUAL(pkt, NULL);

	/*
	 * type: 1
	 * tr: 0
	 * seqnum: 123
	 * window: 10
	 * payload: "decode" 
	 * length: strlen(payload)
	 */

	CU_ASSERT_EQUAL(pkt_set_type(pkt, 1), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_tr(pkt, 0), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_seqnum(pkt, 123), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_window(pkt, 10), PKT_OK);
	char* payload = "decode"
	int length = strlen(decode);
	CU_ASSERT_EQUAL(pkt_set_payload(pkt, payload), PKT_OK);

	CU_ASSERT_EQUAL(pkt_set_timestamp(pkt, 123456), PKT_OK);

	size_t len_buffer = 50;
	size_t wrong = 10;
	char buffer[len_buffer]
	CU_ASSERT_EQUAL(pkt_encode(pkt, buffer, &wrong), E_NOMEM);
	CU_ASSERT_EQUAL(pkt_encore(pkt, buffer, &len_buffer), PKT_OK);

	pkt_t* pkt_verif = pkt_new();

	CU_ASSERT_EQUAL(pkt_decode(buffer, len_buffer, NULL), E_UNCONSISTENT);
	CU_ASSERT_EQUAL(pkt_decode(buffer, len_buffer, pkt_verif), PKT_OK);

	CU_ASSERT_EQUAL(pkt_get_type(pkt), 1);
	CU_ASSERT_EQUAL(pkt_get_tr(pkt), 0);
	CU_ASSERT_EQUAL(pkt_get_seqnum(pkt), 123);
	CU_ASSERT_EQUAL(pkt_get_window(pkt), 10);
	CU_ASSERT_EQUAL(pkt_get_length(pkt), length);
	int cmp = memcpy(buffer, pkt_get_payload(pkt));
	CU_ASSERT_EQUAL(cmp, 0);
	CU_ASSERT_EQUAL(pkt_get_timestamp(pkt), 123456);

	pkt_del(pkt);
	pkt_del(pkt_verif);

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
		|| (NULL == CU_add_test(pSuite, "Tests de encode-decode", test_encode_decode))
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