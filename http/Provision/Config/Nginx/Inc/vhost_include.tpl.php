<?php
$script_user = drush_get_option('script_user');
if (!$script_user && $server->script_user) {
  $script_user = $server->script_user;
}

$aegir_root = drush_get_option('aegir_root');
if (!$aegir_root && $server->aegir_root) {
  $aegir_root = $server->aegir_root;
}

$nginx_config_mode = drush_get_option('nginx_config_mode');
if (!$nginx_config_mode && $server->nginx_config_mode) {
  $nginx_config_mode = $server->nginx_config_mode;
}

$phpfpm_mode = drush_get_option('phpfpm_mode');
if (!$phpfpm_mode && $server->phpfpm_mode) {
  $phpfpm_mode = $server->phpfpm_mode;
}

// We can use $server here once we have proper inheritance.
// See Provision_Service_http_nginx_ssl for details.
$phpfpm_socket_path = Provision_Service_http_nginx::getPhpFpmSocketPath();

$nginx_is_modern = drush_get_option('nginx_is_modern');
if (!$nginx_is_modern && $server->nginx_is_modern) {
  $nginx_is_modern = $server->nginx_is_modern;
}

$nginx_has_etag = drush_get_option('nginx_has_etag');
if (!$nginx_has_etag && $server->nginx_has_etag) {
  $nginx_has_etag = $server->nginx_has_etag;
}

$nginx_has_http2 = drush_get_option('nginx_has_http2');
if (!$nginx_has_http2 && $server->nginx_has_http2) {
  $nginx_has_http2 = $server->nginx_has_http2;
}

$nginx_has_gzip = drush_get_option('nginx_has_gzip');
if (!$nginx_has_gzip && $server->nginx_has_gzip) {
  $nginx_has_gzip = $server->nginx_has_gzip;
}

$nginx_has_upload_progress = drush_get_option('nginx_has_upload_progress');
if (!$nginx_has_upload_progress && $server->nginx_has_upload_progress) {
  $nginx_has_upload_progress = $server->nginx_has_upload_progress;
}

$satellite_mode = drush_get_option('satellite_mode');
if (!$satellite_mode && $server->satellite_mode) {
  $satellite_mode = $server->satellite_mode;
}
?>
#######################################################
<?php if ($nginx_config_mode == 'extended'): ?>
###  nginx.conf site level extended vhost include start
<?php else: ?>
###  nginx.conf site level basic vhost include start
<?php endif; ?>
#######################################################

###
### Use the main site name if available, instead of
### potentially virtual server_name when alias is set
### as redirection target. See #2358977 for details.
###
if ($main_site_name = '') {
  set $main_site_name "$server_name";
}

<?php if ($nginx_config_mode == 'extended'): ?>
set $nocache_details "Cache";

<?php if ($satellite_mode == 'boa'): ?>
###
### Deny crawlers.
###
if ($is_crawler) {
  return 403;
}

###
### Block semalt botnet.
###
if ($is_botnet) {
  return 403;
}

###
### Include high load protection config if exists.
###
include /data/conf/nginx_high_load.c*;
<?php endif; ?>

###
### Deny not compatible request methods without 405 response.
###
if ( $request_method !~ ^(?:GET|HEAD|POST|PUT|DELETE|OPTIONS)$ ) {
  return 403;
}

<?php if ($nginx_config_mode == 'extended'): ?>
###
### Deny listed requests for security reasons.
###
if ($is_denied) {
  return 403;
}

###
### Add recommended HTTP headers
###
add_header Access-Control-Allow-Origin *;
add_header X-Content-Type-Options nosniff;
add_header X-XSS-Protection "1; mode=block";
<?php endif; ?>

<?php if ($satellite_mode == 'boa'): ?>
###
### Force clean URLs for Drupal 8.
###
rewrite ^/index.php/(.*)$ $scheme://$host/$1 permanent;

###
### Include high level local configuration override if exists.
###
include <?php print $aegir_root; ?>/config/server_master/nginx/post.d/nginx_force_include*;

###
### Include PHP-FPM version override logic if exists.
###
include <?php print $aegir_root; ?>/config/server_master/nginx/post.d/fpm_include*;

###
### Allow to use non-default PHP-FPM version for the site
### listed in the special include file.
###
if ($user_socket = '') {
  set $user_socket "<?php print $script_user; ?>";
}
<?php endif; ?>

###
### HTTPRL standard support.
###
location ^~ /httprl_async_function_callback {
  location ~* ^/httprl_async_function_callback {
    access_log off;
    set $nocache_details "Skip";
    try_files  $uri @nobots;
  }
}

###
### HTTPRL test mode support.
###
location ^~ /admin/httprl-test {
  location ~* ^/admin/httprl-test {
    access_log off;
    set $nocache_details "Skip";
    try_files  $uri @nobots;
  }
}

