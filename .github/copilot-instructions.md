# Copilot Instructions: Postman Collection to LoadRunner Conversion

## Project Overview
This workspace serves as a template for converting Postman collections into LoadRunner C-language test scripts for performance testing. The conversion process involves extracting API details from Postman collections and implementing them as LoadRunner web API calls with proper transaction management, error handling, and authentication.

## Critical Elements to Extract from Postman Collections

### 1. Request Method
- **Location in Postman**: `item[].request.method`
- **LoadRunner Implementation**: 
  - `GET` → Use `web_url()` function
  - `POST` → Use `web_custom_request()` with `"Method=POST"`
  - `PUT` → Use `web_custom_request()` with `"Method=PUT"`
  - `DELETE` → Use `web_custom_request()` with `"Method=DELETE"`

### 2. Host Name / Base URL
- **Location in Postman**: `variable[].key="baseUrl"` or `item[].request.url.host`
- **LoadRunner Implementation**: Store in `{host_name}` parameter
- **Format**: Full URL including protocol (e.g., `https://api.example.com`)

### 3. Endpoint / Path
- **Location in Postman**: `item[].request.url.path[]` array joined with `/`
- **LoadRunner Implementation**: Store in `{endpoint}` parameter
- **Format**: Path starting with `/` (e.g., `/users/1` or `/posts`)
- **Note**: Replace Postman variables `{{variable}}` with LoadRunner parameters `{variable}`

### 4. API Name / Transaction Name
- **Location in Postman**: `item[].name`
- **LoadRunner Implementation**: 
  - Use for transaction name in `lr_start_transaction()`
  - Use as function name in `web_custom_request()` or `web_url()`
  - Store in buffer variable: `sprintf(buffer, "api_name");`
- **Naming Convention**: Use descriptive names (e.g., "Get_All_Users", "Create_New_Post")

### 5. Headers
- **Location in Postman**: `item[].request.header[]`
- **LoadRunner Implementation**: Use `web_add_header()` before the request
- **Common Headers**:
  - `Authorization`: `web_add_header("Authorization", "Bearer {access_token}");`
  - `x-api-key`: `web_add_header("x-api-key", "{api_key}");`
  - `Content-Type`: `web_add_header("Content-Type", "application/json");`
- **Note**: Convert Postman `{{variables}}` to LoadRunner `{parameters}`

### 6. Request Body
- **Location in Postman**: `item[].request.body.raw`
- **LoadRunner Implementation**: 
  - For POST/PUT: Include JSON body directly in `"BODY="` parameter in `web_custom_request()`
  - **IMPORTANT**: Extract the actual JSON from Postman collection and embed it directly in the C code
  - Use multi-line string concatenation for readability
  - Escape all double quotes with backslashes
  - Format JSON with proper indentation across multiple lines using C string concatenation
  - **DO NOT** use parameter placeholders like `{json_body}` - use the actual JSON content

### 7. Response Content Type
- **Location in Postman**: `item[].request.header[].key="Content-Type"` or response headers
- **LoadRunner Implementation**: `"RecContentType=application/json"` or `"RecContentType=text/html"`

### 8. URL Parameters / Path Variables
- **Location in Postman**: `{{variable}}` in URL path
- **LoadRunner Implementation**: Replace with `{parameter_name}` in URL string
- **Example**: `{{userId}}` → `{userId}` in `"URL={host_name}/users/{userId}"`

## LoadRunner Script Structure

### File Organization
- **vuser_init.c**: Initialization, SSL setup, initial authentication
- **Action.c**: Main test logic with method routing and request functions
- **GetPingToken.c**: Authentication/token management
- **vuser_end.c**: Cleanup operations

### Standard Implementation Pattern

#### 1. vuser_init.c
```c
int pause=72;  // Think time in seconds
long file_stream;
char buffer[50];

vuser_init()
{
    web_set_max_html_param_len("1000");
    web_set_sockets_option("SSL_VERSION", "TLS1.2");
    GetPingToken();  // Initial authentication
    return 0;
}
```

#### 2. Action.c - Method Router
```c
int HttpRetCode;

Action()
{
    if (method == "POST"){
        POST_Request();
    }
    else if(method == "GET"){
        GET_Request();   
    }
    else{
        lr_output_message("Invalid method type. Please use POST or GET.");
    }
    return 0;
}
```

