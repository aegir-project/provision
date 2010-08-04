; Bind zonefile
; File managed by Aegir
; Changes here may be lost by user configurations, tread carefully
$TTL <?php print $ttl; ?>

<?php
print("@     IN     SOA  $soa[name] $soa[email] 
			      $soa[serial] ; serial
			      $soa[refresh] ; refresh
			      $soa[retry] ; retry
			      $soa[expire] ; expire
			      $soa[minimum] ; minimum
          )\n");
?>
