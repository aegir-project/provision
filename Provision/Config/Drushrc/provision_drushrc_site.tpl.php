<?php
/**
 * @file
 * Template file for a drushrc site file.
 */
print "<?php \n"; ?>

<?php foreach ($option_keys as $key) :
  print "\n\$options['$key'] = " . var_export(${$key}, TRUE) . ';';
endforeach;
?>

# Aegir additions.
<?php foreach (array('db_type', 'db_port', 'db_host', 'db_user', 'db_passwd', 'db_name') as $key): ?>
$_SERVER['<?php print $key; ?>'] = $options['<?php print $key; ?>'];
<?php endforeach; ?>
