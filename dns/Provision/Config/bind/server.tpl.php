<?php 

$slave_acl = "";
if (is_array($server->slave_servers_ips)) {
  $slaves = implode(";", $server->slave_servers_ips);
  if (!empty($slaves)) {
    $slave_acl = "allow-transfer { $slaves; };\n";
  }
}

foreach ($records as $key => $name) {
  printf('zone "%s" { type master; file "%s/%s.zone"; allow-query { any; }; %s };' . "\n", $name, $dns_zoned_path, $name, $slave_acl);
}
?>
