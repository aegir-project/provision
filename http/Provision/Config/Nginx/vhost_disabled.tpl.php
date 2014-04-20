<?php
$ip_address = !empty($ip_address) ? $ip_address : '*';
?>
server {
  limit_conn   limreq 555;
<?php
if ($ip_address == '*') {
  print "  listen       {$ip_address}:{$http_port};\n";
}
else {
  foreach ($server->ip_addresses as $ip) {
    print "  listen       {$ip}:{$http_port};\n";
  }
}
?>
  server_name  <?php print $this->uri . ' ' . implode(' ', $this->aliases); ?>;
  root         /var/www/nginx-default;
  index        index.html index.htm;
  ### Do not reveal Aegir front-end URL here.
}