###
### CDN Far Future expiration support.
###
location ^~ /cdn/farfuture/ {
  tcp_nodelay   off;
  access_log    off;
  log_not_found off;
<?php if ($nginx_has_etag): ?>
  etag          off;
<?php else: ?>
  add_header ETag "";
<?php endif; ?>
  gzip_http_version 1.0;
  if_modified_since exact;
  set $nocache_details "Skip";
  location ~* ^/cdn/farfuture/.+\.(?:css|js|jpe?g|gif|png|ico|bmp|svg|swf|pdf|docx?|xlsx?|pptx?|tiff?|txt|rtf|class|otf|ttf|woff|eot|less)$ {
    expires max;
    add_header X-Header "CDN Far Future Generator 1.0";
    add_header Cache-Control "no-transform, public";
    add_header Last-Modified "Wed, 20 Jan 1988 04:20:42 GMT";
    add_header Access-Control-Allow-Origin *;
    add_header X-Content-Type-Options nosniff;
    add_header X-XSS-Protection "1; mode=block";
    rewrite ^/cdn/farfuture/[^/]+/[^/]+/(.+)$ /$1 break;
    try_files $uri @nobots;
  }
  location ~* ^/cdn/farfuture/ {
    expires epoch;
    add_header X-Header "CDN Far Future Generator 1.1";
    add_header Cache-Control "private, must-revalidate, proxy-revalidate";
    add_header Access-Control-Allow-Origin *;
    add_header X-Content-Type-Options nosniff;
    add_header X-XSS-Protection "1; mode=block";
    rewrite ^/cdn/farfuture/[^/]+/[^/]+/(.+)$ /$1 break;
    try_files $uri @nobots;
  }
  try_files $uri @nobots;
}
<?php endif; ?>

###
### If favicon else return error 204.
###
location = /favicon.ico {
  access_log    off;
  log_not_found off;
  expires       30d;
  try_files  /sites/$main_site_name/files/favicon.ico $uri =204;
}

###
### Support for https://drupal.org/project/robotstxt module
### and static file in the sites/domain/files directory.
###
location = /robots.txt {
  access_log    off;
  log_not_found off;
<?php if ($nginx_config_mode == 'extended'): ?>
  try_files /sites/$main_site_name/files/$host.robots.txt /sites/$main_site_name/files/robots.txt $uri @cache;
<?php else: ?>
  try_files /sites/$main_site_name/files/$host.robots.txt /sites/$main_site_name/files/robots.txt $uri @drupal;
<?php endif; ?>
}

<?php if ($satellite_mode == 'boa'): ?>
###
### Allow local access to the FPM status page.
###
location = /fpm-status {
  access_log   off;
  allow        127.0.0.1;
  deny         all;
<?php if ($satellite_mode == 'boa'): ?>
  fastcgi_pass unix:/var/run/$user_socket.fpm.socket;
<?php elseif ($phpfpm_mode == 'port'): ?>
  fastcgi_pass 127.0.0.1:9000;
<?php else: ?>
  fastcgi_pass unix:<?php print $phpfpm_socket_path; ?>;
<?php endif; ?>
}

###
### Allow local access to the FPM ping URI.
###
location = /fpm-ping {
  access_log   off;
  allow        127.0.0.1;
  deny         all;
<?php if ($satellite_mode == 'boa'): ?>
  fastcgi_pass unix:/var/run/$user_socket.fpm.socket;
<?php elseif ($phpfpm_mode == 'port'): ?>
  fastcgi_pass 127.0.0.1:9000;
<?php else: ?>
  fastcgi_pass unix:<?php print $phpfpm_socket_path; ?>;
<?php endif; ?>
}
<?php endif; ?>

<?php if ($nginx_config_mode == 'extended'): ?>
###
### Allow local access to support wget method in Aegir settings
### for running sites cron.
###
location = /cron.php {
  tcp_nopush   off;
  keepalive_requests 0;
<?php if ($satellite_mode == 'boa'): ?>
  allow        127.0.0.1;
  deny         all;
<?php endif; ?>
  try_files    $uri =404;
<?php if ($satellite_mode == 'boa'): ?>
  fastcgi_pass unix:/var/run/$user_socket.fpm.socket;
<?php elseif ($phpfpm_mode == 'port'): ?>
  fastcgi_pass 127.0.0.1:9000;
<?php else: ?>
  fastcgi_pass unix:<?php print $phpfpm_socket_path; ?>;
<?php endif; ?>
}

###
### Allow local access to support wget method in Aegir settings
### for running sites cron in Drupal 8.
###
location ^~ /cron/ {
<?php if ($satellite_mode == 'boa'): ?>
  allow        127.0.0.1;
  deny         all;
<?php endif; ?>
<?php if ($nginx_config_mode == 'extended'): ?>
  set $nocache_details "Skip";
<?php endif; ?>
  try_files    $uri @drupal;
}

###
### Send search to php-fpm early so searching for node.js will work.
### Deny bots on search uri.
###
location ^~ /search {
  location ~* ^/search {
    if ($is_bot) {
      return 403;
    }
    try_files $uri @cache;
  }
}

###
### Support for https://drupal.org/project/js module.
###
location ^~ /js/ {
  location ~* ^/js/ {
    if ($is_bot) {
      return 403;
    }
    rewrite ^/(.*)$ /js.php?q=$1 last;
  }
}

<?php if ($nginx_has_upload_progress): ?>
###
### Upload progress support.
### https://drupal.org/project/filefield_nginx_progress
### http://github.com/masterzen/nginx-upload-progress-module
###
location ~ (?<upload_form_uri>.*)/x-progress-id:(?<upload_id>\d*) {
  access_log off;
  rewrite ^ $upload_form_uri?X-Progress-ID=$upload_id;
}
location ^~ /progress {
  access_log off;
  upload_progress_json_output;
  report_uploads uploads;
}
<?php endif; ?>

<?php if ($satellite_mode == 'boa'): ?>
###
### Deny access to Hostmaster web/db server node.
### It is still possible to edit or break web/db server
### node at /node/2/edit, if you know what are you doing.
###
location ^~ /hosting/c/server_master {
  if ($cache_uid = '') {
    return 403;
  }
  if ($is_bot) {
    return 403;
  }
  access_log off;
  return 301 $scheme://$host/hosting/sites;
}

