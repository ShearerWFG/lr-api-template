# Postman to LoadRunner Conversion Template

A production-ready template for converting Postman API collections into LoadRunner C-language performance test scripts. This template provides a standardized framework for implementing API performance tests with proper transaction management, authentication, and error handling.

## 🎯 Purpose

This project serves as a reusable template for performance testing teams to quickly convert Postman collections into LoadRunner scripts. It eliminates the need to recreate common patterns like authentication, transaction management, and error handling for each new API test.

## 📋 Features

- **Method-based routing** - Automatic handling of GET, POST, PUT, and DELETE requests
- **Token authentication** - Built-in OAuth2/Bearer token management with automatic refresh
- **Transaction management** - Proper transaction timing with pass/fail evaluation
- **Error handling** - Comprehensive HTTP status code validation and logging
- **Think time support** - Configurable delays for realistic load simulation
- **Retry logic** - Automatic token refresh and request retry on expiration

## 📁 Project Structure

```
AI_api_template/
├── .github/
│   └── copilot-instructions.md    # Comprehensive conversion guide
├── postman-collection/
│   └── sample-api-collection.json # Example Postman collection
├── Action.c                        # API function lookup table and routing
├── GET.c                           # GET request implementations
├── POST.c                          # POST request implementations
├── GetPingToken.c                  # Authentication and token management
├── vuser_init.c                    # Initialization and SSL setup
├── vuser_end.c                     # Cleanup operations
└── host_name.dat                   # Parameter data file
```

## 🚀 Quick Start

### Prerequisites

- LoadRunner Professional or LoadRunner Enterprise
- Postman collection to convert
- API credentials (client_id, client_secret, api_key, etc.)

### Usage

1. **Clone this template**
   ```bash
   git clone <repository-url>
   ```

2. **Export your Postman collection**
   - Open Postman → Collections
   - Right-click your collection → Export
   - Choose "Collection v2.1" format
   - Save to `postman-collection/` folder

3. **Review the conversion guide**
   - Read `.github/copilot-instructions.md` for detailed mapping instructions
   - Understand the critical elements to extract from your Postman collection

4. **Update authentication (GetPingToken.c)**
   - Modify the authentication endpoint URL
   - Update scope requirements for your API
   - Configure client credentials

5. **Implement your API requests (Action.c)**
   - Add request functions following the GET/POST patterns
   - Update transaction names to match your API operations
   - Configure headers, endpoints, and request bodies

6. **Configure parameters (host_name.dat)**
   - Set `host_name` to your API base URL
   - Add `endpoint` paths for your API calls
   - Configure authentication parameters

7. **Run in LoadRunner**
   - Import the script into VuGen
   - Configure runtime settings
   - Execute and validate

## 📖 Key Components

### Action.c - API Function Lookup Table
Contains the routing logic and lookup table:
- Forward declarations for all API functions
- Function pointer lookup table
- Dynamic API selection based on `{api_name}` parameter
- Load distribution comments support

### GET.c - GET Request Implementations
All GET request functions with:
- Token expiration checking
- Header configuration
- Transaction timing
- HTTP status validation
- Retry logic

### POST.c - POST Request Implementations
All POST request functions with:
- Token expiration checking
- Header configuration
- Request body formatting
- Transaction timing
- HTTP status validation
- Retry logic

### GetPingToken.c - Authentication
Handles OAuth2 client credentials flow:
- Obtains access tokens
- Calculates token expiration (epoch time)
- Stores tokens for subsequent requests
- Supports automatic refresh

### vuser_init.c - Initialization
Sets up the test environment:
- Configures SSL/TLS version
- Sets maximum HTML parameter length
- Performs initial authentication
- Defines global variables (pause, buffer)

## 🔑 Critical Elements Mapping

When converting Postman collections, extract and map:

