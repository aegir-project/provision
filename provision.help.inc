<?php
// $Id$
/**
 * @file ProvisionHelp contains the bulk of the provision help system
 */


/**
 * Constants used to generate the requirement help documentation.
 */
/** Status is irrelevant. Can't be checked **/
define('PROVISION_STATUS_NONE', 0);
/** Requirement is met. **/
define('PROVISION_STATUS_SUCCESS', 1);
/** Requirement not met. Less severe than an error. */
define('PROVISION_STATUS_WARNING', 2);
/** Requirement failed. Fix before site will function. */
define('PROVISION_STATUS_ERROR', 4);

/**
 * Returns a class name for the output of the form element.
 */
function _provision_status_class($status)  {
  static $map;
 
  if (!sizeof($map)) {
   $map = array(
      PROVISION_STATUS_NONE => 'provision-status-none',
      PROVISION_STATUS_SUCCESS => 'provision-status-success',
      PROVISION_STATUS_WARNING => 'provision-status-warning',
      PROVISION_STATUS_ERROR => 'provision-status-error',
    );
  }

  return $map[$status];
}

/**
 * Constants used to configure contextual hosting help
 */
/** Display component of help **/
define('PROVISION_HELP_ENABLED', 1);
/** Don't show help component **/
define('PROVISION_HELP_DISABLED', ~PROVISION_HELP_ENABLED);

/** These are internally used to test against. Use the COLLAPSIBLE AND COLLAPSED instead **/
define('_PROVISION_HELP_CAN_COLLAPSE', 2);
define('_PROVISION_HELP_HAS_COLLAPSED', 4);

/**
 * Show help with the ability to collapse to save space. 
 * This automatically enables the help component
 **/
define('PROVISION_HELP_COLLAPSIBLE', PROVISION_HELP_ENABLED | _PROVISION_HELP_CAN_COLLAPSE);
/** 
 * Display help component with an initial collapsed state.
 * This automatically enables and sets the help component to be collapsible
 */
define('PROVISION_HELP_COLLAPSED', PROVISION_HELP_COLLAPSIBLE | _PROVISION_HELP_HAS_COLLAPSED);


/**
 * Returns a class name for the output of the form elementG
 */
function _provision_help_class($state) {
  static $map;
 
  if (!sizeof($map)) {
    $map = array(
      PROVISION_HELP_ENABLED => 'provision-help',
      PROVISION_HELP_COLLAPSIBLE => 'provision-help-collapsible',
      PROVISION_HELP_COLLAPSED => 'provision-help-collapsed'
    );
  }
  return $map[$state];
}

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

      $output .= '<a name=\'requirements\'><h3>'. t('Requirements') .'</h3></a>';
      $output .= t('<p>For a more detailed breakdown of steps that need to be taken to configure Provisioning to run with your system, please read the <a href="@url">in depth requirement documentation</a></p>',
                  array('@url' => url('admin/help/provision/requirements')));
      $output .= '<a name=\'commands\'><h3>'. t('Commands') .'</h3></a>';
      $commands = module_invoke_all('drush_command');
      $output .= "<dl>";
      foreach ($commands as $command => $info) {
        if (preg_match('/^provision/', $command)) {
          if (sizeof($info['arguments'])) {
            $command .= ' '. implode(' ', (array) key($info['arguments']));            
          }
          if (sizeof($info['optional arguments'])) {
            $command .= ' ['. implode('] [', (array) key($info['optional arguments'])) .']';  
          }
          $output .= '<dt>'."<code>drush.php $command</code>".'</dt>';
          $output .= '<dd>'.  $info["description"] .'</dd>';
        }
      }
      $output .= "</dl>";

      $output .= '<a name=\'options\'><h3>'. t('Options') .'</h3></a>';

      
      $options = module_invoke_all('value_list');

      $output .= "<dl>";
      foreach ($options as $option => $description) {
        $output .= '<dt>'."<code>--$option</code>".'</dt>';
        $output .= '<dd>'.  $description .'</dd>';
      }
      $output .= "</dl>";
      
      return $output;
    case 'admin/help/provision#requirements' :
      $output .= _provision_requirements('user');
      $output .= _provision_requirements('group');
      $output .=  _provision_requirements('backup_path');
      return $output;
  }
}

/**
 * Page callback with in depth requirement documentation
 */
function provision_help_requirements() {
  $output .= _provision_requirements("basic_drupal");  
  $output .= _provision_requirements("basic_unix");  
  $output .= _provision_requirements("basic_server");  
  $modules = module_implements('provision_service');
  foreach ($modules as $module) {
    $service = module_invoke($module, 'provision_service');
    $name = current($service);
    $help = module_invoke($module, 'help', 'admin/help/provision#requirements', null);
    if ($name && $help) {
      $output .= "<a href='requirements-$module'></a><h3>". t($name) .'</h3>';
      $output .=  $help;
    }
  }
  return $output;
}

/**
 * Helper function for displaying contextual help when not used in a form.
 */