#### 3. Request Function Pattern (GET Example)
```c
GET_Request(){
    GET_Request:
    // Token expiration check
    if (time(NULL)<atol(lr_eval_string("{expires_in_epoch}"))) {
        
        // Add headers
        web_add_header("Authorization", "Bearer {access_token}");
        web_add_header("x-api-key", "{api_key}");
        web_add_header("Content-Type", "application/json");
       
        // Set transaction name
        sprintf(buffer, "api_name");
        
        // Start transaction
        lr_start_transaction(buffer);

        // Optional: Response validation
        //web_reg_find("Text=\"expected response text\"", LAST);
        
        // Execute GET request
        web_url("api_name_function",
            "URL={host_name}{endpoint}",
            "Resource=0",
            "RecContentType=application/json",
            "Mode=HTML",
            LAST);
            
        // Get HTTP return code
        HttpRetCode = web_get_int_property(HTTP_INFO_RETURN_CODE);
        lr_output_message("Return Code = %d", HttpRetCode);

        // Evaluate response
        if (HttpRetCode == 200){
            lr_output_message("Request passed with inputs: %s %s", 
                lr_eval_string("{param_name_1}"), lr_eval_string("{param_name_2}")); 
            lr_end_transaction(buffer, LR_PASS);
        }
        else if (HttpRetCode == 204)
            lr_end_transaction(buffer, LR_PASS);
        else {
            lr_output_message("Request failed with inputs: %s %s", 
                lr_eval_string("{param_name_1}"), lr_eval_string("{param_name_2}")); 
            lr_end_transaction(buffer, LR_FAIL);
        }
    }
    else{
        GetPingToken();  // Refresh token
        goto GET_Request;  // Retry request
    }
    
    lr_think_time(pause);
    return 0;
}
```

#### 4. Request Function Pattern (POST Example)
```c
POST_Request(){
    POST_Request:
    // Token expiration check
    if (time(NULL)<atol(lr_eval_string("{expires_in_epoch}"))) {
        
        // Add headers
        web_add_header("Authorization", "Bearer {access_token}");
        web_add_header("x-api-key", "{api_key}");
        web_add_header("Content-Type", "application/json");
       
        // Set transaction name
        sprintf(buffer, "api_name");
        
        // Start transaction
        lr_start_transaction(buffer);

        // Optional: Response validation
        //web_reg_find("Text=\"expected response text\"", LAST);
        
        // Execute POST request with formatted JSON body
        web_custom_request("api_name_function",
            "URL={host_name}{endpoint}",
            "Method=POST",
            "Resource=0",
            "RecContentType=application/json",
            "Mode=HTML",
            "BODY="
            "{"
            "  \"field1\": \"value1\","
            "  \"field2\": \"value2\","
            "  \"nestedObject\": {"
            "    \"nestedField\": \"nestedValue\""
            "  }"
            "}",
            LAST);
            
        // Get HTTP return code and evaluate (same as GET)
        HttpRetCode = web_get_int_property(HTTP_INFO_RETURN_CODE);
        lr_output_message("Return Code = %d", HttpRetCode);

        if (HttpRetCode == 200 || HttpRetCode == 201){
            lr_output_message("Request passed with inputs: %s %s", 
                lr_eval_string("{param_name_1}"), lr_eval_string("{param_name_2}")); 
            lr_end_transaction(buffer, LR_PASS);
        }
        else if (HttpRetCode == 204)
            lr_end_transaction(buffer, LR_PASS);
        else {
            lr_output_message("Request failed with inputs: %s %s", 
                lr_eval_string("{param_name_1}"), lr_eval_string("{param_name_2}")); 
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
```