###
### Deny access to Hostmaster db server node.
### It is still possible to edit or break db server
### node at /node/4/edit, if you know what are you doing.
###
location ^~ /hosting/c/server_localhost {
  if ($cache_uid = '') {
    return 403;
  }
  if ($is_bot) {
    return 403;
  }
  access_log off;
  return 301 $scheme://$host/hosting/sites;
}
<?php endif; ?>

###
### Fix for #2005116
###
location ^~ /hosting/sites {
  if ($is_bot) {
    return 403;
  }
  access_log off;
  set $nocache_details "Skip";
  try_files $uri @drupal;
}

###
### Fix for Aegir & .info .pl domain extensions.
###
location ^~ /hosting {
  if ($is_bot) {
    return 403;
  }
  access_log off;
  set $nocache_details "Skip";
  try_files $uri @cache;
}

<?php if ($satellite_mode == 'boa'): ?>
###
### Deny cache details display.
###
location ^~ /admin/settings/performance/cache-backend {
  access_log off;
  return 301 $scheme://$host/admin/settings/performance;
}

###
### Deny cache details display.
###
location ^~ /admin/config/development/performance/redis {
  access_log off;
  return 301 $scheme://$host/admin/config/development/performance;
}
<?php endif; ?>

###
### Support for backup_migrate module download/restore/delete actions.
###
location ^~ /admin {
  if ($is_bot) {
    return 403;
  }
  access_log off;
  set $nocache_details "Skip";
  try_files $uri @drupal;
}

###
### Avoid caching /civicrm* and protect it from bots.
###
location ^~ /civicrm {
  if ($is_bot) {
    return 403;
  }
  access_log off;
  set $nocache_details "Skip";
  try_files $uri @drupal;
}

###
### Avoid caching /civicrm* and protect it from bots on a multi-lingual site
###
location ~* ^/\w\w/civicrm {
  if ( $is_bot ) {
    return 403;
  }
  access_log off;
  set $nocache_details "Skip";
  try_files $uri @drupal;
}

###
### Support for audio module.
###
location ^~ /audio/download {
  location ~* ^/audio/download/.*/.*\.(?:mp3|mp4|m4a|ogg)$ {
    if ($is_bot) {
      return 403;
    }
    tcp_nopush off;
    access_log    off;
    log_not_found off;
    set $nocache_details "Skip";
    try_files $uri @drupal;
  }
}
<?php endif; ?>

###
### Deny listed requests for security reasons.
###
location ~* (\.(?:git.*|htaccess|engine|config|inc|ini|info|install|make|module|profile|test|pl|po|sh|.*sql|theme|tpl(\.php)?|xtmpl)(~|\.sw[op]|\.bak|\.orig|\.save)?$|^(\..*|Entries.*|Repository|Root|Tag|Template|composer\.(json|lock))$|^#.*#$|\.php(~|\.sw[op]|\.bak|\.orig\.save))$ {
  access_log off;
  return 404;
}

###
### Deny listed requests for security reasons.
###
location ~* /(?:modules|themes|libraries)/.*\.(?:txt|md)$ {
  access_log off;
  return 404;
}

###
### Deny listed requests for security reasons.
###
location ~* ^/sites/.*/files/civicrm/(?:ConfigAndLog|custom|upload|templates_c) {
  access_log off;
  return 404;
}

<?php if ($nginx_config_mode == 'extended'): ?>
###
### Deny often flooded URI for performance reasons
###
location = /autodiscover/autodiscover.xml {
  access_log off;
  return 404;
}

###
### Deny some not supported URI like cgi-bin on the Nginx level.
###
location ~* (?:cgi-bin|vti-bin) {
  access_log off;
  return 404;
}

###
### Deny bots on some weak modules uri.
###
location ~* (?:validation|aggregator|vote_up_down|captcha|vbulletin|glossary/) {
  if ($is_bot) {
    return 403;
  }
  access_log off;
  try_files $uri @cache;
}

###
### Responsive Images support.
### https://drupal.org/project/responsive_images
###
location ~* \.r\.(?:jpe?g|png|gif) {
  if ( $http_cookie ~* "rwdimgsize=large" ) {
    rewrite ^/(.*)/mobile/(.*)\.r(\.(?:jpe?g|png|gif))$ /$1/desktop/$2$3 last;
  }
  rewrite ^/(.*)\.r(\.(?:jpe?g|png|gif))$ /$1$2 last;
  access_log off;
  set $nocache_details "Skip";
  try_files  $uri @drupal;
}

###
### Adaptive Image Styles support.
### https://drupal.org/project/ais
###
location ~* /(?:.+)/files/styles/adaptive/(?:.+)$ {
  if ( $http_cookie ~* "ais=(?<ais_cookie>[a-z0-9-_]+)" ) {
    rewrite ^/(.+)/files/styles/adaptive/(.+)$ /$1/files/styles/$ais_cookie/$2 last;
  }
  access_log off;
  set $nocache_details "Skip";
  try_files  $uri @drupal;
}
<?php endif; ?>

###
### The files/styles support.
###
location ~* /sites/.*/files/styles/(.*)$ {
  access_log off;
  log_not_found off;
  expires    30d;
<?php if ($nginx_config_mode == 'extended'): ?>
  set $nocache_details "Skip";
<?php endif; ?>
  try_files  /sites/$main_site_name/files/styles/$1 $uri @drupal;
}

###
### The s3/files/styles (s3fs) support.
###
location ~* /s3/files/styles/(.*)$ {
  access_log off;
  log_not_found off;
  expires    30d;
<?php if ($nginx_config_mode == 'extended'): ?>
  set $nocache_details "Skip";
<?php endif; ?>
  try_files  /sites/$main_site_name/files/styles/$1 $uri @drupal;
}