function _provision_requirements($req, $section = 'all') {
  $item = _element_info('requirement_help');
  $item['#requirement'] = $req;
  $item['#type'] = 'requirement_help';
/*
  foreach (array('#heading', '#summary', '#suggestion', '#configuration') as $key) {
    if (in_array($section, array('all', $key))) {
      $item['#'. $key] = PROVISION_HELP_ENABLED;
    }
    elseif ($section != 'all') {
      $item['#'. $key] = ($section == $key) ? PROVISION_HELP_ENABLED : PROVISION_HELP_DISABLED;
    }
  }
 */
  $item = provision_requirement_process($item);
  return theme("requirement_help", $item);
}

function provision_get_requirement($req) {
  $func = '_provision_'. $req .'_requirements';
  if (function_exists($func)) {
    $help = $func();
  }
  return $help;
}

function _provision_backup_path_requirements() {
    $username = PROVISION_SCRIPT_USER;
    $group = PROVISION_WEB_GROUP;
    $backup_path = PROVISION_BACKUP_PATH;
    $mkdir_cmd['@backup_path'] = $backup_path;      
    $mkdir_cmd['@provision_link'] = url('admin/settings/provision');
    $mkdir_cmd['@mkdir_cmd'] = <<<EOF
    mkdir $backup_path
    chown $username:$username $backup_path
    chmod 0700 $backup_path
EOF;
  $help['title'] = t('Write access to a directory to store backups');
  $help['summary'] = t('The drush user (<a href="http://drupal.org/project/drush">http://drupal.org/project/drush</a>) needs to be able to maintain the backups repository to ensure that your site is backed up successfully. 
                        It is incredibly important that this path is not accessible via the web server, so that no undesirables can get their
                        hands on your database. The recommended path is directly above your platform path, but it can be anywhere.');

  $help['suggestion'] = t('Based on your server configuration we have determined that your path should be <code>@backup_path</code>', $mkdir_cmd);
  $help['configuration'] = t('Please enter the following commands : <pre>@mkdir_cmd</pre>', $mkdir_cmd);
  return $help;
}


function _provision_user_requirements() {
  $username = PROVISION_SCRIPT_USER;
  $path = PROVISION_DOCROOT_PATH;
  $add_cmd = <<<EOF
    sudo adduser $username
EOF;
  $chmod_cmd = <<<EOF
    sudo chown $username $path
EOF;
  $help['title'] = t('A separate system account for the scripts');
  $help['summary'] = t('The provision framework requires that the scripts run as a non-root system account, to ensure that
    it can correctly set the file permissions on the hosted files. All existing files need to be changed to belong to this
    new system account.');
  $help['suggestion'] = t('Based on your server configuration, we have determined that your user account should be <code>@script_user</code>', array('@script_user' => PROVISION_SCRIPT_USER));
  $help['configuration'][] = t('If your system supports it, run the adduser command (if this command is unavailable, please consult your operating system documentation on how to add new system accounts) : <pre>@cmd</pre>', array('@cmd' => $add_cmd));
  $help['configuration'][] = t('Once you have created the user account, you need to modify the ownership of the files. Use the following command : <pre>@cmd</pre>', array('@cmd' => $chmod_cmd));
  return $help;
}

function _provision_group_requirements() {

    $username = PROVISION_SCRIPT_USER;
    $group = PROVISION_WEB_GROUP;
      

    $vigr_cmd = <<<EOF
    sudo adduser $username $group
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

    $su = <<<EOF
    su -
EOF;

  $help['title'] = t('The system group of the web server');
  $help['summary'] = t('For the provision framework to be able to ensure that the file permissions of the 
                          hosted sites are always as safe as can be, and especially to make sure that the web server does 
                          not have the ability to modify the code of the site, the configured system account needs to be a member
                          of the web server group, in order to be able to correctly set the file permissions.');

  $help['suggestion'] = t('Based on your server configuration we have determined that you should add the 
                        system account "<code>@username</code>" to the "<code>@group</code>" system group.', array("@username" => $username, "@group" => $group));

  $help['configuration'] =t('If your system account is not a member of the web group, you can add them by using the
                          <code>adduser</code> command:
                          <pre>@vigr_cmd</pre>
                          If that command is not available, you will need to edit the /etc/group file directly with
                          your vigr or your favorite editor.
                          Find the line that says : <pre>@vigr1</pre>
                          Then add the username to the end of the line, so that it looks like : <pre>@vigr2</pre>
                          If there were already users in the group, add your user to the group using a comma as separator :
                          <pre>@vigr3</pre>
                          Once you have changed this, you will need to log out and log bag into your terminal session for this
                          setting to take effect. Alternatively you
                          can get a new login shell by typing :<pre>@su</pre>', 
                          array('@vigr_cmd' => $vigr_cmd, '@vigr1' => $vigr1, '@vigr2' => $vigr2, '@vigr3' => $vigr3, '@su' => $su));
  return $help;

}

/**
 * Implementation of hook_elements.
 *
 * Defines a number of form elements that are used for formatting the contextual help
 * in forms.
 */
function provision_elements() {
  $type['requirement_help'] = array(
    '#requirement' => NULL,
    '#status' => PROVISION_STATUS_NONE,
    '#heading' => PROVISION_HELP_ENABLED,
    '#summary' => PROVISION_HELP_ENABLED,
    '#summary_prefix' => t("What is this?"),
    '#configuration' => PROVISION_HELP_COLLAPSED,
    '#configuration_prefix' => t("How do I configure this?"),
    '#default_messages' => array(
      PROVISION_STATUS_SUCCESS => t("You have met this requirement."),
      PROVISION_STATUS_WARNING => t("This requirement has a non critical error."),
      PROVISION_STATUS_ERROR => t("This requirement has a critical error. This system will not operate until it has been fixed")),
    '#process' => array('provision_requirement_process' => array()),
  );
  return $type;
}

/**
 * Places the various help components info the $element array
 */
function provision_requirement_process(&$element) {
  if (!$element['#requirement']) {
    return element;
  }

  $element['#help'] = provision_get_requirement($element['#requirement']);
  return $element;
}

/**
 * Theme function for displaying contextual help.
 *
 * Can control individual components of the help, for display in various places.
 */
function theme_requirement_help(&$element) {
  drupal_add_js(drupal_get_path('module', 'provision') .'/provision.js');
  drupal_add_css(drupal_get_path('module', 'provision') .'/provision.css');
  $req = $element['#requirement'];
  $help = $element['#help']; // this is just to make it easier to work with.

  //place anchor so user can be directed to right page.
  $output .= "<a name='provision-help-$req'></a>";

  $output .= '<div class="provision-'. form_clean_id($element["#type"]) . 
    ' '. _provision_status_class($element['#status']) .'">';

  if ($element['#status'] != PROVISION_STATUS_NONE) {
    $output .= "<div class='message'>". (($element['#message']) ? $element['#message'] : $element['#default_messages'][$element['#status']]) ."</div>";
  }

  $components = array('summary', 'suggestion', 'configuration');
  foreach ($components as $key) {
    if (($element["#$key"] & PROVISION_HELP_ENABLED) && !is_null($help[$key])) {
      $display_type = _provision_help_class($element["#$key"]);
      $output .= "<div class='provision-help-$key $display_type'>";
      if (!is_array($help[$key])) {
        // it is simpler if there's only one way to print the component
        $help[$key] = array($help[$key]);  
      }
      if ($element["#$key".'_prefix'] && ($element["#$key"] & _PROVISION_HELP_CAN_COLLAPSE)) {
        $output .= '<a href="javascript:void(0)" class="provision-help-toggle">'. $element["#$key".'_prefix'] .'</a>';
      }

      $output .= "<div class='provision-help'><p>". implode("</p><p>", $help[$key]) ."</p></div>"; 
      $output .= '</div>';
    } 
  }
  $output .= '</div>';
  if (($element['#heading'] & PROVISION_HELP_ENABLED) && $help['title'] && !$element['#title']) {
    $element['#title'] = $help['title'];
  }
  $element['#value'] = $output;
  return theme('item', $element);
}

/**
 * @TODO: handle element children for requirement help
function _provision_basic_requirements() {
  $help['drupal'] = _provision_basic_drupal_requirements();
  $help['unix'] = _provision_basic_unix_requirements();
  $help['server'] = _provision_basic_server_requirements();

  return $help;
}
 */

function _provision_basic_drupal_requirements() {
  $help['title'] = t('A system capable of running Drupal');
  $help['summary'] = t('If you are reading this via the inline help, this would be kind of obvious. This system is entirely Drupal based, and has the same base requirements that Drupal does.');
  return $help;
}

function _provision_basic_server_requirements() {
  $help['title'] = t('Your own server');
  $help['summary'] = t('The level of access required to be able to configure this system is very far beyond what is commonly available to users with shared hosting.');
  return $help;
}

function _provision_basic_unix_requirements() {
  $help['title'] = t('A unix based operating system');
  $help['summary'] = t('The majority of functionality in this system occurs in the back-end, through command line scripting. There are several features (such as symlinks), that are not available to users on Windows. There are no plans currently to add windows support.</p>');
  return $help;
}


function _provision_provision_setup_requirements() {
  $docroot = PROVISION_DOCROOT_PATH;
  $uri = PROVISION_BASE_URL;
  $drush_path = rtrim(drupal_get_path('module', 'drush'), '/') . '/drush.php';
  $username = PROVISION_SCRIPT_USER;
  $setup_cmd = <<<EOF
    cd $docroot
    $drush_path provision setup
EOF;
  $help['title'] = t('Generate your configuration file using the <code>Provision Setup</code> command');

  $help['summary'][] = t('The provision setup command inspects your environment and creates an initial configuration file in <code>sites/default/provision.settings.php</code>.<br />This configuration file stores important settings such as paths, binary locations and database credentials.');

  $help['summary'][] = t('<strong>If you are using this platform as the back end to an existing Hostmaster installation: </strong><br /> you need to run this command before adding the platform node to your hosting site, to allow the hosting site to communicate with this platform.');
  $help['configuration'] = t('Ensure that you are logged into the shell as %script_user, and then execute the following commands :<pre>@setup_cmd</pre>', 
     array('%script_user' => $username, '@setup_cmd' => $setup_cmd));

  return $help;
}
