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
- **Action.c**: API function lookup table and routing logic only
- **GET.c**: All GET request function implementations
- **POST.c**: All POST request function implementations
- **GetPingToken.c**: Authentication/token management
- **vuser_end.c**: Cleanup operations
- **api_script_template.prm**: Parameter configuration file (defines how .dat files are read)
- **api_name.dat**: API names and HTTP methods (CSV format)
- **host_name.dat**: Base URLs and environment names (CSV format)
- **user.dat**: Test user data (optional, CSV format)
- **Additional .dat files**: Custom parameter data as needed

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

#### 2. Action.c - API Function Lookup System

**IMPORTANT**: Action.c contains ONLY the lookup table and routing logic. All GET functions are implemented in GET.c and all POST functions in POST.c.

**IMPORTANT**: Always implement the lookup table pattern with forward declarations. This allows dynamic API selection based on the `{api_name}` parameter.

```c
// Forward declarations for GET API functions (from GET.c)
int GET_All_Users();
int GET_User_By_ID();

// Forward declarations for POST API functions (from POST.c)
int POST_Create_Post();
int POST_Update_User();

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
    {"GET_All_Users", GET_All_Users},
    {"GET_User_By_ID", GET_User_By_ID},
    
    // POST APIs
    {"POST_Create_Post", POST_Create_Post},
    {"POST_Update_User", POST_Update_User},
    
    {NULL, NULL}  // Sentinel to mark end of table
};

Action()
{
    int i;
    char *apiName;
    ApiFunction selectedFunc = NULL;
    
    apiName = lr_eval_string("{api_name}"); // The {api_name} parameter determines which API to execute
    
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
```

**Key Points**:
- Add forward declarations at the top for all API functions from both GET.c and POST.c files
- Action.c contains ONLY the lookup table and Action() function - no request implementations
- Each API gets its own function (e.g., `GET_All_Users()`, `POST_Create_Post()`) in the appropriate file
- All GET functions are implemented in GET.c
- All POST functions are implemented in POST.c
- Function names in the lookup table must match exactly with the `{api_name}` parameter value
- The lookup table enables data-driven testing where `{api_name}` can be parameterized
- Comments in lookup table can include load distribution notes (e.g., `//25vu` for virtual user allocation)

#### 3. GET.c - GET Request Functions

**Function Naming**: Use descriptive names based on the API operation (e.g., `GET_All_Users()`, `GET_User_By_ID()`)

**File Location**: All GET request implementations go in GET.c

```c
GET_All_Users(){
    GET_All_Users:
    // Token expiration check
    if (time(NULL)<atol(lr_eval_string("{expires_in_epoch}"))) {
        
        // Add headers
        web_add_header("Authorization", "Bearer {access_token}");
        web_add_header("x-api-key", "{api_key}");
        web_add_header("Content-Type", "application/json");
       
        // Set transaction name
        sprintf(buffer, "GET_All_Users");
        
        // Start transaction
        lr_start_transaction(buffer);

        // Optional: Response validation
        //web_reg_find("Text=\"expected response text\"", LAST);
        
        // Execute GET request
        web_url("GET_All_Users",
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
        goto GET_All_Users;  // Retry request
    }
    
    lr_think_time(pause);
    return 0;
}
```

#### 4. POST.c - POST Request Functions

**Function Naming**: Use descriptive names based on the API operation (e.g., `POST_Create_Post()`, `POST_Update_User()`)

**File Location**: All POST request implementations go in POST.c

