<?php
/**
 * @file
 * Provides the Provision_Config_Data_Store class.
 */

/**
 * Base class for data storage.
 *
 * This class provides a file locking mechanism for configuration
 * files that may be susceptible to race conditions.
 *
 * The records loaded from the config and the records set in this
 * instance are kept in separate arrays.
 *
 * When we lock the file, we load the latest stored info.
 */
class Provision_Config_Data_Store extends Provision_Config {
  public $template = 'data_store.tpl.php';
  public $key = 'record';

  private $locked = FALSE;
  protected $fp = NULL;

  public $records = array();
  public $loaded_records = array();

  protected $mode = 0700;


  function __construct($context, $data = array()) {
    parent::__construct($context, $data);

    $this->load_data();
  }

  /**
   * Ensure the file pointer is closed and the lock released upon destruction.
   */
  function __destruct() {
    // release the file lock if we have it.
    $this->close();
  }

  /**
   * Open the file.
   */
  function open() {
    if (!is_resource($this->fp)) {
      $this->fp = fopen($this->filename(), "w+");
    }
  }

  /**
   * Lock the file from other writes.
   *
   * After the file has been locked, we reload the data from the file
   * so that any changes we make will not override previous changes.
   */
  function lock() {
    if (!$this->locked) {
      $this->open();
      flock($this->fp, LOCK_EX);

      // Do one last load before setting our locked status.
      $this->load_data();
      $this->locked = TRUE;
    }
  }

  /**
   * Put the contents in the locked file.
   *
   * We call the lock method here to insure we have the lock.
   */
  function file_put_contents($filename, $text) {
    $this->lock();
    fwrite($this->fp, $text);
    fflush($this->fp);
  }

  /**
   * Release the write log on the data store file.
   */
  function unlock() {
    if ($this->locked && is_resource($this->fp)) {
      flock($this->fp, LOCK_UN);
      $this->locked = FALSE;
    }
  }

  /**
   * Close the file pointer and release the lock (if applicable).
   */
  function close() {
    if (is_resource($this->fp)) {
      fclose($this->fp);
    }
  }

  /**
   * Load the data from the data store into our loaded_records property.
   */
  function load_data() {
    if (!$this->locked) {
      // Once we have the lock we dont need to worry about it changing
      // from under us.
      if (is_readable($this->filename())) {
        include($this->filename());
        $data_key = $this->key;
        if (isset(${$data_key}) && is_array(${$data_key})) {
          $this->loaded_records = ${$data_key};
        }
      }
    }
  }

  /**
   * Return the merged contents of the records from the data store , and the values set by us.
   *
   * This is basically the data that would be written to the file if we were to write it right now.
   */
  function merged_records() {
    return array_merge($this->loaded_records, $this->records);
  }

  /**
   * Expose the merged records to the template file.
   */
  function process() {
    $this->data['records'] = array_filter(array_merge($this->loaded_records, $this->records));
  }
}