###
### The files/imagecache support.
###
location ~* /sites/.*/files/imagecache/(.*)$ {
  access_log off;
  log_not_found off;
  expires    30d;
<?php if ($nginx_config_mode == 'extended'): ?>
  # fix common problems with old paths after import from standalone to Aegir multisite
  rewrite ^/sites/(.*)/files/imagecache/(.*)/sites/default/files/(.*)$ /sites/$main_site_name/files/imagecache/$2/$3 last;
  rewrite ^/sites/(.*)/files/imagecache/(.*)/files/(.*)$               /sites/$main_site_name/files/imagecache/$2/$3 last;
  set $nocache_details "Skip";
<?php endif; ?>
  try_files  /sites/$main_site_name/files/imagecache/$1 $uri @drupal;
}

###
### Send requests with /external/ and /system/ URI keywords to @drupal.
###
location ~* /(?:external|system)/ {
  access_log off;
  log_not_found off;
  expires    30d;
<?php if ($nginx_config_mode == 'extended'): ?>
  set $nocache_details "Skip";
<?php endif; ?>
  try_files  $uri @drupal;
}

###
### Deny direct access to backups.
###
location ~* ^/sites/.*/files/backup_migrate/ {
  access_log off;
  deny all;
}

###
### Deny direct access to config files in Drupal 8.
###
location ~* ^/sites/.*/files/config_.* {
  access_log off;
  deny all;
}

<?php if ($satellite_mode == 'boa'): ?>
###
### Include local configuration override if exists.
###
include <?php print $aegir_root; ?>/config/server_master/nginx/post.d/nginx_vhost_include*;
<?php endif; ?>

<?php if ($nginx_config_mode == 'extended'): ?>
###
### Private downloads are always sent to the drupal backend.
### Note: this location doesn't work with X-Accel-Redirect.
###
location ~* ^/sites/.*/files/private/ {
  if ($is_bot) {
    return 403;
  }
  access_log off;
  rewrite    ^/sites/.*/files/private/(.*)$ $scheme://$host/system/files/private/$1 permanent;
  set $nocache_details "Skip";
  try_files  $uri @drupal;
}
<?php endif; ?>

###
### Deny direct access to private downloads in sites/domain/private.
### Note: this location works with X-Accel-Redirect.
###
location ~* ^/sites/.*/private/ {
  internal;
<?php if ($nginx_config_mode == 'extended'): ?>
  if ($is_bot) {
    return 403;
  }
<?php endif; ?>
  access_log off;
}

<?php if ($nginx_config_mode == 'extended'): ?>
###
### Deny direct access to private downloads also for short, rewritten URLs.
### Note: this location works with X-Accel-Redirect.
###
location ~* /files/private/ {
  internal;
  if ($is_bot) {
    return 403;
  }
  access_log off;
}

###
### Wysiwyg Fields support.
###
location ~* wysiwyg_fields/(?:plugins|scripts)/.*\.(?:js|css) {
  access_log off;
  log_not_found off;
  try_files $uri @nobots;
}

###
### Advagg_css and Advagg_js support.
###
location ~* files/advagg_(?:css|js)/ {
  expires    max;
  access_log off;
<?php if ($nginx_has_etag): ?>
  etag       off;
<?php else: ?>
  add_header ETag "";
<?php endif; ?>
  rewrite    ^/files/advagg_(.*)/(.*)$ /sites/$main_site_name/files/advagg_$1/$2 last;
  add_header X-Header "AdvAgg Generator 2.0";
  add_header Cache-Control "max-age=31449600, no-transform, public";
  add_header Access-Control-Allow-Origin *;
  add_header X-Content-Type-Options nosniff;
  add_header X-XSS-Protection "1; mode=block";
  set $nocache_details "Skip";
  try_files  $uri @nobots;
}

###
### Make css files compatible with boost caching.
###
location ~* \.css$ {
  if ( $request_method = POST ) {
    return 405;
  }
  if ( $cache_uid ) {
    return 405;
  }
  error_page  405 = @uncached;
  access_log  off;
  tcp_nodelay off;
  expires     max; #if using aggregator
  try_files   /cache/perm/$host${uri}_.css $uri =404;
}

###
### Make js files compatible with boost caching.
###
location ~* \.(?:js|htc)$ {
  if ( $request_method = POST ) {
    return 405;
  }
  if ( $cache_uid ) {
    return 405;
  }
  error_page  405 = @uncached;
  access_log  off;
  tcp_nodelay off;
  expires     max; # if using aggregator
  try_files   /cache/perm/$host${uri}_.js $uri =404;
}

###
### Support for static .json files with fast 404 +Boost compatibility.
###
location ~* ^/sites/.*/files/.*\.json$ {
  if ( $cache_uid ) {
    return 405;
  }
  error_page  405 = @uncached;
  access_log  off;
  tcp_nodelay off;
  expires     max; ### if using aggregator
  try_files   /cache/normal/$host${uri}_.json $uri =404;
}

###
### Support for dynamic .json requests.
###
location ~* \.json$ {
  try_files $uri @cache;
}

###
### Helper location to bypass boost static files cache for logged in users.
###
location @uncached {
  access_log off;
  expires max; # max if using aggregator, otherwise sane expire time
}
<?php endif; ?>

