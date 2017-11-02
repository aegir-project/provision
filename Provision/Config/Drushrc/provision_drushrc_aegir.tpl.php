<?php
/**
 * @file
 * Template file for an Aegir-wide drushrc file.
 */
print "<?php \n\n";

print "# !!!WARNING!!! This file is re-generated on each verify of the hostmaster site.\n";
print "# Any changes tou make to this file will thus soon be lost. Instead, create a\n";
print "# file called 'local.drushrc.php' in the same directory as this one (i.e.\n";
print "# '/var/aegir/.drush/'), and add any custom configuration there.\n";

print "# A list of Aegir features and their enabled status.\n";
print "\$options['hosting_features'] = ". var_export($hosting_features, TRUE) . ";\n\n";

print "# A list of modules to be excluded because the hosting feature is not enabled.\n";
print "\$options['exclude'] = ". var_export($drush_exclude, TRUE) . ";\n\n";

print "# Drush 8 looks at ignored-modules instead of exclude.\n";
print "\$options['ignored-modules'] = \$options['exclude'];\n\n";

print "# A list of paths that drush should include even when working outside\n";
print "# the context of the hostmaster site.\n";
print "\$options['include'] = ". var_export($drush_include, TRUE) . ";\n";

print "# Local non-aegir-generated additions.\n";
print "@include_once(dirname(__FILE__) . '/local.drushrc.php');\n";
?>
