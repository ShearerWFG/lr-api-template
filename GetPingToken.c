GetPingToken()
{
	long expires_in;
	char expires_in_char[100];

	lr_start_transaction("PingAuth");
	web_reg_save_param("expires_in", "LB=expires_in\":", "RB=}", LAST);
	web_reg_save_param("access_token", "LB=access_token\":\"", "RB=\",\"", LAST);
	web_submit_data("authorization.ping",
		"Action=https://idpa1-test.westfieldgrp.com/as/token.oauth2",  
		"Method=POST",
		"TargetFrame=",
		"RecContentType=text/html",
		"Snapshot=t25.inf",
		"Mode=HTML",
		ITEMDATA,
		"Name=grant_type", "Value=client_credentials", ENDITEM,
        "Name=access_token_manager_id", "Value=PingFederateJWT", ENDITEM,  //StandardPingFederateJWT
		"Name=client_secret", "Value={client_secret}", ENDITEM,  //Perf id data
		"Name=scope", "Value=api_name:read:write api_name:read", ENDITEM,   //Scope can change with each service
		LAST);
	lr_end_transaction("PingAuth", LR_AUTO);

	expires_in = atoi(lr_eval_string("{expires_in}")) + time(NULL);
	sprintf(expires_in_char, "%d", expires_in);
	
	lr_save_string(lr_eval_string(expires_in_char), "expires_in_epoch");
	

	
	return 0;
}
