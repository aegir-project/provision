; Bind zonefile
; File managed by Aegir
; Changes here may be lost by user configurations, tread carefully
$TTL <?php print $server->dns_ttl; ?>

<?php
print("@     IN     SOA  $server->remote_host $dns_email (
			      " . $records['@']['SOA']['serial'] . " ; serial
			      $server->dns_refresh; refresh
			      $server->dns_retry ; retry
			      $server->dns_expire ; expire
			      $server->dns_negativettl ; minimum
          )\n");

if (!empty($server->dns_default_mx)) {
  if ($server->dns_default_mx[strlen($server->dns_default_mx)-1] != '.') {
    $server->dns_default_mx .= '.';
  }
  print "@\tIN\tMX\t10\t" . $server->dns_default_mx . "\n";
}

print "@\tIN\tNS\t" . $server->remote_host . " ; primary DNS\n";

foreach ($server->dns_slaves as $slave) {
  print "@\tIN\tNS\t$slave ; slave DNS\n";
}

foreach ($records['@'] as $type => $destinations) {
  if ($type != 'SOA' && $type != 'NS') {
    foreach ($destinations as $destination) {
        print "@\tIN\t$type\t$destination\n";
    }
  }
}

foreach ($records as $name => $record) {
  if ($name != '@') {
    foreach ($record as $type => $destinations) {
      foreach ($destinations as $destination) {
        print "$name\tIN\t$type\t$destination\n";
      }
    }
  }
}
foreach ($hosts as $host => $info) {
  foreach ($info['A'] as $ip) {
    print "{$info['sub']}   IN  A     {$ip}\n";
  }
}
