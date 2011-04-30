<?php

/**
 * @file
 * Admin page callbacks file for the search_log module.
 */

/**
 * Display admin settings.
 *
 * @return
 *   array of form content.
 */
function search_log_admin_settings() {
  $form = array();

  $form['logging'] = array(
    '#type'   => 'fieldset',
    '#title'  => t('Search log settings'),
  );

  $terms_options = array(
    SEARCH_LOG_TERMS_LOWERCASE => t('lowercase (%1 stored as %2)', array('%1' => 'Apple iPod', '%2' => 'apple ipod')),
    SEARCH_LOG_TERMS_UPPERCASE_FIRST => t('uppercase first word (%1 stored as %2)', array('%1' => 'Apple iPod', '%2' => 'Apple ipod')),
    SEARCH_LOG_TERMS_UPPERCASE_WORDS => t('uppercase all words (%1 stored as %2)', array('%1' => 'Apple iPod', '%2' => 'Apple Ipod')),
  );
  $form['logging']['search_log_terms'] = array(
    '#type' => 'radios',
    '#title' => t('Search term normalization'),
    '#description' => t('Search terms are normalized before they are stored in Search log.'),
    '#options' => $terms_options,
    '#default_value' => variable_get('search_log_terms', SEARCH_LOG_TERMS_LOWERCASE),
  );

  foreach (module_list() as $name) {
    if (module_hook($name, 'search') && $title = module_invoke($name, 'search', 'name')) {
      $module_options[$name] = $name;
    }
  }
  $form['logging']['search_log_modules_enabled'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Modules'),
    '#description' => t('Select modules to record in Search log. If no modules are checked, all modules which implement !hook_search() will be included.', array('!hook_search()' => l('hook_search()', 'http://api.drupal.org/api/drupal/developer--hooks--core.php/function/hook_search/6'))),
    '#options' => $module_options,
    '#default_value' => variable_get('search_log_modules_enabled', array()),
  );

  $form['logging']['search_log_preprocess'] = array(
    '#type' => 'checkbox',
    '#title' => t('!warning Collect search status with preprocess_search_results()', array('!warning' => '<span class="error">[Experimental]</span>')),
    '#description' => t('Search does not have a hook to obtain the number of search results. This theme function will work in certain circumstances. If enabled, the function will add one extra DB write for each search result page.'),
    '#default_value' => variable_get('search_log_preprocess', FALSE),
  );

  $form['status'] = array(
    '#type'   => 'fieldset',
    '#title'  => t('Search log status'),
  );

  $count = db_result(db_query('SELECT COUNT(qid) FROM {search_log}'));
  $form['status']['search_log_count']['#value'] = '<p>' . t('There are %count entries in the Search log table. !clear', array('%count' => number_format($count), '!clear' => l(t('Clear log'), 'admin/settings/search/search_log/clear'))) . '</p>';

  $form['status']['search_log_cron'] = array(
    '#type' => 'textfield',
    '#title' => t('Days to keep search log'),
    '#description' => t('Search log table can be automatically truncated by cron. Set to 0 to never truncate Search log table.'),
    '#size' => 4,
    '#default_value' => variable_get('search_log_cron', 0),
  );

  $form['#validate'][] = 'search_log_admin_settings_validate';

  return system_settings_form($form);
}

/**
 * Validate admin settings.
 */
function search_log_admin_settings_validate($form, &$form_state) {
  if (!preg_match('/^[0-9]+$/', $form_state['values']['search_log_cron'])) {
    form_set_error('search_log_cron', t('The number of days must be a number 0 or greater.'));
  }
}

/**
 * Form builder; confirm form for truncate search_log table.
 *
 * @ingroup forms
 * @see search_log_confirm_truncate_submit()
 */
function search_log_confirm_truncate(&$form_state) {
  $form = array();
  return confirm_form($form,
    t('Are you sure you want to clear the Search log?'),
    'admin/settings/search/search_log',
    t('This action cannot be undone.'),
    t('Clear'), t('Cancel'));
}

/**
 * Truncate search_log table.
 */
function search_log_confirm_truncate_submit($form, &$form_state) {
  db_query("TRUNCATE {search_log}");
  drupal_set_message(t('The Search log has been cleared.'));

  if (!isset($_REQUEST['destination'])) {
    $form_state['redirect'] = 'admin/settings/search/search_log';
  }
}

/**
 * Display search log report.
 */
