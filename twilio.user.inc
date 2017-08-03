<?php

/**
 * @file
 * Twilio user account and registration related functionality
 */

/**
 * Implements hook_user_categories().
 */
function twilio_user_categories() {
  return array(
    array(
      'name' => 'twilio',
      'title' => t('Mobile SMS'),
      'weight' => 3,
    ),
  );
}

/**
 * Implements hook_user_load().
 */
function twilio_user_load($users) {
  // Load data from the {twilio_user} table for the user account.
  $result = db_select('twilio_user', 'u')
    ->fields('u', array('uid', 'country', 'number', 'status', 'code'))
    ->condition('uid', array_keys($users), 'IN')
    ->execute();
  foreach ($result as $record) {
    if (!empty($record->uid)) {
      // Assign the twilio data to the user object.
      $users[$record->uid]->twilio_user = (array) $record;
    }
  }
}

/**
 * Implements hook_user_insert().
 */
function twilio_user_insert(&$edit, $account, $category) {
  twilio_user_save($edit, $account, $category);
}

/**
 * Implements hook_user_update().
 */
function twilio_user_update(&$edit, $account, $category) {
  twilio_user_save($edit, $account, $category);
}

/**
 * Implements hook_user_delete().
 */
function twilio_user_delete($account) {
  db_delete('twilio_user')
    ->condition('uid', $account->uid)
    ->execute();
}

/**
 * Saves mobile number data to the {twilio_user} table in the database.
 *  8/17/mcl: modify call, only one param in backdrop hook_user_save
 */
//function twilio_user_save(&$edit, $account, $category) {
function twilio_user_save($account) {
	// 8/17/mcl: next 4
	 $now = date("D M j G:i:s T Y"); 
  $msg = "twilio_user_save, TOP, $now";
   drupal_set_message($msg);
  // backdrop_set_message($msg);
  watchdog("twilioport", $msg);
//  dsm($account);
	//return;
	
 /* 
 rewrite this block for backdrop, only one param passed
 if (($category == 'twilio') && isset($edit['twilio'])) {
    $number = (object) $edit['twilio'];
    $number->uid = $account->uid;

    $primary_keys = array();
    if (isset($account->twilio_user['status'])) {
      if ($account->twilio_user['status'] == TWILIO_USER_PENDING && $edit['twilio']['status'] == TWILIO_USER_CONFIRMED) {
        $primary_keys = array('uid');
      }
    }
    
       // 8/17/mcl: next 4
  $now = date("D M j G:i:s T Y"); 
  $msg = "Fixin to write drupal record, $now";
  drupal_set_message($msg);
  watchdog("twilioport", $msg);
  dsm($account);
    
    
    drupal_write_record('twilio_user', $number, $primary_keys);

    $edit['twilio']['number'] = NULL;
    $edit['twilio']['status'] = NULL;
    $edit['twilio']['code'] = NULL;
  }*/
  
  // above block rewritten
 //   if (($category == 'twilio') && isset($edit['twilio'])) {
    if (isset($account->twilio_user['status'])) {
//    	backdrop_set_message("past 1st if block in twilio_user_save");
  //  	return;
    	
    //$number = (object) $edit['twilio'];
   // $number = (object) $account->twilio_user;
    $number = $account->twilio_user;
    $number->uid = $account->uid;
   
   //	backdrop_set_message("Here is the NUMBER object");
   	//dsm($number);
  	//return;



    $primary_keys = array();
    if (isset($account->twilio_user['status'])) {
  //    if ($account->twilio_user['status'] == TWILIO_USER_PENDING && $edit['twilio']['status'] == TWILIO_USER_CONFIRMED) {
      if ($account->twilio_user['status'] == TWILIO_USER_CONFIRMED) {
  //    if (false) {
        $primary_keys = array('uid');
      }
    }
    
       // 8/17/mcl: next 4
  $now = date("D M j G:i:s T Y"); 
  $msg = "Fixin to write drupal record, $now";
  drupal_set_message($msg);
  watchdog("twilioport", $msg);
  dsm($account);
    
    
    drupal_write_record('twilio_user', $number, $primary_keys);

    $edit['twilio']['number'] = NULL;
    $edit['twilio']['status'] = NULL;
    $edit['twilio']['code'] = NULL;
  }
  
  
  
  
  else // 8/17/mcl
  {
  	dsm($edit);
  	dsm($account);
  	dsm($category);
  	}
  
}

/**
 * Implements hook_user_login().
 */
function twilio_user_login(&$edit, $account) {
  // If the users mobile number is in the verification state let them know they
  // need to enter their verification code and link to their settings page.
  if (twilio_edit_access($account) && !empty($account->twilio_user) && $account->twilio_user['status'] == 1) {
    $account_link = l(t("account settings page"), 'user/' . $account->uid . '/edit/twilio');
    drupal_set_message(t("You must confirm your phone number by entering the verification code sent to you via SMS. Go to the !link to enter your verification code.", array('!link' => $account_link)), 'warning');
  }
}


