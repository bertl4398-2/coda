/** 
 * Copyright (c) Francisco de Meneses Neves Ramos dos Santos
 * Email: francisco.santos@epfl.ch
 * Date: 7/9/2012
 */

#include <ccn/ccn.h>
 
/**
 * FRDS: Encode the two statistics parameters, request rate and delivery rate, into
 * ccnb code.
 */

int ccn_encode_statistics(struct ccn_charbuf* c, int req_rate,
		int delivery_rate) {
	int res = 0;
	res |= ccn_charbuf_append_tt(c, CCN_DTAG_IntegerValue, CCN_DTAG);
	res |= ccn_charbuf_append_tt(c, 4, CCN_BLOB);
	res |= ccn_charbuf_append_value(c, req_rate, 4);
	res |= ccn_charbuf_append_closer(c);
	res |= ccn_charbuf_append_tt(c, CCN_DTAG_IntegerValue, CCN_DTAG);
	res |= ccn_charbuf_append_tt(c, 4, CCN_BLOB);
	res |= ccn_charbuf_append_value(c, delivery_rate, 4);
	res |= ccn_charbuf_append_closer(c);
	return res;
}

/**
 * FRDS: Returns the request rate of the content object.
 * @returns request-rate of the content object (-1 if there is an error)
 */

int ccn_request_rate(const unsigned char *msg,
		const struct ccn_parsed_ContentObject *po) {
	struct ccn_buf_decoder decoder;
	struct ccn_buf_decoder* d = &decoder;
	size_t start = po->offset[CCN_PCO_B_ParamA];
	size_t end = po->offset[CCN_PCO_E_ParamA];
	int res = 0;
	d = ccn_buf_decoder_start(d, msg + start, end - start);
	res = ccn_parse_optional_tagged_binary_number(d, CCN_DTAG_IntegerValue, 4,
			4, -1);
	return res;

}

/**
 * FRDS: Returns the delivery rate of the content object.
 * @returns delivery-rate of the content object (-1 if there is an error)
 */
int ccn_delivery_rate(const unsigned char *msg,
		const struct ccn_parsed_ContentObject *po) {
	struct ccn_buf_decoder decoder;
	struct ccn_buf_decoder* d = &decoder;
	size_t start = po->offset[CCN_PCO_B_ParamB];
	size_t end = po->offset[CCN_PCO_E_ParamB];
	int res = 0;
	d = ccn_buf_decoder_start(d, msg + start, end - start);
	res = ccn_parse_optional_tagged_binary_number(d, CCN_DTAG_IntegerValue, 4,
			4, -1);
	return res;
}
