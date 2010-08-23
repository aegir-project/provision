; Bind zonefile
; File managed by Aegir
; Changes here may be lost by user configurations, tread carefully
$TTL <?php print $server->dns_ttl; ?>

<?php
print("@     IN     SOA  $server->remote_host $dns_email 
			      $serial ; serial
			      $server->dns_refresh; refresh
			      $server->dns_retry ; retry
			      $server->dns_expire ; expire
			      $server->dns_negativettl ; minimum
          )\n");
?>


       IN  NS    <?php print $server->remote_host ?>. ; primary DNS 
       IN  NS     ns2.example.com. ; secondary DNS
       IN  MX  10 <?php print $server->remote_host ?>. ; external mail provider
; non server domain hosts
<?php

foreach ($records['A'] as $ip) {
  print "   IN  A     {$ip}\n";
}

foreach ($hosts as $host => $info) {
  foreach ($info['A'] as $ip) {
    print "{$info['sub']}   IN  A     {$ip}\n";
  }
}
?>