function search_log_report($period = NULL) {
  // Get from and to parameters.
  $today = _search_log_get_time();
  $date = getdate($today);

  switch ($period) {
    // Today begins at 00:00
    case 'today':
      $from_date = $to_date = date('Y-m-d', $today);
      break;

    // Week begins on Sunday
    case 'week':
      $from_date = date('Y-m-d', $today - (($date['wday']) * 86400));
      $to_date = date('Y-m-d', $today + (6 - $date['wday']) * 86400);
      break;

    case 'month':
      $from_date = date('Y-m-d', mktime(0, 0, 0, $date['mon'], 1, $date['year']));
      $to_date = date('Y-m-d', mktime(0, 0, 0, $date['mon'] + 1, 1, $date['year']) - 86400);
      break;

    case 'year':
      $from_date = date('Y-m-d', mktime(0, 0, 0, 1, 1, $date['year']));
      $to_date = date('Y-m-d', mktime(0, 0, 0, 12, 31, $date['year']));
      break;

    default:
      $from_date = $_SESSION['search_log']['from'];
      $to_date = $_SESSION['search_log']['to'];
      break;
  }

  $from_time = strtotime($from_date);
  $to_time = strtotime($to_date);

  if ($from_time && $to_time) {
    // Setup session vars.
    $modules = $_SESSION['search_log']['modules'];
    $status = $_SESSION['search_log']['status'];
    $rows_n = $_SESSION['search_log']['rows'] ? $_SESSION['search_log']['rows'] : 50;

    // Build header.
    $header = array(
      array('data' => t('Search term'), 'field' => 'q'),
      array('data' => t('Module'), 'field' => 'module'),
    );
    if ($failed_enabled = db_result(db_query_range('SELECT qid FROM {search_log} WHERE result < 0', 0, 1))) {
      array_push($header, array('data' => t('Result'), 'field' => 'result'));
    }
    array_push($header, array('data' => t('Total'), 'field' => 'total', 'sort' => 'desc'));

    // Build query.
    $query_where_status = '';
    if ($status == SEARCH_LOG_STATUS_SUCCESS) {
      $query_where_status = ' AND result >= 0';
    }
    elseif ($status == SEARCH_LOG_STATUS_FAILED) {
      $query_where_status = ' AND result < 0';
    }
    $query_where_modules = count($modules) ? ' AND module IN ("' . implode('","', $modules) . '")' : '';

    $query = 'SELECT q, module, SUM(counter) as total, result FROM {search_log} WHERE day >= %d AND day <= %d' . $query_where_modules . $query_where_status . ' GROUP BY q, module';
    $query_count = 'SELECT COUNT(DISTINCT q, module) FROM {search_log} WHERE day >= %d AND day <= %d' . $query_where_modules . $query_where_status;
    $result = pager_query($query . tablesort_sql($header), $rows_n, 0, $query_count, $from_time, $to_time);

    // Build results.
    $rows = array();
    while ($data = db_fetch_object($result)) {
      $row = array();
      $row[] = l($data->q, "search/$data->module/$data->q");
      $row[] = $data->module;
      if ($failed_enabled) {
        $row[] = $data->result < 0 ? '<span class="error">' . t('Failed') . '</span>' : '';
      }
      $row[] = $data->total;
      $rows[] = $row;
    }

    if ($rows) {
      $total = db_result(db_query('SELECT SUM(counter) FROM {search_log} WHERE day >= %d AND day <= %d' . $query_where_modules . $query_where_status, $from_time, $to_time));
      $unique = db_result(db_query($query_count, $from_time, $to_time));
      switch ($status) {
        case SEARCH_LOG_STATUS_ALL:
          $failed = (int)db_result(db_query('SELECT SUM(counter) FROM {search_log} WHERE result < 0 AND day >= %d AND day <= %d' . $query_where_modules, $from_time, $to_time));
          break;

        case SEARCH_LOG_STATUS_SUCCESS:
          $failed = 0;
          break;

        case SEARCH_LOG_STATUS_FAILED:
          $failed = $total;
          break;
      }

      return theme('search_log_report',
        theme('table', $header, $rows, array('id' => 'search-log')),
        theme('pager', array(), $rows_n, 0),
        theme('search_log_summary', $total, $unique, $failed),
        drupal_get_form('search_log_report_form', $from_date, $to_date, $modules, $status, $rows_n)
      );
    }
  }

  return theme('search_log_report',
    NULL,
    NULL,
    t('No searches found for period.'),
    drupal_get_form('search_log_report_form', $from_date, $to_date, $modules, $status, $rows_n)
  );
}

/**
 * Report filter form.
 */
