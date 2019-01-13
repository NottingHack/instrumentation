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

$sLogFile = '/home/instrumentation/logs/invoices.log'; /** TODO: change me. **/
writeMsg("Invoicing start");
$oInstDB->autocommit(false);
//$oInstDB->set_charset("utf8");

// Create an invoice entry for everyone who should get an invoice
if (!(invoice_prepare($oInstDB, $err)))
{
  writeMsg("invoice_prepare failed: " . $err);
  die();
}

$sSQLTrans = '
  select 
    m.member_id,
    m.email,
    i.invoice_id,
    monthname(i.invoice_from) as month,
    concat_ws(\' \', ifnull(m.firstname, \'\'), ifnull(m.surname, \'\')) as name,
    cast((abs(m.balance)/100) as decimal(20,2)) as balance,
    m.balance as bal_raw
  from invoices i
  inner join members m on i.member_id = m.member_id
  where i.invoice_status = \'GENERATING\' 
  for update';
if ($oResult = $oInstDB->query($sSQLTrans))
{
  while ($oRow = $oResult->fetch_assoc()) 
  {
    if (!generate_invoice($oInstDB, $oRow, $oSmarty))
    {
      writeMsg("Failed to generate invoice [" . $oRow["invoice_id"] . "]!");       
      die();
    }
  }
  
  $oResult->free();
} else
{
  writeMsg("Failed to select generating invoices");
  die();
}

writeMsg("Generating invoices complete, commiting...");
$oInstDB->commit();

writeMsg("Stage 2: Send emails");
//send_pending_emails($oInstDB);

writeMsg("Done.");


function generate_invoice($oLink, $oRow, $oSmarty)
{
  writeMsg("generate_invoice(" . $oRow["invoice_id"] . ")");
  
  $count = 0;
  // Get transactions for invoice
  if ($oStmt = $oLink->prepare("
    select 
      t.transaction_datetime,
      t.transaction_type,
      t.transaction_desc,
      concat('£', cast((t.amount/100) as decimal(20,2))) as amount
    from invoices i
    inner join transactions t 
       on t.member_id = i.member_id
      and t.transaction_datetime >= i.invoice_from
      and t.transaction_datetime <  i.invoice_to
    where i.invoice_id = ?
      and t.transaction_status = 'COMPLETE'
    order by t.transaction_datetime")) 
    {
      $oStmt->bind_param("i", $oRow["invoice_id"]);
      $oStmt->execute();
      $oStmt->bind_result($transaction_datetime, $transaction_type, $transaction_desc, $amount);
      $aTransactions = array();
      while ($oStmt->fetch()) 
      {
         array_push($aTransactions, $transaction_datetime, $transaction_type, $transaction_desc, $amount);
         $count++;
      }   
    }
    $oStmt->close();  
    
    $sTextTable = generate_text_table($aTransactions);

    if ($oRow["bal_raw"] < 0)
      $in_credit = 0;
    else
      $in_credit = 1;
    
    $oSmarty->assign('in_credit', $in_credit);
    $oSmarty->assign('member_name', $oRow["name"]);
    $oSmarty->assign('member_balance', $oRow["balance"]);
    $oSmarty->assign('subject', $oRow["month"] . " Snackspace invoice");
    $oSmarty->assign('title', $oRow["month"] . " Snackspace invoice");
    $oSmarty->assign('month', $oRow["month"]); 
    $oSmarty->assign('invoice_id', $oRow["invoice_id"]);
    $oSmarty->assign('img_nhlogo', COMMON_DIR . "images/logo.jpg"); // html email only
    $oSmarty->assign('text_table', $sTextTable);                    // text email only
    $oSmarty->assign('transaction_count', $count);    
    $oSmarty->assign('transactions', $aTransactions);    
    $oSmarty->assign('tr', array('bgcolor="#eeeeee"','bgcolor="#dddddd"') );

    // log email
    $aEmail = array();
    $aEmail["member_id"]      = $oRow["member_id"];
    $aEmail["email_to"]       = $oRow["email"];
    $aEmail["email_cc"]       = "";
    $aEmail["email_bcc"]      = "";
    $aEmail["email_subj"]     = $oRow["month"] . " Snackspace invoice";
    $aEmail["email_body"]     = $oSmarty->fetch('emails/invoice.tpl');
    $aEmail["email_body_alt"] = $oSmarty->fetch('emails/invoice_alt.tpl');

    $iEmail_id = log_email($oLink, $aEmail, $err);
    if ($iEmail_id == -1)
    {
      writeMsg("Failed to log email: [" . $err . "]");
      // nb. Auto commit is off, so this will never get commited at present.
      update_invoice($oLink, $oRow["invoice_id"], -1, "FAILED");
      return false;
    } else
    {
      writeMsg("Update invoice to GENERATED");
      if (!update_invoice($oLink, $oRow["invoice_id"], $iEmail_id, "GENERATED", $err))
      {
        writeMsg("Failed to update invoice: [" . $err . "]");
        return false;
      } 
      return true;
    }
}

function generate_text_table($aTransactions)
{
  $tbl = "";
  
  // find the longest description
  $iMaxLen = strlen(" Transaction description ");
  for ($x = 2; $x < count($aTransactions); $x += 4) 
  {
    if (strlen($aTransactions[$x]) > $iMaxLen)
      $iMaxLen = strlen($aTransactions[$x])+2;
  }

  // Output table header. 
  $tbl =  "+----------------------+------------------+" .  str_repeat("-", $iMaxLen) . "+------------+\n";
  $tbl .= "| Transaction date     | Transaction type | Transaction description" . str_repeat(" ", $iMaxLen - strlen(" Transaction description "))  . " | Amount     |\n";
  $tbl .= "+----------------------+------------------+" .  str_repeat("-", $iMaxLen) . "+------------+\n";

  // Output table body
  for ($x = 0; $x < count($aTransactions); $x += 4) 
  {
    $tbl .= "| " . $aTransactions[$x  ] . "  | ";
    $tbl .=  $aTransactions[$x+1] . str_repeat(" ", strlen("Transaction type") - strlen($aTransactions[$x+1])) . " | ";
    $tbl .=  $aTransactions[$x+2] . str_repeat(" ", $iMaxLen - strlen($aTransactions[$x+2])-1) . "| ";
    $tbl .=  $aTransactions[$x+3] . str_repeat(" ", strlen("Amount     ") - strlen($aTransactions[$x+3])) . " |\n";
  }  
 
  // Table footer
  $tbl .=  "+----------------------+------------------+" .  str_repeat("-", $iMaxLen) . "+------------+\n";
  return $tbl;
}

function update_invoice($oLink, $iInvoice_id, $iEmail_id, $sNewStatus, &$err)
{
  if ($oStmt = $oLink->prepare("call sp_invoice_update(?, ?, ?, @err)")) 
  {
    $oStmt->bind_param("iis", $iInvoice_id, $iEmail_id, $sNewStatus);
    if (!$oStmt->execute())
    {
      $err = sprintf("SQL execute error: [%s].", $oStmt->error);
      $oStmt->close();   
      return false;
    }
    $oStmt->close();   
  } else
  {
    $err = 'mysqli_prepare failed (exec sp)';
    return false;
  }

  // Get result
  if ($oStmt =  $oLink->prepare("select @err as err")) 
  {
    $oStmt->execute();
    $oStmt->bind_result($err);
    $oStmt->fetch();
    $oStmt->close();  

    if (is_null($err))
    {
      $err = "unknown error in/with sp_invoice_update";
      return false;
    }

    if ($err != '')
      return false; 
  } else
  {
    $err = 'mysqli_prepare failed (select result)';
    return false;
  }
  
  return true;
}



function invoice_prepare($connection, &$err)
{
  if ($stmt = $connection->prepare("call sp_invoice_prepare(null, null, null, @err)")) 
  {
    if (!$stmt->execute())
    {
      $err = sprintf("SQL execute error: [%s].", $stmt->error);
      $stmt->close();   
      return false;
    }
    $stmt->close();   
  } else
  {
    $err = 'mysqli_prepare failed (exec sp)';
    return false;
  }

  // Get result
  if ($stmt =  $connection->prepare("select @err as err")) 
  {
    $stmt->execute();
    $stmt->bind_result($err);
    $stmt->fetch();
    $stmt->close();  

    if (is_null($err))
    {
      $err = "unknown error in/with sp_invoice_prepare";
      return false;
    }

    if ($err != '')
      return false; 
  } else
  {
    $err = 'mysqli_prepare failed (select result)';
    return false;
  }
  
  return true;
}



// Retuns -1 on failure, otherwise email_id
function log_email($oConnection, $aEmail, &$err)
{
  if ($oStmt = $oConnection->prepare("call sp_log_email(?, ?, ?, ?, ?, ?, ?, @err, @email_id)")) 
  {
    $oStmt->bind_param("issssss", $aEmail["member_id"],$aEmail["email_to"],$aEmail["email_cc"],$aEmail["email_bcc"],$aEmail["email_subj"],$aEmail["email_body"],$aEmail["email_body_alt"]);
    if (!$oStmt->execute())
    {
      $err = sprintf("SQL execute error: [%s].", $oStmt->error);
      $oStmt->close();   
      return -1;
    }
    $oStmt->close();   
  } else
  {
    $err = 'mysqli_prepare failed (exec sp)';
    return -1;
  }

  // Get result
  if ($oStmt =  $oConnection->prepare("select @err as err, @email_id as email_id")) 
  {
    $oStmt->execute();
    $oStmt->bind_result($err, $iEmail_id);
    $oStmt->fetch();
    $oStmt->close();  

    if (is_null($err))
    {
      $err = "unknown error in/with sp_invoice_prepare";
      return -1;
    }

    if ($err != '')
      return -1; 
  } else
  {
    $err = 'mysqli_prepare failed (select result)';
    return -1;
  }
  
  return $iEmail_id;
}

function writeMsg($sMsg) 
{
   global $sLogFile;
        
  $sOutput = date("M d H:i:s") . ": " . $sMsg . "\n";
  file_put_contents($sLogFile, $sOutput, FILE_APPEND | LOCK_EX);
}

?>