#### 5. Authentication Pattern (GetPingToken.c)
```c
GetPingToken()
{
    long expires_in;
    char expires_in_char[100];

    lr_start_transaction("PingAuth");
    
    // Capture response parameters
    web_reg_save_param("expires_in", "LB=expires_in\":", "RB=}", LAST);
    web_reg_save_param("access_token", "LB=access_token\":\"", "RB=\",\"", LAST);
    
    // Submit authentication request
    web_submit_data("authorization.ping",
        "Action=https://idpa1-test.westfieldgrp.com/as/token.oauth2",
        "Method=POST",
        "TargetFrame=",
        "RecContentType=text/html",
        "Snapshot=t25.inf",
        "Mode=HTML",
        ITEMDATA,
        "Name=grant_type", "Value=client_credentials", ENDITEM,
        "Name=access_token_manager_id", "Value=PingFederateJWT", ENDITEM,
        "Name=client_secret", "Value={client_secret}", ENDITEM,
        "Name=scope", "Value=api_name:read:write api_name:read", ENDITEM,
        LAST);
        
    lr_end_transaction("PingAuth", LR_AUTO);

    // Calculate token expiration epoch time
    expires_in = atoi(lr_eval_string("{expires_in}")) + time(NULL);
    sprintf(expires_in_char, "%d", expires_in);
    lr_save_string(lr_eval_string(expires_in_char), "expires_in_epoch");
    
    return 0;
}
```

## Conversion Checklist

When converting a new Postman collection, follow these steps:

### Phase 1: Extract Collection Variables
- [ ] Identify all collection-level variables (`variable[]`)
- [ ] Map to LoadRunner parameters:
  - `baseUrl` → `{host_name}`
  - `apiKey` → `{api_key}`
  - Custom variables → `{variable_name}`

### Phase 2: Analyze Each Request
For each `item[]` in the collection:
- [ ] Extract request name → Use as transaction name
- [ ] Extract method (GET, POST, PUT, DELETE)
- [ ] Extract URL path → Store as `{endpoint}` or build complete URL
- [ ] Extract headers → Convert to `web_add_header()` calls
- [ ] Extract body (if POST/PUT) → **Copy the actual JSON and format it with proper indentation**
- [ ] Identify path variables → Replace `{{var}}` with `{var}`

### Phase 3: Implement LoadRunner Script
- [ ] Create/update Action.c with method router
- [ ] Implement request function(s) following the pattern
- [ ] Add proper transaction management (`lr_start_transaction`/`lr_end_transaction`)
- [ ] Add token expiration check and refresh logic
- [ ] Add HTTP return code validation
- [ ] Add appropriate logging with `lr_output_message()`
- [ ] Add think time with `lr_think_time(pause)`

### Phase 4: Error Handling
- [ ] Define success status codes (200, 204, etc.)
- [ ] Add error logging with input parameters
- [ ] Implement transaction pass/fail logic
- [ ] Add optional response validation with `web_reg_find()`

### Phase 5: Authentication
- [ ] Update GetPingToken.c if auth mechanism differs
- [ ] Extract required scope from API documentation
- [ ] Configure token expiration handling

## Parameter Naming Conventions

### Standard Parameters
- `{host_name}` - Base URL/hostname
- `{endpoint}` - API endpoint path
- `{api_key}` - API key for authentication
- `{access_token}` - OAuth/Bearer token
- `{expires_in_epoch}` - Token expiration time (Unix epoch)
- `{json_body}` - Request body for POST/PUT
- `{client_secret}` - OAuth client secret

### Custom Parameters
- `{param_name_1}`, `{param_name_2}` - Input parameters for logging
- `{userId}`, `{accountId}` - Path/query parameters
- Use descriptive names matching Postman variables

## Common LoadRunner Functions

### Request Functions
- `web_url()` - For GET requests
- `web_custom_request()` - For POST, PUT, DELETE requests
- `web_submit_data()` - For form data submissions
- `web_add_header()` - Add HTTP headers

### Transaction Management
- `lr_start_transaction("name")` - Begin transaction timing
- `lr_end_transaction("name", LR_PASS/LR_FAIL)` - End with status

### Data Handling
- `lr_eval_string("{param}")` - Evaluate parameter value
- `lr_save_string("value", "param_name")` - Save value to parameter
- `web_reg_save_param("param", "LB=", "RB=", LAST)` - Extract from response

### Logging & Debugging
- `lr_output_message("message %s", variable)` - Log message
- `web_get_int_property(HTTP_INFO_RETURN_CODE)` - Get HTTP status code
- `web_reg_find("Text=\"expected\"", LAST)` - Validate response content

### Timing
- `lr_think_time(seconds)` - Add delay between requests
- `time(NULL)` - Get current Unix epoch time