function search_log_report_form(&$form_state, $from_date = NULL, $to_date = NULL, $modules = array(), $status = NULL, $rows = 50) {
  $form = array();

  // Search period
  $today = date('Y-m-d', _search_log_get_time());

  $form['period'] = array(
    '#type'       => 'fieldset',
    '#title'      => t('Search period'),
  );

  $links[] = array(
    'title' => t('Today'),
    'href'  => 'admin/reports/search/today'
  );
  $links[] = array(
    'title' => t('This week'),
    'href'  => 'admin/reports/search/week'
  );
  $links[] = array(
    'title' => t('This month'),
    'href'  => 'admin/reports/search/month'
  );
  $links[] = array(
    'title' => t('This year'),
    'href'  => 'admin/reports/search/year'
  );
  $form['period']['links']['#value'] = theme('links', $links, array('class' => 'search-log-links'));

  $form['period']['from_date'] = array(
    '#type' => 'textfield',
    '#title' => t('From'),
    '#default_value' => $from_date ? $from_date : $today,
    '#attributes' => array('class' => 'jscalendar'),
  );

  $form['period']['to_date'] = array(
    '#type' => 'textfield',
    '#title' => t('To'),
    '#default_value' => $to_date ? $to_date : $today,
    '#description' => t('Enter custom period for search reporting.'),
    '#attributes' => array('class' => 'jscalendar'),
  );

  // Search modules.
  $module_options = array();
  $query = db_query('SELECT DISTINCT module FROM {search_log}');
  while ($row = db_fetch_object($query)) {
    $module_options[$row->module] = $row->module;
  }
  if (!empty($module_options)) {
    $module_default = !empty($modules) ? $modules : array_keys($module_options);

    $form['modules'] = array(
      '#type' => 'fieldset',
      '#title' => t('Search modules'),
    );

    $form['modules']['modules'] = array(
      '#type' => 'checkboxes',
      '#description' => t('Select modules to include in search reporting.'),
      '#options' => $module_options,
      '#default_value' => $module_default,
    );
  }

  // Search status.
  if (db_result(db_query_range('SELECT qid FROM {search_log} WHERE result < 0', 0, 1))) {
    $status_options = array(
      SEARCH_LOG_STATUS_ALL => t('All'),
      SEARCH_LOG_STATUS_SUCCESS => t('Success'),
      SEARCH_LOG_STATUS_FAILED => t('Failed')
    );

    $form['status'] = array(
      '#type' => 'fieldset',
      '#title' => t('Search status'),
    );

    $form['status']['status'] = array(
      '#type' => 'radios',
      '#description' => t('Select status to include in search reporting.'),
      '#options' => $status_options,
      '#default_value' => $status ? $status : SEARCH_LOG_STATUS_ALL,
      '#required' => TRUE,
    );
  }
  else {
    $form['status'] = array(
      '#type' => 'hidden',
      '#default_value' => SEARCH_LOG_STATUS_ALL,
    );
  }

  $form['settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Report settings'),
  );

  $form['settings']['rows'] = array(
    '#type' => 'textfield',
    '#title' => t('Rows in report'),
    '#size' => 4,
    '#default_value' => $rows,
  );

  $form['submit'] = array('#type' => 'submit', '#value' => t('Update Report'));
  $form['#action'] = url('admin/reports/search');
  return $form;
}

/**
 * Report filter form validation.
 */
function search_log_report_form_validate($form, &$form_state) {
  $today = _search_log_get_time();

  $from = strtotime($form_state['values']['from_date']);
  if (!$from) {
    $from = $today;
  }

  if ($from > $today) {
    form_set_error('from_date', t('From date cannot be after today.'));
  }

  $to = strtotime($form_state['values']['to_date']);
  if (!$to) {
    $to = $today;
  }

  if ($from > $to) {
    form_set_error('from_date', t('From date cannot be after To date.'));
  }

  if (isset($form_state['values']['modules'])) {
    $modules = array_flip($form_state['values']['modules']);
    unset($modules[0]);
    if (count($modules) < 1) {
      form_set_error('modules', t('At least one Search module must be selected.'));
    }
  }

  if ((int)$form_state['values']['rows'] < 10) {
    form_set_error('rows', t('The number of rows must be a number 10 or greater.'));
  }
}

/**
 * Report filter submit.
 */
function search_log_report_form_submit($form, &$form_state) {
  $modules = isset($form_state['values']['modules']) ? array_flip($form_state['values']['modules']) : array();
  unset($modules[0]);

  $_SESSION['search_log'] = array(
    'from' => $form_state['values']['from_date'],
    'to' => $form_state['values']['to_date'],
    'modules' => array_keys($modules),
    'status' => $form_state['values']['status'],
    'rows' => $form_state['values']['rows'],
  );
}

/**
 * Theme report.
 */
function theme_search_log_report($table, $pager, $summary, $filters) {
  drupal_add_css(drupal_get_path('module', 'search_log') .'/search_log.css');

  $output = '<div class="search-log-filter">' . $filters . '</div>';
  $output .= '<div class="search-log-results">';
  $output .= '<fieldset class="summary">' . $summary . '</fieldset>';
  $output .= $table;
  $output .= $pager;
  $output .= '</div>';

  return $output;
}

/**
 * Theme report summary information.
 */
function theme_search_log_summary($total = 0, $unique = 0, $failed = 0) {
  $output  = '<div><strong>Total searches:</strong> ' . $total . '</div>';
  $output .= '<div><strong>Unique search terms:</strong> ' . $unique . ' (' . sprintf("%01.2f", 100*$unique/$total) . '%)</div>';
  $output .= '<div><strong>Failed searches:</strong> ' . $failed . ' (' . sprintf("%01.2f", 100*$failed/$total) . '%)</div>';

  return $output;
}
