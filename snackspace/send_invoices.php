<?php
/**
 * Snackspace Invoices
 *
 * Sends invoices to all users, as well as overview to mailing list
 * Billing period is 28th to 28th
 *
 * @author James Hayward <jhayward1980@gmail.com>
 * @version 1.0
 */

require('../common.php');
require(SECURE_DIR . 'inst_db.php');

// we only run this on the 28th... is it the 28th??
if (date("d") != "28") {
	//die("Not the right day!\nSI000");
}

$sSQLSelect = "SELECT * FROM members WHERE member_number > 0 ORDER BY member_number";

$oResult = $oInstDB->query($sSQLSelect);

if ($oResult === FALSE) {
	die("Unable to find any data\nSI001");
}

$aMembers = array();
$fTotal = 0;
while ($aMember = $oResult->fetch_assoc()) {
	$fTotal += ($aMember['balance'] * -1);
	
	send_invoice($aMember);
}

// now send to mailing list



function send_invoice($aMember) {
	global $oInstDB;
	
	// get their transactions for this month
	$iFortyDays = 40 * 24 * 60 * 60;
	$iLastDay = mktime(0,0,0, date("n"), 28, date("Y")); // 28th of this month
	//$iFirstDay = mktime(0,0,0, date("n", $iLastDay-$iFortyDays), 28, date("Y", $iLastDay-$iFortyDays)); // 28th of last month
	$iFirstDay = mktime(0,0,0, 12, 28, 2011); // 28th of last month
	
	$sSQLTrans = 'SELECT UNIX_TIMESTAMP(transaction_datetime) as timestamp,amount,transaction_type,transaction_status,transaction_desc FROM transactions WHERE transaction_datetime BETWEEN "' . date("Y-m-d", $iFirstDay) . '" AND "' . date("Y-m-d", $iLastDay) . '" AND member_id = "' . $aMember['member_id'] . '" ORDER BY transaction_datetime';
	$oResult = $oInstDB->query($sSQLTrans);
	
	// Shall we send them an invoice?
	// default is no
	$bSend = false;
	if ($aMember['balance'] < 0) {
		// They owe money - yes!
		$bSend = true;
	}
	elseif ($oResult->num_rows > 0) {
		// They've had activity this month - yes!
		$bSend = true;
	}
	
	if ($bSend === true) {
		// ok, let's send it.
		
		$aTrans = array();
		while ($aRow = $oResult->fetch_assoc()) {
			$aTrans[] = array(
							  'date'	=>	$aRow['timestamp'],
							  'date1'	=>	date("r", $aRow['timestamp']),
							  'amount'	=>	$aRow['amount'],
							  'type'	=>	$aRow['transaction_type'],
							  'status'	=>	$aRow['transaction_status'],
							  'desc'	=>	$aRow['transaction_desc'],
							  );
		}
		var_dump($aMember);
		var_dump($aTrans);
	}
	
}

?>
