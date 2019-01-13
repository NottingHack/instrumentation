<?php


function send_pending_emails($oLink)
{
  $sQuery = "
    select 
      concat_ws(' ', ifnull(m.firstname, ''), ifnull(m.surname, '')) as name,
      e.email_id,
      e.member_id,
      e.email_to,
      e.email_cc,
      e.email_bcc,
      e.email_subj,
      e.email_body,
      e.email_body_alt
    from emails e
    inner join members m on m.member_id = e.member_id
    where e.email_status = 'PENDING'
    for update";

  if ($oResult = $oLink->query($sQuery)) 
  {    
    while ($oRow = $oResult->fetch_assoc()) 
    {
      if (send_email($oRow))
      {
        $bRet = update_email_status($oLink, $oRow["email_id"], "SENT", $err);
      } else
      {
        $bRet = update_email_status($oLink, $oRow["email_id"], "FAILED", $err);
      }
      writeMsg("Commit..");
      $oLink->commit();
      
      if (!$bRet)
      {
        writeMsg("Failed to update email status: [" . $err . "]");
        die();
      }
    }
  }
}

function send_email($oRow)
{
  writeMsg("Send email #" . $oRow["email_id"]);
  $oMail = new PHPMailer();
    
  $oMail->From = MAIL_FROM_ADDR;
  $oMail->FromName = MAIL_FROM_NAME;
  $oMail->AddAddress($oRow["email_to"], $oRow["name"]);  
  $oMail->Subject = $oRow["email_subj"] . " (##" . $oRow["email_id"] . "##)";
  $oMail->MsgHTML($oRow["email_body"]);
  $oMail->AltBody = $oRow["email_body_alt"];
  $oMail->CharSet = 'UTF-8';

  if (!$oMail->Send()) 
  {
    writeMsg("Failed to send email: [" . $oMail->ErrorInfo . "]");
    return false;
  } else
  {
    writeMsg("Email sent.");
    return true;
  }  
}

function update_email_status($oLink, $iEmailId, $sStatus, &$err)
{
  if ($oStmt = $oLink->prepare("call sp_email_update(?, ?, @err)")) 
  {
    $oStmt->bind_param("is", $iEmailId, $sStatus);
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
      $err = "unknown error in/with sp_email_update";
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

?>