| Postman Element | LoadRunner Implementation |
|----------------|---------------------------|
| `request.method` | `web_url()` for GET, `web_custom_request()` for POST/PUT/DELETE |
| `variable[].baseUrl` | `{host_name}` parameter |
| `request.url.path` | `{endpoint}` parameter |
| `item[].name` | Transaction name and function name |
| `request.header[]` | `web_add_header()` calls |
| `request.body.raw` | `"BODY={json_body}"` parameter |
| `{{variable}}` | `{parameter}` in LoadRunner |

## 💡 Best Practices

1. **Token Management** - Always check token expiration before API calls
2. **Transaction Naming** - Use descriptive names matching business operations
3. **Error Logging** - Include input parameters in error messages for debugging
4. **Think Time** - Add realistic delays (`lr_think_time()`) between requests
5. **Status Codes** - Handle multiple success codes (200, 201, 204)
6. **Response Validation** - Use `web_reg_find()` for critical response checks
7. **Parameter Files** - Store test data in `.dat` files for data-driven testing

## 🛠️ LoadRunner Functions Reference

### Request Functions
- `web_url()` - Execute GET requests
- `web_custom_request()` - Execute POST/PUT/DELETE requests
- `web_add_header()` - Add HTTP headers

### Transaction Management
- `lr_start_transaction(name)` - Begin timing
- `lr_end_transaction(name, status)` - End with LR_PASS or LR_FAIL

### Data Handling
- `lr_eval_string("{param}")` - Evaluate parameter value
- `lr_save_string(value, "param")` - Save to parameter
- `web_reg_save_param()` - Extract from response

### Logging
- `lr_output_message()` - Write to log
- `web_get_int_property(HTTP_INFO_RETURN_CODE)` - Get HTTP status

## 📝 Example Conversion

### Postman Collection
```json
{
  "name": "Get User",
  "request": {
    "method": "GET",
    "header": [{"key": "Authorization", "value": "Bearer {{apiKey}}"}],
    "url": "{{baseUrl}}/users/{{userId}}"
  }
}
```

### LoadRunner Script
```c
GET_User(){
    GET_User:
    if (time(NULL)<atol(lr_eval_string("{expires_in_epoch}"))) {
        web_add_header("Authorization", "Bearer {access_token}");
        web_add_header("x-api-key", "{api_key}");
        
        sprintf(buffer, "Get_User");
        lr_start_transaction(buffer);
        
        web_url("Get_User",
            "URL={host_name}/users/{userId}",
            "Resource=0",
            "RecContentType=application/json",
            "Mode=HTML",
            LAST);
            
        HttpRetCode = web_get_int_property(HTTP_INFO_RETURN_CODE);
        
        if (HttpRetCode == 200)
            lr_end_transaction(buffer, LR_PASS);
        else
            lr_end_transaction(buffer, LR_FAIL);
    }
    else{
        GetPingToken();
        goto GET_User;
    }
    
    lr_think_time(pause);
    return 0;
}
```

## 🔄 Typical Workflow

1. **Extract** Postman collection details
2. **Configure** authentication in GetPingToken.c
3. **Implement** request functions in Action.c
4. **Parameterize** test data in .dat files
5. **Validate** in VuGen
6. **Execute** performance tests

## 📚 Documentation

Detailed conversion instructions are available in `.github/copilot-instructions.md`, including:
- Comprehensive element extraction guide
- Complete LoadRunner patterns
- Step-by-step conversion checklist
- Parameter naming conventions
- Common pitfalls and solutions

## 🤝 Contributing

This template is designed to be extended. When adding new patterns or improvements:
1. Update the relevant .c files
2. Document changes in copilot-instructions.md
3. Add examples to the sample collection
4. Update this README

## 📄 License

[Specify your license here]

## 👥 Authors

Performance Testing Team

## 🆘 Support

For questions or issues with this template:
1. Review `.github/copilot-instructions.md`
2. Check existing sample implementations
3. Consult LoadRunner documentation
4. Contact the performance testing team

## 🔖 Version History

- **v1.0.0** - Initial template with GET/POST support, OAuth2 authentication, and comprehensive conversion guide

---

**Note**: Remember to update authentication endpoints, API keys, and other sensitive information before running performance tests. Never commit credentials to version control.
