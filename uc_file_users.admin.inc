<?php
/**
 * @file
 * Administrative pages for managing Ubercart file downloads and their users.
 */

/**
 * Form to select the download file to show the users of.
 */
function uc_file_users_form($form, &$form_state) {

  $form[] = array(
    '#type' => 'help',
    '#markup' => t('» <a href="!url">See all file downloads</a>', array('!url' => url('admin/store/products/files'))),
  );

  $form['filename_fid'] = array(
    '#type' => 'textfield',
    '#title' => t('Download file name'),
    '#description' => t('Start typing the name or id of the download file, then select the one you want from the drop-down.'),
    '#size' => 60,
    '#autocomplete_path' => '_uc_file_users_autocomplete',
    '#required' => TRUE,
    '#ajax' => array(
      'callback' => 'uc_file_users_form_callback',
      'wrapper' => 'uc-file-users-info',
    ),
  );

  $form['uc_file_users_info'] = array(
    '#type' => 'fieldset',
    '#attributes' => array('id' => 'uc-file-users-info'),
  );
  $parts = array_map('trim', explode('|', $form_state['values']['filename_fid'] ?? ''));
  $emails = array();
  $num_users = 0;
  if (count($parts) == 2 && !empty($parts[0]) && !empty($parts[1])) {
    $file_users = db_query('
      SELECT
        ufu.uid,
        u.name,
        u.mail
      FROM {uc_file_users} ufu
      LEFT JOIN {users} u ON u.uid = ufu.uid
      WHERE ufu.fid = :fid
      ', array(':fid' => $parts[1]))
      ->fetchAllAssoc('uid', PDO::FETCH_ASSOC);
    $form['uc_file_users_info']['file_users'] = array(
      '#type' => 'value',
      '#value' => $file_users,
    );
    $form['uc_file_users_info']['filename'] = array(
      '#type' => 'value',
      '#value' => $parts[0],
    );
    $form['uc_file_users_info']['fid'] = array(
      '#type' => 'value',
      '#value' => $parts[1],
    );
    foreach ($file_users as $data) {
      $emails[] = $data['mail'];
    }
    $form['uc_file_users_info']['emails'] = array(
      '#markup' => implode("\n", $emails),
      '#prefix' => '<div id="uc-file-users-emails">',
      '#suffix' => '</div>',
    );
  }
  if (!empty($file_users)) {
    $num_users = count($file_users);
  }
  if (!empty($parts[0])) {
    $form['uc_file_users_info']['info'] = array(
      '#type' => 'help',
      '#markup' => t('File %filename has @num users.', array(
        '%filename' => $parts[0],
        '@num' => $num_users,
      )),
    );
  }

  $form[] = array(
    '#type' => 'help',
    '#markup' => t('Choose "See file users" to see all users of the selected file. Choose "Copy emails" to copy the email addresses of all users to the clipboard.'),
  );

  $form['actions']['see_users'] = array(
    '#type' => 'submit',
    '#value' => t('See file users'),
  );

  $form['actions']['copy_emails'] = array(
    '#type' => 'submit',
    '#value' => t('Copy emails'),
  );

  $module_path = backdrop_get_path('module', 'uc_file_users');
  $form['#attached'] = array(
    'css' => array($module_path . '/css/uc_file_users.css'),
    'js' => array($module_path . '/js/uc_file_users.js'),
  );

  return $form;
}

/**
 * Callback function to display users of a selected file.
 */
function uc_file_users_form_callback($form, &$form_state) {
  return $form['uc_file_users_info'];
}

/**
 * Submission function for uc_file_users_form() to see users.
 */
function uc_file_users_form_submit($form, &$form_state) {
  switch ($form_state['triggering_element']['#id']) {
    case 'edit-see-users':
      $fid = $form_state['values']['fid'];
      $form_state['redirect'] = 'admin/store/products/file-users/' . $fid;
      break;

    case 'edit-copy-emails':
      // All the action happens in Javascript.
      $file_users = $form_state['values']['file_users'];
      backdrop_set_message(t('The email addresses for @num users have been copied to the clipboard.', array('@num' => count($file_users))));
      break;
  }
}

/**
 * Form to display users for a specified download file.
 */
function uc_file_users_file_form($form, &$form_state, $fid) {
  $filename = db_query('
    SELECT filename
    FROM {uc_files}
    WHERE fid = :fid
    ', array(':fid' => $fid))
    ->fetchField();
  if (empty($filename)) {
    backdrop_set_message(t('%fid is not a valid file ID.', array('%fid' => $fid)), 'error');
    backdrop_goto('admin/store/products/file-users');
    return;
  }
  backdrop_set_title(t('File download users for %filename', array('%filename' => $filename)), PASS_THROUGH);

  $header = array(
    array(
      'data' => t('User'),
      'field' => 'name',
    ),
    array(
      'data' => t('Mail'),
      'field' => 'mail',
    ),
    array(
      'data' => t('Downloads'),
      'field' => 'accessed',
    ),
  );
  $query = db_select('uc_file_users', 'ufu')
     ->extend('PagerDefault')
     ->extend('TableSort');
  $query->leftJoin('users', 'u', 'u.uid = ufu.uid');
  $query->fields('ufu', array('uid', 'accessed'));
  $query->fields('u', array('name', 'mail'));
  $query
    ->condition('ufu.fid', $fid)
    ->limit(25)
    ->orderByHeader($header);
    $query->setCountQuery($query->countQuery());
    $result = $query->execute();
    $rows = array();
  foreach ($result as $obj) {
    $rows[] = array(
      l($obj->name, 'user/' . $obj->uid),
      $obj->mail,
      $obj->accessed,
    );
  }
  $form['table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' => t('No users have downloaded this file.'),
  );
  $form['pager'] = array('#theme' => 'pager');

  return $form;
}

/**
 * Autocomplete function for download file names.
 */
function _uc_file_users_autocomplete($string) {
  if (empty($string)) {
    return '';
  }
  $matches = array();
  $query = db_select('uc_files', 'ucf');
  $return = $query
    ->fields('ucf', array('filename', 'fid'))
    ->condition('ucf.filename', ('%' . db_like($string) . '%'), 'LIKE')
    ->range(0, 10)
    ->execute();
  foreach ($return as $row) {
    $entry = $row->filename . ' | ' . $row->fid;
    $entry_sanitized = check_plain($entry);
    $matches[$entry] = $entry_sanitized;
  }
  backdrop_json_output($matches);
}
