<?php
/**
 * @file
 * Provision configuration generation classes.
 */

namespace Aegir\Provision;

use Symfony\Component\Filesystem\Exception\IOException;
use Symfony\Component\Filesystem\Filesystem;

/**
 * Class Configuration
 *
 * @package Aegir\Provision
 */
class Configuration {
  
  /**
   * Provision 4.x
   */
  
  /**
   * A \Aegir\Provision\Context object this configuration relates to.
   *
   * @var \Aegir\Provision\Context
   */
  public $context = NULL;
  
  /**
   * A \Aegir\Provision\Service object this configuration relates to.
   *
   * @var \Aegir\Provision\Service
   */
  public $service = NULL;
  
  /**
   * @var Filesystem
   */
  public $fs;
  
  /**
   * LEGACY
   */
  /**
   * Template file, a PHP file which will have access to $this and variables
   * as defined in $data.
   */
  public $template = NULL;

  /**
   * Associate array of variables to make available to the template.
   */
  public $data = array();

  /**
   * If set, replaces file name in log messages.
   */
  public $description = NULL;

  /**
   * Octal Unix mode for permissons of the created file.
   */
  protected $mode = NULL;

  /**
   * Unix group name for the created file.
   */
  protected $group = NULL;

  /**
   * An optional data store class to instantiate for this config.
   */
  protected $data_store_class = NULL;

  /**
   * The data store.
   */
  public $store = NULL;

  /**
   * Forward $this->... to $this->context->...
   * object.
   */
  function __get($name) {
    if (isset($this->context)) {
      return $this->context->$name;
    }
  }

  /**
   * Constructor, overriding not recommended.
   *
   * @param $context
   *   An alias name for d(), the Provision_Context that this configuration
   *   is relevant to.
   * @param $data
   *   An associative array to potentially manipulate in process() and make
   *   available as variables to the template.
   */
  function __construct($context, $service, $data = array()) {
    if (is_null($this->template)) {
      throw new Exception(dt("No template specified for: %class", array('%class' => get_class($this))));
    }

    // Accept both a reference and an alias name for the context.
    $this->context = $context;
    $this->service = $service;
    $this->fs = new Filesystem();

    if (sizeof($data)) {
      $this->data = $data;
    }

    if (!is_null($this->data_store_class) && class_exists($this->data_store_class)) {
      $class = $this->data_store_class;
      $this->store = new $class($context, $data);
    }

  }

  /**
   * Process and add to $data before writing the configuration.
   *
   * This is a stub to be implemented by subclasses.
   */
  function process() {
    if (is_object($this->store)) {
      $this->data['records'] = array_filter(array_merge($this->store->loaded_records, $this->store->records));
    }
    return TRUE;
  }

  /**
   * The filename where the configuration is written.
   *
   * This is a stub to be implemented by subclasses.
   */
  function filename() {
    return FALSE;
  }

  /**
   * Load template from filename().
   *
   * @see hook_provision_config_load_templates()
   * @see hook_provision_config_load_templates_alter()
   */
  private function load_template() {
    return file_get_contents(__DIR__ . '/Service/' . ucfirst($this->service->getName()) . '/' . ucfirst($this->service->getType()) . '/Configuration/' . $this->template);

    // Allow other Drush commands to change the template used first.
//    $templates = drush_command_invoke_all('provision_config_load_templates', $this);
//    // Ensure that templates is at least an array.
//    if (!is_array($templates)) {
//      $templates = array();
//    }
//    // Allow other Drush commands to alter the templates from other commands.
////    drush_command_invoke_all_ref('provision_config_load_templates_alter', $templates, $this);
//    if (!empty($templates) && is_array($templates)) {
//      foreach ($templates as $file) {
//        if (is_readable($file)) {
//          drush_log(dt('Template loaded from hook(s): :file', array(
//            ':file' => $file,
//          )));
//          return file_get_contents($file);
//        }
//      }
//    }
//
//    // If we've got this far, then try to find a template from this class or
//    // one of its parents.
//    if (isset($this->template)) {
//      $class_name = get_class($this);
//      while ($class_name) {
//        // Iterate through the config file's parent classes until we
//        // find the template file to use.
//        $base_dir = provision_class_directory($class_name);
//        $file = $base_dir . '/' . $this->template;
//
//        if (is_readable($file)) {
//          drush_log(dt('Template loaded from Provision Config class :class_name: :file', array(
//            ':class_name' => $class_name,
//            ':file' => $file,
//          )));
//          return file_get_contents($file);
//        }
//
//        $class_name = get_parent_class($class_name);
//      }
//    }
//
//    // We've failed to find a template if we've reached this far.
//    drush_log(dt('No template found for Provision Config class: ', array(':class' => get_class($this))), 'warning');
//    return FALSE;
  }