###
### Map /files/ shortcut early to avoid overrides in other locations.
###
location ^~ /files/ {

  ###
  ### Sub-location to support files/styles with short URIs.
  ###
  location ~* /files/styles/(.*)$ {
    access_log off;
    log_not_found off;
    expires    30d;
<?php if ($nginx_config_mode == 'extended'): ?>
    set $nocache_details "Skip";
<?php endif; ?>
    rewrite  ^/files/(.*)$  /sites/$main_site_name/files/$1 last;
    try_files  /sites/$main_site_name/files/styles/$1 $uri @drupal;
  }

  ###
  ### Sub-location to support files/imagecache with short URIs.
  ###
  location ~* /files/imagecache/(.*)$ {
    access_log off;
    log_not_found off;
    expires    30d;
<?php if ($nginx_config_mode == 'extended'): ?>
    # fix common problems with old paths after import from standalone to Aegir multisite
    rewrite ^/files/imagecache/(.*)/sites/default/files/(.*)$ /sites/$main_site_name/files/imagecache/$1/$2 last;
    rewrite ^/files/imagecache/(.*)/files/(.*)$               /sites/$main_site_name/files/imagecache/$1/$2 last;
    set $nocache_details "Skip";
<?php endif; ?>
    rewrite  ^/files/(.*)$  /sites/$main_site_name/files/$1 last;
    try_files  /sites/$main_site_name/files/imagecache/$1 $uri @drupal;
  }

  location ~* ^.+\.(?:pdf|jpe?g|gif|png|ico|bmp|svg|swf|docx?|xlsx?|pptx?|tiff?|txt|rtf|cgi|bat|pl|dll|class|otf|ttf|woff|eot|less|avi|mpe?g|mov|wmv|mp3|ogg|ogv|wav|midi|zip|tar|t?gz|rar|dmg|exe|apk|pxl|ipa|css|js)$ {
    expires       30d;
    tcp_nodelay   off;
    access_log    off;
    log_not_found off;
    rewrite  ^/files/(.*)$  /sites/$main_site_name/files/$1 last;
    try_files   $uri =404;
  }
<?php if ($nginx_config_mode == 'extended'): ?>
  try_files $uri @cache;
<?php else: ?>
  try_files $uri @drupal;
<?php endif; ?>
}

###
### Map /downloads/ shortcut early to avoid overrides in other locations.
###
location ^~ /downloads/ {
  location ~* ^.+\.(?:pdf|jpe?g|gif|png|ico|bmp|svg|swf|docx?|xlsx?|pptx?|tiff?|txt|rtf|cgi|bat|pl|dll|class|otf|ttf|woff|eot|less|avi|mpe?g|mov|wmv|mp3|ogg|ogv|wav|midi|zip|tar|t?gz|rar|dmg|exe|apk|pxl|ipa)$ {
    expires       30d;
    tcp_nodelay   off;
    access_log    off;
    log_not_found off;
    rewrite  ^/downloads/(.*)$  /sites/$main_site_name/files/downloads/$1 last;
    try_files   $uri =404;
  }
<?php if ($nginx_config_mode == 'extended'): ?>
  try_files $uri @cache;
<?php else: ?>
  try_files $uri @drupal;
<?php endif; ?>
}

###
### Serve & no-log static files & images directly,
### without all standard drupal rewrites, php-fpm etc.
###
location ~* ^.+\.(?:jpe?g|gif|png|ico|bmp|svg|swf|docx?|xlsx?|pptx?|tiff?|txt|rtf|cgi|bat|pl|dll|class|otf|ttf|woff|eot|less|mp3|wav|midi)$ {
  expires       30d;
  tcp_nodelay   off;
  access_log    off;
  log_not_found off;
  rewrite     ^/images/(.*)$  /sites/$main_site_name/files/images/$1 last;
  rewrite     ^/.+/sites/.+/files/(.*)$  /sites/$main_site_name/files/$1 last;
  try_files   $uri =404;
}

###
### Serve bigger media/static/archive files directly,
### without all standard drupal rewrites, php-fpm etc.
###
location ~* ^.+\.(?:avi|mpe?g|mov|wmv|ogg|ogv|zip|tar|t?gz|rar|dmg|exe|apk|pxl|ipa)$ {
  expires     30d;
  tcp_nodelay off;
  tcp_nopush  off;
  access_log    off;
  log_not_found off;
  rewrite     ^/.+/sites/.+/files/(.*)$  /sites/$main_site_name/files/$1 last;
  try_files   $uri =404;
}

###
### Serve & no-log some static files directly,
### but only from the files directory to not break
### dynamically created pdf files or redirects for
### legacy URLs with asp/aspx extension.
###
location ~* ^/sites/.+/files/.+\.(?:pdf|aspx?)$ {
  expires       30d;
  tcp_nodelay   off;
  access_log    off;
  log_not_found off;
  try_files   $uri =404;
}

<?php if ($satellite_mode == 'boa'): ?>
###
### Pseudo-streaming server-side support for Flash Video (FLV) files.
###
location ~* ^.+\.flv$ {
  flv;
  tcp_nodelay off;
  tcp_nopush off;
  expires 30d;
  access_log    off;
  log_not_found off;
  try_files $uri =404;
}

###
### Pseudo-streaming server-side support for H.264/AAC files.
###
location ~* ^.+\.(?:mp4|m4a)$ {
  mp4;
  mp4_buffer_size 1m;
  mp4_max_buffer_size 5m;
  tcp_nodelay off;
  tcp_nopush off;
  expires 30d;
  access_log    off;
  log_not_found off;
  try_files $uri =404;
}
<?php endif; ?>

###
### Serve & no-log some static files as is, without forcing default_type.
###
location ~* /(?:cross-?domain)\.xml$ {
  access_log  off;
  tcp_nodelay off;
  expires     30d;
  try_files   $uri =404;
}

