// Forward declarations for GET API functions
int GET_Request();

// Forward declarations for POST API functions
int POST_Request();

// Global variables
int HttpRetCode;

// Function pointer type definition
typedef int (*ApiFunction)();

// API function lookup table structure
typedef struct {
    char *name;
    ApiFunction func;
} ApiLookupEntry;

// Lookup table for API functions
ApiLookupEntry apiLookupTable[] = {
    // GET APIs
    {"GET_Request", GET_Request},
    
    // POST APIs
    {"POST_Request", POST_Request},
    
    {NULL, NULL}  // Sentinel to mark end of table
};

Action()
{
    int i;
    char *apiName;
    ApiFunction selectedFunc = NULL;
    
    apiName = lr_eval_string("{api_name}"); // The method in which the {api_name} parameter is set will determine the test scenario
    
    // Search lookup table for matching API name
    for (i = 0; apiLookupTable[i].name != NULL; i++) {
        if (strcmp(apiName, apiLookupTable[i].name) == 0) {
            selectedFunc = apiLookupTable[i].func;
            break;
        }
    }
    
    // Execute the function if found
    if (selectedFunc != NULL) {
        selectedFunc();
    }
    else {
        lr_output_message("Invalid or unknown API name: %s", apiName);
    }
    
    return 0;
}

POST_Request(){
	POST_Request:
	if (time(NULL)<atol(lr_eval_string("{expires_in_epoch}"))) {

		
	       web_add_header("Authorization", "Bearer {access_token}");	        
	       web_add_header("x-api-key", "{api_key}");  //API Key
	       web_add_header("Content-Type", "application/json");  //Must add Header of Content-Type for Mulesoft. If Body is using Json, use application/json
       
		sprintf(buffer, "api_name");
	
		lr_start_transaction(buffer);

	    //web_reg_find("Text=\"text string example from valid response goes here\"", LAST);        
		web_custom_request("api_name_function",
			"URL={host_name}{endpoint}", //{host_name} - https://gravitee-perf-gw.apps.svarakrnopnshftapi.westfieldgrp.corp {endpoint} - /posts/1
			"Method=POST",
			"Resource=0",
			"RecContentType=application/json",
			"Mode=HTML",
			"BODY={json_body}", //add body for POST request
			LAST);	
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
		goto POST_Request;
	}
	
	lr_think_time(pause);	
    return 0;
}

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