  /**
   * Render template, making variables available from $variables associative
   * array.
   */
  private function render_template($template, $variables) {

    // Allow modules to alter the variables before writing to the template.
    // @see hook_provision_config_variables_alter()
//    drush_command_invoke_all_ref('provision_config_variables_alter', $variables, $template, $this);
    
//    drush_errors_off();
    extract($variables, EXTR_SKIP);  // Extract the variables to a local namespace
    ob_start();                      // Start output buffering
    eval('?>' . $template);                 // Generate content
    $contents = ob_get_contents();   // Get the contents of the buffer
    ob_end_clean();                  // End buffering and discard
//    drush_errors_on();
    return $contents;                // Return the contents
  }

  /**
   * Write out this configuration.
   *
   * 1. Make sure parent directory exists and is writable.
   * 2. Load template with load_template().
   * 3. Process $data with process().
   * 4. Make existing file writable if necessary and possible.
   * 5. Render template with $this and $data and write out to filename().
   * 6. If $mode and/or $group are set, apply them for the new file.
   */
  function write() {

    // Make directory structure if it does not exist.
    $filename = $this->filename();
    if ($filename && !$this->fs->exists([dirname($filename)])) {
        try {
            $this->fs->mkdir([dirname($filename)]);
        }
        catch (IOException $e) {
            throw new \Exception("Could not create directory " . dirname($filename) . ": " . $e->getMessage());
        }
    }

    $status = FALSE;
    if ($filename && is_writeable(dirname($filename))) {
      // manipulate data before passing to template.
      $this->process();

      if ($template = $this->load_template()) {
        // Make sure we can write to the file
        if (!is_null($this->mode) && !($this->mode & 0200) && $this->fs->exists(($filename))) {
          try {
              $this->fs->chmod([$filename], $this->mode);
          }
          catch (IOException $e) {
              throw new \Exception('Could not change permissions of ' . $filename . ' to ' . $this->mode);
          }
        }
        
        try {
          $this->fs->dumpFile($filename, $this->render_template($template, $this->data));
        }
        catch (IOException $e) {
          throw new \Exception('Could not write file to ' . $filename);
        }
        
        // Change the permissions of the file if needed
        if (!is_null($this->mode)) {
          try {
            $this->fs->chmod([$filename], $this->mode);
          }
          catch (IOException $e) {
            throw new \Exception('Could not change permissions of ' . $filename . ' to ' . $this->mode);
          }
        }
        if (!is_null($this->group)) {
          try {
            $this->fs->chgrp([$filename], $this->group);
          }
          catch (IOException $e) {
            throw new \Exception('Could not change group ownership of ' . $filename . ' to ' . $this->group);
          }
        }
      }
    }
    return $status;
  }

  // allow overriding w.r.t locking
  function file_put_contents($filename, $text) {
    provision_file()->file_put_contents($filename, $text)
      ->succeed('Generated config in file_put_contents()' . (empty($this->description) ? $filename : $this->description), 'success');
  }

  /**
   * Remove configuration file as specified by filename().
   */
  function unlink() {
    return provision_file()->unlink($this->filename())->status();
  }

}
