GET_Request(){
	GET_Request:
	if (time(NULL)<atol(lr_eval_string("{expires_in_epoch}"))) {

		
	       web_add_header("Authorization", "Bearer {access_token}");	        
	       web_add_header("x-api-key", "{api_key}");  //API Key
	       web_add_header("Content-Type", "application/json");  //Must add Header of Content-Type for Mulesoft. If Body is using Json, use application/json
       
		sprintf(buffer, "api_name");
	
		lr_start_transaction(buffer);

	    //web_reg_find("Text=\"text string example from valid response goes here\"", LAST);        
		web_url("api_name_function",
			"URL={host_name}{endpoint}", //{env} - https://gravitee-perf-gw.apps.svarakrnopnshftapi.westfieldgrp.corp {endpoint} - /users/{userId}/accounts
			//"Method=GET",
			"Resource=0",
			"RecContentType=application/json", //text/html
			"Mode=HTML",
            LAST );	
		HttpRetCode = web_get_int_property(HTTP_INFO_RETURN_CODE);
		lr_output_message("Return Code = %d", HttpRetCode);

		if (HttpRetCode == 200){
			lr_output_message("Request passed with inputs: %s %s", lr_eval_string("{param_name_1}"), lr_eval_string("{param_name_2}")); 
			lr_end_transaction(buffer, LR_PASS);
		}
		else if (HttpRetCode == 204)
			lr_end_transaction(buffer, LR_PASS);
		else {
			lr_output_message("Request failed with inputs: %s %s", lr_eval_string("{param_name_1}"), lr_eval_string("{param_name_2}")); 
			lr_end_transaction(buffer, LR_FAIL);
		}
	}
	else{
		GetPingToken();
		goto GET_Request;
	}
	
	lr_think_time(pause);	
    return 0;
}
