#include "httpProtocol.h"

static const char *TAG = "HTTP_CLIENT";

static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static size_t output_len = 0; // Declare output_len to keep track of the length
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        // ESP_LOGI(TAG, "HTTP_EVENT_ERROR occurred");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        // ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        // ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        // ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        // ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            // Write out data to the output buffer
            if (evt->user_data)
            {
                memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                output_len += evt->data_len; // Update output_len
            }
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        // ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        // Log the complete response after finishing
        if (evt->user_data)
        {
            char *response = (char *)evt->user_data;
            response[output_len] = '\0'; // Null-terminate the string
            // ESP_LOGI(TAG, "Response: %s", response); // Log the full response
        }
        output_len = 0; // Reset output_len for the next request
        break;
    case HTTP_EVENT_DISCONNECTED:
        // ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT: // Handle the redirect event
        // ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT to %s", (char *)evt->data); // Cast evt->data to char*
        break;
    default:
        // ESP_LOGI(TAG, "Unhandled event: %d", evt->event_id);
        break;
    }
    return ESP_OK;
}

req_ctx_t *doPost(req_ctx_t *request)
{
    // Buffer to store the response
    uint_least8_t output_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};

    // Initialize the HTTP client configuration
    esp_http_client_config_t config = {
        // .url = "https://device.ap-in-1.anedya.io/v1/submitData",
        .url = request->url,
        .event_handler = _http_event_handler,
        .user_data = output_buffer,
        .cert_pem = LETSENCRYPT_ROOT_CA,
        .disable_auto_redirect = true,
    };

    // Initialize the HTTP client
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Set the request method to POST
    esp_http_client_set_method(client, HTTP_METHOD_POST);

    // Set the request headers
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Auth-mode", "key");
    esp_http_client_set_header(client, "Authorization", CONNECTION_KEY);

    // Set the POST data
    // const char *post_data = "{\"data\":[{\"variable\": \"temperature\",\"value\":25.90,\"timestamp\":0}]}"; // JSON payload
    char *post_data =(char *) request->body; // JSON payload

    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    // Perform the request
    esp_err_t err = esp_http_client_perform(client);
    request->status = esp_http_client_get_status_code(client);
    // request->response=output_buffer;

    // Check for errors
    if (err == ESP_OK)
    {
        int64_t content_length = esp_http_client_get_content_length(client);

        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %lld",
                 esp_http_client_get_status_code(client),
                 content_length);
        ESP_LOGI(TAG, "Response: %s\n", output_buffer);
        // printf("Response: %.*s", (int)content_length, (char *)output_buffer);
        // ESP_LOGI(TAG, "Response: %.*s", (int)content_length, (char *)output_buffer);
        // ESP_LOG_BUFFER_HEX("HTTP_REQUEST", output_buffer, strlen(output_buffer));
        // Clear the output buffer
        memset(output_buffer, 0, sizeof(output_buffer));
    }
    else
    {
        ESP_LOGI(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        xEventGroupSetBits(ConnectionEvents, CONNECTION_FAIL);
    }

    // Cleanup

    esp_http_client_cleanup(client);
    return request;
}

req_ctx_t *doGet(req_ctx_t *request)
{
    // Buffer to store the response
    char output_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};

    // Initialize the HTTP client configuration
    esp_http_client_config_t config = {
        // .url = "https://device.ap-in-1.anedya.io/v1/submitData",
        .url = request->url,
        .event_handler = _http_event_handler,
        .user_data = output_buffer,
        .cert_pem = LETSENCRYPT_ROOT_CA,
        .disable_auto_redirect = true,
    };

    // Initialize the HTTP client
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Set the request method to POST
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    // Set the request headers
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Auth-mode", "key");
    esp_http_client_set_header(client, "Authorization", CONNECTION_KEY);

    // Perform the request
    esp_err_t err = esp_http_client_perform(client);
    request->status = esp_http_client_get_status_code(client);

    // Check for errors
    if (err == ESP_OK)
    {
        int64_t content_length = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %lld",
                 esp_http_client_get_status_code(client),
                 content_length);
        // ESP_LOGI(TAG, "Response: %s", output_buffer);
        // printf("Response: %.*s", (int)content_length, (char *)output_buffer);
        // ESP_LOGI(TAG, "Response: %.*s", (int)content_length, (char *)output_buffer);
        // ESP_LOG_BUFFER_HEX("HTTP_REQUEST", output_buffer, strlen(output_buffer));
        // Clear the output buffer
        memset(output_buffer, 0, sizeof(output_buffer));
    }
    else
    {
        ESP_LOGI(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        xEventGroupSetBits(ConnectionEvents, CONNECTION_FAIL);
        // return -1;
    }

    // Cleanup
    esp_http_client_cleanup(client);
    return request;
}