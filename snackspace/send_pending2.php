<?php
/**
 * Snackspace Invoices
 *
 * Send monthly invoice to all users with either transactions in the last month, or a non-zero ballance
 * TODO: Send overview to mailing list
 * Billing period is 1st to the end of the month.
 *
 * @author James Hayward <jhayward1980@gmail.com>, Daniel Swann <nh@dswann.co.uk>
 * @version 1.1
 */

require('../common.php');
require(SECURE_DIR . 'inst_db.php');
require('send_emails.php');

function send_pend()
{

  $sLogFile = '/home/instrumentation/logs/email.log'; /** TODO: change me. **/
  $oInstDB->set_charset("utf8");


  writeMsg("Send pending emails only");
  send_pending_emails($oInstDB);
  writeMsg("Done.");
}

function writeMsg($sMsg) 
{
   global $sLogFile;

  $sOutput = date("M d H:i:s") . ": " . $sMsg . "\n";
  file_put_contents($sLogFile, $sOutput, FILE_APPEND | LOCK_EX);
}


?>


