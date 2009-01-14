<?php
// $Id$

// Admin configuration hooks

/**
 * Implementation of hook_menu().
 **/
function provision_dns_menu($may_cache) {
  if (!$may_cache) {
    $items[] = array(
      'path' => 'admin/settings/provision_dns',
      'title' => t('DNS Provisioning Settings'),
      'description' => t('Configure DNS Provisioning'),
      'callback' => 'drupal_get_form',
      'callback arguments' => 'provision_dns_settings',
      'access' => TRUE,
    );
    $items[] = array(
      'path' => 'admin/settings/provision_dns/general',
      'title' => t('General'),
      'type' => MENU_DEFAULT_LOCAL_TASK,
      'weight' => -1,
    );

    $items[] = array(
      'path' => 'admin/build/provision_dns',
      'title' => t('DNS Provisioning Admin'),
      'description' => t('Manually administer DNS zones and records'),
      'callback' => 'provision_dns_admin',
      #'callback arguments' => 'provision_dns_admin',
      'access' => TRUE,
    );
    $items[] = array(
      'path' => 'admin/build/provision_dns/list',
      'title' => t('List Zones'),
      'type' => MENU_DEFAULT_LOCAL_TASK,
      'weight' => -10,
    );
    $items[] = array(
      'path' => 'admin/build/provision_dns/add',
      'title' => t('Add Zone'),
      'description' => t('Add a new DNS zone (domain) to the system'),
      'callback' => 'drupal_get_form',
      'callback arguments' => 'provision_dns_zone_form',
      'type' => MENU_LOCAL_TASK,
    );
    
    $zid = arg(3);
    $items[] = array(
      'path' => 'admin/build/provision_dns/'. $zid .'/edit',
      'title' => t('Edit Zone'),
      'callback' => 'drupal_get_form',
      'callback arguments' => array('provision_dns_zone_form', $zid),
      'type' => MENU_CALLBACK,
    );
    $items[] = array(
      'path' => 'admin/build/provision_dns/'. $zid .'/delete',
      'title' => t('Delete Zone'),
      'callback' => 'drupal_get_form',
      'callback arguments' => array('provision_dns_zone_delete_confirm', $zid),
      'type' => MENU_CALLBACK,
    );

    $rid = arg(5);
    $items[] = array(
      'path' => 'admin/build/provision_dns/'. $zid .'/records',
      'title' => t('Zone Records'),
      'callback' => 'provision_dns_records',
      'callback arguments' => $zid,
      'access' => user_access('access administration pages'),
      'type' => MENU_CALLBACK,
    );
    $items[] = array(
      'path' => 'admin/build/provision_dns/'. $zid .'/records/list',
      'title' => t('List Zone Records'),
      'type' => MENU_DEFAULT_LOCAL_TASK,
      'weight' => -10,
    );
    $items[] = array(
      'path' => 'admin/build/provision_dns/' . $zid .'/records/add',
      'title' => t('Add Zone Record'),
      'description' => t('Add a new DNS zone record (subdomain) to the system'),
      'callback' => 'drupal_get_form',
      'callback arguments' => array('provision_dns_record_form', $zid), 
      'type' => MENU_LOCAL_TASK,
    );
    $items[] = array(
      'path' => 'admin/build/provision_dns/'. $zid .'/records/'. $rid . '/edit',
      'title' => t('Edit Zone Record'),
      'callback' => 'drupal_get_form',
      'callback arguments' => array('provision_dns_record_form', $zid, $rid),
      'type' => MENU_CALLBACK,
    );
    $items[] = array(
      'path' => 'admin/build/provision_dns/'. $zid .'/records/'. $rid . '/delete',
      'title' => t('Delete Zone Record'),
      'callback' => 'drupal_get_form',
      'callback arguments' => array('provision_dns_record_delete_confirm', $zid, $rid),
      'type' => MENU_CALLBACK,
    );
  }
  return $items;
}

/**
 * Menu callback: general settings for DNS provisioning
 *
 * Allow the admin to define a set of TLDs which are valid for this platform. These are used when splitting
 **/
