# Aegir web server configuration file

#######################################################
###  nginx.conf main
#######################################################

 ## MIME types
  include            /etc/nginx/fastcgi_params;
  default_type       application/octet-stream;

 ## Size Limits
  client_body_buffer_size         1k;
  client_header_buffer_size       1k;
  client_max_body_size           10m;
  large_client_header_buffers   3 3k;
  connection_pool_size           256;
  request_pool_size               4k;
  server_names_hash_bucket_size  128;

 ## Timeouts 
  client_body_timeout             60;
  client_header_timeout           60;
  keepalive_timeout            75 20;
  send_timeout                    60;

 ## General Options
  ignore_invalid_headers          on;
  limit_zone gulag $binary_remote_addr 1m;
  recursive_error_pages           on;
  sendfile                        on;

 ## TCP options  
  tcp_nodelay on;
  tcp_nopush  on;

 ## Compression
  gzip              on;
  gzip_buffers      16 8k;
  gzip_comp_level   9;
  gzip_http_version 1.1;
  gzip_min_length   10;
  gzip_types        text/plain text/css image/png image/gif image/jpeg application/x-javascript text/xml application/xml application/xml+rss text/javascript image/x-icon;
  gzip_vary         on;
  gzip_static       on;
  gzip_proxied      any;
  gzip_disable      "MSIE [1-6]\.";

 ## Log Format
  log_format        main '"$remote_addr" $host [$time_local] '
                         '"$request" $status $body_bytes_sent '
                         '$request_length $bytes_sent "$http_referer" '
                         '"$http_user_agent" $request_time "$gzip_ratio"';

  client_body_temp_path      /var/cache/nginx/client_body_temp 1 2;
  access_log                   /var/log/nginx/access.log main;
  error_log                     /var/log/nginx/error.log crit;
      

#######################################################
###  nginx default server
#######################################################

server {

  listen <?php print $http_port; ?>;

  server_name  _;
  
  location / {
     root   /var/www/nginx-default;
     index  index.html index.htm;
  }

  error_page   500 502 503 504  /50x.html;
  location = /50x.html {
     root   /var/www/nginx-default;
  }

}

#######################################################
###  nginx virtual domains
#######################################################

# virtual hosts
include <?php print $http_vhostd_path ?>/*;

# other configuration, not touched by aegir
include <?php print $http_confd_path ?>/*;

