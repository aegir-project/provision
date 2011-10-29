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

 ## Default index files
  index         index.php index.html;

 ## Size Limits
  client_body_buffer_size        64k;
  client_header_buffer_size      32k;
  client_max_body_size          100m;
  large_client_header_buffers 32 32k;
  connection_pool_size           256;
  request_pool_size               4k;
  server_names_hash_bucket_size  512;
  server_names_hash_max_size    8192;
  types_hash_max_size           8192;
  types_hash_bucket_size         512;
  fastcgi_buffer_size           128k;
  fastcgi_buffers             256 4k;
  fastcgi_busy_buffers_size     256k;
  fastcgi_temp_file_write_size  256k;

 ## Timeouts
  client_body_timeout             60;
  client_header_timeout           60;
  send_timeout                    60;
  lingering_time                  30;
  lingering_timeout                5;
  fastcgi_connect_timeout         60;
  fastcgi_send_timeout           300;
  fastcgi_read_timeout           300;

 ## Open File Performance
  open_file_cache max=8000 inactive=30s;
  open_file_cache_valid          60s;
  open_file_cache_min_uses         3;
  open_file_cache_errors          on;

 ## FastCGI Caching
  fastcgi_cache_path /var/lib/nginx/speed
                     levels=2:2:2
                     keys_zone=speed:50m
                     inactive=8h
                     max_size=1g;

 ## General Options
  ignore_invalid_headers          on;
  limit_zone gulag $binary_remote_addr 10m;
  recursive_error_pages           on;
  reset_timedout_connection       on;
  fastcgi_intercept_errors        on;

 ## TCP options 
  tcp_nopush  on;

 ## SSL performance
  ssl_session_cache   shared:SSL:10m;
  ssl_session_timeout            10m;

 ## Compression
  gzip_buffers      16 8k;
  gzip_comp_level   5;
  gzip_http_version 1.1;
  gzip_min_length   10;
  gzip_types        text/plain text/css application/x-javascript text/xml application/xml application/xml+rss text/javascript;
  gzip_vary         on;
  gzip_proxied      any;
  gzip_disable      "MSIE [1-6]\.";
<?php
$nginx_has_gzip = drush_get_option('nginx_has_gzip');
if ($nginx_has_gzip) {
   print "  gzip_static       on;\n";
}
$nginx_has_upload_progress = drush_get_option('nginx_has_upload_progress');
if ($nginx_has_upload_progress) {
   print "  upload_progress uploads 1m;\n";
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

<?php print $extra_config; ?>
#######################################################
###  nginx default server
#######################################################

<?php
$ip_address = !empty($ip_address) ? $ip_address : '*';
?>
server {
  limit_conn   gulag 10; # like mod_evasive - this allows max 10 simultaneous connections from one IP address
  listen       <?php print $ip_address . ':' . $http_port; ?>;
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
include <?php print $http_pred_path ?>/*;
include <?php print $http_platformd_path ?>/*;
include <?php print $http_vhostd_path ?>/*;
include <?php print $http_postd_path ?>/*;
