<?php
$nginx_config_mode = drush_get_option('nginx_config_mode');
if (!$nginx_config_mode && $server->nginx_config_mode) {
  $nginx_config_mode = $server->nginx_config_mode;
}
$phpfpm_mode = drush_get_option('phpfpm_mode');
if (!$phpfpm_mode && $server->phpfpm_mode) {
  $phpfpm_mode = $server->phpfpm_mode;
}
$nginx_is_modern = drush_get_option('nginx_is_modern');
if (!$nginx_is_modern && $server->nginx_is_modern) {
  $nginx_is_modern = $server->nginx_is_modern;
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

###
### Master location for subdir support (start)
###
location ^~ /<?php print $subdir; ?> {

  root  <?php print "{$this->root}"; ?>;

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

  ###
  ### Deny listed requests for security reasons.
  ###
  if ($is_denied) {
    return 403;
  }

###
### HTTPRL standard support.
###
location ^~ /<?php print $subdir; ?>/httprl_async_function_callback {
  location ~* ^/<?php print $subdir; ?>/httprl_async_function_callback {
    access_log off;
    add_header X-Header "HTTPRL 2.0";
    set $nocache_details "Skip";
    try_files /httprl_async_function_callback $uri @nobots_<?php print $subdir; ?>;
  }
}

###
### HTTPRL test mode support.
###
location ^~ /<?php print $subdir; ?>/admin/httprl-test {
  location ~* ^/<?php print $subdir; ?>/admin/httprl-test {
    access_log off;
    add_header X-Header "HTTPRL 2.1";
    set $nocache_details "Skip";
    try_files /admin/httprl-test $uri @nobots_<?php print $subdir; ?>;
  }
}

###
### CDN Far Future expiration support.
###
location ^~ /<?php print $subdir; ?>/cdn/farfuture/ {
  tcp_nodelay   off;
  access_log    off;
  log_not_found off;
  etag          off;
  gzip_http_version 1.0;
  if_modified_since exact;
  set $nocache_details "Skip";
  location ~* ^/<?php print $subdir; ?>/(cdn/farfuture/.+\.(?:css|js|jpe?g|gif|png|ico|bmp|svg|swf|pdf|docx?|xlsx?|pptx?|tiff?|txt|rtf|class|otf|ttf|woff|eot|less))$ {
    expires max;
    add_header Access-Control-Allow-Origin *;
    add_header X-Header "CDN Far Future Generator 1.0";
    add_header Cache-Control "no-transform, public";
    add_header Last-Modified "Wed, 20 Jan 1988 04:20:42 GMT";
    rewrite ^/<?php print $subdir; ?>/cdn/farfuture/[^/]+/[^/]+/(.+)$ /$1 break;
    try_files /$1 $uri @nobots_<?php print $subdir; ?>;
  }
  location ~* ^/<?php print $subdir; ?>/(cdn/farfuture/) {
    expires epoch;
    add_header Access-Control-Allow-Origin *;
    add_header X-Header "CDN Far Future Generator 1.1";
    add_header Cache-Control "private, must-revalidate, proxy-revalidate";
    rewrite ^/<?php print $subdir; ?>/cdn/farfuture/[^/]+/[^/]+/(.+)$ /$1 break;
    try_files /$1 $uri @nobots_<?php print $subdir; ?>;
  }
  try_files $uri @nobots_<?php print $subdir; ?>;
}
<?php endif; ?>

  ###
  ### If favicon else return error 204.
  ###
  location = /<?php print $subdir; ?>/favicon.ico {
    access_log    off;
    log_not_found off;
    expires       30d;
    try_files     /sites/$main_site_name/files/favicon.ico /sites/$host/files/favicon.ico /favicon.ico $uri =204;
  }

  ###
  ### Support for http://drupal.org/project/robotstxt module
  ### and static file in the sites/domain/files directory.
  ###
  location = /<?php print $subdir; ?>/robots.txt {
    access_log    off;
    log_not_found off;
<?php if ($nginx_config_mode == 'extended'): ?>
    try_files /sites/$main_site_name/files/$host.robots.txt /sites/$main_site_name/files/robots.txt /sites/$host/files/robots.txt /robots.txt $uri @cache_<?php print $subdir; ?>;
<?php else: ?>
    try_files /sites/$main_site_name/files/$host.robots.txt /sites/$main_site_name/files/robots.txt /sites/$host/files/robots.txt /robots.txt $uri @drupal_<?php print $subdir; ?>;
<?php endif; ?>
  }

<?php if ($nginx_config_mode == 'extended'): ?>
  ###
  ### Allow local access to support wget method in Aegir settings
  ### for running sites cron.
  ###
  location = /<?php print $subdir; ?>/cron.php {

    include       fastcgi_params;

    fastcgi_param db_type   <?php print urlencode($db_type); ?>;
    fastcgi_param db_name   <?php print urlencode($db_name); ?>;
    fastcgi_param db_user   <?php print urlencode($db_user); ?>;
    fastcgi_param db_passwd <?php print urlencode($db_passwd); ?>;
    fastcgi_param db_host   <?php print urlencode($db_host); ?>;
    fastcgi_param db_port   <?php print urlencode($db_port); ?>;

    fastcgi_param  HTTP_HOST           <?php print $subdir; ?>.$host;
    fastcgi_param  RAW_HOST            $host;
    fastcgi_param  SITE_SUBDIR         <?php print $subdir; ?>;

    fastcgi_param  REDIRECT_STATUS     200;
    fastcgi_index  index.php;

    set $real_fastcgi_script_name cron.php;
    fastcgi_param SCRIPT_FILENAME <?php print "{$this->root}"; ?>/$real_fastcgi_script_name;

    tcp_nopush   off;
    keepalive_requests 0;
<?php if ($satellite_mode == 'boa'): ?>
    access_log   off;
    allow        127.0.0.1;
    deny         all;
<?php endif; ?>
    try_files    /cron.php $uri =404;
<?php if ($phpfpm_mode == 'port'): ?>
    fastcgi_pass 127.0.0.1:9000;
<?php else: ?>
    fastcgi_pass unix:/var/run/php5-fpm.sock;
<?php endif; ?>
  }

  ###
  ### Send search to php-fpm early so searching for node.js will work.
  ### Deny bots on search uri.
  ###
  location ^~ /<?php print $subdir; ?>/search {
    location ~* ^/<?php print $subdir; ?>/search {
      if ($is_bot) {
        return 403;
      }
      try_files /search $uri @cache_<?php print $subdir; ?>;
    }
  }

  ###
  ### Support for https://drupal.org/project/js module.
  ###
  location ^~ /<?php print $subdir; ?>/js/ {
    location ~* ^/<?php print $subdir; ?>/js/ {
      if ($is_bot) {
        return 403;
      }
      rewrite ^/<?php print $subdir; ?>/(.*)$ /js.php?q=$1 last;
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
  location ^~ /<?php print $subdir; ?>/progress {
    access_log off;
    upload_progress_json_output;
    report_uploads uploads;
  }
<?php endif; ?>

<?php if ($satellite_mode == 'boa'): ?>
  ###
  ### Deny cache details display.
  ###
  location ^~ /<?php print $subdir; ?>/admin/settings/performance/cache-backend {
    access_log off;
    rewrite ^ $scheme://$host/<?php print $subdir; ?>/admin/settings/performance permanent;
  }

  ###
  ### Deny cache details display.
  ###
  location ^~ /<?php print $subdir; ?>/admin/config/development/performance/redis {
    access_log off;
    rewrite ^ $scheme://$host/<?php print $subdir; ?>/admin/config/development/performance permanent;
  }
<?php endif; ?>

  ###
  ### Support for backup_migrate module download/restore/delete actions.
  ###
  location ^~ /<?php print $subdir; ?>/admin {
    if ($is_bot) {
      return 403;
    }
    access_log off;
    set $nocache_details "Skip";
    try_files /admin $uri @drupal_<?php print $subdir; ?>;
  }

  ###
  ### Avoid caching /civicrm* and protect it from bots.
  ###
  location ^~ /<?php print $subdir; ?>/civicrm {
    if ($is_bot) {
      return 403;
    }
    set $nocache_details "Skip";
    try_files /civicrm $uri @drupal_<?php print $subdir; ?>;
  }

  ###
  ### Support for audio module.
  ###
  location ^~ /<?php print $subdir; ?>/audio/download {
    location ~* ^/<?php print $subdir; ?>/(audio/download/.*/.*\.(?:mp3|mp4|m4a|ogg))$ {
      if ($is_bot) {
        return 403;
      }
      tcp_nopush off;
      access_log off;
      set $nocache_details "Skip";
      try_files /$1 $uri @drupal_<?php print $subdir; ?>;
    }
  }
<?php endif; ?>

  ###
  ### Deny listed requests for security reasons.
  ###
  location ~* (/\..*|settings\.php$|\.(?:git|htaccess|engine|make|config|inc|ini|info|install|module|profile|pl|po|sh|.*sql|theme|tpl(?:\.php)?|xtmpl)$|^(?:Entries.*|Repository|Root|Tag|Template))$ {
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
  location ~* /files/civicrm/(?:ConfigAndLog|upload|templates_c) {
    access_log off;
    return 404;
  }

<?php if ($nginx_config_mode == 'extended'): ?>
  ###
  ### Responsive Images support.
  ### http://drupal.org/project/responsive_images
  ###
  location ~* ^/<?php print $subdir; ?>/.*\.r\.(?:jpe?g|png|gif) {
    if ( $http_cookie ~* "rwdimgsize=large" ) {
      rewrite ^/<?php print $subdir; ?>/(.*)/mobile/(.*)\.r(\.(?:jpe?g|png|gif))$ /<?php print $subdir; ?>/$1/desktop/$2$3 last;
    }
    rewrite ^/<?php print $subdir; ?>/(.*)\.r(\.(?:jpe?g|png|gif))$ /<?php print $subdir; ?>/$1$2 last;
    access_log off;
    add_header X-Header "RI Generator 1.0";
    set $nocache_details "Skip";
    try_files  $uri @drupal_<?php print $subdir; ?>;
  }

  ###
  ### Adaptive Image Styles support.
  ### http://drupal.org/project/ais
  ###
  location ~* ^/<?php print $subdir; ?>/(?:.+)/files/styles/adaptive/(?:.+)$ {
    if ( $http_cookie ~* "ais=(?<ais_cookie>[a-z0-9-_]+)" ) {
      rewrite ^/<?php print $subdir; ?>/(.+)/files/styles/adaptive/(.+)$ /<?php print $subdir; ?>/$1/files/styles/$ais_cookie/$2 last;
    }
    access_log off;
    add_header X-Header "AIS Generator 1.0";
    set $nocache_details "Skip";
    try_files  $uri @drupal_<?php print $subdir; ?>;
  }
<?php endif; ?>

  ###
  ### Imagecache and imagecache_external support.
  ###
  location ~* ^/<?php print $subdir; ?>/((?:external|system|files/imagecache|files/styles)/.*) {
    access_log off;
    log_not_found off;
    expires    30d;
<?php if ($nginx_config_mode == 'extended'): ?>
    add_header X-Header "IC Generator 1.0";
    set $nocache_details "Skip";
<?php endif; ?>
    try_files /$1 $uri @drupal_<?php print $subdir; ?>;
  }

  ###
  ### Deny direct access to backups.
  ###
  location ~* ^/<?php print $subdir; ?>/sites/.*/files/backup_migrate/ {
    access_log off;
    deny all;
  }

  ###
  ### Deny direct access to config files in Drupal 8.
  ###
  location ~* ^/<?php print $subdir; ?>/sites/.*/files/config_.* {
    access_log off;
    deny all;
  }

<?php if ($nginx_config_mode == 'extended'): ?>
  ###
  ### Private downloads are always sent to the drupal backend.
  ### Note: this location doesn't work with X-Accel-Redirect.
  ###
  location ~* ^/<?php print $subdir; ?>/(sites/.*/files/private/.*) {
    if ($is_bot) {
      return 403;
    }
    access_log off;
    rewrite    ^/<?php print $subdir; ?>/sites/.*/files/private/(.*)$ $scheme://$host/<?php print $subdir; ?>/system/files/private/$1 permanent;
    add_header X-Header "Private Generator 1.0a";
    set $nocache_details "Skip";
    try_files /$1 $uri @drupal_<?php print $subdir; ?>;
  }
<?php endif; ?>

  ###
  ### Deny direct access to private downloads in sites/domain/private.
  ### Note: this location works with X-Accel-Redirect.
  ###
  location ~* ^/<?php print $subdir; ?>/sites/.*/private/ {
<?php if ($nginx_config_mode == 'extended'): ?>
    if ($is_bot) {
      return 403;
    }
<?php endif; ?>
    access_log off;
    internal;
  }

<?php if ($nginx_config_mode == 'extended'): ?>
  ###
  ### Deny direct access to private downloads also for short, rewritten URLs.
  ### Note: this location works with X-Accel-Redirect.
  ###
  location ~* /<?php print $subdir; ?>/files/private/ {
    if ($is_bot) {
      return 403;
    }
    access_log off;
    internal;
  }

  ###
  ### Wysiwyg Fields support.
  ###
  location ~* ^/<?php print $subdir; ?>/(.*/wysiwyg_fields/(?:plugins|scripts)/.*\.(?:js|css)) {
    access_log off;
    log_not_found off;
    try_files /$1 $uri @nobots_<?php print $subdir; ?>;
  }

  ###
  ### Advagg_css and Advagg_js support.
  ###
  location ~* ^/<?php print $subdir; ?>/(.*/files/advagg_(?:css|js).*) {
    expires    max;
    access_log off;
<?php if ($nginx_is_modern): ?>
    etag       off;
<?php else: ?>
    add_header ETag "";
<?php endif; ?>
    add_header Cache-Control "max-age=31449600, no-transform, public";
    add_header Access-Control-Allow-Origin *;
    add_header X-Header "AdvAgg Generator 2.0";
    set $nocache_details "Skip";
    try_files /$1 $uri @nobots_<?php print $subdir; ?>;
  }

  ###
  ### Make css files compatible with boost caching.
  ###
  location ~* ^/<?php print $subdir; ?>/(.*\.css)$ {
    access_log  off;
    tcp_nodelay off;
    expires     max; #if using aggregator
    add_header  X-Header "Boost Citrus 2.1";
    try_files   /cache/perm/$host${uri}_.css /$1 $uri =404;
  }

  ###
  ### Make js files compatible with boost caching.
  ###
  location ~* ^/<?php print $subdir; ?>/(.*\.(?:js|htc))$ {
    access_log  off;
    tcp_nodelay off;
    expires     max; # if using aggregator
    add_header  X-Header "Boost Citrus 2.2";
    try_files   /cache/perm/$host${uri}_.js /$1 $uri =404;
  }

  ###
  ### Support for static .json files with fast 404 +Boost compatibility.
  ###
  location ~* ^/<?php print $subdir; ?>/sites/.*/files/(.*\.json)$ {
    access_log  off;
    tcp_nodelay off;
    expires     max; ### if using aggregator
    add_header  X-Header "Boost Citrus 2.3";
    add_header  Access-Control-Allow-Origin *;
    try_files   /cache/normal/$host${uri}_.json /$1 $uri =404;
  }

  ###
  ### Support for dynamic .json requests.
  ###
  location ~* (.*\.json)$ {
    try_files /$1 $uri @cache_<?php print $subdir; ?>;
  }
<?php endif; ?>

  ###
  ### Serve & no-log static files & images directly,
  ### without all standard drupal rewrites, php-fpm etc.
  ###
  location ~* ^/<?php print $subdir; ?>/(.+\.(?:jpe?g|gif|png|ico|bmp|svg|swf|pdf|docx?|xlsx?|pptx?|tiff?|txt|rtf|cgi|bat|pl|dll|aspx?|class|otf|ttf|woff|eot|less))$ {
    expires       30d;
    tcp_nodelay   off;
    access_log    off;
    log_not_found off;
    add_header  Access-Control-Allow-Origin *;
    try_files   /$1 $uri =404;
  }

  ###
  ### Serve & log bigger media/static/archive files directly,
  ### without all standard drupal rewrites, php-fpm etc.
  ###
  location ~* ^/<?php print $subdir; ?>/(.+\.(?:avi|mpe?g|mov|wmv|mp3|mp4|m4a|ogg|ogv|flv|wav|midi|zip|tar|t?gz|rar|dmg|exe))$ {
    expires     30d;
    tcp_nodelay off;
    tcp_nopush  off;
    add_header  Access-Control-Allow-Origin *;
    try_files   /$1 $uri =404;
  }

  ###
  ### Serve & no-log some static files as is, without forcing default_type.
  ###
  location ~* ^/<?php print $subdir; ?>/((?:cross-?domain)\.xml)$ {
    access_log  off;
    tcp_nodelay off;
    expires     30d;
    add_header  X-Header "XML Generator 1.0";
    try_files   /$1 $uri =404;
  }

<?php if ($nginx_config_mode == 'extended'): ?>
  ###
  ### Allow some known php files (like serve.php in the ad module).
  ###
  location ~* ^/<?php print $subdir; ?>/(.*/(?:modules|libraries)/(?:contrib/)?(?:ad|tinybrowser|f?ckeditor|tinymce|wysiwyg_spellcheck|ecc|civicrm|fbconnect|radioactivity)/.*\.php)$ {

<?php if ($satellite_mode == 'boa'): ?>
    limit_conn   limreq 88;
<?php endif; ?>

    include       fastcgi_params;

    fastcgi_param db_type   <?php print urlencode($db_type); ?>;
    fastcgi_param db_name   <?php print urlencode($db_name); ?>;
    fastcgi_param db_user   <?php print urlencode($db_user); ?>;
    fastcgi_param db_passwd <?php print urlencode($db_passwd); ?>;
    fastcgi_param db_host   <?php print urlencode($db_host); ?>;
    fastcgi_param db_port   <?php print urlencode($db_port); ?>;

    fastcgi_param  HTTP_HOST           <?php print $subdir; ?>.$host;
    fastcgi_param  RAW_HOST            $host;
    fastcgi_param  SITE_SUBDIR         <?php print $subdir; ?>;

    fastcgi_param  REDIRECT_STATUS     200;
    fastcgi_index  index.php;

    set $real_fastcgi_script_name $1;
    fastcgi_param SCRIPT_FILENAME <?php print "{$this->root}"; ?>/$real_fastcgi_script_name;

    tcp_nopush   off;
    keepalive_requests 0;
    access_log   off;
    if ($is_bot) {
      return 403;
    }
    try_files    /$1 $uri =404;
<?php if ($phpfpm_mode == 'port'): ?>
    fastcgi_pass 127.0.0.1:9000;
<?php else: ?>
    fastcgi_pass unix:/var/run/php5-fpm.sock;
<?php endif; ?>
  }

  ###
  ### Deny crawlers and never cache known AJAX and webform requests.
  ###
  location ~* ^/<?php print $subdir; ?>/(.*(?:ahah|ajax|batch|autocomplete|webform|done|progress/|x-progress-id|js/.*).*)$ {
    if ($is_bot) {
      return 403;
    }
    access_log off;
    log_not_found off;
<?php if ($nginx_config_mode == 'extended'): ?>
    set $nocache_details "Skip";
    try_files /$1 $uri @nobots_<?php print $subdir; ?>;
<?php else: ?>
    try_files /$1 $uri @drupal_<?php print $subdir; ?>;
<?php endif; ?>
  }

  ###
  ### Serve & no-log static helper files used in some wysiwyg editors.
  ###
  location ~* ^/<?php print $subdir; ?>/(sites/.*/(?:modules|libraries)/(?:contrib/)?(?:tinybrowser|f?ckeditor|tinymce|flowplayer|jwplayer|videomanager)/.*\.(?:html?|xml))$ {
    if ($is_bot) {
      return 403;
    }
    access_log      off;
    tcp_nodelay     off;
    expires         30d;
    try_files /$1 $uri =404;
  }

  ###
  ### Serve & no-log any not specified above static files directly.
  ###
  location ~* ^/<?php print $subdir; ?>/(sites/.*/files/.*) {
    root  <?php print "{$this->root}"; ?>;
    rewrite     ^/<?php print $subdir; ?>/sites/(.*)$ /sites/$main_site_name/$1 last;
    access_log      off;
    tcp_nodelay     off;
    expires         30d;
    try_files /$1 $uri =404;
  }

  ###
  ### Make feeds compatible with boost caching and set correct mime type.
  ###
  location ~* ^/<?php print $subdir; ?>/(.*\.xml)$ {
    if ( $request_method = POST ) {
      return 405;
    }
    if ( $cache_uid ) {
      return 405;
    }
    error_page 405 = @drupal_<?php print $subdir; ?>;
    access_log off;
    add_header Expires "Tue, 24 Jan 1984 08:00:00 GMT";
    add_header Cache-Control "must-revalidate, post-check=0, pre-check=0";
    add_header X-Header "Boost Citrus 2.4";
    charset    utf-8;
    types { }
    default_type text/xml;
    try_files /cache/normal/$host${uri}_.xml /cache/normal/$host${uri}_.html /$1 $uri @drupal_<?php print $subdir; ?>;
  }

  ###
  ### Deny bots on never cached uri.
  ###
  location ~* ^/<?php print $subdir; ?>/((?:.*/)?(?:admin|user|cart|checkout|logout|comment/reply)) {
    if ($is_bot) {
      return 403;
    }
    access_log off;
    set $nocache_details "Skip";
    try_files /$1 $uri @drupal_<?php print $subdir; ?>;
  }

  ###
  ### Protect from DoS attempts on never cached uri.
  ###
  location ~* ^/<?php print $subdir; ?>/((?:.*/)?(?:node/[0-9]+/edit|node/add)) {
    if ($is_bot) {
      return 403;
    }
    access_log off;
    set $nocache_details "Skip";
    try_files /$1 $uri @drupal_<?php print $subdir; ?>;
  }

  ###
  ### Protect from DoS attempts on never cached uri.
  ###
  location ~* ^/<?php print $subdir; ?>/((?:.*/)?(?:node/[0-9]+/delete|approve)) {
    if ($cache_uid = '') {
      return 403;
    }
    if ($is_bot) {
      return 403;
    }
    access_log off;
    set $nocache_details "Skip";
    try_files /$1 $uri @drupal_<?php print $subdir; ?>;
  }
<?php endif; ?>

<?php if ($satellite_mode == 'boa'): ?>
  ###
  ### Rewrite legacy requests with /index.php to extension-free URL.
  ###
  if ( $args ~* "^q=(?<query_value>.*)" ) {
    rewrite ^/<?php print $subdir; ?>/index.php$ $scheme://$host/<?php print $subdir; ?>/?q=$query_value? permanent;
  }
<?php endif; ?>

  ###
  ### Redirect to working homepage.
  ###
  location = /<?php print $subdir; ?> {
    rewrite ^ $scheme://$host/<?php print $subdir; ?>/? permanent;
  }

  ###
  ### Catch all unspecified requests.
  ###
  location /<?php print $subdir; ?>/ {
<?php if ($nginx_config_mode == 'extended'): ?>
<?php if ($satellite_mode == 'boa'): ?>
  if ( $http_user_agent ~* wget ) {
    return 403;
  }
<?php endif; ?>
    try_files $uri @cache_<?php print $subdir; ?>;
<?php else: ?>
    try_files $uri @drupal_<?php print $subdir; ?>;
<?php endif; ?>
  }

  ###
  ### Send other known php requests/files to php-fpm without any caching.
  ###
<?php if ($nginx_config_mode == 'extended'): ?>
  location ~* ^/<?php print $subdir; ?>/((core/)?(boost_stats|rtoc|js))\.php$ {
<?php else: ?>
  location ~* ^/<?php print $subdir; ?>/(cron|boost_stats|update|authorize)\.php$ {
<?php endif; ?>
<?php if ($satellite_mode == 'boa'): ?>
    limit_conn   limreq 88;
    if ($is_bot) {
      return 404;
    }
<?php endif; ?>

    include       fastcgi_params;

    fastcgi_param db_type   <?php print urlencode($db_type); ?>;
    fastcgi_param db_name   <?php print urlencode($db_name); ?>;
    fastcgi_param db_user   <?php print urlencode($db_user); ?>;
    fastcgi_param db_passwd <?php print urlencode($db_passwd); ?>;
    fastcgi_param db_host   <?php print urlencode($db_host); ?>;
    fastcgi_param db_port   <?php print urlencode($db_port); ?>;

    fastcgi_param  HTTP_HOST           <?php print $subdir; ?>.$host;
    fastcgi_param  RAW_HOST            $host;
    fastcgi_param  SITE_SUBDIR         <?php print $subdir; ?>;

    fastcgi_param  REDIRECT_STATUS     200;
    fastcgi_index  index.php;

    set $real_fastcgi_script_name $1.php;
    fastcgi_param SCRIPT_FILENAME <?php print "{$this->root}"; ?>/$real_fastcgi_script_name;

    tcp_nopush   off;
    keepalive_requests 0;
    access_log   off;
    try_files    /$1.php =404; ### check for existence of php file first
<?php if ($phpfpm_mode == 'port'): ?>
    fastcgi_pass  127.0.0.1:9000;
<?php else: ?>
    fastcgi_pass  unix:/var/run/php5-fpm.sock;
<?php endif; ?>
  }

<?php if ($nginx_config_mode == 'extended'): ?>
  ###
  ### Allow access to /authorize.php and /update.php only for logged in admin user.
  ###
  location ~* ^/<?php print $subdir; ?>/((?:core/)?(authorize|update))\.php$ {
    set $real_fastcgi_script_name $1.php;
    error_page 418 = @allowupdate_<?php print $subdir; ?>;
    if ( $cache_uid ) {
      return 418;
    }
    return 404;
  }
<?php endif; ?>

  ###
  ### Rewrite legacy requests with /<?php print $subdir; ?>/index.php to extension-free URL.
  ###
  if ( $args ~* "^q=(?<query_value>.*)" ) {
    rewrite ^/<?php print $subdir; ?>/index.php$ $scheme://$host/<?php print $subdir; ?>/?q=$query_value? permanent;
  }

  ###
  ### Send all non-static requests to php-fpm, restricted to known php file.
  ###
  location = /<?php print $subdir; ?>/index.php {
<?php if ($satellite_mode == 'boa'): ?>
    internal;
    limit_conn    limreq 88;
    add_header    X-Device "$device";
    add_header    X-GeoIP-Country-Code "$geoip_country_code";
    add_header    X-GeoIP-Country-Name "$geoip_country_name";
<?php endif; ?>
<?php if ($nginx_config_mode == 'extended'): ?>
    add_header    X-Speed-Cache "$upstream_cache_status";
    add_header    X-Speed-Cache-UID "$cache_uid";
    add_header    X-Speed-Cache-Key "$key_uri";
    add_header    X-NoCache "$nocache_details";
    add_header    X-This-Proto "$http_x_forwarded_proto";
    add_header    X-Server-Name "$main_site_name";
<?php endif; ?>

    root          <?php print "{$this->root}"; ?>;

    include       fastcgi_params;

    fastcgi_param db_type   <?php print urlencode($db_type); ?>;
    fastcgi_param db_name   <?php print urlencode($db_name); ?>;
    fastcgi_param db_user   <?php print urlencode($db_user); ?>;
    fastcgi_param db_passwd <?php print urlencode($db_passwd); ?>;
    fastcgi_param db_host   <?php print urlencode($db_host); ?>;
    fastcgi_param db_port   <?php print urlencode($db_port); ?>;

    fastcgi_param  HTTP_HOST           <?php print $subdir; ?>.$host;
    fastcgi_param  RAW_HOST            $host;
    fastcgi_param  SITE_SUBDIR         <?php print $subdir; ?>;

    fastcgi_param  REDIRECT_STATUS     200;
    fastcgi_index  index.php;

    set $real_fastcgi_script_name index.php;
    fastcgi_param  SCRIPT_FILENAME     <?php print "{$this->root}"; ?>/$real_fastcgi_script_name;

    add_header    Cache-Control "no-store, no-cache, must-revalidate, post-check=0, pre-check=0";
    tcp_nopush    off;
    keepalive_requests 0;
    try_files     /index.php =404; ### check for existence of php file first
<?php if ($phpfpm_mode == 'port'): ?>
    fastcgi_pass  127.0.0.1:9000;
<?php else: ?>
    fastcgi_pass  unix:/var/run/php5-fpm.sock;
<?php endif; ?>
<?php if ($nginx_has_upload_progress): ?>
    track_uploads uploads 60s; ### required for upload progress
<?php endif; ?>
<?php if ($nginx_config_mode == 'extended'): ?>
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
    fastcgi_cache_key "$is_bot$device$host$request_method$key_uri$cache_uid$http_x_forwarded_proto$sent_http_x_local_proto$cookie_respimg";
    fastcgi_cache_valid 200 10s;
    fastcgi_cache_valid 302 1m;
    fastcgi_cache_valid 301 403 404 5s;
    fastcgi_cache_valid 500 502 503 504 1s;
    fastcgi_ignore_headers Cache-Control Expires;
    fastcgi_pass_header Set-Cookie;
    fastcgi_pass_header X-Accel-Expires;
    fastcgi_pass_header X-Accel-Redirect;
    fastcgi_no_cache $cookie_NoCacheID $http_authorization $http_pragma $nocache;
    fastcgi_cache_bypass $cookie_NoCacheID $http_authorization $http_pragma $nocache;
    fastcgi_cache_use_stale error http_500 http_503 invalid_header timeout updating;
<?php endif; ?>
  }

  ###
  ### Deny access to any not listed above php files with 404 error.
  ###
  location ~* ^.+\.php$ {
    return 404;
  }

}
###
### Master location for subdir support (end)
###

<?php if ($nginx_config_mode == 'extended'): ?>
###
### Boost compatible cache check.
###
location @cache_<?php print $subdir; ?> {
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
  error_page 405 = @drupal_<?php print $subdir; ?>;
  add_header Expires "Tue, 24 Jan 1984 08:00:00 GMT";
  add_header Cache-Control "no-store, no-cache, must-revalidate, post-check=0, pre-check=0";
  add_header X-Header "Boost Citrus 1.9";
  charset    utf-8;
  try_files  /cache/normal/$host${uri}_$args.html @drupal_<?php print $subdir; ?>;
}
<?php endif; ?>

###
### Send all not cached requests to drupal with clean URLs support.
###
location @drupal_<?php print $subdir; ?> {
<?php if ($nginx_config_mode == 'extended'): ?>
  error_page 418 = @nobots_<?php print $subdir; ?>;
  if ($args) {
    return 418;
  }
<?php endif; ?>
  rewrite ^/<?php print $subdir; ?>/(.*)$  /<?php print $subdir; ?>/index.php?q=$1 last;
}

<?php if ($nginx_config_mode == 'extended'): ?>
###
### Send all known bots to $args free URLs.
###
location @nobots_<?php print $subdir; ?> {
  if ($is_bot) {
    rewrite ^ $scheme://$host$uri? permanent;
  }
  ###
  ### Return 404 on special PHP URLs to avoid revealing version used,
  ### even indirectly. See also: https://drupal.org/node/2116387
  ###
  if ( $args ~* "=PHP[A-Z0-9]{8}-" ) {
    return 404;
  }
  rewrite ^/<?php print $subdir; ?>/(.*)$  /<?php print $subdir; ?>/index.php?q=$1 last;
}

###
### Internal location for /authorize.php and /update.php restricted access.
###
location @allowupdate_<?php print $subdir; ?> {
<?php if ($satellite_mode == 'boa'): ?>
  limit_conn   limreq 88;
<?php endif; ?>
  include       fastcgi_params;

  fastcgi_param db_type   <?php print urlencode($db_type); ?>;
  fastcgi_param db_name   <?php print urlencode($db_name); ?>;
  fastcgi_param db_user   <?php print urlencode($db_user); ?>;
  fastcgi_param db_passwd <?php print urlencode($db_passwd); ?>;
  fastcgi_param db_host   <?php print urlencode($db_host); ?>;
  fastcgi_param db_port   <?php print urlencode($db_port); ?>;

  fastcgi_param  HTTP_HOST           <?php print $subdir; ?>.$host;
  fastcgi_param  RAW_HOST            $host;
  fastcgi_param  SITE_SUBDIR         <?php print $subdir; ?>;

  fastcgi_param  REDIRECT_STATUS     200;
  fastcgi_index  index.php;

  fastcgi_param SCRIPT_FILENAME <?php print "{$this->root}"; ?>/$real_fastcgi_script_name;

  tcp_nopush   off;
  keepalive_requests 0;
  access_log   off;
  try_files    /$real_fastcgi_script_name =404; ### check for existence of php file first

<?php if ($phpfpm_mode == 'port'): ?>
  fastcgi_pass 127.0.0.1:9000;
<?php else: ?>
  fastcgi_pass unix:/var/run/php5-fpm.sock;
<?php endif; ?>
}
<?php endif; ?>

#######################################################
<?php if ($nginx_config_mode == 'extended'): ?>
###  nginx.conf site level extended vhost include end
<?php else: ?>
###  nginx.conf site level basic vhost include end
<?php endif; ?>
#######################################################
