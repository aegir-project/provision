<?php

/**
 * copy of frontend's hosting_dns_split_url
 */
function _dns_split_url($url) {
  $tlds = split("\n", variable_get('hosting_dns_tlds', ''));
  $first_dot = strpos($url, '.');

  # Strip off the first part
  $host = substr($url, 0, $first_dot);
  $zone = substr($url, $first_dot);

  # If the remaining hostname is a TLD
  if (in_array($zone, $tlds)) {
    # Add the first part back, and that's the zone (the host is @)
    $zone = $host . $zone;
    $host = '@';
  } else {
    # that's the zone (and the host is the first part)
    $zone = substr($zone, 1); # strip the leading .
  }

  return array('host' => $host, 'zone' => $zone);
}