```c
POST_Create_Post(){
    POST_Create_Post:
    // Token expiration check
    if (time(NULL)<atol(lr_eval_string("{expires_in_epoch}"))) {
        
        // Add headers
        web_add_header("Authorization", "Bearer {access_token}");
        web_add_header("x-api-key", "{api_key}");
        web_add_header("Content-Type", "application/json");
       
        // Set transaction name
        sprintf(buffer, "POST_Create_Post");
        
        // Start transaction
        lr_start_transaction(buffer);

        // Optional: Response validation
        //web_reg_find("Text=\"expected response text\"", LAST);
        
        // Execute POST request with formatted JSON body
        web_custom_request("POST_Create_Post",
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
        goto POST_Create_Post;
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

## LoadRunner Parameter Files (.dat and .prm)

### Overview
LoadRunner uses two file types for data-driven testing:
- **.dat files**: CSV data tables containing parameter values
- **.prm file**: Configuration file that defines how to read .dat files

### Parameter File (.prm) Structure

The .prm file defines parameter behavior with these key settings:

```ini
[parameter:parameter_name]
ColumnName="column_name"           # Which column to read from .dat file
Delimiter=","                      # Column separator (comma for CSV)
GenerateNewVal="EachIteration"     # When to get new value: EachIteration, EachOccurrence, Once
OriginalValue=""                   # Default/initial value
OutOfRangePolicy="ContinueWithLast" # What to do when data runs out
ParamName="parameter_name"         # Parameter name used in script {parameter_name}
SelectNextRow="Sequential"         # How to iterate: Sequential, Random, Unique, "Same line as X"
StartRow="1"                       # Which row to start reading from
Table="filename.dat"               # Which .dat file contains the data
TableLocation="Local"              # Where the file is located
Type="Table"                       # Parameter type: Table, Userid, etc.
auto_allocate_block_size="1"       # Block allocation for distributed testing
value_for_each_vuser=""            # Specific value assignment
```

### Data File (.dat) Structure

.dat files are CSV format with:
- **First row**: Column headers
- **Subsequent rows**: Data values
- **Delimiter**: Comma (,)

**Example - api_name.dat:**
```csv
api_name,method
POST_methodName,POST
GET_methodName,GET
```

**Example - host_name.dat:**
```csv
host_name,env
https://api.example.com,PROD
https://test-api.example.com,TEST
```

### Parameter Correlation: .prm ↔ .dat

#### Example 1: Basic Parameter
**.prm Configuration:**
```ini
[parameter:api_name]
ColumnName="api_name"
Table="api_name.dat"
SelectNextRow="Sequential"
```

**.dat File (api_name.dat):**
```csv
api_name,method
POST_Create_User,POST
GET_User_By_ID,GET
```

**Result:** `{api_name}` will be `POST_Create_User` in iteration 1, `GET_User_By_ID` in iteration 2

#### Example 2: Synchronized Parameters (Same Line)
**.prm Configuration:**
```ini
[parameter:api_name]
ColumnName="api_name"
Table="api_name.dat"
SelectNextRow="Sequential"

[parameter:method]
ColumnName="method"
Table="api_name.dat"
SelectNextRow="Same line as api_name"
```

**.dat File (api_name.dat):**
```csv
api_name,method
POST_Create_User,POST
GET_User_By_ID,GET
```

**Result:** When `{api_name}` = `POST_Create_User`, `{method}` = `POST` (from same row)

### Standard Parameters in Template

#### 1. api_name Parameter
- **Purpose**: Determines which API function to execute via lookup table
- **File**: api_name.dat
- **Column**: api_name
- **Usage**: Used in Action.c to select function from lookup table

#### 2. method Parameter
- **Purpose**: HTTP method type (GET, POST, PUT, DELETE)
- **File**: api_name.dat (same file as api_name)
- **Column**: method
- **Synchronization**: `SelectNextRow="Same line as api_name"`
- **Usage**: Can be used for logging or validation

#### 3. host_name Parameter
- **Purpose**: API base URL
- **File**: host_name.dat
- **Column**: host_name
- **Usage**: `"URL={host_name}{endpoint}"`

#### 4. vuser Parameter
- **Purpose**: Virtual user ID
- **Type**: Built-in LoadRunner parameter (Userid)
- **No .dat file needed**: Automatically generated by LoadRunner

### Adding Custom Parameters from Postman Collections

When a Postman collection contains additional variables or path parameters, add them to the .prm file:

#### Step 1: Identify Postman Variables
From Postman collection:
```json
{
  "variable": [
    {"key": "userId", "value": "123"},
    {"key": "accountId", "value": "456"},
    {"key": "apiKey", "value": "secret123"}
  ]
}
```

Or from URL path variables:
```
{{baseUrl}}/users/{{userId}}/accounts/{{accountId}}
```

#### Step 2: Create .dat File for New Parameters

**Example - user_params.dat:**
```csv
userId,accountId
123,456
789,012
345,678
```

**Example - api_credentials.dat:**
```csv
api_key,client_secret
key123abc,secret456def
key789xyz,secret012uvw
```

#### Step 3: Add Parameter Configuration to .prm File

**For path parameters (userId, accountId):**
```ini
[parameter:userId]
ColumnName="userId"
Delimiter=","
GenerateNewVal="EachIteration"
OriginalValue=""
OutOfRangePolicy="ContinueWithLast"
ParamName="userId"
SelectNextRow="Sequential"
StartRow="1"
Table="user_params.dat"
TableLocation="Local"
Type="Table"
auto_allocate_block_size="1"
value_for_each_vuser=""

