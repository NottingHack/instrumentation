<html>
<head>
<title>Nottinghack password reset</title>
</head>
<body>

<table width="100%" height="100%" valign="middle">
<tr>
<td>
<center>

<?php

require_once "../db.php";
require_once "../krb5_auth.php";


if (isset($_GET["reset_id"]))
  process_reset();
else
  generate_reset();


function process_reset()
{
  $oInstDB = db_link();
  echo "<table border=\"1\" cellspacing=\"0\" cellpadding=\"25\"><tr><td>"; 
  
  
  if (isset($_POST['submit'])) 
  {
    if ($_POST["newpass1"] != $_POST["newpass2"])
    {
      echo "Error: Passwords don't match";
    } else if (strlen($_POST["newpass1"]) < 4)
    {
      echo "Error: Password too short";
    } else    
    {
      if ($oInstDB->sp_password_reset_complete($_GET["reset_id"], $_GET["key"], $username, $err))
      {
        if ($err == '')
        {
          /* Do the password update */
          
          /* Check user actaully exists in krb database, and create if not */
          $krb5 = krb_auth();
          $ret = "";
          switch ($krb5->user_exists($username))
          {
            case TRUE:
              if (!$krb5->change_password($username,$_POST["newpass1"]))
                $ret = "Failed to set password (1)";  
              break;
              
            case FALSE:
              if(!$krb5->add_user($username,$_POST["newpass1"]))
                $ret = "Failed to set password (2)";
              break;
              
            default: /* Probably a connection failure to kerberos */
              $ret = "Failed to set password (3)";
          }
          
          if ($ret=="")
          {
            $user_wiki = $username;
            $user_wiki[0] = strtoupper($user_wiki[0]);
            echo "<p>Password updated.</p>";
            echo "<p>";
            echo "Wiki username: <i>$user_wiki</i><br />";
            echo "HMS/nh-web username: <i>$username</i><br />";
            echo "Spacenet username: <i>$username@NOTTINGHACK.ORG.UK</i><br />";
            echo "</p>";
          }
          else
            echo "Error: $ret";
        }
        else
          echo "Error: $err";     
      } else
      {
        echo "Error: Password reset failed";
      }
    }
  } else
  {
    /* Validate reset_id / key is valid */
    if ($oInstDB->sp_password_reset_validate($_GET["reset_id"], $_GET["key"], $handle, $err))
    {
      if (($err != '') || (strlen($handle) <=0))
      {
        if (strlen($err) > 0)
          echo "Error: " . $err;
        else
          echo "An error occurred processing the password reset request (1)";
      } else
      {
        /* Request is valid */
        ?>
        
          <p>Hi <? echo $handle; ?>, please enter a new password:
          <form name="resetpasswordform" action="" method="post">
            <table border="1" cellspacing="0" cellpadding="3">
              <tr>
                <td>New password:</td>
                <td><input name="newpass1" type="password" value="" id="newpass1" /></td>
              </tr>
              <tr>
                <td>Retype password:</td>
                <td><input name="newpass2" type="password" value="" id="newpass2" /></td>
              </tr>            
              <tr>
                <td colspan="2" class="footer"><input type="submit" name="submit" value="Submit" class="mainoption" style="width: 100%"/></td>
              </tr>
            </table>
          </form>      
        <?              
      }
    } else
    {
      echo "An error occurred processing the password reset request (2)";
    }
  }
  
  echo "</td></tr></table>";
}

function generate_reset()
{
 /* Code adapted from:
  * http://www.danbriant.com/general/creating-php-password-reset-script/
  */

  if (isset($_POST['submit'])) 
  {
    echo "<table border=\"1\" cellspacing=\"0\" cellpadding=\"25\"><tr><td>";
              
    $oInstDB = db_link();
    $oInstDB->sp_password_reset($_POST['forgotpassword'], $handle, $member_id, $reset_id, $keystr, $err);
    if ($err != "")
    {
      echo $err;
    } else
    {
      /* generate reset URL */
      if (isset($_SERVER['HTTPS'])) 
        $reset_url = "https://" . $_SERVER["SERVER_NAME"].$_SERVER["REQUEST_URI"];  
      else
        $reset_url = "http://" . $_SERVER["SERVER_NAME"].$_SERVER["REQUEST_URI"]; 
      
      $reset_url .= "?reset_id=$reset_id&key=$keystr";
      
      $body_html = "
  <html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"en-gb\">
  <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />
  <head>
  <title>Password reset</title>
  </head>
  <body>

  <p>Hi $handle, </p>
  <p>You requested a password reset for your Nottinghack account. To complete this, please follow the following link:</p>
  <a href=\"$reset_url\">$reset_url</a>
  <p>If you did not request a password reset, please ignore this email.</p>
  </body>
  </html>";

      $body_text = "
  Hi $handle,  

  You requested a password reset for your Nottinghack account. To complete this, please visit the following URL:
  $reset_url

  If you did not request a password reset, please ignore this email.";

      /* save email to db */
      if ($oInstDB->sp_log_email($member_id, $_POST['forgotpassword'], "", "", "Nottinghack password reset", $body_html, $body_text))
      {
        /* Successfully logged */  
        /* TODO: Send logged email... */
        echo "Password reset email sent...";
      } else
      {
        echo "Error: Failed to send password reset";
      }
    
      echo "</td></tr></table>";
    }
    
  } else 
  {

  ?>
        <form name="forgotpasswordform" action="" method="post">
          <table border="1" cellspacing="0" cellpadding="3">
            <caption><b>Password reset</b></caption>
            <tr>
              <td>Email Address:</td>
              <td><input name="forgotpassword" type="text" value="" id="forgotpassword" /></td>
            </tr>
            <tr>
              <td colspan="2" class="footer"><input type="submit" name="submit" value="Submit" class="mainoption" style="width: 100%"/></td>
            </tr>
          </table>
        </form>
  <?php
  }
  ?>

  </center>
  </td>
  </tr>
  </table>

  </body>
  </html>
  <?
}
?>
  