## Best Practices

1. **Always include token expiration check** before API calls
2. **Use goto labels** for retry logic after token refresh
3. **Log both success and failure** with relevant input parameters
4. **Use descriptive transaction names** matching business operations
5. **Set appropriate RecContentType** (application/json, text/html)
6. **Include Resource=0** to avoid treating response as downloadable
7. **Add think time** at the end of request functions for realistic load
8. **Validate critical responses** with web_reg_find() when needed
9. **Handle multiple success codes** (200, 201, 204) appropriately
10. **Use sprintf() for buffer management** when building transaction names
11. **Format JSON bodies with proper indentation** using C string concatenation for readability
12. **Extract actual JSON from Postman** - don't use generic parameter placeholders
13. **Use multi-line BODY strings** with escaped quotes for maintainability

## Example Conversion

### Postman Request
```json
{
  "name": "Create New Post",
  "request": {
    "method": "POST",
    "header": [
      {"key": "Content-Type", "value": "application/json"},
      {"key": "Authorization", "value": "Bearer {{apiKey}}"}
    ],
    "body": {
      "mode": "raw",
      "raw": "{\"title\": \"Test\", \"body\": \"Content\", \"userId\": 1}"
    },
    "url": {
      "raw": "{{baseUrl}}/posts",
      "host": ["{{baseUrl}}"],
      "path": ["posts"]
    }
  }
}
```

### LoadRunner Implementation
```c
POST_Create_New_Post(){
    POST_Create_New_Post:
    if (time(NULL)<atol(lr_eval_string("{expires_in_epoch}"))) {
        
        web_add_header("Authorization", "Bearer {access_token}");
        web_add_header("x-api-key", "{api_key}");
        web_add_header("Content-Type", "application/json");
       
        sprintf(buffer, "Create_New_Post");
        lr_start_transaction(buffer);
        
        web_custom_request("Create_New_Post",
            "URL={host_name}/posts",
            "Method=POST",
            "Resource=0",
            "RecContentType=application/json",
            "Mode=HTML",
            "BODY="
            "{"
            "  \"title\": \"Test\","
            "  \"body\": \"Content\","
            "  \"userId\": 1"
            "}",
            LAST);
            
        HttpRetCode = web_get_int_property(HTTP_INFO_RETURN_CODE);
        lr_output_message("Return Code = %d", HttpRetCode);

        if (HttpRetCode == 200 || HttpRetCode == 201){
            lr_output_message("Post created successfully");
            lr_end_transaction(buffer, LR_PASS);
        }
        else {
            lr_output_message("Post creation failed");
            lr_end_transaction(buffer, LR_FAIL);
        }
    }
    else{
        GetPingToken();
        goto POST_Create_New_Post;
    }
    
    lr_think_time(pause);
    return 0;
}
```

## Notes for AI Assistant

When processing a new Postman collection:
1. Parse the JSON structure to extract all requests
2. For each request, identify the method and create appropriate function
3. Convert all Postman `{{variables}}` to LoadRunner `{parameters}`
4. Generate proper header additions before each request
5. Use the correct LoadRunner function based on HTTP method
6. **Extract the actual JSON body from Postman and format it properly with indentation**
7. **Use C string concatenation to format JSON across multiple lines for readability**
8. **Escape all double quotes in JSON with backslashes**
9. Include complete error handling and logging
10. Maintain consistent naming conventions
11. Add token management and retry logic
12. Follow the established patterns from existing template files

### JSON Body Formatting Guidelines

When converting POST/PUT requests with JSON bodies:
- Extract the complete JSON from `item[].request.body.raw`
- Format the JSON with proper indentation (2 spaces per level)
- Use C string concatenation syntax with each line as a separate string
- Escape all double quotes with backslashes (`\"`)
- Start BODY parameter with `"BODY="` followed by opening quote and brace
- Each subsequent line should be a quoted string on its own line
- Maintain proper JSON structure with commas and braces
- End with closing brace, quote, and comma before `LAST);`

Example formatting:
```c
"BODY="
"{"
"  \"field\": \"value\","
"  \"nested\": {"
"    \"subfield\": \"subvalue\""
"  }"
"}",
LAST);
```