[parameter:accountId]
ColumnName="accountId"
Delimiter=","
GenerateNewVal="EachIteration"
OriginalValue=""
OutOfRangePolicy="ContinueWithLast"
ParamName="accountId"
SelectNextRow="Same line as userId"
StartRow="1"
Table="user_params.dat"
TableLocation="Local"
Type="Table"
auto_allocate_block_size="1"
value_for_each_vuser=""
```

**For authentication parameters:**
```ini
[parameter:api_key]
ColumnName="api_key"
Delimiter=","
GenerateNewVal="Once"
OriginalValue=""
OutOfRangePolicy="ContinueWithLast"
ParamName="api_key"
SelectNextRow="Sequential"
StartRow="1"
Table="api_credentials.dat"
TableLocation="Local"
Type="Table"
auto_allocate_block_size="1"
value_for_each_vuser=""
```

#### Step 4: Use Parameters in Script

In GET.c or POST.c:
```c
web_url("GET_User_Account",
    "URL={host_name}/users/{userId}/accounts/{accountId}",
    "Resource=0",
    "RecContentType=application/json",
    "Mode=HTML",
    LAST);
```

### Parameter Configuration Best Practices

1. **GenerateNewVal Options:**
   - `EachIteration` - New value every iteration (most common)
   - `EachOccurrence` - New value each time parameter is used
   - `Once` - Same value for entire test (e.g., credentials)

2. **SelectNextRow Options:**
   - `Sequential` - Go through rows in order
   - `Random` - Pick random rows
   - `Unique` - Each vuser gets different values (no repeats)
   - `Same line as [parameter]` - Synchronize with another parameter

3. **OutOfRangePolicy Options:**
   - `ContinueWithLast` - Keep using last value when data runs out
   - `AbortVuser` - Stop vuser when data exhausted
   - `ContinueCyclic` - Loop back to first row

4. **File Organization:**
   - Group related parameters in same .dat file
   - Use "Same line as" to keep related data synchronized
   - Separate credentials from test data for security

### Parameter Naming Conventions

- Use descriptive names matching Postman variables
- Use camelCase or snake_case consistently
- Prefix parameters by type if helpful:
  - `user_userId`, `user_userName` for user data
  - `api_endpoint`, `api_version` for API config
  - `auth_token`, `auth_key` for credentials

## Conversion Checklist

When converting a new Postman collection, follow these steps:

### Phase 1: Extract Collection Variables
- [ ] Identify all collection-level variables (`variable[]`)
- [ ] Map to LoadRunner parameters:
  - `baseUrl` → `{host_name}`
  - `apiKey` → `{api_key}`
  - Custom variables → `{variable_name}`
- [ ] Create/update .dat files for new parameters
- [ ] Add parameter definitions to .prm file with appropriate settings
- [ ] Use "Same line as" for related parameters that should stay synchronized

### Phase 2: Analyze Each Request
For each `item[]` in the collection:
- [ ] Extract request name → Use as transaction name
- [ ] Extract method (GET, POST, PUT, DELETE)
- [ ] Extract URL path → Store as `{endpoint}` or build complete URL
- [ ] Extract headers → Convert to `web_add_header()` calls
- [ ] Extract body (if POST/PUT) → **Copy the actual JSON and format it with proper indentation**
- [ ] Identify path variables → Replace `{{var}}` with `{var}`
- [ ] List all unique parameters used across all requests
- [ ] Determine which parameters should be synchronized (same row in .dat file)

### Phase 3: Implement LoadRunner Script
- [ ] Add forward declarations for all API functions at the top of Action.c with file comments (from GET.c / from POST.c)
- [ ] Create API function lookup table with function pointers in Action.c
- [ ] Implement Action() with lookup table search logic in Action.c
- [ ] Implement GET request functions in GET.c following the pattern
- [ ] Implement POST request functions in POST.c following the pattern
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

### Phase 6: Parameter Configuration
- [ ] Update api_name.dat with all API function names and methods
- [ ] Create .dat files for any new parameters from Postman collection
- [ ] Update .prm file with parameter definitions for new parameters
- [ ] Set appropriate GenerateNewVal, SelectNextRow, and OutOfRangePolicy
- [ ] Use "Same line as" to synchronize related parameters
- [ ] Test parameter values are correctly loaded in VuGen

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
13. **CRITICAL: Always implement the lookup table pattern with forward declarations in Action.c**
14. **Add all API functions to the lookup table in Action.c**
15. **CRITICAL: Implement GET functions in GET.c file, POST functions in POST.c file**
16. **Action.c contains ONLY the lookup table and routing - NO request implementations**
17. **CRITICAL: When Postman collection has additional parameters/variables:**
    - Create appropriate .dat files with CSV format (headers in first row)
    - Add parameter configuration blocks to .prm file
    - Use "Same line as" for parameters that should be synchronized
    - Set appropriate GenerateNewVal, SelectNextRow, and OutOfRangePolicy values

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
