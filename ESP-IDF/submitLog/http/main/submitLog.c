#include "submitLog.h"
#include <time.h>
#include <sys/time.h>

static const char* TAG = "ANEDYA_LOG_HANDLER";
long long current_time = 1726311698000;
void submitLog(const char* param_TAG,char* reqId, char* message) {
    char* log_messgae=NULL;
    int len1=asprintf(&log_messgae, "[%s]: %s", param_TAG, message);
    if (len1 < 0) {
        ESP_LOGE(TAG, "Failed to allocate memory for log message");
        return;
    }
    char* log_payload=NULL;
    current_time=current_time+1000;
    int len2=asprintf(&log_payload, "{\"reqId\":\"%s\",\"data\":[{\"timestamp\":%lld,\"log\":\"%s\"}]}", reqId,current_time,log_messgae);
    if (len2 < 0) {
        ESP_LOGE(TAG, "Failed to allocate memory for log payload");
        return;
    }
    req_ctx_t* request = (req_ctx_t*)malloc(sizeof(req_ctx_t));
    request->url = "https://device.ap-in-1.anedya.io/v1/logs/submitLogs";
    request->body = (uint_least8_t*)log_payload;
    request->body_len = strlen(log_payload);

    req_ctx_t* response = doPost(request);
    if (response->status != 200) {
        ESP_LOGE(TAG, "Failed to send Log: %s", response->body);
    } else {
        ESP_LOGI(TAG, "Log sent successfully!!");
   
    }   
}