/**
 * Implements hook_FORMID_form_alter().
 */
function twilio_form_user_register_form_alter(&$form, &$form_state, $form_id) {
  if ($twilio_registration_form = variable_get('twilio_registration_form', 0)) {
    if ($twilio_registration_form == 2) {
      $required = TRUE;
    }
    else {
      $required = FALSE;
    }
    $form['account']['countrycode'] = array(
      "#type" => 'select',
      '#options' => twilio_country_codes(),
      '#title' => t('Country code'),
    );
    $form['account']['number'] = array(
      '#type' => 'textfield',
      '#title' => t('Phone number'),
      '#required' => $required,
    );
    $form['#validate'][] = 'twilio_register_validate';
    $form['#submit'][] = 'twilio_register_submit';
  }
}

/**
 * Custom validation function for phone numbers during registration.
 */
function twilio_register_validate($form, &$form_state) {
  $value = $form_state['values']['number'];
  // Phone number is not required and not entered.
  if (empty($value) && empty($form['account']['number']['#required'])) {
    return;
  }
  // Something has been entered but is non numeric.
  if (!is_numeric($value)) {
    form_set_error('number', t('You must enter a valid phone number'));
  }
  // Something that looks like a number, verify it's not already taken.
  elseif (twilio_verify_number($value)) {
    form_set_error('number', t('This number is already in use and cannot be assigned to more than one account'));
  }
}

/**
 * Custom submit handler for phone numbers during registration.
 */
function twilio_register_submit($form, &$form_state) {
  $value = $form_state['values']['number'];
  // Phone number is not required and not entered.
  if (empty($value) && empty($form['account']['number']['#required'])) {
    return;
  }
  else {
    $account = user_load($form_state['values']['uid']);
    twilio_user_send_confirmation($account, $form_state['values']['number'], $form_state['values']['countrycode']);
  }
}

/**
 * Send confirmation message.
 *
 * @param object $account
 *   The user object of the account to message
 *
 * @param string $number
 *   The phone number to send the message
 *
 * @param string $country
 *   The country code for the number
 *
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function twilio_user_send_confirmation($account, $number, $country) {
  $code = rand(1000, 9999);
  $data = array(
    'uid' => $account->uid,
    'number' => $number,
    'country' => $country,
    'status' => TWILIO_USER_PENDING,
    'code' => $code,
  );


 // 8/17/mcl: next 4
  $now = date("D M j G:i:s T Y"); 
//  $msg = "user_send_conf call, $now";
 // $msg .= "about to call user_save";

  //drupal_set_message($msg);
//  watchdog("twilioport", $msg);

  //dsm($account);  

  // 8/17/mcl
  //drupal_set_message("this is twilio_user_send_confirmation");
  //dsm($account);

  // 8/17/mcl: update array before call to user_save
//  $account->twilio = $data;  
  $account->twilio_user = $data;  

 //  dsm($account);

//  $account = user_save($account, array('twilio' => $data), 'twilio');
 // $account = user_save($account);   // diff hook for backdrop
  
  // 8/17/mcl: call this directly, not firing
  twilio_user_save($account);   // diff hook for backdrop
 
  

  $msg = "\n\n BACK FROM Modified!  user_save";
  drupal_set_message($msg);
  watchdog("twilioport", $msg);
  dsm($account);

  
  $message = "Confirmation code: $code";
  twilio_send($number, $message, $country);
  return $account;
}


// 8/17/mcl: copy from drupal 7 core user.modules
/**
 * Return a user object after checking if any profile category in the path exists.
 */
function user_category_load($uid, &$map, $index) {
  static $user_categories, $accounts;

  // Cache $account - this load function will get called for each profile tab.
  if (!isset($accounts[$uid])) {
    $accounts[$uid] = user_load($uid);
  }
  $valid = TRUE;
  if ($account = $accounts[$uid]) {
    // Since the path is like user/%/edit/category_name, the category name will
    // be at a position 2 beyond the index corresponding to the % wildcard.
    $category_index = $index + 2;
    // Valid categories may contain slashes, and hence need to be imploded.
    $category_path = implode('/', array_slice($map, $category_index));
    if ($category_path) {
      // Check that the requested category exists.
      $valid = FALSE;
      if (!isset($user_categories)) {
        $user_categories = _user_categories();
      }
      foreach ($user_categories as $category) {
        if ($category['name'] == $category_path) {
          $valid = TRUE;
          // Truncate the map array in case the category name had slashes.
          $map = array_slice($map, 0, $category_index);
          // Assign the imploded category name to the last map element.
          $map[$category_index] = $category_path;
          break;
        }
      }
    }
  }
  return $valid ? $account : FALSE;
}