function provision_dns_settings() {
  $form['provision_dns_tlds'] = array(
    '#type' => 'textarea',
    '#title' => t('Supported Top Level Domains'),
    '#description' => t('TLDs which this Aegir site can install. Used to determine the host and zone parts of the FQDN when provisioning sites. Enter one TLD (.com, .net, .co.uk, .net.au) per line.'),
    '#default_value' => variable_get('provision_dns_tlds', ''),
  );
  return system_settings_form($form);
}

function provision_dns_admin() {
  $zone_count = db_result(db_query("SELECT COUNT(*) FROM {provision_dns_soa}"));
  $output .= 'Aegir is managing '. $zone_count . ' zones.';

  $header = array('Name', 'Serial', 'Records', 'Active',  array('data' => t('Operations'), 'colspan' => 3));
  $result = db_query("SELECT zid, origin, serial, active FROM {provision_dns_soa} ORDER BY origin");
  while ($row = db_fetch_array($result)) {
    $records = db_result(db_query("SELECT COUNT(*) FROM {provision_dns_rr} WHERE zid = %d", $row['zid']));
    $rows[] = array(l(t($row['origin']), "admin/build/provision_dns/". $row['zid'] ."/records"),
		        $row['serial'],
			$records,
			$row['active'],
			l(t('records'), "admin/build/provision_dns/". $row['zid'] . "/records"),
			l(t('edit'), "admin/build/provision_dns/". $row['zid'] . "/edit"),
			l(t('delete'), "admin/build/provision_dns/". $row['zid'] . "/delete")
		   );
  }
  $output .= theme('table', $header, $rows);
  return $output;
}

function provision_dns_records($zid) {
  $zone = provision_dns_status('zone', $zid);
  $record_count = db_result(db_query("SELECT COUNT(*) FROM {provision_dns_rr} WHERE zid=%d", $zid));
  $output = sprintf('The %s zone currently has %d records.', $zone->origin, $record_count);
  
  $header = array('Name', 'Type', 'Data', 'AUX', array('data' => t('Operations'), 'colspan' => 2));
  $result = db_query("SELECT rid, name, type, data, aux FROM {provision_dns_rr} WHERE zid=%d ORDER BY name", $zid);
  while ($row = db_fetch_array($result)) {
    $rows[] = array(l(t($row['name']), "admin/build/provision_dns/". $zid . "/records/" . $row['rid']),
                        $row['type'],
                        $row['data'],
                        $row['aux'],
                        l(t('edit'), "admin/build/provision_dns/". $zid . "/records/". $row['rid'] . "/edit"),
                        l(t('delete'), "admin/build/provision_dns/". $zid . "/records/". $row['rid'] . "/delete"),
                   );
  }
  $output .= theme('table', $header, $rows);
  return $output;
}


function provision_dns_zone_form($zid=0) {
  $fields = array('origin', 'ns1', 'ns2', 'mbox', 'serial', 'refresh', 'retry', 'expire', 'minimum', 'ttl', 'active', 'xfer');
  $titles = array(t('Domain Name'), t('Primary NS'), t('Secondary NS'), t('Admin Email'), t('Serial'), t('Refresh'), t('Retry'), t('Expire'), t('Minimum TTL'), t('TTL'), t('Active'), t('XFER Access'));
  $form = array();

  if ($zid) {
    # load up the zone, and prefill the form with its defaults
    $submit = t('Update Zone');
    $zone = provision_dns_status('zone', array('zid' => $zid));
    $form['zid'] = array(
      '#type' => 'value',
      '#value' => $zone->zid,
    );
  } else {
    # create a 'default' zone object, with all the right fields set to defaults..
    $submit = t('Add Zone');
    $zone = new stdClass();
    foreach ($fields as $key) {
      $zone->$key = variable_get('provision_dns_'.$key,NULL);
    }
    $zone->active = TRUE;
    $zone->serial = date('Ymd') . '01';
  }

  foreach ($fields as $i => $key) {
    $form[$key] = array(
      '#type' => ($key == 'active')?'checkbox':'textfield',
      '#title' => $titles[$i],
      '#default_value' => $zone->$key,
    );
  }

  $form['active']['#weight'] = -10;
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => $submit,
  );
  return $form;
}

function provision_dns_zone_form_validate($form_id, $form_values) {

  if (!$form_values['zid']) { # Create a new zone
    # Make sure it doesn't already exist
  }

  # Make sure all fields have valid values..

}

