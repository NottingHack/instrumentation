<?php
/**
 * Snackspace - Manual Tranasction
 *
 * Allows a member to add a transaction to their account.
 * Can only add money spent with this script, not pay off bill.
 *
 * @author James Hayward <jhayward1980@gmail.com>
 * @version 1.0
 */

require('../common.php');
require(SECURE_DIR . 'inst_db.php');

// get member's details
if (!isNumber($_GET['member'])) {
	errorDie("Hack Attempt", "MT001");
}
$sSQLSelect = "SELECT * FROM members WHERE member_id=" . $_GET['member'];

$oResult = $oInstDB->query($sSQLSelect);

if ($oResult === FALSE) {
	errorDie("Member Not Found", "MT002");
}

$aMember = $oResult->fetch_assoc();
$oSmarty->assign("member", $aMember);

if (isset($_POST['submit'])) {
	// Check the form input
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
	if (!isText($aForm['desc'])) {
		$aErrors['desc'] = 'Description' . ' ' . ERROR_TEXT;
	}
	
	if (count($aErrors) > 0) {
		$oSmarty->assign("form", $aForm);
		$oSmarty->assign("errors", $aErrors);
		$oSmarty->display("ss/manual_transaction.tpl");
	}
	else {
		$amount = $aForm['amount'] * -100;
		
		// check that we're not going to go above the credit limit
		/* actually, don't
		if ((-1 * ($aMember['balance'] + $amount)) > $aMember['credit_limit']) {
			// output to user
			$oSmarty->assign("title", "Transaction Declined");
			$aParas = array(
							"Your transaction puts you over your limit.  You will need to either pay off part of your bill, or contact a board member to increase your credit limit.",
							'<a href="pay_bill.php?member=' . $aMember['member_id'] . '">Pay Bill</a>',
							'<a href="index.php">Return to list</a>',
							);
			$oSmarty->assign("message", $aParas);
			$oSmarty->display("message.tpl");
			die;
		}
		*/
		
		// Prepare and execute the stored procedure
		if ($oSP = $oInstDB->prepare("call sp_transaction_log(?, ?, 'MANUAL', 'COMPLETE', ?, null, @tran_id, @err)")) {
			
			$oSP->bind_param("iis", $aMember['member_id'], $amount, $aForm['desc']);
			if (!$oSP->execute()) {
				$oSP->close();
				errorDie("Transaction failed", "MT003");
			}
			$oSP->close();
		}
		else {
			errorDie("Prepare failed", "MT004");
		}
		
		// check the result
		$oResult = $oInstDB->query("select @err as err, @tran_id as tran_id");
		if ($oResult === FALSE) {
			errorDie("Prepare failed", "MT005");
		}
		
		$aResult = $oResult->fetch_assoc();
		if (is_null($aResult['err'])) {
			errorDie("Unknown error", "MT006");
		}
		elseif ($aResult['err'] != '') {
			errorDie("Transaction failed", "MT007");
		}
		
		// close database connection
		$oInstDB->kill($oInstDB->thread_id);
		$oInstDB->close();
		
		// output to user
		$oSmarty->assign("title", "Thank you");
		$oSmarty->assign("refresh", URL . "snackspace/index.php");
		$aParas = array(
						"Your transaction has been processed",
						'<a href="index.php">Return to list</a>',
						);
		$oSmarty->assign("message", $aParas);
		$oSmarty->display("message.tpl");
	}
	
}
else {
	$oSmarty->display("ss/manual_transaction.tpl");
}

?>
