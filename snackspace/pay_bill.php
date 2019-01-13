<?php
/**
 * Snackspace - Pay bill;
 *
 * Allows a member to add a pay off their account in full or part
 *
 * @author James Hayward <jhayward1980@gmail.com>
 * @version 1.0
 */

require('../common.php');
require(SECURE_DIR . 'inst_db.php');

// get member's details
if (!isNumber($_GET['member'])) {
	errorDie("Hack Attempt", "PB001");
}
$sSQLSelect = "SELECT * FROM members WHERE member_id=" . $_GET['member'];

$oResult = $oInstDB->query($sSQLSelect);

if ($oResult === FALSE) {
	errorDie("Member Not Found", "PB002");
}

$aMember = $oResult->fetch_assoc();
$oSmarty->assign("member", $aMember);

if (isset($_POST['submit'])) {
	$aForm = array(
				  'amount'		=>	$_POST['amount'],
				  'desc'		=>	$_POST['desc'],
				  );
	
	$aErrors = array();
	
	if (!isNumber($aForm['amount'])) {
		$aErrors['amount'] = 'Amount' . ' ' . ERROR_NUM;
	}
	elseif ($aForm['amount'] <= 0) {
		$aErrors['amount'] = 'Amount must be positive';
	}
	elseif (($aMember['balance'] + ($aForm['amount'] * 100)) > 500) {
		$aErrors['amount'] = 'This would take you too far in credit!';
	}
	if (!isText($aForm['desc'])) {
		$aErrors['desc'] = 'Description' . ' ' . ERROR_TEXT;
	}
	
	if (count($aErrors) > 0) {
		$oSmarty->assign("form", $aForm);
		$oSmarty->assign("errors", $aErrors);
		$oSmarty->display("ss/pay_bill.tpl");
	}
	else {
		$amount = $aForm['amount'] * 100;
		
		// Prepare and execute the stored procedure
		if ($oSP = $oInstDB->prepare("call sp_transaction_log(?, ?, 'MANUAL', 'COMPLETE', ?, null, @tran_id, @err)")) {
			
			$oSP->bind_param("iis", $aMember['member_id'], $amount, $aForm['desc']);
			if (!$oSP->execute()) {
				$oSP->close();
				errorDie("Transaction failed", "PB003");
			}
			$oSP->close();
		}
		else {
			errorDie("Prepare failed", "PB004");
		}
		
		// check the result
		$oResult = $oInstDB->query("select @err as err, @tran_id as tran_id");
		if ($oResult === FALSE) {
			errorDie("Prepare failed", "PB005");
		}
		
		$aResult = $oResult->fetch_assoc();
		if (is_null($aResult['err'])) {
			errorDie("Unknown error", "PB006");
		}
		elseif ($aResult['err'] != '') {
			errorDie("Transaction failed", "PB007");
		}
		
		// close database connection
		$oInstDB->kill($oInstDB->thread_id);
		$oInstDB->close();
		
		// output to user
		$oSmarty->assign("title", "Thank you");
		$oSmarty->assign("refresh", URL . "snackspace/index.php");
		$aParas = array(
						"Your payment was received, please put your money in the Snackspace tub.",
						'<a href="index.php">Return to list</a>',
						);
		$oSmarty->assign("message", $aParas);
		$oSmarty->display("message.tpl");
	}
}
else {
	$oSmarty->display("ss/pay_bill.tpl");
}

?>