<?php if ($nginx_config_mode == 'extended'): ?>
###
### Allow some known php files (like serve.php in the ad module).
###
location ~* /(?:modules|libraries)/(?:contrib/)?(?:ad|tinybrowser|f?ckeditor|tinymce|wysiwyg_spellcheck|ecc|civicrm|fbconnect|radioactivity|statistics)/.*\.php$ {
<?php if ($satellite_mode == 'boa'): ?>
  limit_conn   limreq 88;
<?php endif; ?>
  tcp_nopush   off;
  keepalive_requests 0;
  access_log   off;
  if ($is_bot) {
    return 403;
  }
  try_files    $uri =404;
<?php if ($satellite_mode == 'boa'): ?>
  fastcgi_pass unix:/var/run/$user_socket.fpm.socket;
<?php elseif ($phpfpm_mode == 'port'): ?>
  fastcgi_pass 127.0.0.1:9000;
<?php else: ?>
  fastcgi_pass unix:<?php print $phpfpm_socket_path; ?>;
<?php endif; ?>
}

###
### Deny crawlers and never cache known AJAX requests.
###
location ~* /(?:ahah|ajax|batch|autocomplete|done|progress/|x-progress-id|js/.*) {
  if ($is_bot) {
    return 403;
  }
  access_log off;
  log_not_found off;
<?php if ($nginx_config_mode == 'extended'): ?>
  set $nocache_details "Skip";
  try_files $uri @nobots;
<?php else: ?>
  try_files $uri @drupal;
<?php endif; ?>
}

###
### Serve & no-log static helper files used in some wysiwyg editors.
###
location ~* ^/sites/.*/(?:modules|libraries)/(?:contrib/)?(?:tinybrowser|f?ckeditor|tinymce|flowplayer|jwplayer|videomanager)/.*\.(?:html?|xml)$ {
  if ($is_bot) {
    return 403;
  }
  access_log      off;
  tcp_nodelay     off;
  expires         30d;
  try_files $uri =404;
}

###
### Serve & no-log any not specified above static files directly.
###
location ~* ^/sites/.*/files/ {
  access_log      off;
  tcp_nodelay     off;
  expires         30d;
  try_files $uri =404;
}

###
### Make feeds compatible with boost caching and set correct mime type.
###
location ~* \.xml$ {
  location ~* ^/autodiscover/autodiscover\.xml {
    access_log off;
    return 400;
  }
  if ( $request_method = POST ) {
    return 405;
  }
  if ( $cache_uid ) {
    return 405;
  }
  error_page 405 = @drupal;
  access_log off;
  add_header X-Header "Boost Citrus 1.0";
  add_header Expires "Tue, 24 Jan 1984 08:00:00 GMT";
  add_header Cache-Control "no-store, no-cache, must-revalidate, post-check=0, pre-check=0";
  add_header Access-Control-Allow-Origin *;
  add_header X-Content-Type-Options nosniff;
  add_header X-XSS-Protection "1; mode=block";
  charset    utf-8;
  types { }
  default_type text/xml;
  try_files /cache/normal/$host${uri}_.xml /cache/normal/$host${uri}_.html $uri @drupal;
}

###
### Deny bots on never cached uri.
###
location ~* ^/(?:.*/)?(?:admin|user|cart|checkout|logout|comment/reply) {
  if ($is_bot) {
    return 403;
  }
  access_log off;
  set $nocache_details "Skip";
  try_files $uri @drupal;
}

###
### Protect from DoS attempts on never cached uri.
###
location ~* ^/(?:.*/)?(?:node/[0-9]+/edit|node/add) {
  if ($is_bot) {
    return 403;
  }
  access_log off;
  set $nocache_details "Skip";
  try_files $uri @drupal;
}

###
### Protect from DoS attempts on never cached uri.
###
location ~* ^/(?:.*/)?(?:node/[0-9]+/delete|approve) {
  if ($cache_uid = '') {
    return 403;
  }
  if ($is_bot) {
    return 403;
  }
  access_log off;
  set $nocache_details "Skip";
  try_files $uri @drupal;
}

<?php if ($satellite_mode == 'boa'): ?>
###
### Support for ESI microcaching: http://groups.drupal.org/node/197478.
###
### This may enhance not only anonymous visitors, but also
### logged in users experience, as it allows you to separate
### microcache for ESI/SSI includes (valid for just 5 seconds)
### from both default Speed Booster cache for anonymous visitors
### (valid by default for 10s or 1h, unless purged on demand via
### recently introduced Purge/Expire modules) and also from
### Speed Booster cache per logged in user (valid for 10 seconds).
###
### Now you have three different levels of Speed Booster cache
### to leverage and deliver the 'live content' experience for
### all visitors, and still protect your server from DoS or
### simply high load caused by unexpected high traffic etc.
###
location ~ ^/(?<esi>esi/.*)"$ {
  ssi on;
  ssi_silent_errors on;
  internal;
  limit_conn limreq 888;
  add_header X-Device "$device";
  add_header X-Speed-Micro-Cache "$upstream_cache_status";
  add_header X-Speed-Micro-Cache-Expire "5s";
  add_header X-NoCache "$nocache_details";
  add_header X-GeoIP-Country-Code "$geoip_country_code";
  add_header X-GeoIP-Country-Name "$geoip_country_name";
  add_header X-This-Proto "$http_x_forwarded_proto";
  add_header X-Server-Name "$main_site_name";
  add_header Cache-Control "no-store, no-cache, must-revalidate, post-check=0, pre-check=0";
  add_header Access-Control-Allow-Origin *;
  add_header X-Content-Type-Options nosniff;
  add_header X-XSS-Protection "1; mode=block";
  ###
  ### Set correct, local $uri.
  ###
  fastcgi_param QUERY_STRING q=$esi;
  fastcgi_param SCRIPT_FILENAME $document_root/index.php;
