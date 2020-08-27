
#include <Update.h>

const char* serverIndex =
  "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
  "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
  "<input type='file' name='update'>"
  "<input type='submit' value='Update'>"
  "</form>"
  "<div id='prg'>progress: 0%</div>"
  "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')"
  "},"
  "error: function (a, b, c) {"
  "}"
  "});"
  "});"
  "</script>";


static esp_err_t serverIndex_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  //httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  return httpd_resp_send(req, (const char *)serverIndex, strlen(serverIndex));
}

static esp_err_t upload_handler(httpd_req_t *req) {

  uint8_t[1024] content;
  
  size_t recv_size = sizeof(content);
  Serial.printf("Update: %u\n", req->content_len);
  if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
    Update.printError(Serial);
  }

  int ret = httpd_req_recv(req, &content, recv_size);
  while (ret > 0) {
    Update.write(content);
    if (ret <= 0) {  /* 0 return value indicates connection closed */
      /* Check if timeout occurred */
      if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
        /* In case of timeout one can choose to retry calling
           httpd_req_recv(), but to keep it simple, here we
           respond with an HTTP 408 (Request Timeout) error */
        httpd_resp_send_408(req);
      }
      /* In case of error, returning ESP_FAIL will
         ensure that the underlying socket is closed */
      return ESP_FAIL;
    }
    ret = httpd_req_recv(req, content, recv_size);
  }
  if (Update.end(true)) { //true to set the size to the current progress
    Serial.printf("Update Success:\nRebooting...\n");
  } else {
    Update.printError(Serial);
  }




  httpd_resp_set_type(req, "text/html");
  //httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  const char * resp = "beautyful!";
  return httpd_resp_send(req, (const char *)resp, strlen(resp));
}