function provision_dns_zone_form_submit($form_id, $form_values) {
  if ($form_values['zid']) { # Update an existing zone
    provision_dns_zone('update', $form_values);   
  } else { 		     # Create a new zone
    provision_dns_zone('add', $form_values);
  } 
  return 'admin/build/provision_dns';
}

function provision_dns_zone_delete_confirm($zid) {
  $name = db_result(db_query("SELECT origin FROM {provision_dns_soa} WHERE zid = %d", $zid));
  $form['zid'] = array('#type' => 'value', '#value' => $zid);
  $output = confirm_form($form, t('Are you sure you want to delete the zone "'. $name .'"?'), "admin/build/provision_dns", "This action cannot be undone.", t('Delete'), t('Cancel'));
  return $output;
}

function provision_dns_zone_delete_confirm_submit($form_id, $form_values) {
  provision_dns_zone('delete', array('zid' => $form_values['zid']));
  return 'admin/build/provision_dns';
}


function provision_dns_record_form($zid, $rid=0) {
  $form = array();
  $form['zid'] = array('#type' => 'value', '#value' => $zid);

  if ($rid) { # Update an existing record
    $submit = t('Update Record');
    $record = provision_dns_status('rr', array('zid' => $zid, 'rid' => $rid));
    $form['rid'] = array('#type' => 'value', '#value' => $rid);
  } else {
    $submit = t('Add Zone Record');
    $record = new stdClass();
    $record->name = '';
    $record->type = 'A';
    $record->data = '';
    $record->aux = 0;
    $record->ttl = '86400';
  }

  $form['name']  = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('The name of the record (subdomain)'),
    '#default_value' => $record->name,
  );
  $form['type'] = array(
    '#type' => 'select',
    '#title' => t('Type'),
    '#description' => t('The type of record'),
    '#default_value' => $record->type,
    '#options' => array('A' => 'A', 'AAAA' => 'AAAA', 'ALIAS' => 'ALIAS', 'CNAME' => 'CNAME',
			'HINFO' => 'HINFO', 'MX' => 'MX', 'NAPTR' => 'NAPTR', 'NS' => 'NS',
			'PTR' => 'PTR', 'RP' => 'RP', 'SRV' => 'SRV', 'TXT' => 'TXT'),
  );
  $form['data'] = array(
    '#type' => 'textfield',
    '#title' => t('Data'),
    '#description' => t('The data where this record points to'),
    '#default_value' => $record->data,
  );
  $form['aux'] = array(
    '#type' => 'weight',
    '#title' => t('AUX/PRI'),
    '#description' => t('Auxiliary or Priority setting for this record'),
    '#default_value' => $record->aux,
  );
  $form['ttl'] = array(
    '#type' => 'textfield',
    '#title' => t('TTL'),
    '#description' => t('Time to Live setting for this record (numeric)'),
    '#default_value' => $record->ttl,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => $submit,
  );

  return $form;
}

function provision_dns_record_form_validate($form_id, $form_values) {
  # TODO: validate the form values!
}

function provision_dns_record_form_submit($form_id, $form_values) {
  if ($form_values['rid']) {  # Update an existing zone
    provision_dns_rr('update', $form_values['zid'], $form_values);
  } else {
    provision_dns_rr('add', $form_values['zid'], $form_values);
  }
  return 'admin/build/provision_dns/'. $form_values['zid'] .'/records';
}

function provision_dns_record_delete_confirm($zid, $rid) {
  $origin = db_result(db_query("SELECT origin FROM {provision_dns_soa} WHERE zid = %d", $zid));
  $name = db_result(db_query("SELECT name FROM {provision_dns_rr} WHERE zid = %d AND rid = %d", $zid, $rid));

  $form['zid'] = array('#type' => 'value', '#value' => $zid);
  $form['rid'] = array('#type' => 'value', '#value' => $rid);
  $output = confirm_form($form, t('Are you sure you want to delete the zone record "'. $name .'" from the ' . $origin . ' zone?'), "admin/build/provision_dns/". $zid . "/records", "This action cannot be undone.", t('Delete'), t('Cancel'));
  return $output;
}

function provision_dns_record_delete_confirm_submit($form_id, $form_values) {
  provision_dns_rr('delete', $form_values['zid'], array('rid' => $form_values['rid']));
  return 'admin/build/provision_dns/'. $form_values['zid'] .'/records';
}

