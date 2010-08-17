<?php include('provision_drushrc.tpl.php'); ?>

# Aegir additions
<?php foreach (array('db_type', 'db_port', 'db_host', 'db_user', 'db_passwd', 'db_name') as $key) { ?>
$_SERVER['<?php print $key; ?>'] = $options['<?php print $key; ?>'];
<?php } ?>
