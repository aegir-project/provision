<?php
/**
 * @file ProvisionHelp contains the bulk of the provision help system
 */

/**
 * Implementation of hook_help()
 */
function provision_help($section) {
  switch ($section) {
    case 'admin/help#provision':
      $output .= t('<p>The Provision framework is a powerful set of modules that lets you to accomplish a lot of maintenance tasks through the unix command line such as installing new sites, backing them up, rolling back to previous backups and facilitating upgrades.</p>');
      $output .= t('<p>Additionally, the Provision framework is one component of the distributed hosting infrastructure provided by the <code>hostmaster</code> install profile. 
                    The Hostmaster profile is capable of driving several provision backends, in a distributed manner, and provides an interface for the functionality of provision.</p>');
      $output .= t('<p>It is not required to run the hosting front end to use the Provision framework, but the system does not provide much in the way of a web accessible front end, by design.</p>');

      $output .= '<a name=\'requirements\'><h3>' . t('Requirements') . '</h3></a>';
      $output .= t('<p>For a more detailed breakdown of steps that need to be taken to configure Provisioning to run with your system, please read the <a href="@url">in depth requirement documentation</a></p>',
                  array('@url' => url('admin/help/provision/requirements')));
      $output .= '<a name=\'commands\'><h3>' . t('Commands') . '</h3></a>';
      $commands = module_invoke_all('drush_command');
      $output .= "<dl>";
      foreach ($commands as $command => $info) {
        if (preg_match('/^provision/', $command)) {
          if (sizeof($info['arguments'])) {
            $command .= ' ' . implode(' ', (array) key($info['arguments']));            
          }
          if (sizeof($info['optional arguments'])) {
            $command .= ' [' . implode('] [', (array) key($info['optional arguments'])) . ']';  
          }
          $output .= '<dt>' . "<code>drush.php $command</code>" . '</dt>';
          $output .= '<dd>' .  $info["description"] . '</dd>';
        }
      }
      $output .= "</dl>";

      $output .= '<a name=\'options\'><h3>' . t('Options') . '</h3></a>';

      
      $options = module_invoke_all('value_list');

      $output .= "<dl>";
      foreach ($options as $option => $description) {
        $output .= '<dt>' . "<code>--$option</code>" . '</dt>';
        $output .= '<dd>' .  $description . '</dd>';
      }
      $output .= "</dl>";
      
      return $output;
    case 'admin/help/provision#requirements' :
      $username = PROVISION_SCRIPT_USER;
      $group = PROVISION_WEB_GROUP;
      
      $backup_path = PROVISION_BACKUP_PATH;
      $mkdir_cmd['@backup_path'] = $backup_path;      
      $mkdir_cmd['@provision_link'] = url('admin/settings/provision');
      $mkdir_cmd['@mkdir_cmd'] = <<<EOF
    [$username@hm2 ~]$ mkdir $backup_path
    [$username@hm2 ~]$ chown $username:$username $backup_path
    [$username@hm2 ~]$ chmod 0700 $backup_path
EOF;
    $vigr_cmd = <<<EOF
    [$username@hm2 ~]$ sudo /usr/sbin/vigr
EOF;
    $vigr1 = <<<EOF
    $group::99:
EOF;
    $vigr2 = <<<EOF
    $group::99:$username
EOF;
    $vigr3 = <<<EOF
    $group::99:anotheruser,$username
EOF;

    $output .= "<ol>";
    $output .= '<li>' . t('<p><strong>The user account running the script, and the group of the httpd daemon.</strong> 
                          The provision framework takes special care to make sure that the file permissions of the 
                          hosted sites are always as safe as can be, especially to make sure that the web server does 
                          not have the ability to modify the code of the site, therefor this information is required 
                          to assure that safety while keeping the sites accessible. This username needs to be a member
                          of the web server group, in order to be able to correctly set the file permissions.</p>
                          <p>Based on your server configuration we have determined that you should set the 
                          username to "<code>@username</code>" and the group to "<code>@group</code>", 
                          but you can change these in the <a href="@provision_link">provisioning section</a>.</p>
                          <p> 
                          <strong>To configure: </strong> If your user is not a member of the web group, you can add them by using the
                          <code>vigr</code> command: <pre>@vigr_cmd</pre>
                          Once you have run the vigr command, find the line that says : <pre>@vigr1</pre>
                          Then add the username to the end of the line, so that it looks like : <pre>@vigr2</pre>
                          If there were already users in the group, you add your user to the group using a comma as separator :
                          <pre>@vigr3</pre>.</p>', 
                          array("@username" => $username, "@group" => $group, "@provision_link" => url('admin/settings/provision'),
                            '@vigr_cmd' => $vigr_cmd, '@vigr1' => $vigr1, '@vigr2' => $vigr2, '@vigr3' => $vigr3)) . '</li>';

    
    $output .= '<li>' . t('<p><strong>Write access to a directory to store backups.</strong> 
                          The drush user needs to be able to maintain the backups repository to ensure that your site is backed up successfully. 
                          It is incredibly important that this path is not accessible via the web server, so that no undesirables can get their
                          hands on your database. The recommended path is directly above your platform path, but it can be anywhere.</p>

                          <p>Based on your server configuration we have determined that your path should be <code>@backup_path</code>, 
                          but you can change this in the <a href="@provision_link">provisioning section</a>.</p>
                         Please enter the following commands :
                          <pre>@mkdir_cmd</pre>',$mkdir_cmd) . '</li>';
    $output .= "</ol>";
    return $output;
  }
}

/**
 * Page callback with in depth requirement documentation
 */
function provision_help_requirements() {
  $output .= t('<p>Unfortunately, due to the requirements of some of the functionality, significantly more access than is usually allowed on a shared hosting solution is required,
                and as such, a virtual server or dedicated hosting system will be required to run this system.</p>');
  $output .= t('<p>Some of the features of the system also require the ability to create symlinks, which means that it needs to run on a unix-like operating system. There are no plans currently to add windows support.</p>');  
  $modules = module_implements('provision_service');
  foreach ($modules as $module) {
    $service = module_invoke($module, 'provision_service');
    $name = current($service);
    $help = module_invoke($module, 'help', 'admin/help/provision#requirements');
    if ($name && $help) {
      $output .= '<a href="requirements-$module"></a><h3>' . t($name) . '</h3>';
      $output .=  $help;
    }
  }

  return $output;
}
