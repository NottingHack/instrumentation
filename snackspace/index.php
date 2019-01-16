<?php
/**
 * Snackspace Home
 *
 * Shows a list of all members and their current electronic balance.
 *
 * @author James Hayward <jhayward1980@gmail.com>
 * @version 1.0
 */

require('../common.php');
require(SECURE_DIR . 'inst_db.php');

// Select all details from the members table

$sSQLSelect = "SELECT * FROM members WHERE member_number > 0 ORDER BY member_number";

$oResult = $oInstDB->query($sSQLSelect);

if ($oResult === FALSE) {
	errorDie("Unable to find any data", "SS001");
}

$aMembers = array();
$fTotal = 0;
while ($oRow = $oResult->fetch_assoc()) {
	$fTotal += ($oRow['balance'] * -1);
	$aMember = array(
					 'id'		=>	$oRow['member_id'],
					 'number'	=>	$oRow['member_number'],
					 'name'		=>	$oRow['name'],
					 'balance'	=>	($oRow['balance'] * -1) / 100,
					 'limit'	=>	$oRow['credit_limit'] /100,
					 );
	$aMembers[] = $aMember;
}

$oInstDB->kill($oInstDB->thread_id);
$oInstDB->close();

$fTotal = $fTotal / 100;

$oSmarty->assign("members", $aMembers);
$oSmarty->assign("total", $fTotal);
$oSmarty->assign("refresh", URL . 'snackspace/');
$oSmarty->assign("refreshtime", '30');
$oSmarty->display('ss/index.tpl');

?>