<?php if ($satellite_mode == 'boa'): ?>
  fastcgi_pass  unix:/var/run/$user_socket.fpm.socket;
<?php elseif ($phpfpm_mode == 'port'): ?>
  fastcgi_pass  127.0.0.1:9000;
<?php else: ?>
  fastcgi_pass  unix:<?php print $phpfpm_socket_path; ?>;
<?php endif; ?>
  ###
  ### Use Nginx cache for all visitors.
  ###
  set $nocache "";
  if ( $http_cookie ~* "NoCacheID" ) {
    set $nocache "NoCache";
  }
  fastcgi_cache speed;
  fastcgi_cache_methods GET HEAD;
  fastcgi_cache_min_uses 1;
  fastcgi_cache_key "$scheme$is_bot$device$host$request_method$key_uri$cache_uid$http_x_forwarded_proto$sent_http_x_local_proto$cookie_respimg";
  fastcgi_cache_valid 200 5s;
  fastcgi_cache_valid 301 1m;
  fastcgi_cache_valid 302 403 404 1s;
  fastcgi_cache_lock on;
  fastcgi_ignore_headers Cache-Control Expires;
  fastcgi_pass_header Set-Cookie;
  fastcgi_pass_header X-Accel-Expires;
  fastcgi_pass_header X-Accel-Redirect;
  fastcgi_no_cache $cookie_NoCacheID $http_authorization $http_pragma $nocache;
  fastcgi_cache_bypass $cookie_NoCacheID $http_authorization $http_pragma $nocache;
  fastcgi_cache_use_stale error http_500 http_503 invalid_header timeout updating;
  tcp_nopush off;
  keepalive_requests 0;
  expires epoch;
}

###
### Workaround for https://www.drupal.org/node/2599326.
###
if ( $args ~* "/autocomplete/" ) {
  return 405;
}
error_page 405 = @drupal;

###
### Rewrite legacy requests with /index.php to extension-free URL.
###
if ( $args ~* "^q=(?<query_value>.*)" ) {
  rewrite ^/index.php$ $scheme://$host/?q=$query_value? permanent;
}
<?php endif; ?>
<?php endif; ?>

###
### Catch all unspecified requests.
###
location / {
<?php if ($nginx_config_mode == 'extended'): ?>
<?php if ($satellite_mode == 'boa'): ?>
  if ( $http_user_agent ~* wget ) {
    return 403;
  }
<?php endif; ?>
  try_files $uri @cache;
<?php else: ?>
  try_files $uri @drupal;
<?php endif; ?>
}

<?php if ($nginx_config_mode == 'extended'): ?>
###
### Boost compatible cache check.
###
location @cache {
  if ( $request_method = POST ) {
    set $nocache_details "Method";
    return 405;
  }
  if ( $args ~* "nocache=1" ) {
    set $nocache_details "Args";
    return 405;
  }
  if ( $sent_http_x_force_nocache = "YES" ) {
    set $nocache_details "Skip";
    return 405;
  }
  if ( $http_cookie ~* "NoCacheID" ) {
    set $nocache_details "AegirCookie";
    return 405;
  }
  if ( $cache_uid ) {
    set $nocache_details "DrupalCookie";
    return 405;
  }
  error_page 405 = @drupal;
  add_header X-Header "Boost Citrus 1.0";
  add_header Expires "Tue, 24 Jan 1984 08:00:00 GMT";
  add_header Cache-Control "no-store, no-cache, must-revalidate, post-check=0, pre-check=0";
  add_header Access-Control-Allow-Origin *;
  add_header X-Content-Type-Options nosniff;
  add_header X-XSS-Protection "1; mode=block";
  charset    utf-8;
  try_files  /cache/normal/$host${uri}_$args.html @drupal;
}
<?php endif; ?>

###
### Send all not cached requests to drupal with clean URLs support.
###
location @drupal {
<?php if ($nginx_config_mode == 'extended'): ?>
  error_page 418 = @nobots;
  if ($args) {
    return 418;
  }
<?php endif; ?>
  ###
  ### For Drupal >= 7
  ###
  if ($sent_http_x_generator) {
    add_header X-Info-Gen "Modern";
    rewrite ^ /index.php?$query_string last;
  }
  ###
  ### For Drupal <= 6
  ###
  rewrite ^/(.*)$ /index.php?q=$1 last;
}

<?php if ($nginx_config_mode == 'extended'): ?>
###
### Special location for bots custom restrictions; can be overridden.
###
location @nobots {
  ###
  ### Support for Accelerated Mobile Pages (AMP) when bots are redirected below
  ###
  # if ( $query_string ~ "^amp$" ) {
  #  rewrite ^/(.*)$  /index.php?q=$1 last;
  # }

  ###
  ### Send all known bots to $args free URLs (optional)
  ###
  # if ($is_bot) {
  #   return 301 $scheme://$host$request_uri;
  # }

  ###
  ### Return 404 on special PHP URLs to avoid revealing version used,
  ### even indirectly. See also: https://drupal.org/node/2116387
  ###
  if ( $args ~* "=PHP[A-Z0-9]{8}-" ) {
    return 404;
  }

  ###
  ### For Drupal >= 7
  ###
  if ($sent_http_x_generator) {
    add_header X-Info-Gen "Modern";
    rewrite ^ /index.php?$query_string last;
  }
  ###
  ### For Drupal <= 6
  ###
  rewrite ^/(.*)$ /index.php?q=$1 last;
}

