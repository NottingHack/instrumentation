<?php

require_once "../../db.php";
require_once "../../krb5_auth.php";

function writeMsg($sMsg) 
{
  $sLogFile = "/home/instrumentation/logs/wiki.log";
        
  $sOutput = date("M d H:i:s") . ": " . $sMsg . "\n";
  file_put_contents($sLogFile, $sOutput, FILE_APPEND);
}

$include_error = false;

$result = Array();
if (!isset($_POST["function"]))
{
  $result['error'] = 'Missing function';
  die(json_encode($result));
}

if (!verifiy_hash())
{
  $result['error'] = 'Decode error';
  die(json_encode($result));
}
  

switch ($_POST["function"])
{
  case "login":
    $result = login();
    break;
  
  case "set_password":
    $result = set_password();
    break;
/*    
  case "get_member_details":
    $result = get_member_details();
    break;
*/
  default:
    $result['error'] = 'Unknown function';
    break;
}
  
echo json_encode($result);

function verifiy_hash()
{
  global $wiki_salt;
  global $wiki_secret;
  
  if (!isset($_POST["hash"]))
    return false;
  
  $hash = $_POST["hash"];
  $_POST["hash"] = $wiki_secret;
  
  $str = json_encode($_POST);
  $gen_hash = crypt($str, $wiki_salt);
  
  if (strlen ($gen_hash) < 10)
    return false;
  
  if ($gen_hash == $hash)
    return true;
  else  
    return false;
}

function get_member_details()
{
  
  $result = Array();
  if (!isset($_POST["username"]))
  {
    $result['success'] = false;
    $result['error'] = 'Missing username';
  } else
  {
    $username = $_POST["username"];
    
    /* Replace anything that isn't a-Z, 0-9 with an underscore (mostly after spaces...) */
    $username = preg_replace('/[^a-zA-Z0-9]/','_', $username);   
    
    $oInstDB = db_link();
    if ($oInstDB->sp_wiki_login($username, $email, $name, $ret))
    {
      if ($ret==1)
      {
        $result['name'] = $name;
        $result['email'] = $email; 
        $result['success'] = true;
      } else
      {
        $result['success'] = false;
        $result['error'] = 'Unknown username / no Wiki permission';
      }
    } else
    {
      $result['success'] = false;
      $result['error'] = 'DB check failed';
    }
  }
  return $result;
}

function login()
{
  $result = Array();
  if ((!isset($_POST["username"])) || (!isset($_POST["password"])))
  {
    $result['access_granted'] = false;
    $result['error'] = 'Missing username/password';
  } else
  {
    $username = $_POST["username"];
    $password = $_POST["password"];
    
    /* Replace anything that isn't a-Z, 0-9 with an underscore (mostly after spaces...) */
    $username = preg_replace('/[^a-zA-Z0-9]/','_', $username);   
    
    $oInstDB = db_link();
    if ($oInstDB->sp_wiki_login($username, $email, $name, $ret))
    {
      if ($ret==1)
      {
        /* check password */
        $krb5 = krb_auth();
        if ($krb5->check_password($username, $password))
        {      
          $result['access_granted'] = true;
          $result['name'] = $name;
          $result['email'] = $email;
        } else
        {
          $result['access_granted'] = false;
          $result['error'] = "Incorrect password / password check failed [$password], [$username]";
        }
      } else
      {
        $result['access_granted'] = false;
        $result['error'] = 'Unknown username / no Wiki permission';
      }
    } else
    {
      $result['access_granted'] = false;
      $result['error'] = 'DB check failed';
    }
  }
  return $result;
}

?>
  
