<?php


require('../common.php');
require(SECURE_DIR . 'inst_db.php');
require('send_emails.php');


  $oMail = new PHPMailer();
    
  $oMail->From ="hostmaster@nottinghack.org.uk";
  $oMail->FromName = "Nottinghack";
  $oMail->AddAddress("check-auth2@verifier.port25.com", "SPF tester");  
  $oMail->Subject = "SPF Test";
  $oMail->MsgHTML("Test");
  $oMail->AltBody = "Test";
  $oMail->CharSet = 'UTF-8';

  $oMail->SMTPDebug  = 3;

  if (!$oMail->Send()) 
  {
    printf("Failed to send email: [" . $oMail->ErrorInfo . "]\n");
  } else
  {
    printf("Email sent.\n");
  }  
?>
