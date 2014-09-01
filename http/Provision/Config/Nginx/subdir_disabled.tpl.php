location ^~ /<?php print $subdir; ?>/ {
  root         /var/www/nginx-default;
  index        index.html index.htm;
  ### Do not reveal Aegir front-end URL here.
}
