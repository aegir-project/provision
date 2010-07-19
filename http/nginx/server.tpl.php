# Aegir web server main configuration file

#######################################################
###  nginx.conf main
#######################################################

 ## FastCGI params
  fastcgi_param  SCRIPT_FILENAME    $document_root$fastcgi_script_name;
  fastcgi_param  QUERY_STRING       $query_string;
  fastcgi_param  REQUEST_METHOD     $request_method;
  fastcgi_param  CONTENT_TYPE       $content_type;
  fastcgi_param  CONTENT_LENGTH     $content_length;
  fastcgi_param  SCRIPT_NAME        $fastcgi_script_name;
  fastcgi_param  REQUEST_URI        $request_uri;
  fastcgi_param  DOCUMENT_URI       $document_uri;
  fastcgi_param  DOCUMENT_ROOT      $document_root;
  fastcgi_param  SERVER_PROTOCOL    $server_protocol;
  fastcgi_param  GATEWAY_INTERFACE  CGI/1.1;
  fastcgi_param  SERVER_SOFTWARE    ApacheSolaris/$nginx_version;
  fastcgi_param  REMOTE_ADDR        $remote_addr;
  fastcgi_param  REMOTE_PORT        $remote_port;
  fastcgi_param  SERVER_ADDR        $server_addr;
  fastcgi_param  SERVER_PORT        $server_port;
  fastcgi_param  SERVER_NAME        $server_name;
  fastcgi_param  REDIRECT_STATUS    200;
  fastcgi_index  index.php;

 ## Size Limits
  client_body_buffer_size        64k;
  client_header_buffer_size       1k;
  client_max_body_size           25m;
  large_client_header_buffers  4 32k;
  connection_pool_size           256;
  request_pool_size               4k;
  server_names_hash_bucket_size  128;

 ## Timeouts 
  client_body_timeout             60;
  client_header_timeout           60;
  send_timeout                    60;

 ## General Options
  ignore_invalid_headers          on;
  limit_zone gulag $binary_remote_addr 10m;
  recursive_error_pages           on;

 ## TCP options  
  tcp_nopush  on;

 ## Compression
  gzip_buffers      16 8k;
  gzip_comp_level   9;
  gzip_http_version 1.1;
  gzip_min_length   10;
  gzip_types        text/plain text/css image/png image/gif image/jpeg application/x-javascript text/xml application/xml application/xml+rss text/javascript image/x-icon;
  gzip_vary         on;
  gzip_proxied      any;
  gzip_disable      "MSIE [1-6]\.";
<?php 
$this->server->shell_exec('nginx -V');
if (preg_match("/(with-http_gzip_static_module)/", implode('', drush_shell_exec_output()), $match)) {
   print '  gzip_static       on\;';
}
if (preg_match("/(nginx-upload-progress-module)/", implode('', drush_shell_exec_output()), $match)) {
   print '  upload_progress uploads 1m\;';
}
?>

 ## Log Format
  log_format        main '"$remote_addr" $host [$time_local] '
                         '"$request" $status $body_bytes_sent '
                         '$request_length $bytes_sent "$http_referer" '
                         '"$http_user_agent" $request_time "$gzip_ratio"';

  client_body_temp_path  /var/lib/nginx/body 1 2;
  access_log             /var/log/nginx/access.log main;
  error_log              /var/log/nginx/error.log crit;
      

#######################################################
###  nginx default server
#######################################################

server {
  limit_conn   gulag 10; # like mod_evasive - this allows max 10 simultaneous connections from one IP address
<?php foreach ($server->ip_addresses as $ip) : ?>
  listen       <?php print $ip . ':' . $http_port; ?>;
<?php endforeach; ?>
  server_name  _;
  location / {
     root   /var/www/nginx-default;
     index  index.html index.htm;
  }
}

#######################################################
###  nginx virtual domains
#######################################################

# virtual hosts
include <?php print $http_vhostd_path ?>/*;

