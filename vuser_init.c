int pause=72;
long file_stream;
char buffer[50];

vuser_init()
{
	
	web_set_max_html_param_len("1000");

	web_set_sockets_option("SSL_VERSION", "TLS1.2");
	GetPingToken();
	
	return 0;
}