###
### Send all non-static requests to php-fpm, restricted to known php file.
###
location = /index.php {
<?php if ($satellite_mode == 'boa'): ?>
  limit_conn    limreq 88;
  add_header X-Device "$device";
  add_header X-GeoIP-Country-Code "$geoip_country_code";
  add_header X-GeoIP-Country-Name "$geoip_country_name";
<?php endif; ?>
<?php if ($nginx_config_mode == 'extended'): ?>
  add_header X-Speed-Cache "$upstream_cache_status";
  add_header X-Speed-Cache-UID "$cache_uid";
  add_header X-Speed-Cache-Key "$key_uri";
  add_header X-NoCache "$nocache_details";
  add_header X-This-Proto "$http_x_forwarded_proto";
  add_header X-Server-Name "$main_site_name";
  add_header Access-Control-Allow-Origin *;
  add_header X-Content-Type-Options nosniff;
  add_header X-XSS-Protection "1; mode=block";
<?php endif; ?>
  add_header Cache-Control "no-store, no-cache, must-revalidate, post-check=0, pre-check=0";
  tcp_nopush    off;
  keepalive_requests 0;
  try_files     $uri =404; ### check for existence of php file first
<?php if ($satellite_mode == 'boa'): ?>
  fastcgi_pass  unix:/var/run/$user_socket.fpm.socket;
<?php elseif ($phpfpm_mode == 'port'): ?>
  fastcgi_pass  127.0.0.1:9000;
<?php else: ?>
  fastcgi_pass  unix:<?php print $phpfpm_socket_path; ?>;
<?php endif; ?>
<?php if ($nginx_has_upload_progress): ?>
  track_uploads uploads 60s; ### required for upload progress
<?php endif; ?>
  ###
  ### Use Nginx cache for all visitors.
  ###
  set $nocache "";
  if ( $nocache_details ~ (?:AegirCookie|Args|Skip) ) {
    set $nocache "NoCache";
  }
  fastcgi_cache speed;
  fastcgi_cache_methods GET HEAD; ### Nginx default, but added for clarity
  fastcgi_cache_min_uses 1;
  fastcgi_cache_key "$scheme$is_bot$device$host$request_method$key_uri$cache_uid$http_x_forwarded_proto$sent_http_x_local_proto$cookie_respimg";
  fastcgi_cache_valid 200 10s;
  fastcgi_cache_valid 301 1m;
  fastcgi_cache_valid 302 403 404 1s;
  fastcgi_cache_lock on;
  fastcgi_ignore_headers Cache-Control Expires;
  fastcgi_pass_header Set-Cookie;
  fastcgi_pass_header X-Accel-Expires;
  fastcgi_pass_header X-Accel-Redirect;
  fastcgi_no_cache $cookie_NoCacheID $http_authorization $http_pragma $nocache;
  fastcgi_cache_bypass $cookie_NoCacheID $http_authorization $http_pragma $nocache;
  fastcgi_cache_use_stale error http_500 http_503 invalid_header timeout updating;
}
<?php endif; ?>

###
### Send other known php requests/files to php-fpm without any caching.
###
<?php if ($nginx_config_mode == 'extended'): ?>
location ~* ^/(?:core/)?(?:boost_stats|rtoc|js)\.php$ {
<?php else: ?>
location ~* ^/(?:index|cron|boost_stats|update|authorize|xmlrpc)\.php$ {
<?php endif; ?>
<?php if ($satellite_mode == 'boa'): ?>
  limit_conn   limreq 88;
  if ($is_bot) {
    return 404;
  }
<?php endif; ?>
  tcp_nopush   off;
  keepalive_requests 0;
  access_log   off;
  try_files    $uri =404; ### check for existence of php file first
<?php if ($satellite_mode == 'boa'): ?>
  fastcgi_pass unix:/var/run/$user_socket.fpm.socket;
<?php elseif ($phpfpm_mode == 'port'): ?>
  fastcgi_pass 127.0.0.1:9000;
<?php else: ?>
  fastcgi_pass unix:<?php print $phpfpm_socket_path; ?>;
<?php endif; ?>
}

<?php if ($nginx_config_mode == 'extended'): ?>
###
### Allow access to /authorize.php and /update.php only for logged in admin user.
###
location ~* ^/(?:core/)?(?:authorize|update)\.php$ {
  error_page 418 = @allowupdate;
  if ( $cache_uid ) {
    return 418;
  }
  return 404;
}

###
### Internal location for /authorize.php and /update.php restricted access.
###
location @allowupdate {
  limit_conn   limreq 88;
  tcp_nopush   off;
  keepalive_requests 0;
  access_log   off;
  try_files    $uri =404; ### check for existence of php file first
<?php if ($satellite_mode == 'boa'): ?>
  fastcgi_pass unix:/var/run/$user_socket.fpm.socket;
<?php elseif ($phpfpm_mode == 'port'): ?>
  fastcgi_pass 127.0.0.1:9000;
<?php else: ?>
  fastcgi_pass unix:<?php print $phpfpm_socket_path; ?>;
<?php endif; ?>
}
<?php endif; ?>

###
### Deny access to any not listed above php files with 404 error.
###
location ~* ^.+\.php$ {
  return 404;
}

#######################################################
<?php if ($nginx_config_mode == 'extended'): ?>
###  nginx.conf site level extended vhost include end
<?php else: ?>
###  nginx.conf site level basic vhost include end
<?php endif; ?>
#######################################################
