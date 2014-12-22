<?php
/**
 * @file
 * Template file for an Aegir-wide drushrc file.
 */
print "<?php \n\n";

print "# A list of Aegir features and their enabled status.\n";
print "\$options['hosting_features'] = ". var_export($hosting_features, TRUE) . ";\n\n";

print "# A list of modules to be excluded because the hosting feature is not enabled.\n";
print "\$options['exclude'] = ". var_export($drush_exclude, TRUE) . ";\n\n";

print "# A list of paths that drush should include even when working outside\n";
print "# the context of the hostmaster site.\n";
print "\$options['include'] = ". var_export($drush_include, TRUE) . ";\n";
?>
