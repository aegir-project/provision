<?php

/**
 * Base class for subdir support.
 *
 * This class will publish the config files to remote
 * servers automatically.
 */
class Provision_Config_Nginx_SubdirVhost extends Provision_Config_Http {
  public $template = 'subdir_vhost.tpl.php';
  public $description = 'subdirectory vhost support';

  // hack: because the parent class doesn't support multiple config
  // files, we need to keep track of the alias we're working on.
  protected $current_alias;

  /**
   * Guess the URI this subdir alias is related too.
   */
  function uri() {
    $e = explode('/', $this->current_alias, 2);
    return $e[0];
  }

  /**
   * Guess the subdir part of the subdir alias.
   */
  function subdir() {
    $e = explode('/', $this->current_alias, 2);
    return $e[1];
  }

  /**
   * Check if the (real) parent site (drushrc) exists.
   */
  function parent_site() {
    $u = explode('/config/', $this->data['http_vhostd_path'], 2);
    $p = $u[0] . '/.drush/';
    $parent_site_drushrc = $p . $this->uri() . '.alias.drushrc.php';
    drush_log(dt('Checking %vhost drushrc: %drushrc', array('%vhost' => $this->uri(), '%drushrc' => $parent_site_drushrc)), 'notice');
    if (provision_file()->exists($parent_site_drushrc)->status()) {
      $e = TRUE;
    }
    else {
      $e = FALSE;
    }
    return $e;
  }

  function write() {
    $count = "0";
    $site_has_parent = FALSE;
    $mode_is_install = FALSE;

    $command = drush_get_command();
    $command = explode(" ", $command['command']);
    if (preg_match("/^provision-install/", $command[0])) {
      drush_log(dt('Subdir Install Mode Detected: %command', array('%command' => $command[0])), 'notice');
      $mode_is_install = TRUE;
    }
    elseif (preg_match("/^provision-verify/", $command[0])) {
      drush_log(dt('Subdir Verify Mode Detected: %command', array('%command' => $command[0])), 'notice');
    }
    else {
      drush_log(dt('Subdir Other Mode Detected: %command', array('%command' => $command[0])), 'notice');
    }

    foreach (d()->aliases as $alias) {
      if (strpos($alias, '/')) {
        $this->current_alias = $alias;
        if ($this->parent_site()) {
          $site_has_parent = TRUE;
          drush_log(dt('Parent site %vhost exists for alias %alias, skipping', array('%vhost' => $this->uri(), '%alias' => $alias)), 'notice');
          if ($count == "0" && $this->uri() && $mode_is_install) {
            $site_name = '@' . $this->uri();
            drush_log(dt('Parent site %vhost re-verify required to include subdir config for %alias', array('%vhost' => $site_name, '%alias' => $alias)), 'warning');
            //
            //   drush_invoke_process('@none', 'cache-clear', array('drush'));
            //   provision_backend_invoke($site_name, 'provision-verify');
            //   drush_invoke_process('@none', 'cache-clear', array('drush'));
            //
            // Running automated re-verify for the parent site is currently
            // too dangerous. It will destroy/delete the parent site's database
            // if the parent and the subdir site use different installation
            // profiles, unless both profiles exist in the same platform.
            //
            // This is too serious limitation and we need to find a better
            // way to automate parent site re-verify when needed to add
            // required include line, which enables all subdir sites.
            //
            // With Drush 4 this could be done with separate task created like this:
            //
            //   drush_log(dt('Run parent site %vhost Verify via frontend', array('%vhost' => $site_name)), 'notice');
            //   provision_backend_invoke('@hostmaster', 'hosting-task', array($site_name, 'verify'), array('force' => TRUE));
            //
            // Unfortunatelly, it doesn't work with Drush 5 and current Aegir 2.x,
            // and is even more dangerous, because instead of creating separate
            // re-verify task for the parent site, it will run it "inline",
            // immediatelly, so in the wrong context, which, depending on other
            // conditions will destroy *hostmaster* database, so it is mentioned
            // here as a nostalgic reminiscence of good old Drush 4, which allowed
            // to create frontend tasks from the backend, safely.
            //
            // Without this re-verify it is fully possible to use any profile
            // with any platform for multiple subdir sites, under the same,
            // single, parent URL roof, with or without the parent site hosted
            // on the main URL (domain name). Which is good news!
            //
          }
          $count++;
        }
        else {
          drush_log("Subdirectory alias `$alias` found. Creating vhost configuration file.", 'notice');
          parent::write();
        }
      }
    }
  }

  function process() {
    parent::process();
    $this->data['uri'] = $this->uri();
    $this->data['subdir'] = $this->subdir();
    $this->data['subdirs_path'] = $this->data['http_subdird_path'];
  }

  function filename() {
    return $this->data['http_vhostd_path'] . '/' . $this->uri();
  }
}
